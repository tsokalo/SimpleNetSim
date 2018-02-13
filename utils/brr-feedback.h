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
#include <iostream>

#include "header.h"
#include "utils/brr-retrans-request.h"
#include "utils/ack-info.h"
#include "utils/service-messtype.h"

namespace ncr {

struct FeedbackInfo {

	FeedbackInfo() {
		addr = 0;
		ttl = 0;
		updated = false;
	}
	FeedbackInfo(const FeedbackInfo &other) {
		this->addr = other.addr;
		this->p = other.p;
		this->rcvMap = other.rcvMap;
		this->rrInfo = other.rrInfo;
		this->ttl = other.ttl;
		this->ackInfo = other.ackInfo;
		this->rcvNum = other.rcvNum;
		this->type.copy(other.type);

		this->updated = true;
	}

	FeedbackInfo& operator=(const FeedbackInfo& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->addr = other.addr;
			this->p = other.p;
			this->rcvMap = other.rcvMap;
			this->rrInfo = other.rrInfo;
			this->ttl = other.ttl;
			this->ackInfo = other.ackInfo;
			this->rcvNum = other.rcvNum;
			this->type.copy(other.type);

			this->updated = true;
		}
		return *this;
	}

	void Reset() {
		this->rcvMap.clear();
		this->rrInfo.clear();
		this->ackInfo.clear();
		this->updated = false;
		this->rcvNum.clear();
		this->type = ServiceMessType::NONE;
	}

	void Serialize(std::stringstream &ss) {

		ss << addr << DELIMITER;
		ss << p.val() << DELIMITER;
		ss << type.GetAsInt() << DELIMITER;
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
		type = w;
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
	 * we use string conversion for serialization in the simulator
	 * in real applications, the data will be much compressed with careful bit to bit conversion
	 * GetSerializedSize() give the header size for real applications
	 */
	uint32_t GetSerializedSize() {
		uint32_t ssize = 0;

		ssize += 1; // own address
		ssize += 2; // own priority (conversion from double to int16_t)

		ssize += 1; // number of items in the list
		for (auto v : rcvMap) {
			ssize += 1; //v.first - address
			ssize += ceil((double)CODER_INFO_FILTER_DEPTH / 8); //length of the map in bytes
		}

		ssize += rrInfo.GetSerializedSize();
		ssize += ackInfo.GetSerializedSize();

		ssize += 1; // time to live
		// bit updated - neglect

		ssize += 1; // number of items in the list
		for(auto v : rcvNum)
		{
			ssize += 2; // v.first - generation ID
			ssize += 1; // number of items in the list
			for(auto w : v.second)
			{
				ssize += 1; // w.first - address
				ssize += 2; // w.second - number received
			}
		}

		ssize += 1; // service message type

		return ssize;
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
	 * kind of the message
	 */
	ServiceMessType type;

};

}

#endif /* BRRFEEDBACK_H_ */
