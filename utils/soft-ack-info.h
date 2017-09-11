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

struct SoftAckInfoStatus
{
	enum SoftAckInfoStatus_
	{
		UP_TO_DATE,
		NOT_UP_TO_DATE
	};
	enum SoftAckInfo_
	{
		ACKED,
		NACKED // :)
	};

	SoftAckInfoStatus()
	{
		sais = NOT_UP_TO_DATE;
		sai = NACKED;
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

	void set_up_to_date(GenId gid, bool v)
	{
		SIM_LOG_FUNC(BRR_LOG);

		this->operator[](gid).sais = v ? SoftAckInfoStatus::UP_TO_DATE : SoftAckInfoStatus::NOT_UP_TO_DATE;
	}

	bool is_up_to_date(GenId gid)
	{
		if(this->find(gid) == this->end())return false;
		return (this->operator[](gid).sais == SoftAckInfoStatus::UP_TO_DATE);
	}

	void set_acked(GenId gid, bool v)
	{
		SIM_LOG_FUNC(BRR_LOG);

		this->operator[](gid).sai = v ? SoftAckInfoStatus::ACKED : SoftAckInfoStatus::NACKED;
	}

	bool is_acked(GenId gid)
	{
		if(this->find(gid) == this->end())return false;
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
