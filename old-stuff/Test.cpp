/*
 * test.cpp
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#include <iostream>
#include <random>
#include <memory>
#include <vector>
#include <numeric>
#include <functional>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include "test.h"
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/distributions/inverse_gaussian.hpp>
#include "filter.h"
#include "CommNet.h"

using namespace std;

double g_p_source = 1;

double
p_lin_ind (uint16_t f_size, uint16_t rcv_dof, uint16_t Gs)
{
  return (1 - 1 / pow (f_size, Gs - rcv_dof));
}
void
UpdateWithPolicy (Policy policy, MarkovState st, uint16_t Gs, Losses l, uint16_t k, uint16_t s)
{
  switch (policy)
    {
  case WFF_POLICY:
    {
      if (st.relayDof == Gs) g_p_source = 0.5;
      break;
    }
  case WFH_POLICY:
    {
      if (st.relayDof + st.destDof >= Gs) g_p_source = 0.5;
      break;
    }
  case ZH_POLICY:
    {
      if (l.e2 > l.e3)
        {
          g_p_source = 1;
        }
      else
        {
          //          cout << "st.relayDofRel + st.destDof: "  << st.relayDofRel + st.destDof << endl;
          if (st.relayDofRel + st.destDof == Gs)
            {
              //              cout << "SWITCH SOURCE OFF" << endl;
              g_p_source = 0;
            }
        }
      break;
    }
  case PLNCOOL_POLICY:
    {
      double sent_before_relay = (double) Gs * ((1 - l.e1) * l.e3 - (1 - l.e2)) / (((1 - l.e3) * ((1 - l.e1) * l.e3
              - (1 - l.e2))) - ((1 - l.e1) * l.e3 * (2 - l.e2 - l.e3)));
      if (st.relayDof >= round (sent_before_relay * (1 - l.e1))) g_p_source = 0.5;
      break;
    }
  case TRY_K_POLICY:
    {
      if (st.relayDof >= k) g_p_source = 0.5;
      if (st.relayDofRel + st.destDof == Gs) g_p_source = 0;
      assert(st.relayDofRel + st.destDof <= Gs);
      break;
    }
  case TRY_S_POLICY:
    {
      if (st.relayDofRel + st.destDof == Gs) g_p_source = 0.5;
      if (st.destDof >= k) g_p_source = 0;
      assert(st.relayDofRel + st.destDof <= Gs);
      break;
    }
  case ZH_REAL_POLICY:
    {
      if (l.e2 + 0.000001 > l.e3)
        {
          g_p_source = 1;
        }
      else
        {
          double lim = (double) Gs / (1 - l.e1 * l.e3);
          //          if (k == floor (lim) || k == ceil (lim))
          //            {
          //              std::default_random_engine generator;
          //              std::uniform_real_distribution<double> distribution (0.0, 1.0);
          //              g_p_source = (distribution (generator) > lim) ? 0 : 1;
          //            }
          //          else if (k > ceil (lim))
          //            {
          //              g_p_source = 0;
          //            }
          if (k >= round (lim))
            {
              g_p_source = 0;
            }
        }

      break;
    }
  case TRY_S_RNO_POLICY:
    {
      if (l.e2 > l.e3)
        {
          g_p_source = 1;
        }
      else
        {
          if (k >= round ((double) Gs / (1 - l.e1 * l.e3) + s))
            {
              g_p_source = 0;
            }
        }
      break;
    }
    }
}
void
MakeStep (Policy policy, MarkovState &st, TransProb p, uint16_t Gs, Losses l, std::default_random_engine &generator,
        std::uniform_real_distribution<double> &distribution, int16_t k, uint16_t s)
{
  double p_lin_ind_dest = (l.e2 < l.e3) ? p_lin_ind (F_SIZE, st.destDof + st.relayDofRel, Gs) : p_lin_ind (F_SIZE, st.destDof,
          Gs);//p_lin_ind_relay = p_lin_ind_dest;
  bool lin_dep = (distribution (generator) >= p_lin_ind_dest);

  if (st.trans == SOURCE_TRANS)
    {
      bool relay_rcvd = (distribution (generator) < 1 - l.e1);
      bool dest_rcvd = (distribution (generator) < 1 - l.e3);
      //      cout << "SOURCE: " << relay_rcvd << "\t" << dest_rcvd << "\t" << lin_dep << "\t" << endl;

      if (relay_rcvd && !dest_rcvd && !lin_dep) // relay only received
        {
          st.relayDofRel++;
          st.relayDof++;
        }
      if (!relay_rcvd && dest_rcvd && !lin_dep) // destination only received
        {
          st.destDof++;
          assert (!(st.relayDofRel + st.destDof > Gs && st.relayDofRel == 0));
          if (st.relayDofRel + st.destDof > Gs) st.relayDofRel--;
        }
      if (relay_rcvd && dest_rcvd && !lin_dep) // both relay and destination received
        {
          st.relayDof++;
          st.destDof++;
          assert (!(st.relayDofRel + st.destDof > Gs && st.relayDofRel == 0));
          if (st.relayDofRel + st.destDof > Gs) st.relayDofRel--;
        }
    }
  else // relay transmits
    {
      bool dest_rcvd = (distribution (generator) < 1 - l.e2);
      //      cout << "RELAY: " << dest_rcvd << endl;
      if (dest_rcvd) // destination received
        {
          st.relayDofRel--;
          st.destDof++;
        }
    }

  st.trans = (distribution (generator) < g_p_source) ? SOURCE_TRANS : RELAY_TRANS; // define the next sender
  if (st.relayDofRel == 0) st.trans = SOURCE_TRANS;

  UpdateWithPolicy (policy, st, Gs, l, k, s);
}
void
MakeTransition (Policy policy, MarkovState &st, TransProb p, double v, uint16_t Gs, Losses l, int16_t k, uint16_t s)
{
  double p_lin_ind_relay = p_lin_ind (F_SIZE, st.relayDof, Gs);
  double p_lin_ind_dest = p_lin_ind (F_SIZE, st.destDof, Gs);
  p.Prdr = p.Prdr * p_lin_ind_dest;
  p.Prds = p.Prds * p_lin_ind_dest + p.Prdr;
  assert (p.Prds <= 1);
  p.Prnr = (1 - p.Prds) * (1 - g_p_source);
  p.Prns = (1 - p.Prds) * g_p_source;

  p.Psrr = p.Psrr * p_lin_ind_relay;
  p.Psrs = p.Psrs * p_lin_ind_relay + p.Psrr;
  p.Psds = p.Psds * p_lin_ind_dest + p.Psrs;
  p.Psdr = p.Psdr * p_lin_ind_dest + p.Psds;
  assert (p.Psdr <= 1);
  p.Psnr = (1 - p.Psdr) * (1 - g_p_source);
  p.Psns = (1 - p.Psdr) * g_p_source;

  if (st.trans == SOURCE_TRANS)
    {
      if (v < p.Psrr)
        {
          if (st.relayDofRel + st.destDof < Gs)
            {
              st.relayDofRel++;
              st.relayDof++;
            }
          st.trans = RELAY_TRANS;
          if (st.relayDofRel == 0) st.trans = SOURCE_TRANS;
        }
      else if (v < p.Psrs)
        {
          if (st.relayDofRel + st.destDof < Gs)
            {
              st.relayDofRel++;
              st.relayDof++;
            }
          st.trans = SOURCE_TRANS;
        }
      else if (v < p.Psds)
        {
          st.destDof++;
          if (st.relayDofRel + st.destDof > Gs) st.relayDofRel--;
          st.trans = SOURCE_TRANS;
        }
      else if (v < p.Psdr)
        {
          st.destDof++;
          if (st.relayDofRel + st.destDof > Gs) st.relayDofRel--;
          st.trans = RELAY_TRANS;
          if (st.relayDofRel == 0) st.trans = SOURCE_TRANS;
        }
    }
  else
    {
      assert(st.relayDofRel > 0);
      if (v < p.Prdr)
        {
          st.destDof++;
          st.trans = RELAY_TRANS;
          st.relayDofRel--;
          if (st.relayDofRel == 0) st.trans = SOURCE_TRANS;
        }
      else if (v < p.Prds)
        {
          st.destDof++;
          st.trans = SOURCE_TRANS;
          st.relayDofRel--;
        }
    }
  UpdateWithPolicy (policy, st, Gs, l, k, s);
}

void
InitState (MarkovState &st)
{
  st.destDof = 0;
  st.relayDofRel = 0;
  st.relayDof = 0;
  st.trans = SOURCE_TRANS;
  g_p_source = 1;
}
double
GetTotalSentCalc (Policy policy, int16_t Gs, Losses l)
{
  switch (policy)
    {
  case WFF_POLICY:
    {
      return (l.e3 > l.e1) ? ((double) Gs / (1 - l.e1) * ((2 - l.e2 + l.e3 - 2 * l.e1) / (2 - l.e2 - l.e3))) : ((double) Gs
              / (1 - l.e1));
    }
  case WFH_POLICY:
    {
      return (double) Gs / (1 - l.e1 * l.e3) * ((2 - l.e2 + l.e3 - 2 * l.e1 * l.e3) / (2 - l.e2 - l.e3));
    }
  case ZH_POLICY:
    {
      return (l.e2 > l.e3) ? (double) Gs / (1 - l.e3) : ((double) Gs / (1 - l.e1) + (double) Gs * (1 - 1 / (1 - l.e1) * (1
              - l.e3) / (1 - l.e2)));
    }
  case PLNCOOL_POLICY:
    {
      double sent_before_relay = (double) Gs * ((1 - l.e1) * l.e3 - (1 - l.e2)) / (((1 - l.e3) * ((1 - l.e1) * l.e3
              - (1 - l.e2))) - ((1 - l.e1) * l.e3 * (2 - l.e2 - l.e3)));
      double sent_after_relay = sent_before_relay * (1 - l.e1) * l.e3 / ((1 - l.e1) * l.e3 - (1 - l.e2));
      return sent_before_relay + 2 * sent_after_relay;
    }
  case TRY_K_POLICY:
    {
      return 0;
    }
    }
  return 0;
}
double
GetTotalSentCalcBestRoute (int16_t Gs, Losses l)
{
  return (1 / (1 - l.e3) < (1 / (1 - l.e1) + 1 / (1 - l.e2))) ? (double) Gs / (1 - l.e3) : (double) Gs * (1 / (1 - l.e1) + 1
          / (1 - l.e2));
}
string
GetBestRoute (int16_t Gs, Losses l)
{
  return (1 / (1 - l.e3) < (1 / (1 - l.e1) + 1 / (1 - l.e2))) ? "direct" : "relayed";
}
void
PrintSymbol (NcSymbol sym)
{
  for(auto i : sym.get_data())cout << i << " ";
  cout << endl;
}
void
PrintState (MarkovState st)
{
  cout << "DOF D: " << st.destDof << ", DOF rel R: " << st.relayDofRel << ", DOF R: " << st.relayDof << ", next " << ((st.trans
          == SOURCE_TRANS) ? "Source" : "Relay") << endl;
}
long double
factorial_partial (uint16_t n, uint16_t k)
{
  return (n == k) ? 1 : factorial_partial (n - 1, k) * n;
}
long double
factorial (uint16_t n)
{
  return factorial_partial (n, 0);
}

long double
permutations (uint16_t n, uint16_t k)
{
  assert(n >= k);
  return (k > n - k) ? ((long double) factorial_partial (n, k) / (long double) factorial (n - k))
          : ((long double) factorial_partial (n, n - k) / (long double) factorial (k));
}

long double
calc_prob_r (uint16_t n, uint16_t k, double p)
{
  assert(n >= k);
  //  cout << "k: " << k << ", n: " << n << ", e1: " << e1 << ", e3: " << e3 << ", permutations: " << permutations (n, k)
  //          << ", pow (1 - e1 * e3, k): " << pow (1 - e1 * e3, k) << ", pow (e1 * e3, n - k): " << pow (e1 * e3, n - k)
  //          << ", prob: " << permutations (n, k) * pow (1 - e1 * e3, k) * pow (e1 * e3, n - k) << endl;
  return (k == 0) ? 0 : (calc_prob_r (n, k - 1, p) + permutations (n, k) * pow (1 - p, k) * pow (p, n - k));
}

double
calc_datarate_eqv (uint16_t r, uint16_t ki, uint16_t k, uint16_t g, double e1, double e3)
{
  return (double) g / (double) (r + k + ki) * (1 - calc_prob_r (r + k, g - 1, e1 * e3));
}
template<typename T>
  T
  normal_pdf (T x, T m, T s)
  {
    T inv_sqrt_2pi = 0.3989422804014327;
    T a = (x - m) / s;

    return inv_sqrt_2pi / s * std::exp (-T (0.5) * a * a);
  }
std::pair<long double, long double>
binom_approx_normal (uint16_t n, uint16_t k, double p)
{
  std::pair<long double, long double> res;
  res.first = permutations (n, k) * pow (1 - p, k) * pow (p, n - k);
  res.second = normal_pdf<long double> ((double) k, (double) n * (1 - p), sqrt ((double) n * p * (1 - p)));
  return res;
}
void
is_binom_approx_normal_adequate (uint16_t n, double p)
{
  cout << "Mean: " << n * p << ", std: " << n * p * (1 - p) << ", rule: " << 1 / sqrt ((double) n) * (sqrt (p / (1 - p))
          - sqrt ((1 - p) / p)) << endl;
}
double
calc_completion_time (Losses l, uint16_t s, uint16_t Gs)
{
  double r = (double) Gs / (1 - l.e1 * l.e3);
  double m = ((double) Gs * l.e3 * (1 - l.e1) / (1 - l.e1 * l.e3) - (double) s * (2 - l.e2 - l.e3)) / (1 - l.e2);
  return (r + 2 * s + m);
}
uint16_t
calc_overhead (double retrans_prob, double loss_prob, uint16_t want_to_deliver)
{
  double z = sqrt (2) * boost::math::erf_inv (2 * retrans_prob - 1);
  double temp = -1 * z * sqrt (loss_prob) + sqrt (z * z * loss_prob + 4 * ((double) want_to_deliver - 0.5));
  double ave_deliver = pow (temp / 2, 2);
  return floor (ave_deliver / (1 - loss_prob));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
Test1 ()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  Policy policy = TRY_K_POLICY;

  cout << "Program start" << endl;
  cout << "Gs" << "\t" << "e1\t" << "\t" << "e2\t" << "\t" << "e3\t" << "\t" << "k" << "\t" << "trans sim" << "\t"
          << "trans calc" << "\t" << "trans relay" << "\t" << "best route" << "\t" << "calc-sim" << "\t" << "gain sim" << "\t"
          << "gain calc" << "\t" << "route" << endl;
  std::cout.setf (std::ios::fixed, std::ios::floatfield);
  std::cout.precision (6);

  for (uint16_t Gs = 100; Gs <= 100; Gs += 10)
    {

      double tot_compl_gain = 0;
      uint32_t counter = 0;
      vector<double> completion_gain;

      for (double e1 = 0.009999; e1 < 1; e1 += 0.0999)
        {
          for (double e2 = 0.009999; e2 < 1; e2 += 0.0999)
            {
              for (double e3 = 0.009999; e3 < 1; e3 += 0.0999)
                {
                  for (uint16_t k = 1; k < Gs; k += 1)
                    {
                      Losses l;
                      l.e1 = e1;
                      l.e2 = e2;
                      l.e3 = e3;
                      TransProb p;
                      p.Prdr = (1 - l.e2) * (1 - g_p_source);
                      p.Prds = (1 - l.e2) * g_p_source;
                      assert (p.Prds <= 1);
                      p.Psrr = (1 - l.e1) * l.e3 * (1 - g_p_source);
                      p.Psrs = (1 - l.e1) * l.e3 * g_p_source;
                      p.Psds = (1 - l.e3) * g_p_source;
                      p.Psdr = (1 - l.e3) * (1 - g_p_source);
                      assert (p.Psdr <= 1);

                      MarkovState st;

                      int32_t r = 100;
                      vector<int32_t> num_trans;
                      vector<int32_t> num_trans_relay;
                      while (r-- > 0)
                        {
                          InitState (st);
                          SentPkts snt;
                          snt.source = 0;
                          snt.relay = 0;
                          while (st.destDof != Gs)
                            {
                              if (st.trans == RELAY_TRANS) snt.relay++;
                              if (st.trans == SOURCE_TRANS) snt.source++;
                              MakeTransition (policy, st, p, distribution (generator), Gs, l, k);
                            }
                          num_trans.push_back (snt.relay + snt.source);
                          num_trans_relay.push_back (snt.relay);
                        }
                      double ave_trans = (double) accumulate (num_trans.begin (), num_trans.end (), 0, plus<int32_t> ())
                              / (double) num_trans.size ();
                      double ave_trans_relay = (double) accumulate (num_trans_relay.begin (), num_trans_relay.end (), 0, plus<
                              int32_t> ()) / (double) num_trans_relay.size ();
                      double ave_trans_calc = GetTotalSentCalc (policy, Gs, l);
                      double ave_trans_best_route_calc = GetTotalSentCalcBestRoute (Gs, l);
                      double completition_time_gain = ave_trans_best_route_calc / ave_trans;
                      double completition_time_gain_calc = ave_trans_best_route_calc / ave_trans_calc;

                      cout << Gs << "\t" << e1 << "\t" << e2 << "\t" << e3 << "\t" << k << "\t" << ave_trans << "\t"
                              << ave_trans_calc << "\t" << ave_trans_relay << "\t" << ave_trans_best_route_calc << "\t"
                              << (ave_trans - ave_trans_calc) / ave_trans << "\t" << completition_time_gain << "\t"
                              << completition_time_gain_calc << "\t" << GetBestRoute (Gs, l) << endl;
                      completion_gain.push_back (completition_time_gain);
                      tot_compl_gain += completition_time_gain;
                      counter++;
                    }
                  cout << endl;
                }
              cout << endl;
            }
          cout << endl;
        }
      //      double ave_gain = (double) accumulate (completion_gain.begin (), completion_gain.end (), 0)
      //              / (double) completion_gain.size ();
      double ave_gain = tot_compl_gain / (double) counter;
      cout << endl << Gs << "\t" << ave_gain << endl;
    }
}
/*
 * find the optimal start of the relay for ZHP policy: source stops when R+D=Gs, relay does not transmit if e2 > e3.
 * Result: relay can wait up to the time point when R+D=Gs
 */
void
Test2 ()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  Policy policy = TRY_K_POLICY;

  uint16_t Gs = 100;

  double tot_compl_gain = 0;
  uint32_t counter = 0;
  vector<double> completion_gain;

  double e1 = 0.2;
  double e2 = 0.2;
  double e3 = 0.8;
  cout << "Program start" << endl;
  cout << round ((double) Gs * (1 - e1) / (1 - e1 * e3)) << endl;
  cout << "Gs" << "\t" << "e1\t" << "\t" << "e2\t" << "\t" << "e3\t" << "\t" << "k" << "\t" << "trans sim" << "\t"
          << "trans calc" << "\t" << "trans relay" << "\t" << "best route" << "\t" << "calc-sim" << "\t" << "gain sim" << "\t"
          << "gain calc" << "\t" << "route" << endl;
  for (uint16_t k = 1; k < round ((double) Gs * (1 - e1) / (1 - e1 * e3) + 1); k += 1)
    {
      //  uint16_t k = 90;
      Losses l;
      l.e1 = e1;
      l.e2 = e2;
      l.e3 = e3;
      TransProb p;

      MarkovState st;

      int32_t num_it = 1000;
      int32_t r = num_it;
      SentPkts snt;
      snt.source = 0;
      snt.relay = 0;
      while (r-- > 0)
        {
          InitState (st);

          while (st.destDof != Gs)
            {
              p.Prdr = (1 - l.e2) * (1 - g_p_source);
              p.Prds = (1 - l.e2) * g_p_source;
              assert (p.Prds <= 1);
              p.Psrr = (1 - l.e1) * l.e3 * (1 - g_p_source);
              p.Psrs = (1 - l.e1) * l.e3 * g_p_source;
              p.Psds = (1 - l.e3) * g_p_source;
              p.Psdr = (1 - l.e3) * (1 - g_p_source);
              assert (p.Psdr <= 1);
              if (st.trans == RELAY_TRANS) snt.relay++;
              if (st.trans == SOURCE_TRANS) snt.source++;
              //          PrintState (st);
              MakeTransition (policy, st, p, distribution (generator), Gs, l, k);

              //          if (st.relayDofRel + st.destDof == Gs) break;
            }
        }
      double ave_trans = (double) (snt.relay + snt.source) / (double) num_it;
      double ave_trans_relay = (double) snt.relay / (double) num_it;
      double ave_trans_calc = GetTotalSentCalc (policy, Gs, l);
      double ave_trans_best_route_calc = GetTotalSentCalcBestRoute (Gs, l);
      double completition_time_gain = ave_trans_best_route_calc / ave_trans;
      double completition_time_gain_calc = ave_trans_best_route_calc / ave_trans_calc;

      cout << Gs << "\t" << e1 << "\t" << e2 << "\t" << e3 << "\t" << k << "\t" << ave_trans << "\t" << ave_trans_calc << "\t"
              << ave_trans_relay << "\t" << ave_trans_best_route_calc << "\t" << (ave_trans - ave_trans_calc) / ave_trans
              << "\t" << completition_time_gain << "\t" << completition_time_gain_calc << "\t" << GetBestRoute (Gs, l) << endl;
      completion_gain.push_back (completition_time_gain);
      tot_compl_gain += completition_time_gain;
      counter++;
    }

}
/*
 * find the optimal stop of the source for ZHP policy: relay starts when R+D=Gs
 * Result: source should stop when R+D=Gs
 */
void
Test3 ()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  Policy policy = TRY_S_POLICY;

  uint16_t Gs = 100;

  double tot_compl_gain = 0;
  uint32_t counter = 0;
  vector<double> completion_gain;

  double e1 = 0.5;
  double e2 = 0.2;
  double e3 = 0.8;
  cout << "Program start" << endl;
  cout << "Gs" << "\t" << "e1\t" << "\t" << "e2\t" << "\t" << "e3\t" << "\t" << "k" << "\t" << "trans sim" << "\t"
          << "trans calc" << "\t" << "trans relay" << "\t" << "best route" << "\t" << "calc-sim" << "\t" << "gain sim" << "\t"
          << "gain calc" << "\t" << "route" << endl;
  uint16_t start_point = round ((double) Gs * (1 - e3) / (1 - e1 * e3)) + 1;
  for (uint16_t k = start_point; k < Gs; k += 1)
    {
      //        uint16_t k = 90;
      Losses l;
      l.e1 = e1;
      l.e2 = e2;
      l.e3 = e3;
      TransProb p;

      MarkovState st;

      int32_t num_it = 1000;
      int32_t r = num_it;
      SentPkts snt;
      snt.source = 0;
      snt.relay = 0;
      double completion_time = 0;
      while (r-- > 0)
        {
          InitState (st);
          int16_t c = 0, m = 0, r = 0;
          while (st.destDof != Gs)
            {
              p.Prdr = (1 - l.e2) * (1 - g_p_source);
              p.Prds = (1 - l.e2) * g_p_source;
              assert (p.Prds <= 1);
              p.Psrr = (1 - l.e1) * l.e3 * (1 - g_p_source);
              p.Psrs = (1 - l.e1) * l.e3 * g_p_source;
              p.Psds = (1 - l.e3) * g_p_source;
              p.Psdr = (1 - l.e3) * (1 - g_p_source);
              assert (p.Psdr <= 1);
              if (st.trans == RELAY_TRANS) snt.relay++;
              if (st.trans == RELAY_TRANS) r++;
              if (st.trans == SOURCE_TRANS) snt.source++;
              if (st.trans == SOURCE_TRANS) c++;
              if (st.trans == SOURCE_TRANS && r == 0) m++;
              MakeTransition (policy, st, p, distribution (generator), Gs, l, k);
            }
          //          cout << "c - m: " << c - m << endl;
          completion_time += calc_completion_time (l, c - m, Gs);
        }
      double ave_trans = (double) (snt.relay + snt.source) / (double) num_it;
      double ave_trans_relay = (double) snt.relay / (double) num_it;
      double ave_calc_completion_time = completion_time / (double) num_it;
      double ave_trans_calc = GetTotalSentCalc (policy, Gs, l);
      double ave_trans_best_route_calc = GetTotalSentCalcBestRoute (Gs, l);
      double completition_time_gain = ave_trans_best_route_calc / ave_trans;
      double completition_time_gain_calc = ave_trans_best_route_calc / ave_trans_calc;

      cout << Gs << "\t" << e1 << "\t" << e2 << "\t" << e3 << "\t" << k << "\t" << ave_trans << "\t" << ave_trans_calc << "\t"
              << ave_trans_relay << "\t" << ave_trans_best_route_calc << "\t" << (ave_trans - ave_trans_calc) / ave_trans
              << "\t" << completition_time_gain << "\t" << completition_time_gain_calc << "\t" << GetBestRoute (Gs, l) << "\t"
              << ave_calc_completion_time << endl;
      completion_gain.push_back (completition_time_gain);
      tot_compl_gain += completition_time_gain;
      counter++;
    }

}
void
Test4 ()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  Policy policy = ZH_POLICY;

  cout << "Gs" << "\t" << "e1\t" << "\t" << "e2\t" << "\t" << "e3\t" << "\t" << "k" << "\t" << "trans sim" << "\t"
          << "trans calc" << "\t" << "trans relay" << "\t" << "best route" << "\t" << "calc-sim" << "\t" << "gain sim" << "\t"
          << "gain calc" << "\t" << "route" << endl;
  std::cout.setf (std::ios::fixed, std::ios::floatfield);
  std::cout.precision (6);

  uint16_t Gs = 100;
  //  for (uint16_t Gs = 100; Gs <= 100; Gs += 10)
  //    {

  double tot_compl_gain = 0;
  uint32_t counter = 0;
  vector<double> completion_gain;
  double e1 = 0.7;
  //      for (double e1 = 0.009999; e1 < 1; e1 += 0.0999)
  //        {
  for (double e2 = 0.009999; e2 < 1; e2 += 0.0999)
    {
      for (double e3 = 0.009999; e3 < 1; e3 += 0.0999)
        {
          Losses l;
          l.e1 = e1;
          l.e2 = e2;
          l.e3 = e3;
          TransProb p;

          MarkovState st;

          int32_t num_it = 1000;
          int32_t r = num_it;
          SentPkts snt;
          snt.source = 0;
          snt.relay = 0;
          while (r-- > 0)
            {
              InitState (st);

              while (st.destDof != Gs)
                {
                  p.Prdr = (1 - l.e2) * (1 - g_p_source);
                  p.Prds = (1 - l.e2) * g_p_source;
                  assert (p.Prds <= 1);
                  p.Psrr = (1 - l.e1) * l.e3 * (1 - g_p_source);
                  p.Psrs = (1 - l.e1) * l.e3 * g_p_source;
                  p.Psds = (1 - l.e3) * g_p_source;
                  p.Psdr = (1 - l.e3) * (1 - g_p_source);
                  assert (p.Psdr <= 1);
                  if (st.trans == RELAY_TRANS) snt.relay++;
                  if (st.trans == SOURCE_TRANS) snt.source++;
                  //          PrintState (st);
                  MakeTransition (policy, st, p, distribution (generator), Gs, l, 0);
                }
            }
          double ave_trans = (double) (snt.relay + snt.source) / (double) num_it;
          double ave_trans_relay = (double) snt.relay / (double) num_it;
          double ave_trans_calc = GetTotalSentCalc (policy, Gs, l);
          double ave_trans_best_route_calc = GetTotalSentCalcBestRoute (Gs, l);
          double completition_time_gain = ave_trans_best_route_calc / ave_trans;
          double completition_time_gain_calc = ave_trans_best_route_calc / ave_trans_calc;

          cout << Gs << "\t" << e1 << "\t" << e2 << "\t" << e3 << "\t" << 0 << "\t" << ave_trans << "\t" << ave_trans_calc
                  << "\t" << ave_trans_relay << "\t" << ave_trans_best_route_calc << "\t" << (ave_trans - ave_trans_calc)
                  / ave_trans << "\t" << completition_time_gain << "\t" << completition_time_gain_calc << "\t" << GetBestRoute (
                  Gs, l) << endl;
          completion_gain.push_back (completition_time_gain);
          tot_compl_gain += completition_time_gain;
          counter++;
        }
      cout << endl;
    }
  //          cout << endl;
  //        }
  //      double ave_gain = (double) accumulate (completion_gain.begin (), completion_gain.end (), 0)
  //              / (double) completion_gain.size ();
  double ave_gain = tot_compl_gain / (double) counter;
  cout << endl << Gs << "\t" << ave_gain << endl;
  //    }
}
void
Test5 ()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  Policy policy = ZH_REAL_POLICY;

  cout << "Program start" << endl;
  cout << "Gs" << "\t" << "e1\t" << "\t" << "e2\t" << "\t" << "e3\t" << "\t" << "k" << "\t" << "trans sim" << "\t"
          << "trans calc" << "\t" << "trans relay" << "\t" << "best route" << "\t" << "calc-sim" << "\t" << "gain sim" << "\t"
          << "gain calc" << "\t" << "route" << endl;
  std::cout.setf (std::ios::fixed, std::ios::floatfield);
  std::cout.precision (10);

  for (uint16_t Gs = 50; Gs <= 50; Gs += 10)
    {
      double tot_compl_gain = 0;
      uint32_t counter = 0;
      vector<double> completion_gain;

      double e1 = 0.7;
      //      for (double e1 = 0.009999; e1 < 1; e1 += 0.0999)
      //        {
      for (double e2 = 0.009999; e2 < 1; e2 += 0.0999)
        {
          for (double e3 = 0.009999; e3 < 1; e3 += 0.0999)
            {
              Losses l;
              l.e1 = e1;
              l.e2 = e2;
              l.e3 = e3;
              TransProb p;

              MarkovState st;

              int32_t num_it = 10000;
              int32_t r = num_it;
              SentPkts snt;
              snt.source = 0;
              snt.relay = 0;
              double pd = 0;
              double pd2 = 0;
              while (r-- > 0)
                {
                  InitState (st);

                  int32_t c = 0;
                  bool failed = false;
                  double lim = (double) Gs / (1 - l.e1 * l.e3);
                  while (st.destDof != Gs)
                    {
                      p.Prdr = (1 - l.e2) * (1 - g_p_source);
                      p.Prds = (1 - l.e2) * g_p_source;
                      assert (p.Prds <= 1);
                      p.Psrr = (1 - l.e1) * l.e3 * (1 - g_p_source);
                      p.Psrs = (1 - l.e1) * l.e3 * g_p_source;
                      p.Psds = (1 - l.e3) * g_p_source;
                      p.Psdr = (1 - l.e3) * (1 - g_p_source);
                      assert (p.Psdr <= 1);
                      if (st.trans == RELAY_TRANS) snt.relay++;
                      if (st.trans == SOURCE_TRANS) snt.source++;
                      if (st.trans == SOURCE_TRANS) c++;
                      //          PrintState (st);
                      MakeTransition (policy, st, p, distribution (generator), Gs, l, c);

                      if (g_p_source == 0 && st.relayDofRel + st.destDof < Gs)//snt.source > ceil (lim) &&
                        {
                          failed = true;
                        }
                      assert(!(failed && g_p_source != 0));
                      if (g_p_source == 0 && st.relayDofRel == 0)//snt.source > ceil (lim) &&
                        {
                          break;
                        }
                    }
                  if (l.e2 + 0.000001 <= l.e3)
                    {
                      double p = calc_prob_r (round (lim), Gs - 1, l.e1 * l.e3);
                      pd2 += p;
                      //                      cout << "p: " << p << ", mean: " << round (lim) << endl;
                    }
                  if (failed) pd++;
                }
              pd /= (double) num_it;
              pd2 /= (double) num_it;
              double ave_trans = (double) (snt.relay + snt.source) / (double) num_it;
              double ave_trans_source = (double) (snt.source) / (double) num_it;
              double ave_trans_relay = (double) snt.relay / (double) num_it;
              double ave_trans_source_before_relay = (double) Gs / (1 - e1 * e3);
              double ave_trans_source_after_relay = ave_trans - ave_trans_relay - (double) Gs / (1 - e1 * e3);
              double ave_trans_calc = GetTotalSentCalc (policy, Gs, l);
              double ave_trans_best_route_calc = GetTotalSentCalcBestRoute (Gs, l);
              double completition_time_gain = ave_trans_best_route_calc / ave_trans;
              double completition_time_gain_calc = ave_trans_best_route_calc / ave_trans_calc;

              cout << Gs << "\t" << e1 << "\t" << e2 << "\t" << e3 << "\t" << 0 << "\t" << ave_trans << "\t"
                      << ave_trans_source << "\t" << ave_trans_relay << "\t" << ave_trans_best_route_calc << "\t" << (ave_trans
                      - ave_trans_calc) / ave_trans << "\t" << completition_time_gain << "\t" << completition_time_gain_calc
                      << "\t" << GetBestRoute (Gs, l) << "\t" << pd << "\t" << "\t" << pd2 << endl;
              completion_gain.push_back (completition_time_gain);
              tot_compl_gain += completition_time_gain;
              counter++;
            }
          cout << endl;
          //          break;
        }
      //          cout << endl;
      //        }
      //      double ave_gain = (double) accumulate (completion_gain.begin (), completion_gain.end (), 0)
      //              / (double) completion_gain.size ();
      double ave_gain = tot_compl_gain / (double) counter;
      cout << endl << Gs << "\t" << ave_gain << endl;
    }
}

void
Test6 ()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  Policy policy = TRY_S_RNO_POLICY;

  cout << "Program start" << endl;
  cout << "Gs" << "\t" << "e1\t" << "\t" << "e2\t" << "\t" << "e3\t" << "\t" << "k" << "\t" << "trans sim" << "\t"
          << "trans calc" << "\t" << "trans relay" << "\t" << "best route" << "\t" << "calc-sim" << "\t" << "gain sim" << "\t"
          << "gain calc" << "\t" << "route" << endl;
  std::cout.setf (std::ios::fixed, std::ios::floatfield);
  std::cout.precision (6);
  uint16_t Gs = 100;

  double e1 = 0.7;
  //      for (double e1 = 0.009999; e1 < 1; e1 += 0.0999)
  //        {
  for (double e2 = 0.009999; e2 < 1; e2 += 0.0999)
    {
      for (double e3 = 0.009999; e3 < 1; e3 += 0.0999)
        {
          uint16_t best_s = 0;
          double best_rd_dof = 0;
          double best_gain = -1000000;
          for (uint16_t s = 0;; s++)
            {
              Losses l;
              l.e1 = e1;
              l.e2 = e2;
              l.e3 = e3;
              TransProb p;

              MarkovState st;

              int32_t num_it = 1000;
              int32_t r = num_it;
              SentPkts snt;
              snt.source = 0;
              snt.relay = 0;
              double pd = 0;
              double rd_dof = 0;
              while (r-- > 0)
                {
                  InitState (st);

                  int32_t c = 0;
                  bool failed = false;
                  bool do_once = true;
                  while (st.destDof != Gs)
                    {
                      p.Prdr = (1 - l.e2) * (1 - g_p_source);
                      p.Prds = (1 - l.e2) * g_p_source;
                      assert (p.Prds <= 1);
                      p.Psrr = (1 - l.e1) * l.e3 * (1 - g_p_source);
                      p.Psrs = (1 - l.e1) * l.e3 * g_p_source;
                      p.Psds = (1 - l.e3) * g_p_source;
                      p.Psdr = (1 - l.e3) * (1 - g_p_source);
                      assert (p.Psdr <= 1);
                      if (st.trans == RELAY_TRANS) snt.relay++;
                      if (st.trans == SOURCE_TRANS) snt.source++;
                      if (st.trans == SOURCE_TRANS) c++;
                      //          PrintState (st);
                      MakeTransition (policy, st, p, distribution (generator), Gs, l, c, s);
                      if (g_p_source == 0 && do_once)
                        {
                          do_once = false;
                          rd_dof += (st.relayDofRel + st.destDof);
                        }
                      if (g_p_source == 0 && st.relayDofRel == 0 && st.destDof != Gs)
                        {
                          failed = true;
                          break;
                        }
                    }
                  if (failed)
                    {
                      pd++;
                    }

                }

              rd_dof /= (double) (num_it - pd);
              pd /= (double) num_it;
              double ave_trans = (double) (snt.relay + snt.source) / (double) num_it;
              double ave_trans_best_route_calc = GetTotalSentCalcBestRoute (Gs, l);

              double gain = (ave_trans_best_route_calc - ave_trans / (1 - pd)) / (ave_trans / (1 - pd));
              if (best_gain < gain)
                {
                  best_gain = gain;
                  best_s = s;
                  best_rd_dof = rd_dof;
                  cout << Gs << "\t" << e1 << "\t" << e2 << "\t" << e3 << "\t" << best_gain << "\t" << (double) Gs / (1 - e1
                          * e3) + best_s << "\t" << best_s << "\t" << best_rd_dof << endl;
                }
              if (g_p_source > 0) break;
            }
          cout << endl;
          cout << Gs << "\t" << e1 << "\t" << e2 << "\t" << e3 << "\t" << best_gain << "\t" << (double) Gs / (1 - e1 * e3)
                  + best_s << "\t" << best_s << "\t" << best_rd_dof << endl;
          cout << endl;
        }
      cout << endl;
    }

}
void
Test7 ()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  Policy policy = ZH_POLICY;

  cout << "Gs" << "\t" << "e1\t" << "\t" << "e2\t" << "\t" << "e3\t" << "\t" << "k" << "\t" << "trans sim" << "\t"
          << "trans calc" << "\t" << "trans relay" << "\t" << "best route" << "\t" << "calc-sim" << "\t" << "gain sim" << "\t"
          << "gain calc" << "\t" << "route" << endl;
  std::cout.setf (std::ios::fixed, std::ios::floatfield);
  std::cout.precision (6);

  uint16_t Gs = 100;
  //  for (uint16_t Gs = 100; Gs <= 100; Gs += 10)
  //    {

  double tot_compl_gain = 0;
  vector<double> completion_gain;
  double e1 = 0.7;
  //      for (double e1 = 0.009999; e1 < 1; e1 += 0.0999)
  //        {
  for (double e2 = 0.009999; e2 < 1; e2 += 0.0999)
    {
      for (double e3 = 0.009999; e3 < 1; e3 += 0.0999)
        {
          Losses l;
          l.e1 = e1;
          l.e2 = e2;
          l.e3 = e3;
          TransProb p;

          MarkovState st;

          int32_t num_it = 1000;
          int32_t r = num_it;
          SentPkts snt;
          snt.source = 0;
          snt.relay = 0;
          while (r-- > 0)
            {
              InitState (st);

              while (st.destDof != Gs)
                {
                  p.Prdr = (1 - l.e2) * (1 - g_p_source);
                  p.Prds = (1 - l.e2) * g_p_source;
                  assert (p.Prds <= 1);
                  p.Psrr = (1 - l.e1) * l.e3 * (1 - g_p_source);
                  p.Psrs = (1 - l.e1) * l.e3 * g_p_source;
                  p.Psds = (1 - l.e3) * g_p_source;
                  p.Psdr = (1 - l.e3) * (1 - g_p_source);
                  assert (p.Psdr <= 1);
                  if (st.trans == RELAY_TRANS) snt.relay++;
                  if (st.trans == SOURCE_TRANS) snt.source++;
                  //                  PrintState (st);
                  MakeStep (policy, st, p, Gs, l, generator, distribution);
                }
            }
          double ave_trans = (double) (snt.relay + snt.source) / (double) num_it;
          double ave_trans_relay = (double) snt.relay / (double) num_it;
          double ave_trans_calc = GetTotalSentCalc (policy, Gs, l);
          double ave_trans_best_route_calc = GetTotalSentCalcBestRoute (Gs, l);
          double completition_time_gain = ave_trans_best_route_calc / ave_trans;
          double completition_time_gain_calc = ave_trans_best_route_calc / ave_trans_calc;

          cout << Gs << "\t" << e1 << "\t" << e2 << "\t" << e3 << "\t" << 0 << "\t" << ave_trans << "\t" << ave_trans_calc
                  << "\t" << ave_trans_relay << "\t" << ave_trans_best_route_calc << "\t" << (ave_trans - ave_trans_calc)
                  / ave_trans << "\t" << completition_time_gain << "\t" << completition_time_gain_calc << "\t" << GetBestRoute (
                  Gs, l) << endl;
          completion_gain.push_back (completition_time_gain);
          tot_compl_gain += completition_time_gain;
        }
      cout << endl;
    }
  //          cout << endl;
  //        }
  //    }
}
void
TestFilter ()
{
  //
  // Test binary filter
  //
  typedef AveBinaryFilter<160> filter;
  typedef std::shared_ptr<filter> filter_ptr;

  filter_ptr f = filter_ptr (new filter ());

  std::default_random_engine generator;
  std::binomial_distribution<bool> distribution (1, 0.5);

  uint32_t duration = std::numeric_limits<uint32_t>::max ();
  while (duration-- != 0)
    {
      bool val = distribution (generator);
      f->add (val);
      f->update ();
      std::cout << std::numeric_limits<uint32_t>::max () - duration << ": " << val << " -> " << f->val () << std::endl;
    }

  //  //
  //  // Test filter with floating point
  //  //
  //  typedef AveFloatingFilter filter;
  //  typedef std::shared_ptr<filter> filter_ptr;
  //
  //  filter_ptr f = filter_ptr (new filter (100));
  //
  //  std::default_random_engine generator;
  //  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  //
  //  uint32_t duration = std::numeric_limits<uint32_t>::max ();
  //  while (duration-- != 0)
  //    {
  //      double val = distribution (generator);
  //      f->add (val);
  //      std::cout << std::numeric_limits<uint32_t>::max () - duration << ": " << val << " -> " << f->val () << std::endl;
  //    }
}
void
approx_binom_with_normal ()
{
  double e1 = 0.7;
  double e2 = 0.2;
  double e3 = 0.8;
  uint16_t Gs = 100;
  double sum_bin = 0, sum_norm = 0;
  cout << "num" << "\t" << "BIN" << "\t" << "NORM" << endl;
  uint16_t n = round ((double) Gs / (1 - e1 * e3));

  is_binom_approx_normal_adequate (n, e1 * e3);
  vector<double> binom_pdf;

  for (uint16_t k = 0; k <= n; k++)
    {
      std::pair<long double, long double> res = binom_approx_normal (n, k, e1 * e3);
      binom_pdf.push_back (res.first);
      cout << k << "\t" << res.first << "\t" << res.second << endl;
      sum_bin += res.first;
      sum_norm += res.second;
    }
  //  std::vector<double> diff (binom_pdf.size ());
  //  double sum = std::accumulate (binom_pdf.begin (), binom_pdf.end (), 0.0);
  //  double mean = sum / binom_pdf.size ();
  //
  //  cout << "expected mean: " << n * (1 - e1 * e3) << ", got mean: " << mean << endl;
  //  transform (binom_pdf.begin (), binom_pdf.end (), diff.begin (), std::bind2nd (std::minus<double> (), mean));
  //  //  double sq_sum = std::inner_product (diff.begin (), diff.end (), diff.begin (), 0.0);
  //  //  double stdev = std::sqrt (sq_sum / v.size ());
  //  cout << "expected std: " << n * (1 - e1 * e3) << ", got mean: " << mean << endl;

  cout << "sum bin: " << sum_bin << ", sum norm: " << sum_norm << endl;
}
void
add_val (double lowest_ratio, double highest_ratio, uint16_t rcv_group, vector<double> &link)
{
  if (link.size () == rcv_group) return;
  double actual;
  if (link.empty ())
    actual = lowest_ratio;
  else if (link.size () == rcv_group - 1)
    actual = highest_ratio;
  else
    actual = (highest_ratio - lowest_ratio) / (double) (rcv_group - 1) * link.size () + lowest_ratio;

  link.push_back (actual);
  add_val (lowest_ratio, highest_ratio, rcv_group, link);
}
uint16_t
pfi (uint16_t m, double pfi_v)
{
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution (0.0, 1.0);
  uint16_t k = floor (m * pfi_v);
  double gen = distribution (generator);
  //  std::cout << k << "\t" << m * pfi_v << "\t" << pfi_v << "\t" << gen << std::endl;
  if ((m * pfi_v - (double) k > 0.0001) && gen < m * pfi_v - (double) k) k++;
  return k;
}
void
TestBrr ()
{
  uint32_t field_size = 64;
  uint32_t gen_size = 2;
  uint32_t num_it = 1000, num_it_c = num_it;
  uint16_t rcv_group = 4;

//  double lowest_loss_ratio = 0.2, highest_loss_ratio = 0.6;

  std::cout << "Group\tGenSize\tOverscore\tUnderscore" << std::endl;
  while (rcv_group != 10)
    {
      uint16_t src_id = 0, dst_id = rcv_group - 1;
//      vector<double> link_loss;
//      add_val (lowest_loss_ratio, highest_loss_ratio, rcv_group - 2, link_loss);
      vector<double> link_loss(rcv_group - 2, 0.4);

      //  double forward_ratio = 0.55;
      //  vector<double> link_forward (rcv_group - 2, forward_ratio);

      vector<double> link_forward;
      for (uint16_t i = 0; i < link_loss.size (); i++)
        {
          double prod = 1;
          for (uint16_t j = 0; j < i; j++)
            {
              prod *= link_loss.at (j);
            }
          link_forward.push_back (prod);
          //      std::cout << "Loss probability of relay " << i << ": " << link_loss.at (i) << std::endl;
          //      std::cout << "Forwarding probability of relay " << i << ": " << prod << std::endl;
        }

      while (gen_size != 512)
        {
          double underL = 0, overL = 0;
          while (num_it_c-- != 0)
            {
              uint16_t totalRcv = 0, totalSnt = 0, sntRank = 0;
              std::shared_ptr<CommNet> net = std::shared_ptr<CommNet> (new CommNet (rcv_group));
              auto link_loss_it = link_loss.begin();
              for (uint16_t i = 1; i < rcv_group - 1; i++)
                {
                  net->ConnectNodes (src_id, i, *link_loss_it, 1);
                  net->ConnectNodes (i, dst_id, 0.00001 * i, 1);
                  link_loss_it++;
                }
              net->SetSource (src_id);
              net->SetDestination (dst_id);
              net->SetNcVars (gen_size, field_size);
              for (uint16_t i = 0; i < rcv_group; i++)
                {
                  net->GetNode (i)->Init ();
                }

              //
              // source sends
              //
              for (uint16_t i = 0; i < gen_size; i++)
                {
                  net->DoBroadcast (src_id);
                }

              auto link_forward_it = link_forward.begin();
              for (uint16_t i = 1; i < rcv_group - 1; i++)
                {
                  totalRcv += net->GetNode (i)->GetUniqueDof ();
                  uint16_t k = pfi (net->GetNode (i)->GetDof (), *link_forward_it);
                  for (uint16_t j = 0; j < k; j++)
                    {
                      net->DoBroadcast (i);
                      totalSnt++;
                    }
                  link_forward_it++;
                }
              sntRank += net->GetNode (dst_id)->GetDof ();
              //                  std::cout << "Group received:\tGroup sent:\tGroup sent unique:" << std::endl;
              //            std::cout << totalRcv << "\t" << totalSnt << "\t" << sntRank << "\t" << std::endl;

              overL += totalSnt - sntRank;
              underL += totalRcv - sntRank;

              //      std::cout << "Overun:\tUnderscore:" << std::endl;
              //      std::cout << overL << "\t" << underL << std::endl;
            }
          overL /= (double) num_it;
          underL /= (double) num_it;

          std::cout << rcv_group - 2 << "\t" << gen_size << "\t" << overL / (double) gen_size * 100 << "\t" << underL
                  / (double) gen_size * 100 << std::endl;

          gen_size *= 2;
          num_it_c = num_it;
        }
      rcv_group++;
      gen_size = 2;
    }
}
