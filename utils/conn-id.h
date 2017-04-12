/*
 * conn-id.h
 *
 *  Created on: 02.10.2016
 *      Author: tsokalo
 */

#ifndef CONNID_H_
#define CONNID_H_


#include <stdint.h>
#include <iostream>
#include "../header.h"

namespace ncr {



struct ConnId {
	ConnId() {
		src = 0;
		dst = 0;
		this->flowId = 0;
	}
	ConnId(const UanAddress& source, const UanAddress& dest, int16_t flowId = -1) {
		src = source;
		dst = dest;
		this->flowId = flowId;
	}
	ConnId(const ConnId& arg) {
		src = arg.src;
		dst = arg.dst;
		flowId = arg.flowId;
	}
	ConnId&
	operator=(ConnId arg) {
		src = arg.src;
		dst = arg.dst;
		flowId = arg.flowId;
		return *this;
	}
	bool operator <(const ConnId& l) const {
		std::cout << *this << " <> " << l << std::endl;

		if (flowId == -1 || l.flowId == -1) {
			return !(l.src == src && l.dst == dst);
		}
		else {
			return !(l.src == src && l.dst == dst && l.flowId == flowId);
		}
	}

	friend bool operator==(const ConnId& l, const ConnId& r) {
		return (r.flowId == l.flowId && r.src == l.src);
	}
	friend std::ostream&
	operator<<(std::ostream& os, const ConnId& connId) {
		os << "<" << connId.src << "," << connId.dst << "," << connId.flowId << ">";
		return os;
	}
	bool IsFlowIdValid() {
		return (flowId != -1);
	}
	void MakeInvalid() {
		flowId = -1;
	}
	void SwapSrcDst() {
		UanAddress temp = dst;
		dst = src;
		src = temp;
	}

	/*
	 * a node from whom the owner of this object receives data;
	 * not necessarily the originator of traffic
	 */
	UanAddress src;
	/*
	 * a node whom the traffic is targeted
	 */
	UanAddress dst;
	/*
	 * flow ID is in fact unique for certain pair of src and dst
	 */
	int16_t flowId;
};

}//ncr
#endif /* CONNID_H_ */
