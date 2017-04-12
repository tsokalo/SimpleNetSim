/*
 * TrafficSink.h
 *
 *  Created on: 01.12.2016
 *      Author: tsokalo
 */

#ifndef TRAFFICSINK_H_
#define TRAFFICSINK_H_

#include <functional>
#include <deque>
#include <tuple>
#include <memory>
#include <stdlib.h>

#include "header.h"
#include "utils/ssn.h"

namespace ncr {

class TrafficSink {

	typedef std::function<void(MessType)> set_msg_type_func;

public:
	TrafficSink() {

	}
	virtual ~TrafficSink() {

	}

	symb_ssn_t Receive(OrigSymbol symb) {
		assert(symb.size() >= m_ssn.size());
		assert(m_setMessType);

		memcpy(m_ssn.data(), &symb[0], m_ssn.size());
		m_setMessType(ORIG_MSG_TYPE);

		return m_ssn.val();
	}

	void SetMessTypeCallback(set_msg_type_func f) {
		m_setMessType = f;
	}

private:

	symb_ssn_t m_ssn;
	set_msg_type_func m_setMessType;

};
}//ncr

#endif /* TRAFFICSINK_H_ */
