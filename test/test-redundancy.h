/*
 * test-ccack.h
 *
 *  Created on: 16.01.2017
 *      Author: tsokalo
 */

#ifndef TESTREDUNDANCY_H
#define TESTREDUNDANCY_H

#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <stdint.h>
#include <random>
#include <array>
#include <memory>

namespace ncr {

void TestRedundancy() {

	std::vector<double> eps = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5 };
	typedef std::pair<double, double> p_t;
	std::vector<p_t> devs = { p_t { 0.0, 0.5 }, p_t { 0.5, 0.6915 }, p_t { 1.0, 0.8413 }, p_t { 1.5, 0.9332 }, p_t { 2.0, 0.9773 }, p_t { 2.5, 0.9938 }, p_t {
			3.0, 0.9987 }, p_t { 3.5, 0.9997674 }, p_t { 4.0, 0.99996833 }, p_t { 4.5, 0.999996602 }, p_t { 4.9, 0.9999995208 } };
	double genSize = 32;

	auto calc_r = [&](double e, double dev)
	{
		double gamma = pow(dev, 2) * e / genSize / 4.0;
		gamma = (gamma > 1) ? 1 : gamma;
		double alpha = 1 + 2 * sqrt(gamma * (gamma + 1));

		return alpha / (1 - e);
	};

	for (auto e : eps) {
		for (auto dev : devs) {
			std::cout << std::setprecision(10) << e << "\t" << dev.second << "\t" << calc_r(e, dev.first) << std::endl;
		}
	}
}

}

#endif /* TESTREDUNDANCY_H */
