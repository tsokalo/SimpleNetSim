/*
 * retrans-gen-id.h
 *
 *  Created on: 24.02.2017
 *      Author: tsokalo
 */

#ifndef UTILS_RETRANS_GEN_ID_H_
#define UTILS_RETRANS_GEN_ID_H_

#include <iostream>

#include "ssn.h"

namespace ncr
{

struct RetransGenId {
	RetransGenId() {
		genId = MAX_GEN_SSN + 1;
	}
	bool geq(GenId genId) {
		return (gen_ssn_t(this->genId) < gen_ssn_t(genId));
	}
	bool is_default() {
		return (genId == MAX_GEN_SSN + 1);
	}
	void set_default() {
		genId = MAX_GEN_SSN + 1;
	}
	void set_if_older(GenId genId) {
		if (is_default()) {
			this->genId = genId;
		}
		else {
			if (geq(genId)) {
				this->genId = genId;
			}
		}
	}

	RetransGenId&
	operator=(const GenId& genId) // copy assignment
			{
		this->genId = genId;
		return *this;
	}

	RetransGenId&
	operator=(const RetransGenId& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->genId = other.genId;
		}
		return *this;
	}
	friend std::ostream&
	operator<<(std::ostream& os, const RetransGenId& l) {

		os << l.genId;
		return os;
	}

	GenId genId;
};
}

#endif /* UTILS_RETRANS_GEN_ID_H_ */
