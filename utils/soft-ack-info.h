/*
 * soft-ack-info.h
 *
 *  Created on: 11.09.2017
 *      Author: tsokalo
 */

#ifndef UTILS_SOFT_ACK_INFO_H_
#define UTILS_SOFT_ACK_INFO_H_

#include <iostream>
#include <map>
#include <stdint.h>

#include "utils/log.h"

namespace ncr {

struct SoftAckInfoStatus {
	enum SoftAckInfoStatus_ {
		/*
		 * 1. there must be no data for the corresponding generation in the forwarding plan
		 * 2.1. if received the soft feedback from all cooperating nodes; SoftAckInfo_ can be both ACKED and NACKED
		 * 2.2. if SoftAckInfo_ is ACKED
		 */
		UP_TO_DATE,
		/*
		 * if 1. or 2. does not fulfill
		 */
		NOT_UP_TO_DATE
	};
	enum SoftAckInfo_ {
		/*
		 * if no soft feedback is present for any of the cooperating nodes
		 */
		NO_INFO,
		/*
		 * if any subgroup of the cooperating nodes can decode the sent message
		 */
		ACKED,
		/*
		 * if the subgroup of nodes, which delivered the soft feedback, cannot decode the original message
		 */
		NACKED // :)
	};

	SoftAckInfoStatus() {
		sais = NOT_UP_TO_DATE;
		sai = NO_INFO;
	}

	SoftAckInfoStatus& operator=(const SoftAckInfoStatus& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->sais = other.sais;
			this->sai = other.sai;
		}
		return *this;
	}

	friend std::ostream& operator<<(std::ostream& o, SoftAckInfoStatus& m) {

		o << m.sais << "," << m.sai;
		return o;
	}

	SoftAckInfoStatus_ sais;
	SoftAckInfo_ sai;
};

struct SoftAckInfo: public std::map<GenId, SoftAckInfoStatus> {

	void set_up_to_date(GenId gid, bool v) {
		SIM_LOG_FUNC(BRR_LOG);

		this->operator[](gid).sais = v ? SoftAckInfoStatus::UP_TO_DATE : SoftAckInfoStatus::NOT_UP_TO_DATE;
		this->operator[](gid).sai = !v ? SoftAckInfoStatus::NACKED : this->operator[](gid).sai;
	}

	bool is_up_to_date(GenId gid) {
		if (this->find(gid) == this->end()) return false;
		return (this->operator[](gid).sais == SoftAckInfoStatus::UP_TO_DATE);
	}

	void set_acked(GenId gid, bool v) {
		SIM_LOG_FUNC(BRR_LOG);

		this->operator[](gid).sai = v ? SoftAckInfoStatus::ACKED : SoftAckInfoStatus::NACKED;
		this->operator[](gid).sais = v ? SoftAckInfoStatus::UP_TO_DATE : this->operator[](gid).sais;
	}

	bool is_acked(GenId gid) {
		if (this->find(gid) == this->end()) return false;
		return (this->operator[](gid).sai == SoftAckInfoStatus::ACKED);
	}

	friend std::ostream& operator<<(std::ostream& o, SoftAckInfo& m) {
		o << "Size: " << m.size() << " [";
		for (auto v : m)
			o << v.first << ":" << v.second << " ";
		o << "]";
		return o;
	}
};

}

#endif /* UTILS_SOFT_ACK_INFO_H_ */
