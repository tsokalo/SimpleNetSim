/*
 * test-feedback-estimator.h
 *
 *  Created on: 18.01.2017
 *      Author: tsokalo
 */

#ifndef TEST_FEEDBACK_ACCURACY_TWO_H_
#define TEST_FEEDBACK_ACCURACY_TWO_H_

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

///
///												|
///	<------------- Configuration --------------> <--------------Test--------------->
///												|
///         +-----------+      +-----------+      +------------+	  +------------+
///         |  encoder  |+---->| decoder   |+---->|  decoder_1 |+-+-->|  decoder_x |
///         +-----------+      +-----------+      +------------+  |   +------------+
///                                  | 			|				  |
///									 |			  +------------+  |   +------------+
///									 +----------->|  decoder_2 |+-+-->|  decoder_y |
///												  +------------+  |  +------------+
///												|		  		  |
///														  		  |	  +------------+
///														 		  +-->|  decoder_z |
///												|					  +------------+
///
///
///												|

namespace ncr {

enum RelayStrategy {
	EXPECTATION, NUMBER_RCVD, PIVOTS, MIN_MAX, CCACK, ALL_VECTORS
};

struct FAConfig {
	uint16_t numRelays; // decoder x,y,z,...
	double lossRatio; // from decoder 1,2 to the group x,y,z together
	uint32_t genSize;
	uint32_t symSize;
	double numStd;
	RelayStrategy relayStrategy;
	uint16_t feedbackF; // number of received data packets per one sent feedback
	uint16_t vDof; // number of DOF conveyed between encoder and decoder

	FAConfig& operator=(const FAConfig& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->numRelays = other.numRelays;
			this->lossRatio = other.lossRatio;
			this->genSize = other.genSize;
			this->symSize = other.symSize;
			this->numStd = other.numStd;
			this->relayStrategy = other.relayStrategy;
			this->feedbackF = other.feedbackF;
			this->vDof = other.vDof;
		}
		return *this;
	}

	friend std::ostream& operator<<(std::ostream& o, FAConfig& m) {

		o << m.numRelays << "\t" << m.lossRatio << "\t" << m.genSize << "\t" << m.symSize << "\t" << m.numStd << "\t" << (uint16_t) m.relayStrategy << "\t"
				<< m.feedbackF << "\t" << m.vDof;
		return o;
	}
};

template<class T>
class NodeObject {

	typedef std::shared_ptr<Ccack> ccack_ptr;

public:
	NodeObject(FAConfig fac) :
			m_encFactory(fac.genSize, fac.symSize), m_decFactory(fac.genSize, fac.symSize) {
		m_fac = fac;

		m_hashMatrixSet = hash_matrix_set_ptr(new HashMatrixSet(2, m_fac.genSize, 8));
		m_ccack = ccack_ptr(new Ccack(m_fac.genSize, m_hashMatrixSet));
		m_id = 0;
	}
	virtual ~NodeObject() {
	}

	virtual std::vector<uint8_t> SendData(bool force = true) = 0;
	virtual void RcvData(std::vector<uint8_t> payload, uint16_t id = 0) = 0;
	virtual void RcvFeedback(CoderHelpInfo chi, uint16_t id) = 0;
	virtual CoderHelpInfo SendFeedback() = 0;

	bool IsComplete() {
		return m_coder->rank() == m_fac.genSize;
	}
	uint16_t Rank() {
		return m_coder->rank();
	}
	void SetId(uint16_t id) {
		m_id = id;
	}
	uint16_t GetId() {
		return m_id;
	}

protected:

	CodingMatrix GetCodingMatrix() {
		CodingMatrix m;
		CodingVector symbol_coefficients(m_fac.genSize);
		for (uint16_t i = 0; i < m_coder->coefficient_vector_size(); i++) {
			uint8_t* coef_vector = m_coder->coefficient_vector_data(i);
			symbol_coefficients.assign(coef_vector, coef_vector + m_fac.genSize);
			if (!(m_coder->is_symbol_uncoded(i) || m_coder->is_symbol_partially_decoded(i))) {
				for (auto &v : symbol_coefficients)
					v = 0;
			}
			m.push_back(symbol_coefficients);
		}

		return m;
	}

	CodingVector GetCcackInfo() {
		return m_ccack->GetHashVector();
	}

	CoderInfo GetCoderInfo() {
		DecodedMap dm;
		for (uint32_t i = 0; i < m_fac.genSize; ++i)
			dm.push_back(m_coder->is_symbol_uncoded(i));
		SeenMap sm;
		for (uint32_t i = 0; i < m_fac.genSize; ++i)
			sm.push_back(m_coder->is_symbol_pivot(i));
		return CoderInfo(m_coder->rank(), m_fac.genSize, sm, dm);
	}

	encoder::factory m_encFactory;
	decoder::factory m_decFactory;
	FAConfig m_fac;
	ccack_ptr m_ccack;
//	std::map<uint16_t, ccack_ptr> m_ccacks;// for CCACK_SEPARATE
	std::vector<uint8_t> m_payload;
	T m_coder;
	uint16_t m_id;

private:
	hash_matrix_set_ptr m_hashMatrixSet;
};

class SrcNode: public NodeObject<encoder_ptr> {
public:

	SrcNode(FAConfig fac) :
			NodeObject(fac) {

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

	std::vector<uint8_t> SendData(bool force = true) {
		m_coder->write_payload(m_payload.data());
		return m_payload;
	}
	void RcvData(std::vector<uint8_t> payload, uint16_t id = 0) {
		assert(0);
	}
	void RcvFeedback(CoderHelpInfo chi, uint16_t id) {
		assert(0);
	}
	CoderHelpInfo SendFeedback() {
		assert(0);
		return CoderHelpInfo();
	}
};

class RelayNode: public NodeObject<decoder_ptr> {

public:

	enum StrategyOption {
		NO_OPTION, NO_FEEDBACK, NO_DATA
	};

	RelayNode(FAConfig fac) :
			NodeObject(fac) {

		m_coder = m_decFactory.build();
		m_payload.resize(m_coder->payload_size());
		m_numToSend = 0;
		m_numForward = 0;
		m_aNumForward = 0;
		m_strategy = fac.relayStrategy;
		m_strategyOption = NO_OPTION;
		m_feedbackTimer = fac.feedbackF;

		double eps = m_fac.lossRatio;

		double gamma = pow(m_fac.numStd, 2) * eps / (double) m_fac.genSize / 4.0;
		gamma = (gamma > 1) ? 1 : gamma;
		double alpha = 1 + 2 * sqrt(gamma * (gamma + 1));

		assert(eps != 1);
		m_cr = alpha / (1 - eps);

		SIM_LOG(TEST_LOG, "Using coding rate: " << m_cr);

	}
	virtual ~RelayNode() {
	}

	void SetStrategy(RelayStrategy fs) {
		m_strategy = fs;
	}
	void SetStrategyOption(StrategyOption so) {
		m_strategyOption = so;
	}
	bool HasData() {

		if(m_id == 2)return false;

		if (m_strategyOption == NO_DATA) return false;
		return (m_numToSend > 0);
	}
	bool HasFeedback() {
		if (m_strategyOption == NO_FEEDBACK || m_strategy == EXPECTATION) return false;
		return (m_feedbackTimer == 0);
	}
	std::vector<uint8_t> SendData(bool force = false) {

		SIM_LOG(TEST_LOG, "Relay " << m_id << " remains to send " << m_numToSend);

		m_coder->write_payload(m_payload.data());
		m_ccack->SaveSnt(ExtractCodingVector(m_payload, m_fac.genSize));
		if (!force) {
			assert(m_numToSend > 0);
			m_numToSend--;
		}
		return m_payload;
	}
	void RcvData(std::vector<uint8_t> payload, uint16_t id = 0) {

		m_coder->read_payload(payload.data());
		m_ccack->SaveRcv(ExtractCodingVector(payload, m_fac.genSize));
		m_feedbackTimer = (m_feedbackTimer == 0) ? 0 : m_feedbackTimer - 1;
		rcvd[id]++;

		SIM_LOG(TEST_LOG, "Relay " << m_id << " rank " << Rank());
	}
	//
	// the node does not decide itself when the received data should be forwarded
	// the central entity takes over this job
	//
	void IncForward() {

		assert(m_strategyOption != NO_DATA);

		m_numForward++;
		m_aNumForward = m_numForward;
		UpdateNumToSend();
	}
	uint16_t GetNumForward() {
		return m_numForward;
	}
	void RcvFeedback(CoderHelpInfo chi, uint16_t id) {

		SIM_LOG(TEST_LOG, "Relay " << m_id);

		assert(m_strategyOption != NO_DATA);
		assert(m_strategy != EXPECTATION);

		m_feedbacks[id] = chi;
		UpdateActToForward();
	}
	CoderHelpInfo SendFeedback() {

		SIM_LOG(TEST_LOG, "Relay " << m_id);

		CoderHelpInfo chi;
		chi.m = GetCodingMatrix();
		chi.hashVec = GetCcackInfo();
		chi.c = GetCoderInfo();
		chi.rcvd = rcvd;
		m_feedbackTimer = m_fac.feedbackF;

		return chi;
	}

private:

	CoderHelpInfo GetCoderHelpInfo() {
		return CoderHelpInfo(GetCodingMatrix(), GetCoderInfo(), GetCcackInfo());
	}

	void UpdateActToForward() {

		if (m_feedbacks.size() < m_fac.numRelays && m_aNumForward == 0) {
			m_aNumForward = m_numForward;
			return;
		}

		auto merge_maps = [](BooleanVector &m1, BooleanVector m2)
		{
			assert(m1.size() == m2.size());
			for(uint16_t i = 0; i < m1.size(); i++)
			{
				if(m1.at(i) || m2.at(i))m1.at(i) = true;
			}
		};

		auto get_merged_seen = [this, &merge_maps]()
		{
			SeenMap mutual(m_fac.genSize, 0);
			for (auto it : m_feedbacks) {
				auto t = it.second.c.seen;
				merge_maps(mutual, t);
			}
			return mutual;
		};
		auto get_merged_decoded = [this, &merge_maps]()
		{
			SeenMap mutual(m_fac.genSize, 0);
			for (auto it : m_feedbacks) {
				auto t = it.second.c.decoded;
				merge_maps(mutual, t);
			}
			return mutual;
		};
		auto get_merged_rank = [](SeenMap mutual)
		{
			uint16_t rank = 0;
			for(auto m : mutual)if(m)rank++;
			return rank;
		};

		auto own_chi = GetCoderHelpInfo();

		switch (m_strategy) {
		case EXPECTATION: {

			// do nothing
			break;
		}
		case NUMBER_RCVD: {

			m_aNumForward = 0;
			auto e = pow(m_fac.lossRatio, 1 / (double) m_fac.numRelays);
			auto expectedToRcv = ceil(m_numForward * m_cr * (1 - e));
			for (auto it : m_feedbacks) {
				auto s = (it.second.rcvd[m_id] > expectedToRcv) ? 0 : expectedToRcv - it.second.rcvd[m_id];
				m_aNumForward = (s > m_aNumForward) ? s : m_aNumForward;
			}

			SIM_LOG(TEST_LOG, "Relay " << m_id << " expected to receive " << expectedToRcv << ", remaining to forward " << m_aNumForward);
			break;
		}
		case PIVOTS: {

			auto rem = get_merged_seen();
			auto loc = own_chi.c.seen;
			assert(rem.size() == loc.size());
			uint16_t c = 0;
			for (auto i = 0; i < rem.size(); i++) {
				c += (rem.at(i) && loc.at(i)) ? 1 : 0;
			}
			assert(c <= m_coder->rank());
			m_aNumForward = m_coder->rank() - c;
			break;
		}
		case MIN_MAX: {

			auto rem = own_chi.c;
			rem.seen = get_merged_seen();
			rem.decoded = get_merged_decoded();
			rem.rank = get_merged_rank(rem.seen);
			auto loc = own_chi.c;
			FeedbackEstimator fs(loc, rem);
			m_aNumForward = fs.GetN();
			break;
		}
		case CCACK: {

			m_aNumForward = 0;

			for (auto it : m_feedbacks) {
				auto chi = it.second;
				m_ccack->RcvHashVector(chi.hashVec);
				auto c = m_ccack->GetHeardSymbNum();

				auto s = m_coder->rank() > c ? m_coder->rank() - c : 0;
				m_aNumForward = (s > m_aNumForward) ? s : m_aNumForward;
			}

			break;
		}
		case ALL_VECTORS: {

			auto get_s = [this](CoderHelpInfo chi)
			{
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
					std::vector<uint8_t> s(m_fac.genSize);
					auto dec_o = m_coder;

					for (uint16_t i = 0; i < dec_o->coefficient_vector_size(); i++) {
						uint8_t* coef_vector = dec_o->coefficient_vector_data(i);
						s.assign(coef_vector, coef_vector + m_fac.genSize);
						dec->read_symbol(fake_symbol.data(), s.data());
					}
					auto finRank = dec->rank();

					assert(finRank >= origRank);
					auto k = finRank - origRank;
					return k;
				};

			m_aNumForward = 0;

			for (auto it : m_feedbacks) {
				auto chi = it.second;
				m_ccack->RcvHashVector(chi.hashVec);
				auto c = m_ccack->GetHeardSymbNum();

				auto s = get_s(chi);
				m_aNumForward = (s > m_aNumForward) ? s : m_aNumForward;
			}

			break;
		}
		default: {
			assert(0);
		}
		}
		UpdateNumToSend();
	}

	void UpdateNumToSend() {

		m_numToSend = ceil(m_aNumForward * m_cr);

		SIM_LOG(TEST_LOG, "Relay " << m_id << ": " << m_coder->rank() << " / " << m_numToSend << " / " << m_aNumForward << " / " << m_numForward);
	}

	uint16_t m_numForward; // how much of received should be forward
	uint16_t m_aNumForward; // how much is remaining to forward
	uint16_t m_numToSend; // how much is remaining to send counting the redundant packets
	uint16_t m_feedbackTimer;
	RelayStrategy m_strategy;
	StrategyOption m_strategyOption;
	double m_cr;

	std::map<uint16_t, CoderHelpInfo> m_feedbacks;
	std::map<uint16_t, uint16_t> rcvd;// <from node> <number>
};

struct FALogItem {

	FAConfig fac;

	uint16_t sntByVs;
	uint16_t sntByUs;
	uint16_t vRank;
	uint16_t hvRank;
	uint16_t huRank;

	friend std::ostream& operator<<(std::ostream& o, FALogItem& m) {

		o << m.fac << "\t" << m.sntByVs << "\t" << m.sntByUs << "\t" << m.vRank << "\t" << m.hvRank << "\t" << m.huRank;
		return o;
	}
};

FALogItem RunFA2Test(FAConfig fac) {

	typedef std::shared_ptr<RelayNode> relay_ptr;

	std::random_device r;

	std::default_random_engine generator(r());
	std::uniform_real_distribution<double> distribution(0.0, 1.0);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create source node
	//
	auto src = std::shared_ptr<SrcNode>(new SrcNode(fac));

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create relays
	//
	auto v = relay_ptr(new RelayNode(fac));
	v->SetId(0);
	auto v1 = relay_ptr(new RelayNode(fac));
	v1->SetId(1);
	v1->SetStrategyOption(RelayNode::NO_FEEDBACK);
	auto v2 = relay_ptr(new RelayNode(fac));
	v2->SetId(2);
	v2->SetStrategyOption(RelayNode::NO_FEEDBACK);

	std::vector<relay_ptr> us;
	for (auto i = 0; i < fac.numRelays; i++) {
		auto relay = relay_ptr(new RelayNode(fac));
		us.push_back(relay);
		relay->SetId(i + 3);
		relay->SetStrategyOption(RelayNode::NO_DATA);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create hyper-relays (virtual relay)
	//
	auto hv = relay_ptr(new RelayNode(fac));
	hv->SetId(100);
	auto hu = relay_ptr(new RelayNode(fac));
	hu->SetId(101);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// create channel
	//
	double loss_ratio = pow(fac.lossRatio, 1 / (double) fac.numRelays);

	SIM_LOG(TEST_LOG, "Created network..");

	////////////////////////////////////////////////////////////////////////////////////////////////
	// configure relay v
	//
	for (uint16_t i = 0; i < fac.vDof; i++) {
		v->RcvData(src->SendData());
		v->IncForward();
	}
	SIM_LOG(TEST_LOG, "Rank of v: " << v->Rank());

	////////////////////////////////////////////////////////////////////////////////////////////////
	// configure relays v_1 and v_2
	//
	while (hv->Rank() != v->Rank()) {
		auto payload = v->SendData(true);
		auto a = distribution(generator) < 0.5;
		auto b = distribution(generator) < 0.5;

		if (a) {
			v1->RcvData(payload);
			hv->RcvData(payload);
		}
		if (b) {
			v2->RcvData(payload);
			hv->RcvData(payload);
		}

		if (a || a & b) v1->IncForward();
		if (b && !a) v2->IncForward();
	}

	SIM_LOG(TEST_LOG, "Rank of v1: " << v1->Rank() << ", number to forward: " << v1->GetNumForward());
	SIM_LOG(TEST_LOG, "Rank of v2: " << v2->Rank() << ", number to forward: " << v2->GetNumForward());
	SIM_LOG(TEST_LOG, "Rank of hv: " << hv->Rank());

	////////////////////////////////////////////////////////////////////////////////////////////////
	// define broadcast functions
	//
	uint32_t snt_by_vs = 0, snt_by_us = 0;

	auto do_broadcast_us = [&](relay_ptr u)
	{
		if(!u->HasFeedback()) return false;

		auto feedback = u->SendFeedback();
		snt_by_us++;
		v1->RcvFeedback(feedback, u->GetId());
		v2->RcvFeedback(feedback, u->GetId());
	};

	auto do_broadcast_vs = [&](relay_ptr v)->bool
	{
		if(!v->HasData()) return false;

		auto payload = v->SendData();
		snt_by_vs++;
		for(auto u : us)
		{
			if (distribution(generator) > loss_ratio) {
				u->RcvData(payload, v->GetId());
				hu->RcvData(payload);
				do_broadcast_us(u);
			}
		}
		return true;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////		Test start		///////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	SIM_LOG(TEST_LOG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
	SIM_LOG(TEST_LOG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
	SIM_LOG(TEST_LOG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");

	while (v1->HasData() || v2->HasData()) {

		if (distribution(generator) > 0.5) {
			do_broadcast_vs(v1);
		} else {
			do_broadcast_vs(v2);
		}
	}

	SIM_LOG(TEST_LOG, "Vs sent:\t" << snt_by_vs << " symbols");
	SIM_LOG(TEST_LOG, "Us sent:\t" << snt_by_us << " symbols");
	SIM_LOG(TEST_LOG, "V rank:\t" << v->Rank());
	SIM_LOG(TEST_LOG, "Vs rank:\t" << hv->Rank());
	SIM_LOG(TEST_LOG, "Us rank:\t" << hu->Rank());

	// achievable ranks of destination

	FALogItem fali;
	fali.fac = fac;

	fali.sntByVs = snt_by_vs;
	fali.sntByUs = snt_by_us;
	fali.vRank = v->Rank();
	fali.hvRank = hv->Rank();
	fali.huRank = hu->Rank();

	return fali;
}

void TestFeedbackAccuracy2() {

	FAConfig fac;
	fac.numRelays = 2;
	fac.genSize = 8;
	fac.symSize = 1;
	fac.relayStrategy = CCACK;
	fac.numStd = 0;
	fac.lossRatio = 0.2;
	fac.feedbackF = 1;
	fac.vDof = 6;

	RunFA2Test(fac);

//
//	std::vector<uint16_t> genSizes = { 8, 16, 32 };
//	std::vector<RelayStrategy> relStrategies = {RelayNode::ALWAYS_SEND, RelayNode::NOT_MORE_THAN_RANK, RelayNode::SUBS_DECODED, RelayNode::SUBS_PIVOTS,
//		RelayNode::MIN_MAX, RelayNode::CCACK, RelayNode::ALL_VECTORS};
//	std::vector<uint16_t> numRelays = { 2, 3, 4 };
//
//	for (auto genSize : genSizes) {
//		for (auto relStrategy : relStrategies) {
//			for (auto numRelay : numRelays) {
//				fac.numRelays = numRelay;
//				init_channel();
//
//				std::vector<uint16_t> kMaxs = { 1, 2 };
//				kMaxs.push_back(genSize);
//				for (auto kMax : kMaxs) {
//					fac.genSize = genSize;
//					fac.relayStrategy = relStrategy;
//					fac.kMax = kMax;
//
//					uint32_t num_iter = 100, c = 0;
//
//					while (c++ != num_iter) {
//						auto log = RunFA2Test(fac);
//						// simulation for number of iterations 100, generation size {8,16,32}, number of relays {2, 3, 4}, relay strategy {0,1,...,6}, frequency of feedback {1,2, generation size}, relay strategy option {0}
//						// relay strategy: {0 - 6} = { RelayNode::ALWAYS_SEND, RelayNode::NOT_MORE_THAN_RANK, RelayNode::SUBS_DECODED, RelayNode::SUBS_PIVOTS,	RelayNode::MIN_MAX, RelayNode::CCACK, RelayNode::ALL_VECTORS}
//						// relay strategy option: {0, 1} = {RelayNode::SEND_EXACT_AS_STRATEGY_SAYS, RelayNode::SEND_AT_LEAST_ONE_ON_REQUEST}
//						// line format: iteration number|frequency of feedback|relay strategy|source strategy|
//						// destination strategy|generation size|sent by source|sent by relays|sent by destination|
//						// destination achievable rank|relay strategy option|destination achieved rank|number of relays
//						std::cout << c << "\t" << fac.kMax << "\t" << log << std::endl;
//					}
//				}
//			}
//		}
//	}

}
}
#endif /* TEST_FEEDBACK_ACCURACY_TWO_H_ */
