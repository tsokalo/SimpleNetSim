/*
 * brr-header.h
 *
 *  Created on: 07.01.2017
 *      Author: tsokalo
 */

#ifndef HEADERMINFO_H
#define HEADERMINFO_H

#include <sstream>
#include "header.h"
#include "utils/brr-tx-plan.h"
#include "brr-pkt-header.h"

namespace ncr {

struct HeaderMInfo {

	HeaderMInfo() {
	}
	HeaderMInfo(const HeaderInfo& other) {
		this->addr = other.addr;
		this->txPlan = other.txPlan;
	}
	HeaderMInfo& operator=(const HeaderMInfo& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->addr = other.addr;
			this->p = other.p;
			this->pf = other.pf;
			this->txPlan = other.txPlan;
		}
		return *this;
	}

	HeaderMInfo& operator=(const HeaderInfo& other) // copy assignment
			{
		this->addr = other.addr;
		this->txPlan = other.txPlan;
		return *this;
	}

	void Serialize(std::stringstream &ss) {
		ss << addr << DELIMITER;
		ss << (uint16_t) p.size() << DELIMITER;
		for (auto p_ : p)
			ss << p_.first << DELIMITER << p_.second << DELIMITER;
		ss << (uint16_t) pf.size() << DELIMITER;
		for (auto pf_ : pf)
			ss << pf_.first << DELIMITER << pf_.second << DELIMITER;
		txPlan.Serialize(ss);
	}
	void Deserialize(std::stringstream &ss) {
		ss >> addr;
		double v;
		uint16_t n;
		ss >> n;
		p.clear();
		for (uint16_t i = 0; i < p.size(); i++) {
			UanAddress a;
			ss >> a;
			ss >> v;
			p[a] = v;
		}
		ss >> n;
		pf.clear();
		for (uint16_t i = 0; i < n; i++) {
			UanAddress a;
			ss >> a;
			ss >> v;
			pf[a] = v;
		}
		txPlan.clear();
		txPlan.Deserialize(ss);
	}
	/*
	 * we use string conversion for serialization in the simulator
	 * in real applications, the data will be much compressed with careful bit to bit conversion
	 * GetSerializedSize() give the header size for real applications
	 */
	uint32_t GetSerializedSize() {
		uint32_t ssize = 0;

		ssize += 1; // own address

		for (auto v : p) {
			ssize += 1; //v.first - sink address
			ssize += 2; //v.second.val() - priority value (conversion from double to int16_t)
		}

		for (auto v : pf) {
			ssize += 1; //v.first - sink address
			ssize += 2; //v.second - value of the filtering coefficient (conversion from double to int16_t)
		}

		for(auto v : txPlan)
		{
			ssize += 2; //v.first - generation ID
			ssize += 2; //v.second.num_all - number of packets to send
			//v.second.all_prev_acked - neglect with the size of this flag
		}

		ssize += 3; // number of items in each of the list (1 byte per list)

		return ssize;
	}
	/*
	 * sender address
	 */
	UanAddress addr;
	std::unordered_map<UanAddress, priority_t> p;
	pf_t pf;

	TxPlan txPlan;
};
}
#endif /* HEADERMINFO_H */
