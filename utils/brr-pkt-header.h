/*
 * brr-header.h
 *
 *  Created on: 07.01.2017
 *      Author: tsokalo
 */

#ifndef HEADERINFO_H
#define HEADERINFO_H

#include <sstream>
#include "header.h"
#include "utils/brr-tx-plan.h"

namespace ncr{

struct HeaderInfo {
	HeaderInfo& operator=(const HeaderInfo& other) // copy assignment
	{
		if (this != &other) { // self-assignment check expected
			this->addr = other.addr;
			this->p = other.p;
			this->pf = other.pf;
			this->txPlan = other.txPlan;
		}
		return *this;
	}

	void Serialize(std::stringstream &ss)
	{
		ss << addr << DELIMITER;
		ss << p.val() << DELIMITER;
		ss << (uint16_t)pf.size() << DELIMITER;
		for(auto p : pf)ss << p.first << DELIMITER << p.second << DELIMITER;
		txPlan.Serialize(ss);
	}
	void Deserialize(std::stringstream &ss)
	{
		ss >> addr;
		double v;
		ss >> v;
		p = v;
		uint16_t n;
		ss >> n;
		pf.clear();
		for(uint16_t i = 0; i < n; i++)
		{
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
	priority_t p;
	pf_t pf;

	TxPlan txPlan;
};
}
#endif /* HEADERINFO_H */
