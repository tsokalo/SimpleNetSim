/*
 * TrafficGenerator.cpp
 *
 *  Created on: Sep 30, 2013
 *      Author: tsokalo
 */

#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

#include "traffic-generator.h"
#include "utils/brr-pkt-header.h"

#include <time.h>
#include <math.h>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <chrono>

#define THREASHOLD_1    10000000
#define THREASHOLD_2    25000
#define THREASHOLD_3    1

using namespace boost::chrono;

namespace ncr {

template<long long speed>
struct cycle_count {
	typedef typename boost::ratio_multiply<boost::ratio<speed>, boost::mega>::type frequency; // Mhz
	typedef typename boost::ratio_divide<boost::ratio<10>, frequency>::type period;
	typedef long long rep;
	typedef boost::chrono::duration<rep, period> duration;
	typedef boost::chrono::time_point<cycle_count> time_point;

	static time_point now() {
		static long long tick = 0;
		// return exact cycle count
		return time_point(duration(++tick)); // fake access to clock cycle count
	}
};

class approx_cycle_count {
public:
	approx_cycle_count() {
	}
	~approx_cycle_count() {
	}
	//  static long long frequency; // MHz
	typedef nanoseconds duration;
	typedef duration::rep rep;
	typedef duration::period period;
	static const long long nanosec_per_sec = period::den;
	typedef boost::chrono::time_point<approx_cycle_count> time_point;
	static time_point now(long long frequency) {
		static long long tick = 0;
		// return cycle count as an approximate number of nanoseconds
		// compute as if nanoseconds is only duration in the std::lib
		return time_point(duration(++tick * nanosec_per_sec / frequency * 1000 / 1000000));
	}
};

TrafficGenerator::TrafficGenerator(send_orig_sym_func send, uint32_t symbSize, Datarate apiRate) {

	m_symbSize = symbSize;
	m_iat = m_symbSize * 8 * pow(10, 9) / apiRate;
	srand(static_cast<uint32_t> (time(0)));
	m_symb.resize(m_symbSize);
	std::generate(m_symb.begin(), m_symb.end(), rand);
	m_send = send;

	m_stopGen = false;

	m_pcSpeed.parameter = 4000;
	m_pcSpeed.influence = 1;
	m_constSchift.parameter = -8000;//-8us
	m_constSchift.influence = 1;

	memset(&m_iatAdjust, 0, sizeof(IatAdjustPrimitive));
}
TrafficGenerator::~TrafficGenerator() {
	Stop();
	SIM_LOG(TRAF_GEN_LOG, "TrafficGenerator destructor is finished");
}
void TrafficGenerator::Start(uint32_t num) {
	while (num-- != 0) {
		m_ssn++;
		std::generate(m_symb.begin(), m_symb.end(), rand);
		memcpy(&m_symb[0], m_ssn.data(), m_ssn.size());
		SIM_LOG(TRAF_GEN_LOG, "Send symbol with id " << m_ssn << " and size " << m_symb.size() << ", remaining " << num);
		m_send(m_symb);
	}
}
void TrafficGenerator::Start() {
	m_startThread = thread_ptr(new std::thread(std::bind(&TrafficGenerator::DoGenerate, this)));
}
void TrafficGenerator::Stop() {
	m_stopGen = true;

	SIM_LOG(TRAF_GEN_LOG, "Waiting for the generation thread to finish");
	if (m_startThread) m_startThread->join();
}
void TrafficGenerator::DoGenerate() {

	InitIatAdjust();

	while (!m_stopGen) {

		if (m_stopGen) return;

		if (m_iat >= THREASHOLD_1) {
			int64_t correction = (m_constSchift.parameter + (int64_t) m_iat < 0) ? (-m_iat) : m_constSchift.parameter;
			//			boost::this_thread::sleep(boost::chrono::nanoseconds(m_iat + correction));
			std::this_thread::sleep_for(std::chrono::nanoseconds(m_iat + correction));
		}
		else if (m_iat >= THREASHOLD_2) {
			int64_t correction = (m_constSchift.parameter + (int64_t) m_iat < 0) ? (-m_iat) : m_constSchift.parameter;
			boost::chrono::system_clock::time_point go = boost::chrono::system_clock::now() + boost::chrono::nanoseconds(m_iat + correction);
			while (boost::chrono::system_clock::now() < go)
				;
		}
		else {
			typedef approx_cycle_count clock;
			clock::time_point stop = clock::now(m_pcSpeed.parameter) + nanoseconds(m_iat);
			while (clock::now(m_pcSpeed.parameter) < stop)
				// no multiplies or divides in this loop
				;
		}

		if (m_stopGen) return;

		//
		// Adjust timer constants
		//
		m_iatAdjust.numPkt++;
		if (m_iatAdjust.numPkt * m_iat > 2000000000) {
			m_iatAdjust.lastRcv = boost::chrono::system_clock::now();
			boost::chrono::microseconds interarrTime = boost::chrono::duration_cast<boost::chrono::microseconds>(m_iatAdjust.lastRcv - m_iatAdjust.firstRcv);
			long double timeSec = interarrTime.count();
			long double aveIat = timeSec / (long double) (m_iatAdjust.numPkt) * 1000;

			int64_t diff = m_iat - aveIat;
			AdjustIat(diff);
			m_iatAdjust.firstRcv = m_iatAdjust.lastRcv;
			m_iatAdjust.numPkt = 0;
		}

		SIM_LOG(TRAF_GEN_LOG, "Generated packet with size: " << m_symbSize << ", next packet to send after " << m_iat << "(ns)");

		if (m_stopGen) return;
		//
		// Send
		//
		m_ssn++;
		memcpy(&m_symb[0], m_ssn.data(), m_ssn.size());
		m_send(m_symb);
	}
}

void TrafficGenerator::InitIatAdjust() {
	m_iatAdjust.numPkt = 0;
	m_iatAdjust.firstRcv = boost::chrono::system_clock::now();
}
void TrafficGenerator::AdjustIat(int64_t diff) {
	//
	// Adjust speed factor
	//
	if (m_iat < THREASHOLD_2) {
		double relError = diff / m_iat;

		if (m_pcSpeed.diff * diff < 0)// if difference has changed its sign
		{
			if (relError > 0.1 || relError < -0.1) m_pcSpeed.influence = (m_pcSpeed.influence / 2 <= 0.05) ? 0.05 : m_pcSpeed.influence / 2;
		}
		if (relError > 1 || relError < -1) m_pcSpeed.influence = 1;

		if (relError > 0.25) relError = 0.25;
		if (relError < -0.25) relError = -0.25;

		if (relError > 0.01 || relError < -0.01) {
			m_pcSpeed.parameter = m_pcSpeed.parameter + m_pcSpeed.parameter * m_pcSpeed.influence * relError;
			m_pcSpeed.parameter = (m_pcSpeed.parameter < MIN_PROC_FREQ) ? MIN_PROC_FREQ : m_pcSpeed.parameter;
		}

		m_pcSpeed.diff = diff;
		if (TRAF_GEN_LOG) std::cout << "m_pcSpeed.parameter: " << m_pcSpeed.parameter << ", relError: " << relError << ", m_pcSpeed.influence: "
				<< m_pcSpeed.influence << std::endl;

		return;
	}

	//
	// Adjust shift duration factor
	//
	if (m_iat < THREASHOLD_1) {
		double relError = diff / m_iat;

		if (m_constSchift.diff * diff < 0)// if difference has changed its sign
		{
			if (relError > 0.1 || relError < -0.1) m_constSchift.influence = (m_constSchift.influence / 2 <= 0.025) ? 0.025 : m_constSchift.influence / 2;
		}
		if (relError > 1 || relError < -1) m_constSchift.influence = 1;

		if (relError > 1) relError = 1;
		if (relError < -1) relError = -1;

		if (relError > 0.01 || relError < -0.01) {
			m_constSchift.parameter = m_constSchift.parameter + m_iat * m_constSchift.influence * relError;
			m_constSchift.parameter = (m_constSchift.parameter + m_iat < 0) ? (-m_iat) : m_constSchift.parameter;
		}
		m_constSchift.diff = diff;
		if (TRAF_GEN_LOG) std::cout << "m_constSchift.parameter: " << m_constSchift.parameter << ", relError: " << relError << ", m_constSchift.influence: "
				<< m_constSchift.influence << std::endl;
	}
}

}//ncr
