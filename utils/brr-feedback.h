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
		requestAck = false;
	}
	FeedbackInfo(const FeedbackInfo &other) {
		this->addr = other.addr;
		this->p = other.p;
		this->rcvMap = other.rcvMap;
		this->rrInfo = other.rrInfo;
		this->netDiscovery = other.netDiscovery;
		this->ttl = other.ttl;
		this->ackInfo = other.ackInfo;
		this->rcvNum = other.rcvNum;
		this->requestAck = other.requestAck;

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
			this->rcvNum = other.rcvNum;
			this->requestAck = other.requestAck;

			this->updated = true;
		}
		return *this;
	}

	void Reset() {
		this->rcvMap.clear();
		this->rrInfo.clear();
		this->netDiscovery = false;
		this->ackInfo.clear();
		this->updated = false;
		this->rcvNum.clear();
		this->requestAck = false;
	}

	void Serialize(std::stringstream &ss) {

		ss << addr << DELIMITER;
		ss << p.val() << DELIMITER;
		ss << (uint16_t) netDiscovery << DELIMITER;
		ss << (uint16_t) requestAck << DELIMITER;
		ss << ttl << DELIMITER;
		ss << (uint16_t) rcvMap.size() << DELIMITER;
		for (auto r : rcvMap)
			ss << r.first << DELIMITER << r.second.Serialize();
		rrInfo.Serialize(ss);
		ackInfo.Serialize(ss);
		auto serialize_rcv_num = [this](std::stringstream &ss)
		{
			ss << (uint16_t) rcvNum.size() << DELIMITER;
			auto it = rcvNum.begin_orig_order();
			while (it != rcvNum.last_orig_order()) {
				ss << it->first << DELIMITER;
				ss << (uint16_t) it->second.size() << DELIMITER;
				for(auto i : it->second) ss << i.first << DELIMITER << i.second << DELIMITER;
				it = rcvNum.next_orig_order(it);
			}
		};
		serialize_rcv_num(ss);

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
		ss >> w;
		requestAck = w;
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
		auto deserialize_rcv_num = [this](std::stringstream &ss)
		{
			uint16_t n = 0, m = 0;
			ss >> n;
			for(auto i = 0; i < n; i++)
			{
				GenId gid = 0;
				ss >> gid;
				ss >> m;
				for(auto j = 0; j < m; j++)
				{
					UanAddress id;
					uint16_t v;
					ss >> id;
					ss >> v;
					rcvNum[gid][id] = v;
				}
			}
		};
		rcvNum.clear();
		deserialize_rcv_num(ss);

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
	/*
	 * how many symbols from whom for each generation I've received
	 */
	RcvNum rcvNum;
	/*
	 * force the receivers of this feedback respond with the feedback containing ACKs
	 */
	bool requestAck;
};

}
#endif /* BRRFEEDBACK_H_ */
