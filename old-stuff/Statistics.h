/*
 * statistics.h
 *
 *  Created on: Aug 9, 2016
 *      Author: tsokalo
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

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

#include <boost/dynamic_bitset.hpp>

#include <boost/math/distributions/chi_squared.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/algorithm/minmax_element.hpp>

#define NUM_BATCHES     20
//
// calculates mean and confidence interval using T-Student theorem and 95% confidence probability
//
std::pair<double, double>
CalcStats (std::vector<double> vals)
{
  uint16_t num_batches = NUM_BATCHES;
  std::size_t batch_size = floor ((double) vals.size () / (double) (num_batches + 1)), j = 0;
  std::vector<double> v;

  //
  // ignore the first batch as the warm up period
  //
  while (j++ < num_batches)
    {
      v.push_back (std::accumulate (vals.begin () + j * batch_size, vals.begin () + (j + 1) * batch_size, 0.0)
              / (double) batch_size);
    }

  double mean = std::accumulate (v.begin (), v.end (), 0.0) / v.size ();
  double stdev = std::sqrt (std::inner_product (v.begin (), v.end (), v.begin (), 0.0) / v.size () - mean * mean);

  using boost::math::quantile;
  using boost::math::complement;

  const double alpha = 1 - 0.95;
  const boost::math::students_t dist (num_batches - 1);

  double T = quantile (complement (dist, alpha / 2));
  //
  // Calculate width of interval (one sided)
  //
  double confInterval = T * stdev / sqrt ((double) num_batches);

  return std::pair<double, double> (mean, confInterval);
}
std::vector<double>
GetJitter (std::vector<double> latency)
{
  std::vector<double> jitter;
  double al = std::accumulate (latency.begin (), latency.end (), 0.0) / latency.size ();
  for(auto l : latency)jitter.push_back(fabs(l - al));
  return jitter;
}
std::vector<double>
GetDatarate (std::vector<double> iat, std::vector<double> data)
{
  assert(iat.size() == data.size());
  std::vector<double> datarate, siat, sdata;
  uint16_t num_batches = NUM_BATCHES;
  std::size_t batch_size = floor ((double) iat.size () / (double) (num_batches + 1)), j = 0;

  while (j++ < num_batches)
    {
      siat .push_back (std::accumulate (iat.begin () + j * batch_size, iat.begin () + (j + 1) * batch_size, 0.0));
      sdata.push_back (std::accumulate (data.begin () + j * batch_size, data.begin () + (j + 1) * batch_size, 0.0));
    }

  j = 0;

  while (j++ < num_batches)
    datarate.push_back (sdata.at (j) / siat.at (j));

  return datarate;
}

#endif /* STATISTICS_H_ */
