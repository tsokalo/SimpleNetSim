/*
 * brr-feedback.h
 *
 *  Created on: 07.01.2017
 *      Author: tsokalo
 */

#ifndef BRRFEEDBACK_H_
#define BRRFEEDBACK_H_

#include <sstream>
#include <string.h>

#include "header.h"
#include "utils/brr-retrans-request.h"
#include "utils/ack-info.h"

namespace ncr {

struct FeedbackInfo {

	FeedbackInfo() {
		netDiscovery = false;
		ttl = 0;
		updated = false;
	}
	FeedbackInfo(const FeedbackInfo &other) {
		this->addr = other.addr;
		this->p = other.p;
		this->rcvMap = other.rcvMap;
		this->rrInfo = other.rrInfo;
		this->netDiscovery = other.netDiscovery;
		this->ttl = other.ttl;
		this->ackInfo = other.ackInfo;

		this->updated = true;
	}

	FeedbackInfo& operator=(const FeedbackInfo& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->addr = other.addr;
			this->p = other.p;
			this->rcvMap = other.rcvMap;
			this->rrInfo = other.rrInfo;
			this->netDiscovery = other.netDiscovery;
			this->ttl = other.ttl;
			this->ackInfo = other.ackInfo;

			this->updated = true;
		}
		return *this;
	}

	void Reset()
	{
		this->rcvMap.clear();
		this->rrInfo.clear();
		this->netDiscovery = false;
		this->ackInfo.clear();
		this->updated = false;
	}

	void Serialize(std::stringstream &ss) {

		ss << addr << DELIMITER;
		ss << p.val() << DELIMITER;
		ss << (uint16_t)netDiscovery << DELIMITER;
		ss << ttl << DELIMITER;
		ss << (uint16_t)rcvMap.size() << DELIMITER;
		for (auto r : rcvMap)
			ss << r.first << DELIMITER << r.second.Serialize();
		rrInfo.Serialize(ss);
		ackInfo.Serialize(ss);

		updated = false;
	}

	void Deserialize(std::stringstream &ss) {

		ss >> addr;
		double v;
		ss >> v;
		p = v;
		uint16_t w;
		ss >> w;
		netDiscovery = w;
		ss >> ttl;
		uint16_t n;
		ss >> n;
		rcvMap.clear();
		for (uint16_t i = 0; i < n; i++) {
			UanAddress a;
			ss >> a;
			RcvMap m;
			m.Deserialize(ss);
			rcvMap[a] = m;
		}
		rrInfo.clear();
		rrInfo.Deserialize(ss);
		ackInfo.Deserialize(ss);

		updated = true;
	}

	/*
	 * sender address
	 */
	UanAddress addr;
	priority_t p;
	/*
	 * Loss maps of input edges
	 */
	std::map<UanAddress, RcvMap> rcvMap;

	RetransRequestInfo rrInfo;
	AckInfo ackInfo;

	/*
	 * network discovery flag
	 */
	bool netDiscovery;
	/*
	 * time to live
	 */
	ttl_t ttl;
	/*
	 * this flag is for local usage only
	 */
	bool updated;
};

}
#endif /* BRRFEEDBACK_H_ */
