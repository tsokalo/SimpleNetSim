/*
 * brr-tx-plan.h
 *
 *  Created on: 09.01.2017
 *      Author: tsokalo
 */

#ifndef BRRTXPLAN_H_
#define BRRTXPLAN_H_

#include <sstream>
#include <string.h>
#include <vector>
#include <stdint.h>

#include "utils/special-map.h"
#include "utils/ssn.h"

namespace ncr {

struct TxPlanItem {
	TxPlanItem() {
		num_all = 0;
		all_prev_acked = false;
	}
	TxPlanItem& operator=(const TxPlanItem& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->num_all = other.num_all;
			this->placement.clear();
			this->placement = other.placement;
			this->all_prev_acked = other.all_prev_acked;
		}
		return *this;
	}
	uint32_t num_all;
	std::vector<uint32_t> placement;
	/*
	 * flag equals true if all previuos generations, that are not listed in the TX plan, are acked
	 */
	bool all_prev_acked;

	std::string Serialize() {
		std::stringstream ss;
		ss << num_all << DELIMITER;
		ss << (all_prev_acked ? 'y' : 'n') << DELIMITER;
		return ss.str();
	}
	void Deserialize(std::stringstream &ss) {
		ss >> num_all;
		uint8_t v;
		ss >> v;
		all_prev_acked = (v == 'y');
	}

	friend std::ostream& operator<<(std::ostream& o, TxPlanItem& m) {

		o << m.num_all << ":" << m.all_prev_acked;
		return o;
	}
};

struct TxPlan: public special_map<GenId, TxPlanItem, gen_ssn_t> {

	void Serialize(std::stringstream &ss) {

		ss << (uint16_t)this->size() << DELIMITER;
		for (auto p : *this) {
			ss << p.first << DELIMITER << p.second.Serialize();
		}
	}
	void Deserialize(std::stringstream &ss) {

		uint16_t n;
		ss >> n;

		for (uint16_t i = 0; i < n; i++) {
			GenId genId;
			TxPlanItem item;
			ss >> genId;
			item.Deserialize(ss);
			this->operator [](genId) = item;
		}
	}

//	TxPlan& operator=(const TxPlan& other) // copy assignment
//			{
//		if (this != &other) { // self-assignment check expected
//
//			for(auto item : other)
//			{
//				this->operator[](item.first) = item.second;
//			}
//		}
//		return *this;
//	}
};

}
#endif /* BRRTXPLAN_H_ */
