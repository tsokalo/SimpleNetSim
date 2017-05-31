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
	 * sender address
	 */
	UanAddress addr;
	std::unordered_map<UanAddress, priority_t> p;
	pf_t pf;

	TxPlan txPlan;
};
}
#endif /* HEADERMINFO_H */
