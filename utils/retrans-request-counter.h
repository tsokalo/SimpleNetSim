/*
 * retrans-request-counter.h
 *
 *  Created on: 07.06.2017
 *      Author: tsokalo
 */

#ifndef UTILS_RETRANS_REQUEST_COUNTER_H_
#define UTILS_RETRANS_REQUEST_COUNTER_H_

#include "header.h"

namespace ncr {

struct RetransRequestCounter: std::map<GenId, uint16_t> {
public:
	RetransRequestCounter(uint16_t numRr) :
			m_numRr(numRr) {
	}

	bool is_expired(GenId gid) {
		if (m_numRr == 0)
			return true;
		if (this->find(gid) == this->end())
			return false;
		return (this->operator[](gid) >= m_numRr);
	}
	void increment(GenId gid) {
		(this->operator[](gid))++;
		assert(this->operator[](gid) <= m_numRr);
	}
	void forget(GenId gid) {
		if (this->find(gid) != this->end())
			this->erase(gid);
	}

private:
	uint16_t m_numRr;
};

}

#endif /* UTILS_RETRANS_REQUEST_COUNTER_H_ */
