/*
 * test-ccack.h
 *
 *  Created on: 16.01.2017
 *      Author: tsokalo
 */

#ifndef TESTOUTOFORDER_H_
#define TESTOUTOFORDER_H_

#include <algorithm>
#include <cstddef>
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <functional>
#include <iomanip>
#include <random>

#include <boost/math/distributions/chi_squared.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/algorithm/minmax_element.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "utils/ssn.h"
#include "test/test-feedback-accuracy2.h"

namespace ncr {

///                      path 1
///				  +-----------------+
///				  |					|
///         +-----------+      +-----------+
///         |  sender   |      | receiver  |
///         +-----------+      +-----------+
///               |      path 2		|
///				  +-----------------+

#define PKT_SIZE 100

struct PathDesc {
	PathDesc(double latency, double jitter, double loss) {
		this->latency = latency;
		this->jitter = jitter;
		this->loss = loss;
	}
	double latency;
	double jitter;
	double loss;
};

std::vector<uint16_t> GenSendSeq(PathDesc path1, PathDesc path2, uint32_t num_v, std::vector<double> &arrivals) {
	using namespace std;

	auto gen_iats = [](double latency, double jitter, uint32_t num_v)
	{
		std::random_device r;
		std::default_random_engine gen (r ());

		std::normal_distribution<> d
		{	latency, jitter};

		std::vector<double> iats;

		auto get_iat = [&]()
		{
			auto v = d (gen);
			return (v < 0 ? 0 : v);
		};

		for (uint32_t i = 0; i < num_v; i++)
		iats.push_back (get_iat ());

		return iats;
	};
	auto get_arrivals = [](std::vector<double> iats)
	{
		std::vector<double> arrivals;
		for(uint32_t i = 0; i < iats.size(); i++)
		{
			arrivals.push_back((i == 0) ? iats.at(i) : arrivals.at(i - 1) + iats.at(i));
		}
		return arrivals;
	};
	auto get_sending_sequence = [&](std::vector<double> arrivals_i, std::vector<double> arrivals_j)
	{
		std::vector<uint16_t> seq;
		auto i_it = arrivals_i.begin();
		auto j_it = arrivals_j.begin();
		while(i_it != arrivals_i.end() && j_it != arrivals_j.end())
		{
			// i - 1; j - 2
			if(i_it == arrivals_i.end())
			{	seq.push_back(2);arrivals.push_back(*j_it);j_it++;}
			else
			if(j_it == arrivals_j.end())
			{	seq.push_back(1);arrivals.push_back(*i_it);i_it++;}
			else
			{
				if(*j_it > *i_it)
				{	seq.push_back(1);arrivals.push_back(*i_it);i_it++;}
				else
				{	seq.push_back(2);arrivals.push_back(*j_it);j_it++;}
			}
		}
		return seq;
	};

	// first path
	auto iats_i = gen_iats(path1.latency, path1.jitter, num_v);
	auto arrivals_i = get_arrivals(iats_i);
	// second path
	auto iats_j = gen_iats(path1.latency, path1.jitter, num_v);
	auto arrivals_j = get_arrivals(iats_j);

	//
	// define receiving order assuming the round robin scheduling
	//
	return get_sending_sequence(arrivals_i, arrivals_j);
}

struct OoO {
	typedef std::vector<uint8_t> pkt_t;

	struct Feedback {
		std::vector<bool> ssns;
		uint32_t rank;
	};
	enum Codec {
		NO_CODEC, FULL_RLNC
	};

};

class SenderNode: public NodeObject<encoder_ptr> {
public:

	SenderNode(FAConfig fac, OoO::Codec c, std::vector<OoO::pkt_t> pkts) :
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

		m_toSend = pkts.size();
		m_pkts = pkts;
		m_c = c;
	}
	virtual ~SenderNode() {
	}

	bool HaveData() {
		return (m_toSend != 0);
	}

	void RcvFeedback(OoO::Feedback fb) {

		SIM_LOG_FUNC(TEST_LOG);

		m_toSend = 0;
		if (OoO::NO_CODEC) {
			for (auto v : fb.ssns)
				if (!v) m_toSend++;
			m_tx = fb.ssns;
		}
		if (OoO::FULL_RLNC) {
			m_toSend = m_coder->rank() - fb.rank;
		}

		SIM_LOG(TEST_LOG, "Number to send: " << m_toSend);
	}

	std::vector<uint8_t> SendData(bool force = true) {

		SIM_LOG_FUNC(TEST_LOG);

		assert(!m_pkts.empty());

		m_toSend--;
		assert(m_toSend >= 0);

		if (m_c == OoO::NO_CODEC) {
			auto p_it = m_pkts.begin();
			for (auto tx : m_tx) {
				if (tx) m_payload = *p_it;
				p_it++;
			}
		}

		if (m_c == OoO::FULL_RLNC) {
			m_coder->write_payload(m_payload.data());
		}
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

private:
	int32_t m_toSend;
	OoO::Codec m_c;
	std::vector<OoO::pkt_t> m_pkts;
	std::vector<bool> m_tx;
};

class ReceiverNode: public NodeObject<decoder_ptr> {

public:

	enum StrategyOption {
		NO_OPTION, NO_FEEDBACK, NO_DATA
	};

	ReceiverNode(FAConfig fac, OoO::Codec c, std::vector<OoO::pkt_t> pkts) :
			NodeObject(fac) {

		m_coder = m_decFactory.build();
		m_payload.resize(m_coder->payload_size());
		m_numToSend = 0;
		m_numForward = 0;
		m_aNumForward = 0;
		m_strategy = fac.relayStrategy;
		m_strategyOption = NO_OPTION;
		m_feedbackTimer = fac.feedbackF;
		m_numFeedbackSent = 0;

		double eps = m_fac.lossRatio;

		double gamma = pow(m_fac.numStd, 2) * eps / (double) m_fac.genSize / 4.0;
		gamma = (gamma > 1) ? 1 : gamma;
		double alpha = 1 + 2 * sqrt(gamma * (gamma + 1));

		assert(eps != 1);
		m_cr = alpha / (1 - eps);

		SIM_LOG(TEST_LOG, "Using coding rate: " << m_cr);

		m_pkts = pkts;
		m_c = c;
	}
	virtual ~ReceiverNode() {
	}

	void SetStrategy(RelayStrategy fs) {
		m_strategy = fs;
	}
	void SetStrategyOption(StrategyOption so) {
		m_strategyOption = so;
	}
	uint16_t GetNumFeedbackSnt() {
		return m_numFeedbackSent;
	}

	OoO::Feedback GetFeedback() {

		SIM_LOG_FUNC(TEST_LOG);

		OoO::Feedback fb;
		fb.ssns = m_tx;
		fb.rank = m_coder->rank();
		return fb;
	}

	std::vector<uint8_t> SendData(bool force = false) {

		assert(0);
		return m_payload;
	}
	void RcvData(std::vector<uint8_t> payload, uint16_t id = 0) {

		SIM_LOG_FUNC(TEST_LOG);

		if (m_c == OoO::NO_CODEC) {
			//
			// extract SSN and mark the received
			//

		}
		if (m_c == OoO::FULL_RLNC) {
			m_coder->read_payload(payload.data());
		}
	}

	void RcvFeedback(CoderHelpInfo chi, uint16_t id) {
		assert(0);
	}
	CoderHelpInfo SendFeedback() {
		CoderHelpInfo chi;
		assert(0);
		return chi;
	}

private:

	uint16_t m_numForward; // how much of received should be forward
	uint16_t m_aNumForward; // how much is remaining to forward
	uint16_t m_numToSend; // how much is remaining to send counting the redundant packets
	uint16_t m_feedbackTimer;
	RelayStrategy m_strategy;
	StrategyOption m_strategyOption;
	double m_cr;
	uint16_t m_numFeedbackSent;

	// feedbacks received
	std::map<uint16_t, CoderHelpInfo> m_feedbacks;
	// feedbacks to send
	std::map<uint16_t, uint16_t> rcvd; // <from node> <number>
	std::map<uint16_t, bool> gotLinDep; // <from node> <received a linear dependent packet>

	OoO::Codec m_c;
	std::vector<OoO::pkt_t> m_pkts;
	std::vector<bool> m_tx;
};

void TestOutOfOrder() {

	FAConfig fac;
	fac.numRelays = 2;
	fac.genSize = 8;
	fac.symSize = PKT_SIZE;
	fac.relayStrategy = MIN_MAX;	//EXPECTATION, NUMBER_RCVD, PIVOTS, MIN_MAX, CCACK, ALL_VECTORS
	fac.numStd = 0;
	fac.lossRatio = 0.2;
	fac.feedbackF = fac.genSize;
	fac.vDof = fac.genSize >> 1;
	fac.hashMatrix = hash_matrix_set_ptr(new HashMatrixSet(2, fac.genSize, 8));


	std::random_device r;

	std::default_random_engine generator(r());
	std::uniform_real_distribution<double> distribution(0.0, 1.0);

	//
	// generate original packets
	//
	std::vector<OoO::pkt_t> pkts;

	uint32_t num_pkts = fac.genSize;
	symb_ssn_t ssn;
	while (num_pkts-- != 0) {
		OoO::pkt_t packet(fac.symSize);
		ssn++;
		std::generate(packet.begin(), packet.end(), rand);
		memcpy(&packet[0], ssn.data(), ssn.size());
		SIM_LOG(TEST_LOG, "Send symbol with id " << ssn << " and size " << packet.size() << ", remaining " << num_pkts);
		pkts.push_back(packet);
	}

	//
	// create sending sequence order
	//
	PathDesc path1(10, 5, 0.5), path2(10, 2, 0.5);
	uint32_t numTransmissionSlots = fac.genSize * 10;
	std::vector<double> arrivals;
	auto seq = GenSendSeq(path1, path2, numTransmissionSlots, arrivals);

	SenderNode snd(fac, OoO::NO_CODEC, pkts);
	ReceiverNode rcv(fac, OoO::NO_CODEC, pkts);
	auto seq_it = seq.begin();
	auto arr_it = arrivals.begin();

	while (snd.HaveData()) {
		while (snd.HaveData()) {

			assert(seq_it != seq.end());
			auto pkt = snd.SendData();
			if(*seq_it == 1)if(distribution(generator) > path1.loss)rcv.RcvData(pkt);
			if(*seq_it == 2)if(distribution(generator) > path2.loss)rcv.RcvData(pkt);
			seq_it++;
			arr_it++;
		}

		snd.RcvFeedback(rcv.GetFeedback());
	}

	double duration = *arr_it;

	std::cout << "Duration: " << duration << std::endl;
}

}
#endif /* TESTOUTOFORDER_H_ */
