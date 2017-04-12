/*
 * correlationtest.h
 *
 *  Created on: Aug 9, 2016
 *      Author: tsokalo
 */

#ifndef CORRELATIONTEST_H_
#define CORRELATIONTEST_H_

#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <stdint.h>
#include <random>
#include <math.h>
#include <memory>
#include <numeric>

#include <boost/dynamic_bitset.hpp>

#include <boost/math/distributions/chi_squared.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/algorithm/minmax_element.hpp>

#include "statistics.h"

void
RunCorrelationTest (void)
{
  std::random_device r;

  uint16_t num_senders = 5, num_receivers = num_senders;
  std::default_random_engine generator (r ());
  std::uniform_real_distribution<double> distribution (0.0, 1.0);

  uint32_t max_symbols = 1000;

  std::array<double, 2> e_levels =
    { 0.1, 0.3 };
  std::array<double, 11> overlap_levels =
    { 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
  for (auto ei = e_levels.begin (); ei != e_levels.end (); ei++)
    {
      double e = *ei;
      for (auto oi = overlap_levels.begin (); oi != overlap_levels.end (); oi++)
        {
          double o = *oi;
          std::vector<double> s_rho_i, s_rho_e, s_delta_e, s_corr;
          uint32_t num_iter = 1000;
          while (num_iter-- != 0)
            {
              //
              // Initialization
              //

              std::map<uint16_t, std::map<uint16_t, boost::dynamic_bitset<> > > rcv_status;

              //
              // senders works
              //

              for (uint16_t i = 0; i < num_senders; i++)
                {
                  uint32_t overlap = max_symbols * o;
                  for (uint32_t n = 0; n < overlap; n++)
                    {
                      auto s = distribution (generator) > e;
                      for (uint16_t j = 0; j < num_receivers; j++)
                        {
                          rcv_status[i][j].push_back (s);
                        }
                    }
                  for (uint32_t n = 0; n < max_symbols - overlap; n++)
                    {
                      for (uint16_t j = 0; j < num_receivers; j++)
                        {
                          rcv_status[i][j].push_back (distribution (generator) > e);
                        }
                    }
                }

              //              for (uint16_t i = 0; i < num_senders; i++)
              //                {
              //                  for (uint16_t j = 0; j < num_receivers; j++)
              //                    {
              //                      std::cout << "rcv_status[" << i << "][" << j << "].count() = " << rcv_status[i][j].count ()
              //                              << ", rcv_status[" << i << "][" << j << "].size() = " << rcv_status[i][j].size () << std::endl;
              //                    }
              //                }

              //
              // calculating loss ratios
              //

              std::map<uint16_t, std::map<uint16_t, double> > l;
              for (uint16_t i = 0; i < num_senders; i++)
                {
                  for (uint16_t j = 0; j < num_receivers; j++)
                    {
                      l[i][j] = (double) (rcv_status[i][j].size () - rcv_status[i][j].count ())
                              / (double) rcv_status[i][j].size ();
                      //                      std::cout << "l[" << i << "][" << j << "] = " << l[i][j] << std::endl;
                    }
                }

              //
              // calculating correlation coefficients
              //

              std::map<uint16_t, std::map<uint16_t, std::map<uint16_t, double> > > c;
              long double t_sum = 0;
              for (uint16_t i = 0; i < num_senders; i++)
                {
                  for (uint16_t j = 0; j < num_receivers; j++)
                    {
                      for (uint16_t k = 0; k < num_receivers; k++)
                        {
                          auto x = rcv_status[i][j];
                          auto y = rcv_status[i][k];
                          auto z = x & y;
                          c[i][j][k] = (x.size () * z.count () - x.count () * y.count ()) / sqrt (x.size () * x.count () - pow (
                                  x.count (), 2)) / sqrt (y.size () * y.count () - pow (y.count (), 2));
//                          std::cout << "c[" << i << "][" << j << "][" << k << "] = " << c[i][j][k] << std::endl;
                          if (j != k) t_sum += c[i][j][k];
                        }
                    }
                }
              double ave_corr_coef = t_sum / (long double) (num_senders * num_receivers * (num_receivers - 1));
              ave_corr_coef = (ave_corr_coef > 1) ? 0 : ave_corr_coef;

              //
              // calculating real rho
              //
              // TODO: ..
              //

              //
              // calculating inaccurate estimation of rho
              //

              std::vector<double> rho_i (num_receivers);
              std::map<uint16_t, std::map<uint16_t, double> > a_e_i;

              for (uint16_t i = 0; i < num_senders; i++)
                {
                  for (uint16_t j = 0; j < num_receivers; j++)
                    {
                      a_e_i[i][j] = 1;
                      for (uint16_t k = 0; k < j; k++)
                        {
                          a_e_i[i][j] *= l[i][k];
                        }
                    }
                }

              for (uint16_t i = 0; i < num_senders; i++)
                {
                  for (uint16_t j = 0; j < num_receivers; j++)
                    {
                      rho_i[j] += a_e_i[i][j] * rcv_status[i][j].count ();
                      //                      std::cout << "I >> " << j << "\t" << a_e_i[i][j] << "\t" << rcv_status[i][j].count () << std::endl;
                    }
                }

              //
              // calculating exact estimation of rho
              //

              std::vector<double> rho_e (num_receivers);
              std::map<uint16_t, std::map<uint16_t, double> > a_e;

              for (uint16_t i = 0; i < num_senders; i++)
                {
                  for (uint16_t j = 0; j < num_receivers; j++)
                    {
                      boost::dynamic_bitset<> grs (max_symbols);
                      for (uint16_t k = 0; k < j; k++)
                        {
                          grs |= rcv_status[i][k];
                        }
                      grs |= (~rcv_status[i][j]);
                      a_e[i][j] = (double) (grs.size () - grs.count ()) / (double) grs.size ();
                    }
                }

              for (uint16_t i = 0; i < num_senders; i++)
                {
                  for (uint16_t j = 0; j < num_receivers; j++)
                    {
                      rho_e[j] += a_e[i][j] * rcv_status[i][j].size ();
                      //                      std::cout << "E >> " << j << "\t" << a_e[i][j] << "\t" << rcv_status[i][j].count () << std::endl;
                    }
                }

              s_rho_i.push_back (std::accumulate (rho_i.begin (), rho_i.end (), 0.0));
              s_rho_e.push_back (std::accumulate (rho_e.begin (), rho_e.end (), 0.0));

              s_delta_e.push_back ((*(s_rho_i.end () - 1) - *(s_rho_e.end () - 1)) / *(s_rho_i.end () - 1));

              s_corr.push_back(ave_corr_coef);
              //
              //              std::cout << "<<< " << e << "\t" << o << "\t" << *(s_rho_i.end () - 1) << "\t" << *(s_rho_e.end () - 1) << "\t"
              //                      << *(s_delta_e.end () - 1) << std::endl;
            }

          auto s_rho_i_a = CalcStats (s_rho_i);
          auto s_rho_e_a = CalcStats (s_rho_e);

          auto s_delta_e_a = CalcStats (s_delta_e);

          auto s_corr_a = CalcStats (s_corr);

          std::cout << e << "\t" << o << "\t" << s_corr_a.first << "\t" << s_corr_a.second
                  << "\t" << s_rho_i_a.first << "\t" << s_rho_i_a.second << "\t" << s_rho_e_a.first << "\t"
                  << s_rho_e_a.second << "\t" << s_delta_e_a.first << "\t" << s_delta_e_a.second << std::endl;
        }
    }
}

#endif /* CORRELATIONTEST_H_ */
