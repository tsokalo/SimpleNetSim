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
#include <array>
#include <memory>

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
	uint16_t num_batches = 20;
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
template<class T>
class NodeT {

	typedef std::shared_ptr<Ccack> ccack_ptr;

public:
	NodeT(uint16_t genSize, uint16_t symbolSize) :
			m_encFactory(genSize, symbolSize), m_decFactory(genSize, symbolSize) {
		m_genSize = genSize;
		m_symbolSize = symbolSize;

		m_hashMatrixSet = hash_matrix_set_ptr(new HashMatrixSet(2, genSize, 8));
		m_ccack = ccack_ptr(new Ccack(genSize, m_hashMatrixSet));
	}
	virtual ~NodeT() {
	}

	virtual std::vector<uint8_t> SendData() = 0;
	virtual void RcvData() = 0;
	virtual void RcvFeedback(CoderHelpInfo chi) = 0;
	virtual CoderHelpInfo SendFeedback() = 0;

	bool IsComplete()
	{
		return m_coder->rank() == m_genSize;
	}

protected:

	CodingMatrix GetCodingMatrix() {
		CodingMatrix m;
		CodingVector symbol_coefficients(m_genSize);
		for (uint16_t i = 0; i < m_coder->coefficient_vector_size(); i++) {
			uint8_t* coef_vector = m_coder->coefficient_vector_data(i);
			symbol_coefficients.assign(coef_vector, coef_vector + m_genSize);
			if (!(m_coder->is_symbol_uncoded(i) || m_coder->is_symbol_partially_decoded(i))) {
				for (auto &v : symbol_coefficients)
					v = 0;
			}
			m.push_back(symbol_coefficients);
			for (auto c : symbol_coefficients)
				std::cout << (uint16_t) c << "\t";
			std::cout << std::endl;
		}

		return m;
	}

	CodingVector GetCcackInfo() {
		return m_ccack->GetHashVector();
	}
	;

	CoderInfo GetCoderInfo() {
		DecodedMap dm;
		for (uint32_t i = 0; i < m_genSize; ++i)
			dm.push_back(m_coder->is_symbol_uncoded(i));
		SeenMap sm;
		for (uint32_t i = 0; i < m_genSize; ++i)
			sm.push_back(m_coder->is_symbol_pivot(i));
		return CoderInfo(m_coder->rank(), m_genSize, sm, dm);
	}
	;

	encoder::factory m_encFactory;
	decoder::factory m_decFactory;
	uint16_t m_genSize;
	uint16_t m_symbolSize;
	ccack_ptr m_ccack;
	std::vector<uint8_t> m_payload;
	T m_coder;

private:
	hash_matrix_set_ptr m_hashMatrixSet;
};

class SrcNode: public NodeT<encoder_ptr> {
public:
	SrcNode(uint16_t genSize, uint16_t symbolSize) :
			NodeT(genSize, symbolSize), m_coder(m_encFactory.build()), m_payload(m_coder->payload_size(), 0) {

		std::vector<uint8_t> data_in(m_symbolSize * m_genSize, 0);
		std::generate(data_in.begin(), data_in.end(), rand);
		m_coder->set_const_gen_size(storage::storage(data_in));
		kodo_core::set_systematic_off (*m_coder);
		m_sendToChannel = sendToChannel;
	}
	virtual ~SrcNode() {
	}

	std::vector<uint8_t> SendData() {
		m_coder->write_payload(m_payload.data());
		return m_payload;
	}
	void RcvData() {
		assert(0);
	}
	void RcvFeedback(CoderHelpInfo chi) {
		assert(0);
	}
	CoderHelpInfo SendFeedback() {
		assert(0);
	}
};

class RelayNode: public NodeT<decoder_ptr> {
	enum FeedbackStrategy {
		ALWAYS_SEND, NOT_MORE_THAN_RANK, SUBS_DECODED, SUBS_PIVOTS, MIN_MAX, CCACK, ALL_VECTORS
	};

public:
	RelayNode(uint16_t genSize, uint16_t symbolSize) :
			NodeT(genSize, symbolSize), m_coder(m_decFactory.build()), m_payload(m_coder->payload_size(), 0) {
		m_numToSend = 0;
		m_alreadySent = 0;
	}
	virtual ~RelayNode() {
	}

	void SetFeedbackStrategy(FeedbackStrategy fs) {
		m_feedbackStrategy = fs;
	}
	bool IsIdle() {
		return m_alreadySent >= m_numToSend;
	}
	std::vector<uint8_t> SendData() {
		m_coder->write_payload(m_payload.data());
		m_ccack->SaveSnt(ExtractCodingVector(m_payload, m_genSize));
		m_alreadySent++;
		return m_payload;
	}
	void RcvData(std::vector<uint8_t> payload) {
		auto rank_before = m_coder->rank();
		m_coder->read_payload(payload.data());
		auto rank_after = m_coder->rank();
		m_ccack->SaveRcv(ExtractCodingVector(payload, m_genSize));
		UpdateNumToSend(rank_after - rank_before);
	}
	void RcvFeedback(CoderHelpInfo chi) {

		auto own_chi = GetCoderHelpInfo();

		switch (m_feedbackStrategy) {
		case ALWAYS_SEND: {
			m_numToSend = -1;	// unlimited
			break;
		}
		case NOT_MORE_THAN_RANK: {
			m_numToSend = m_coder->rank();
			break;
		}
		case SUBS_DECODED: {
			auto rem = chi.c.decoded;
			auto loc = own_chi.c.decoded;
			assert(rem.size() == loc.size());
			uint16_t c = 0;
			for (auto i = 0; i < rem.size(); i++) {
				c = (rem.at(i) && loc.at(i)) ? 1 : 0;
			}
			assert(m_coder->rank() <= c);
			m_numToSend = m_coder->rank() - c;
			break;
		}
		case SUBS_PIVOTS: {
			auto rem = chi.c.seen;
			auto loc = own_chi.c.seen;
			assert(rem.size() == loc.size());
			assert(rem.size() == loc.size());
			uint16_t c = 0;
			for (auto i = 0; i < rem.size(); i++) {
				c = (rem.at(i) && loc.at(i)) ? 1 : 0;
			}
			assert(m_coder->rank() <= c);
			m_numToSend = m_coder->rank() - c;
			break;
		}
		case MIN_MAX: {
			auto rem = chi.c;
			auto loc = own_chi.c;
			FeedbackEstimator fs(loc, rem);
			m_numToSend = fs.GetN();
			break;
		}
		case CCACK: {
			m_ccack->RcvHashVector(chi.hashVec);
			auto c = m_ccack->GetHeardSymbNum();
			assert(m_coder->rank() <= c);
			m_numToSend = m_coder->rank() - c;
			break;
		}
		case ALL_VECTORS: {

			CodingMatrix m = chi.m;
			auto dec = m_decFactory.build();
			//
			// read all external coding coefficients
			//
			std::vector<uint8_t> fake_symbol(dec->symbol_size());
			for (auto s : m) {
				dec->read_symbol(fake_symbol.data(), s.data());
			}
			auto origRank = dec->rank();

			//
			// read all own coding coefficients
			//
			std::vector<uint8_t> s(m_genSize);
			auto dec_o = m_coder;

			for (uint16_t i = 0; i < dec_o->coefficient_vector_size(); i++) {
				uint8_t* coef_vector = dec_o->coefficient_vector_data(i);
				s.assign(coef_vector, coef_vector + m_genSize);
				dec->read_symbol(fake_symbol.data(), s.data());
			}
			auto finRank = dec->rank();

			assert(finRank >= origRank);
			m_numToSend = finRank - origRank;
		}
		default: {
			assert(0);
		}
		}
	}
	CoderHelpInfo SendFeedback() {
		assert(0);
	}

private:

	CoderHelpInfo GetCoderHelpInfo() {
		return CoderHelpInfo(GetCodingMatrix(), GetCcackInfo(), GetCoderInfo());
	}

	void UpdateNumToSend(uint16_t rank_diff) {
		switch (m_feedbackStrategy) {
		case ALWAYS_SEND: {
			m_numToSend = -1;	// unlimited
			break;
		}
		case NOT_MORE_THAN_RANK:
		case SUBS_DECODED:
		case SUBS_PIVOTS:
		case MIN_MAX:
		case CCACK: {
			m_numToSend += rank_diff;
			break;
		}
		case ALL_VECTORS: {

			// do nothing
			break;
		}
		default: {
			assert(0);
		}
		}
	}

	uint16_t m_numToSend;
	uint16_t m_alreadySent;
	FeedbackStrategy m_feedbackStrategy;
};

class DstNode: public NodeT<decoder_ptr> {
public:
	DstNode(uint16_t genSize, uint16_t symbolSize) :
			NodeT(genSize, symbolSize), m_coder(m_decFactory.build()), m_payload(m_coder->payload_size(), 0) {
	}
	virtual ~DstNode() {
	}

	void SendData() {
		assert(0);
	}

	void RcvData(std::vector<uint8_t> payload) {
		m_coder->read_payload(payload.data());
		m_ccack->SaveRcv(ExtractCodingVector(payload, m_genSize));
	}

	void RcvFeedback(CoderHelpInfo chi) {
		assert(0);
	}
	CoderHelpInfo SendFeedback() {
		CoderHelpInfo chi;
		chi.codingCoefs = GetCodingMatrix();
		chi.hashVector = GetCcackInfo();
		chi.coderInfo = GetCoderInfo();

		return chi;
	}
};

void TestFeedbackAccuracy() {
	std::random_device r;

	std::default_random_engine generator(r());
	std::uniform_real_distribution<double> distribution(0.0, 1.0);

	uint32_t gen_size = 64;
	uint32_t symbol_size = 5;

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create source node
	//

	auto src = std::shared_ptr<SrcNode>(new SrcNode(gen_size, symbol_size));

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create relays
	//
	std::vector<std::shared_ptr<RelayNode> > relays;
	uint16_t num_relays = 2;
	for (auto i = 0; i < num_relays; i++) {
		auto relay = std::shared_ptr<RelayNode>(new RelayNode(gen_size, symbol_size));
		relays.push_back(relay);
		relay->SetFeedbackStrategy(RelayNode::ALWAYS_SEND);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create destination
	//
	auto dst = std::shared_ptr<DstNode>(new DstNode(gen_size, symbol_size));

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create hyper-relay (virtual relay)
	//
	auto hrelay = std::shared_ptr<RelayNode>(new RelayNode(gen_size, symbol_size));

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create channel
	//
	std::vector<double> loss_ratio(num_relays, 0.5);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// define broadcast functions
	//
	auto do_broadcast_src = [&]()
	{
		auto payload = src->SendData();

		for(auto i = 0; i < relays.size(); i++)
		{
			if (distribution(generator) < loss_ratio.at(i))
			{
				relays.at(i)->RcvData(payload);
				hrelay->RcvData(payload);
			}
		}
	};

	auto do_broadcast_relay = [&](uint16_t i)
	{
		assert(i < relays.size());
		auto relay = relays.at(i);
		if(!relay->IsIdle())
		{
			auto payload = relay->SendData();
			dst->RcvData(payload);
		}
	};

	auto do_broadcast_dst = [&]()
	{
		auto feedback = dst->SendFeedback();
		for(auto relay : relays) relay->RcvFeedback(feedback);
	};

	enum SrcStrategy {
		SEND_EXACT_SUFFICIENT_STRATEGY, SEND_P_SUFFICIENT_STRATEGY
	};

	SrcStrategy srcS = SEND_EXACT_SUFFICIENT_STRATEGY;
	uint16_t snt_by_src = 0;
	//
	// source sends till the rest subnetwork gets sufficient packets to decode the complete generation
	//
	if (srcS == SEND_EXACT_SUFFICIENT_STRATEGY) {

		while (!hrelay->IsComplete()) {
			do_broadcast_src();
			snt_by_src++;
		}
	}

	//
	// source sends till the rest subnetwork gets sufficient packets to decode the complete generation with probability p
	//
	if (srcS == SEND_P_SUFFICIENT_STRATEGY) {
		double p = 0.99;

		auto get_suff_p_pkts = [&]()->int16_t
		{
			double eps = 1;
			for(auto i = 0; i < num_relays; i++)
			{
				eps *= loss_ratio.at(i);
			}
			double numStd = 3;
			double gamma = pow(numStd, 2) * eps / (double) gen_size / 4.0;
			gamma = (gamma > 1) ? 1 : gamma;
			double alpha = 1 + 2 * sqrt(gamma * (gamma + 1));

			double cr = alpha / (1 - eps);

			return (int16_t)(gen_size * (cr - 1));
		};

		auto n = get_suff_p_pkts();
		while (n-- > 0) {
			do_broadcast_src();
			snt_by_src++;
		}
	}

	std::cout << "SRC sent:\t" << snt_by_src << " symbols" << std::endl;

	enum DstStrategy {
		FEEDBACK_REGULAR, FEEDBACK_RANDOM
	};

	DstStrategy dstS = FEEDBACK_REGULAR;
	//
	// relays send coded packets and the destination send the feedbacks till the destination has the full rank
	//
	uint16_t kMax = 1;
	uint16_t k = 0;
	while (1) {
		if (k-- == 0) k = kMax;
		if (dstS == FEEDBACK_REGULAR) {

			std::default_random_engine gen(r());
			std::uniform_int_distribution<> dis(0, num_relays - 1);
			//
			// a randomly selected relay sends
			//
			auto select = [this]()
			{
				return dis(gen);
			};

			do_broadcast_relay(select());

			//
			// destination sends the feedback after each k packets sent by relays
			//
			if (k == 0) {
				do_broadcast_dst();
			}

			//
			// finish if the destination has the full rank
			//
			auto can_dst_decode = [&](CommNet *commnet)
			{
				return false;
			};
			if (can_dst_decode(this)) break;
		}
		if (dstS == FEEDBACK_RANDOM) {
			//
			// a randomly selected relay or the destination sends
			//
			auto select = [this]()
			{
				std::vector<uint16_t> v;
				uint16_t i = 0;

				for (std::vector<node_ptr>::iterator it = m_nodes.begin(); it != m_nodes.end(); it++) {
					auto id = (*it)->GetId();
					if(id != GetSrc()) v.push_back(std::distance(m_nodes.begin(), it));
				}

				std::uniform_int_distribution<> dis(0, v.size() - 1);
				return m_nodes.at(v.at(dis(m_gen)));
			};

			DoBroadcast(select());
			m_simulator->Execute();
			//
			// finish if the destination has the full rank
			//
			auto can_dst_decode = [&](CommNet *commnet)
			{
				return false;
			};
			if (can_dst_decode(this)) break;
		}
	}
}

void TestFeedbackEstimator() {
	std::random_device r;

	std::default_random_engine generator(r());
	std::uniform_real_distribution<double> distribution(0.0, 1.0);
	std::default_random_engine generator2(r());
	std::uniform_real_distribution<double> distribution2(0.0, 1.0);

	// Set the number of gen_size (i.e. the generation size in RLNC
	// terminology) and the size of a symbol in bytes
	uint32_t gen_size = 64;
	uint32_t symbol_size = 5;

	bool trace_enabled = true;

	// Typdefs for the encoder/decoder type we wish to use
	using rlnc_encoder = kodo_rlnc::full_vector_encoder<fifi_field>;
	using rlnc_decoder = kodo_rlnc::full_vector_decoder<fifi_field>;
	rlnc_encoder::factory m_encFactory(gen_size, symbol_size);
	rlnc_decoder::factory m_decFactory(gen_size, symbol_size);
	std::vector<uint8_t> data_in(symbol_size * gen_size);
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

			auto encoder = m_encFactory.build();
			auto decoder_local = m_decFactory.build();
			auto decoder_remote = m_decFactory.build();
			std::vector<uint8_t> payload(encoder->payload_size());
			std::vector<uint8_t> payload(encoder->payload_size());
			encoder->set_const_gen_size(storage::storage(data_in));
			kodo_core::set_systematic_off(*encoder);

			//
			// source works
			//

			for (uint16_t i = 0; i < gen_size; i++) {
				encoder->write_payload(payload.data());
				if (distribution(generator) < e) decoder_local->read_payload(payload.data());
				if (distribution2(generator2) < e) decoder_remote->read_payload(payload.data());
			}

			//
			// calculating limits
			//

			DecodedMap dm1, dm2;
			for (uint32_t i = 0; i < decoder_local->gen_size(); ++i)
				dm1.push_back(decoder_local->is_symbol_uncoded(i));
			for (uint32_t i = 0; i < decoder_remote->gen_size(); ++i)
				dm2.push_back(decoder_remote->is_symbol_uncoded(i));
			SeenMap sm1, sm2;
			for (uint32_t i = 0; i < decoder_local->gen_size(); ++i)
				sm1.push_back(decoder_local->is_symbol_pivot(i));
			for (uint32_t i = 0; i < decoder_remote->gen_size(); ++i)
				sm2.push_back(decoder_remote->is_symbol_pivot(i));

			CoderInfo group_local(decoder_local->rank(), decoder_local->gen_size(), sm1, dm1);
			CoderInfo group_remote(decoder_remote->rank(), decoder_remote->gen_size(), sm2, dm2);

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
