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

struct ServiceMessType {

public:

	ServiceMessType() {
		t = NONE;
	}

	enum ServiceMessType_ {
		REGULAR,
		/*
		 * request for the point-to-point ACK; only the neighbors of the sender respond to it; this request is not forwarded
		 */
		REQ_PTP_ACK,
		/*
		 * request for the end-to-end ACK; the direct neighbors respond to it if they have ACK for the requested generation(s);
		 * otherwise forward it in direction of the destination
		 */
		REQ_ETE_ACK,
		/*
		 * response to the point-to-point ACK request; behaves as a regular feedback; this response is not forwarded
		 */
		RESP_PTP_ACK,
		/*
		 * response to the end-to-end ACK request; will be forwarded in direction to the source
		 */
		RESP_ETE_ACK,
		/*
		 * network discovery
		 */
		NET_DISC,
		/*
		 * retransmission request
		 */
		REQ_RETRANS,
		/*
		 * non-initialized
		 */
		NONE
	};

	ServiceMessType& operator=(const ServiceMessType& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected

			if (this->t != NONE && other.t != NONE) {
				assert(0);
			}

			this->t = other.t;
		}
		return *this;
	}

	ServiceMessType& operator=(const uint16_t& other) // copy assignment
			{
		this->t = ServiceMessType_(other);
		return *this;
	}

	inline friend bool operator==(const ServiceMessType &a, const ServiceMessType &b) {
		return a.t == b.t;
	}

	inline friend bool operator==(const ServiceMessType &a, const ServiceMessType_ &b) {
		return a.t == b;
	}

	uint16_t GetAsInt() {
		return (uint16_t) t;
	}

private:

	ServiceMessType_ t;
};

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
		this->type = other.type;

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
			this->type = other.type;

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

	static MessType ConvertToMessType(ServiceMessType type) {
		if (type == ServiceMessType::REGULAR || type == ServiceMessType::REQ_PTP_ACK || type == ServiceMessType::REQ_ETE_ACK
				|| type == ServiceMessType::RESP_PTP_ACK || type == ServiceMessType::RESP_ETE_ACK) return FEEDBACK_MSG_TYPE;
		if (type == ServiceMessType::NET_DISC) return NETDISC_MSG_TYPE;
		if (type == ServiceMessType::REQ_RETRANS) return RETRANS_REQUEST_MSG_TYPE;

		assert(0);

		return NONE_MSG_TYPE;
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
