/*
 * LossProcess.h
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#ifndef LOSSPROCESS_H_
#define LOSSPROCESS_H_

#include <chrono>
#include <random>
#include "header.h"

namespace ncr {

class LossProcess {
public:
	LossProcess() :
			m_lost(false) {
		typedef std::chrono::high_resolution_clock myclock;
		myclock::time_point beginning = myclock::now();
		myclock::duration d = myclock::now() - beginning;
		uint8_t seed_v = d.count() + (seed_corrector++);

		m_generator.seed(seed_v);
	}

	virtual ~LossProcess() {
	}

	virtual bool
	IsLost() = 0;

	virtual void
	Toss() = 0;

	virtual double
	GetMean() = 0;

	virtual void
	SetMean(double e) = 0;

protected:

	bool m_lost;
	std::default_random_engine m_generator;

};

class BernoulliLossProcess: public LossProcess {
public:
	BernoulliLossProcess(double e) :
			LossProcess(), m_e(e), m_distribution(0.0, 1.0) {

	}

	virtual ~BernoulliLossProcess() {
	}

	bool IsLost() {
		return m_lost;
	}

	void Toss() {
		m_lost = (m_distribution(m_generator) < m_e);
	}

	double GetMean() {
		return m_e;
	}

	void SetMean(double e) {
		m_e = e;
	}

private:

	double m_e;
	std::uniform_real_distribution<double> m_distribution;

};

class TraceLossProcess: public LossProcess {
public:
	TraceLossProcess(std::string path) :
			LossProcess() {
		m_pos = 0;
		std::ifstream infile(path, std::ios_base::in);

		std::string line;
		bool v;
		uint32_t c = 0;
		while (infile >> v) {
			if(!v)c++;
			m_crcStatus.push_back(!v);
		}
		infile.close();
		assert(!m_crcStatus.empty());
		m_e = (double)c / (double) m_crcStatus.size();
	}

	virtual ~TraceLossProcess() {
	}

	bool IsLost() {
		assert(m_pos < m_crcStatus.size());
		return m_crcStatus.at(m_pos);
	}

	void Toss() {
		m_pos = (m_pos + 1 == m_crcStatus.size()) ? 0 : m_pos + 1;
	}

	double GetMean() {
		return m_e;
	}

	void SetMean(double e) {
		m_e = e;
	}

	uint32_t GetTraceLength()
	{
		return m_crcStatus.size();
	}

private:
	std::vector<bool> m_crcStatus;
	uint16_t m_pos = 0;
	double m_e;

};
}

#endif /* LOSSPROCESS_H_ */
