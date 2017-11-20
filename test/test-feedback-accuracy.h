/*
 * test-feedback-estimator.h
 *
 *  Created on: 18.01.2017
 *      Author: tsokalo
 */

#ifndef TEST_FEEDBACK_ACCURACY_H_
#define TEST_FEEDBACK_ACCURACY_H_

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
#include "utils/log.h"
#include "utils/feedback-estimator.h"

namespace ncr {

template<class T>
class NodeObject {

	typedef std::shared_ptr<Ccack> ccack_ptr;

public:
	NodeObject(uint16_t genSize, uint16_t symbolSize) :
			m_encFactory(genSize, symbolSize), m_decFactory(genSize, symbolSize) {
		m_genSize = genSize;
		m_symbolSize = symbolSize;

		m_hashMatrixSet = hash_matrix_set_ptr(new HashMatrixSet(2, genSize, 8));
		m_ccack = ccack_ptr(new Ccack(genSize, m_hashMatrixSet));
	}
	virtual ~NodeObject() {
	}

	virtual std::vector<uint8_t> SendData() = 0;
	virtual void RcvData(std::vector<uint8_t> payload) = 0;
	virtual void RcvFeedback(CoderHelpInfo chi) = 0;
	virtual CoderHelpInfo SendFeedback() = 0;

	bool IsComplete() {
		return m_coder->rank() == m_genSize;
	}
	uint16_t Rank() {
		return m_coder->rank();
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
//			for (auto c : symbol_coefficients)
//				std::cout << (uint16_t) c << "\t";
//			std::cout << std::endl;
		}

		return m;
	}

	CodingVector GetCcackInfo() {
		return m_ccack->GetHashVector();
	}

	CoderInfo GetCoderInfo() {
		DecodedMap dm;
		for (uint32_t i = 0; i < m_genSize; ++i)
			dm.push_back(m_coder->is_symbol_uncoded(i));
		SeenMap sm;
		for (uint32_t i = 0; i < m_genSize; ++i)
			sm.push_back(m_coder->is_symbol_pivot(i));
		return CoderInfo(m_coder->rank(), m_genSize, sm, dm);
	}

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

class SrcNode: public NodeObject<encoder_ptr> {
public:

	enum Strategy {
		SEND_EXACT_SUFFICIENT_STRATEGY, SEND_P_SUFFICIENT_STRATEGY
	};

	SrcNode(uint16_t genSize, uint16_t symbolSize) :
			NodeObject(genSize, symbolSize) {

		m_coder = m_encFactory.build();
		m_payload.resize(m_coder->payload_size());
		kodo_core::set_systematic_off (*m_coder);

		// Allocate some data to encode. In this case we make a buffer
		// with the same size as the encoder's block size (the max.
		// amount a single encoder can encode)
		std::vector<uint8_t> data_in(m_coder->block_size());

		// Just for fun - fill the data with random data
		std::generate(data_in.begin(), data_in.end(), rand);

		m_coder->set_const_symbols(storage::storage(data_in));
	}
	virtual ~SrcNode() {
	}

	std::vector<uint8_t> SendData() {
		m_coder->write_payload(m_payload.data());
		return m_payload;
	}
	void RcvData(std::vector<uint8_t> payload) {
		assert(0);
	}
	void RcvFeedback(CoderHelpInfo chi) {
		assert(0);
	}
	CoderHelpInfo SendFeedback() {
		assert(0);
		return CoderHelpInfo();
	}
};

class RelayNode: public NodeObject<decoder_ptr> {

public:

	enum Strategy {
		ALWAYS_SEND, NOT_MORE_THAN_RANK, SUBS_DECODED, SUBS_PIVOTS, MIN_MAX, CCACK, ALL_VECTORS
	};

	RelayNode(uint16_t genSize, uint16_t symbolSize) :
			NodeObject(genSize, symbolSize) {

		m_coder = m_decFactory.build();
		m_payload.resize(m_coder->payload_size());
		m_numToSend = 0;
		m_alreadySent = 0;
		m_id = 0;
	}
	virtual ~RelayNode() {
	}

	void SetStrategy(Strategy fs) {
		m_feedbackStrategy = fs;
	}
	void SetId(uint16_t id) {
		m_id = id;
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

		SIM_LOG(TEST_LOG, "Relay " << m_id << ": " << m_coder->rank() << " / " << m_numToSend << " / " << m_alreadySent);

	}
	CoderHelpInfo SendFeedback() {
		assert(0);
	}

private:

	CoderHelpInfo GetCoderHelpInfo() {
		return CoderHelpInfo(GetCodingMatrix(), GetCoderInfo(), GetCcackInfo());
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
		SIM_LOG(TEST_LOG, "Relay " << m_id << ": " << m_coder->rank() << " / " << m_numToSend << " / " << m_alreadySent);
	}

	uint16_t m_numToSend;
	uint16_t m_alreadySent;
	uint16_t m_id;
	Strategy m_feedbackStrategy;
};

class DstNode: public NodeObject<decoder_ptr> {
public:

	enum Strategy {
		FEEDBACK_REGULAR, FEEDBACK_RANDOM
	};

	DstNode(uint16_t genSize, uint16_t symbolSize) :
			NodeObject(genSize, symbolSize) {

		m_coder = m_decFactory.build();
		m_payload.resize(m_coder->payload_size());
	}
	virtual ~DstNode() {
	}

	std::vector<uint8_t> SendData() {
		assert(0);
		return std::vector<uint8_t>();
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
		chi.m = GetCodingMatrix();
		chi.hashVec = GetCcackInfo();
		chi.c = GetCoderInfo();

		return chi;
	}
};

struct FALogItem {

	RelayNode::Strategy relayStrategy;
	SrcNode::Strategy srcStrategy;
	DstNode::Strategy dstStrategy;

	uint16_t genSize;

	uint32_t sntBySrc;
	uint32_t sntByRel;
	uint32_t sntByDst;

	uint16_t aRank; // achievable rank of the hyper-relay

	friend std::ostream& operator<<(std::ostream& o, FALogItem& m) {

		o << (uint16_t) m.relayStrategy << "\t" << (uint16_t) m.srcStrategy << "\t" << (uint16_t) m.dstStrategy << "\t" << m.genSize << "\t" << m.sntBySrc
				<< "\t" << m.sntByRel << "\t" << m.sntByDst << "\t" << m.aRank;
		return o;
	}
};

struct FAConfig {
	uint16_t numRelays;
	std::vector<double> lossRatios;
	uint32_t genSize;
	uint32_t symSize;
	SrcNode::Strategy srcStrategy;
	double numStd;
	RelayNode::Strategy relayStrategy;
	DstNode::Strategy dstStrategy;
	uint16_t kMax;
};

FALogItem RunFATest(FAConfig fac) {
	std::random_device r;

	std::default_random_engine generator(r());
	std::uniform_real_distribution<double> distribution(0.0, 1.0);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create source node
	//
	auto src = std::shared_ptr<SrcNode>(new SrcNode(fac.genSize, fac.symSize));

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create relays
	//
	std::vector<std::shared_ptr<RelayNode> > relays;
	uint16_t num_relays = 2;
	for (auto i = 0; i < num_relays; i++) {
		auto relay = std::shared_ptr<RelayNode>(new RelayNode(fac.genSize, fac.symSize));
		relays.push_back(relay);
		relay->SetStrategy(fac.relayStrategy);
		relay->SetId(i);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create destination
	//
	auto dst = std::shared_ptr<DstNode>(new DstNode(fac.genSize, fac.symSize));

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create hyper-relay (virtual relay)
	//
	auto hrelay = std::shared_ptr<RelayNode>(new RelayNode(fac.genSize, fac.symSize));
	hrelay->SetStrategy(fac.relayStrategy);
	hrelay->SetId(777);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create channel
	//
	std::vector<double> loss_ratio(num_relays, 0.5);

	SIM_LOG(TEST_LOG, "Created network..");

	////////////////////////////////////////////////////////////////////////////////////////////////
	// define broadcast functions
	//
	uint32_t snt_by_src = 0, snt_by_rel = 0, snt_by_dst = 0;

	auto do_broadcast_src = [&]()
	{
		auto payload = src->SendData();
		snt_by_src++;

		for(auto i = 0; i < relays.size(); i++)
		{
			if (distribution(generator) < loss_ratio.at(i))
			{
				relays.at(i)->RcvData(payload);
				hrelay->RcvData(payload);
			}
		}
	};

	auto do_broadcast_relay = [&](uint16_t i)->bool
	{
		assert(i < relays.size());
		auto relay = relays.at(i);
		if(!relay->IsIdle())
		{
			SIM_LOG(TEST_LOG, "Relay " << i << " sends");
			auto payload = relay->SendData();
			snt_by_rel++;

			dst->RcvData(payload);
			return true;
		}
		return false;
	};

	auto do_broadcast_dst = [&]()
	{
		SIM_LOG(TEST_LOG, "Destination sends");
		auto feedback = dst->SendFeedback();
		snt_by_dst++;
		for(auto relay : relays) relay->RcvFeedback(feedback);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////		Simulation start		///////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	//
	// source sends till the rest subnetwork gets sufficient packets to decode the complete generation
	//
	if (fac.srcStrategy == SrcNode::SEND_EXACT_SUFFICIENT_STRATEGY) {

		while (!hrelay->IsComplete()) {
			do_broadcast_src();
		}
	}

	//
	// source sends till the rest subnetwork gets sufficient packets to decode the complete generation with probability p
	//
	if (fac.srcStrategy == SrcNode::SEND_P_SUFFICIENT_STRATEGY) {
		double p = 0.99;

		auto get_suff_p_pkts = [&]()->int16_t
		{
			double eps = 1;
			for(auto i = 0; i < num_relays; i++)
			{
				eps *= loss_ratio.at(i);
			}

			double gamma = pow(fac.numStd, 2) * eps / (double) fac.genSize / 4.0;
			gamma = (gamma > 1) ? 1 : gamma;
			double alpha = 1 + 2 * sqrt(gamma * (gamma + 1));

			double cr = alpha / (1 - eps);

			SIM_LOG(TEST_LOG, "Using coding rate: " << cr);

			return (int16_t)(fac.genSize * cr);
		};

		auto n = get_suff_p_pkts();
		while (n-- > 0) {
			do_broadcast_src();
		}
	}

	//
	// relays send coded packets and the destination send the feedbacks till the destination has the full rank
	//
	uint16_t k = fac.kMax;
	std::default_random_engine gen(r());
	std::uniform_int_distribution<> dis_without_dst(0, num_relays - 1);
	std::uniform_int_distribution<> dis_with_dst(0, num_relays);
	while (1) {

		if (fac.dstStrategy == DstNode::FEEDBACK_REGULAR) {

			//
			// a randomly selected relay sends
			//
			if (!do_broadcast_relay(dis_without_dst(gen))) continue;

			//
			// destination sends the feedback after each k packets sent by relays
			//
			if (--k == 0) {
				do_broadcast_dst();
				k = fac.kMax;
			}

			//
			// if source adds not sufficient redundancy then the destination cannot get the full rank
			// finish when the relay group has transferred all information that it could
			//
			SIM_LOG(TEST_LOG, "DST rank: " << dst->Rank());
			if (dst->Rank() == hrelay->Rank()) break;
		}
		if (fac.dstStrategy == DstNode::FEEDBACK_RANDOM) {
			//
			// a randomly selected relay or the destination sends
			//
			auto id = dis_with_dst(gen);
			if (id == num_relays) do_broadcast_dst();
			else do_broadcast_relay(id);

			//
			// if source adds not sufficient redundancy then the destination cannot get the full rank
			// finish when the relay group has transferred all information that it could
			//
			if (dst->Rank() == hrelay->Rank()) break;
		}
	}
	SIM_LOG(TEST_LOG, "SRC sent:\t" << snt_by_src << " symbols");
	SIM_LOG(TEST_LOG, "REL sent:\t" << snt_by_rel << " symbols");
	SIM_LOG(TEST_LOG, "DST sent:\t" << snt_by_dst << " symbols");
	SIM_LOG(TEST_LOG, "DST A-rank:\t" << hrelay->Rank());			// achievable ranks of destination

	FALogItem fali;
	fali.srcStrategy = fac.srcStrategy;
	fali.relayStrategy = fac.relayStrategy;
	fali.dstStrategy = fac.dstStrategy;

	fali.genSize = fac.genSize;

	fali.sntByDst = snt_by_dst;
	fali.sntByRel = snt_by_rel;
	fali.sntBySrc = snt_by_src;

	fali.aRank = hrelay->Rank();

	return fali;
}

struct FAEvalLog {
	double macEff;
};

void TestFeedbackAccuracy() {

	FAConfig fac;
	fac.numRelays = 2;
	for (auto i = 0; i < fac.numRelays; i++) {
		fac.lossRatios.push_back(0.5 / (double) (i + 1));
	}
	fac.genSize = 8;
	fac.symSize = 1;
	fac.srcStrategy = SrcNode::SEND_P_SUFFICIENT_STRATEGY;
	fac.relayStrategy = RelayNode::NOT_MORE_THAN_RANK;
	fac.dstStrategy = DstNode::FEEDBACK_REGULAR;
	fac.numStd = 0;
	fac.kMax = 1;

	std::vector<uint16_t> genSizes = { 8, 16, 32 };
	std::vector<RelayNode::Strategy> relStrategies = { RelayNode::ALWAYS_SEND, RelayNode::NOT_MORE_THAN_RANK };

	for (auto genSize : genSizes) {
		for (auto relStrategy : relStrategies) {
			std::vector<uint16_t> kMaxs = { 1, 2 };
			kMaxs.push_back(genSize);
			for (auto kMax : kMaxs) {
				fac.genSize = genSize;
				fac.relayStrategy = relStrategy;
				fac.kMax = kMax;

				uint32_t num_iter = 100, c = 0;

				while (c++ != num_iter) {
					auto log = RunFATest(fac);
					// simulation for number of iterations 100, generation size {8,16,32}, relay strategy {0,1}, frequency of feedback {1,2, generation size}
					// relay strategy: {0 - always send; 1 - send not more than the rank}
					// line format: iteration number|frequency of feedback|relay strategy|source strategy|destination strategy|generation size|sent by source|sent by relays|sent by destination|destination achievable rank
					std::cout << c << "\t" << fac.kMax << "\t" << log << std::endl;
				}
			}
		}
	}

}
}
#endif /* TEST_FEEDBACK_ACCURACY_H_ */
