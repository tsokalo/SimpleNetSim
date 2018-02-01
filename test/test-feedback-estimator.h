/*
 * test-feedback-estimator.h
 *
 *  Created on: 18.01.2017
 *      Author: tsokalo
 */

#ifndef TEST_TEST_FEEDBACK_ESTIMATOR_H_
#define TEST_TEST_FEEDBACK_ESTIMATOR_H_

// Copyright Steinwurf ApS 2014.
// Distributed under the "STEINWURF RESEARCH LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <stdint.h>
#include <random>

#include <boost/math/distributions/chi_squared.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/algorithm/minmax_element.hpp>

#include "utils/coder.h"
#include "utils/feedback-estimator.h"

namespace ncr {

//
// calculates mean and confidence interval using T-Student theorem and 95% confidence probability
//
std::pair<double, double> CalcStats(std::vector<double> vals) {
	uint16_t num_batches = 15;
	std::size_t batch_size = floor((double) vals.size() / (double) (num_batches + 1)), j = 0;
	std::vector<double> v;

	//
	// ignore the first batch as the warm up period
	//
	while (j++ < num_batches) {
		v.push_back(std::accumulate(vals.begin() + j * batch_size, vals.begin() + (j + 1) * batch_size, 0.0) / (double) batch_size);
	}

	double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
	double stdev = std::sqrt(std::inner_product(v.begin(), v.end(), v.begin(), 0.0) / v.size() - mean * mean);

	using boost::math::quantile;
	using boost::math::complement;

	const double alpha = 1 - 0.95;
	const boost::math::students_t dist(num_batches - 1);

	double T = quantile(complement(dist, alpha / 2));
	//
	// Calculate width of interval (one sided)
	//
	double confInterval = T * stdev / sqrt((double) num_batches);

	return std::pair<double, double>(mean, confInterval);
}


void TestFeedbackEstimator() {
	std::random_device r;

	std::default_random_engine generator(r());
	std::uniform_real_distribution<double> distribution(0.0, 1.0);
	std::default_random_engine generator2(r());
	std::uniform_real_distribution<double> distribution2(0.0, 1.0);

	// Set the number of symbols (i.e. the generation size in RLNC
	// terminology) and the size of a symbol in bytes
	uint32_t symbols = 64;
	uint32_t symbol_size = 5;

	// Typdefs for the encoder/decoder type we wish to use
	using rlnc_encoder = kodo_rlnc::full_vector_encoder<fifi_field>;
	using rlnc_decoder = kodo_rlnc::full_vector_decoder<fifi_field>;
	rlnc_encoder::factory encoder_factory(symbols, symbol_size);
	rlnc_decoder::factory decoder_factory(symbols, symbol_size);
	std::vector<uint8_t> data_in(symbol_size * symbols);
	std::generate(data_in.begin(), data_in.end(), rand);

	uint16_t e_levels = 10;
	for (uint16_t ei = 0; ei < e_levels; ei++) {
		double e = (ei + 1) / (double) (e_levels + 1);
		uint32_t num_iter = 1000;

		std::vector<double> lb, tb, cv, rv, pv;
		while (num_iter-- != 0) {
			//
			// Initialization
			//

			auto encoder = encoder_factory.build();
			auto decoder_local = decoder_factory.build();
			auto decoder_remote = decoder_factory.build();
			std::vector<uint8_t> payload(encoder->payload_size());
			encoder->set_const_symbols(storage::storage(data_in));
			kodo_core::set_systematic_off(*encoder);

			//
			// source works
			//

			for (uint16_t i = 0; i < symbols; i++) {
				encoder->write_payload(payload.data());
				if (distribution(generator) < e) decoder_local->read_payload(payload.data());
				if (distribution2(generator2) < e) decoder_remote->read_payload(payload.data());
			}

			//
			// calculating limits
			//

			DecodedMap dm1, dm2;
			for (uint32_t i = 0; i < decoder_local->symbols(); ++i)
				dm1.push_back(decoder_local->is_symbol_uncoded(i));
			for (uint32_t i = 0; i < decoder_remote->symbols(); ++i)
				dm2.push_back(decoder_remote->is_symbol_uncoded(i));
			SeenMap sm1, sm2;
			for (uint32_t i = 0; i < decoder_local->symbols(); ++i)
				sm1.push_back(decoder_local->is_symbol_pivot(i));
			for (uint32_t i = 0; i < decoder_remote->symbols(); ++i)
				sm2.push_back(decoder_remote->is_symbol_pivot(i));

			CoderInfo group_local(decoder_local->rank(), decoder_local->symbols(), sm1, dm1);
			CoderInfo group_remote(decoder_remote->rank(), decoder_remote->symbols(), sm2, dm2);

			FeedbackEstimator est(group_local, group_remote);

			//
			// find real value
			//

			uint16_t n_real = decoder_remote->rank();
			uint16_t ncr = 100;
			while (ncr != 0) {
				uint16_t temp = decoder_remote->rank();
				decoder_local->write_payload(payload.data());
				decoder_remote->read_payload(payload.data());
				if (temp == decoder_remote->rank()) ncr--;
			}
			n_real = decoder_remote->rank() - n_real;

			lb.push_back(est.GetLowBound());
			tb.push_back(est.GetTopBound());
			cv.push_back(est.GetN());
			rv.push_back(n_real);
			pv.push_back(est.GetP());
		}

		auto lb_a = CalcStats(lb);
		auto tb_a = CalcStats(tb);
		auto cv_a = CalcStats(cv);
		auto rv_a = CalcStats(rv);
		auto pv_a = CalcStats(pv);

		std::cout << e << "\t" << lb_a.first << "\t" << lb_a.second << "\t" << tb_a.first << "\t" << tb_a.second << "\t" << cv_a.first << "\t" << cv_a.second
				<< "\t" << rv_a.first << "\t" << rv_a.second << "\t" << pv_a.first << "\t" << pv_a.second << std::endl;
	}

}

}
#endif /* TEST_TEST_FEEDBACK_ESTIMATOR_H_ */
