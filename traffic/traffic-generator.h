/*
 * TrafficGenerator.h
 *
 *  Created on: Sep 30, 2013
 *      Author: tsokalo
 *
 *  After stopping the generator application it cannot be stated again
 */

#ifndef TRAFFICGENERATOR_H_
#define TRAFFICGENERATOR_H_

#include <string.h>
#include <pthread.h>
#include <utility>

#include <boost/chrono.hpp>
#include <thread>
#include <memory>

#include "header.h"
#include "utils/ssn.h"

#define THREASHOLD_1    10000000
#define THREASHOLD_2    25000
#define THREASHOLD_3    1
//unit [MHz]
#define MIN_PROC_FREQ          10

namespace ncr {
typedef int16_t ConnectionId;

struct IatAdjustPrimitive {
	uint32_t numPkt;
	boost::chrono::system_clock::time_point firstRcv;
	boost::chrono::system_clock::time_point lastRcv;
};

struct FeedbackParameter {
	FeedbackParameter() {
		parameter = 0;
		influence = 0;
		diff = 0;
	}
	~FeedbackParameter() {
	}
	long long parameter;
	double influence;
	long long diff;
};

typedef FeedbackParameter PcSpeed;
typedef FeedbackParameter ConstSchift;

class TrafficGenerator {
	typedef std::function<void(OrigSymbol)> send_orig_sym_func;
	typedef std::shared_ptr<std::thread> thread_ptr;

public:

	TrafficGenerator(send_orig_sym_func send, uint32_t symbSize = 10, Datarate apiRate = 1);
	virtual ~TrafficGenerator();

	//
	// controllable API
	//
	void Start(uint32_t num);
	//
	// not controllable API
	//
	void Start();

	void Stop();

private:

	void DoGenerate();
	void InitIatAdjust();
	void AdjustIat(int64_t diff);

	bool m_stopGen;

	symb_ssn_t m_ssn;

	thread_ptr m_startThread;

	//
	// IAT sampling feedback
	//
	PcSpeed m_pcSpeed;
	ConstSchift m_constSchift;

	IatAdjustPrimitive m_iatAdjust;

	OrigSymbol m_symb;
	send_orig_sym_func m_send;
	//
	// unit [ns]
	//
	uint64_t m_iat;
	//
	// unit [byte]
	//
	uint32_t m_symbSize;
};
}//ncr
#endif /* TRAFFICGENERATOR_H_ */
