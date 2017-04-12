/*
 * congestion-control.h
 *
 *  Created on: 25.12.2016
 *      Author: tsokalo
 */

#ifndef CONGESTIONCONTROL_H_
#define CONGESTIONCONTROL_H_

#include <random>
#include <assert.h>

#include "header.h"

namespace ncr {

class CongestionControl {

public:
	CongestionControl(NodeType type) :
		m_gen(m_rd()), m_dis(0, 1) {
		m_nodeType = type;
	}
	~CongestionControl() {
	}

	//
	// block the sender
	//
	bool Block(priority_t p, Datarate d) {
		if(!geq(d, p.val()))
		{
			std::cout << "Datarate: " << d << ", priority: " << p << std::endl;
		}
		assert(geq(d, p.val()));
		m_p = p;
		m_d = d;

		if (m_nodeType == SOURCE_NODE_TYPE) {
			if (m_dis(m_gen) > m_p.val() / m_d) return true;
		}
		return false;
	}
	//
	// block the traffic generator
	//
	bool BlockGen(uint16_t txBufSize, uint16_t numBuffering){
		return !(txBufSize < numBuffering);
	}
private:
	NodeType m_nodeType;
	priority_t m_p;
	Datarate m_d;

	std::random_device m_rd;
	std::mt19937 m_gen;
	std::uniform_real_distribution<> m_dis;

};

}

#endif /* CONGESTIONCONTROL_H_ */
