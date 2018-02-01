/*
 * ack-backlog.h
 *
 *  Created on: 24.02.2017
 *      Author: tsokalo
 */

#ifndef UTILS_ACK_BACKLOG_H_
#define UTILS_ACK_BACKLOG_H_

#include <map>
#include <stdint.h>

#include "ssn.h"

namespace ncr {
struct AckBacklog : public std::deque<GenId> {

	typedef AckBacklog::iterator ack_it;
public:
	AckBacklog(uint16_t d) {
		this->d = d;
		SIM_ASSERT_MSG(d < (MAX_GEN_SSN >> 1), "Deepness on SRC equals 4*ACK_WIN_SIZE (4 * m_sp.numGen), MAX_GEN_SSN: " << MAX_GEN_SSN);
	}
	void add(GenId genId) {

		if(is_in(genId))return;

		auto it = this->begin();
		while (it != this->end()) {
			if (gen_ssn_t(*it) > gen_ssn_t(genId)) {
				this->insert(it, genId);
				break;
			}
			it++;
		}
		if (it == this->end()) this->push_back(genId);
		if (this->size() > d) this->pop_front();
	}
	void remove(GenId genId)
	{
		auto it = std::find(this->begin(), this->end(), genId);
		if(it != this->end())
		{
			this->erase(it, it + 1);
		}
	}

	bool is_in(GenId genId) {
		return (std::find(this->begin(), this->end(), genId) != this->end());
	}

	std::deque<GenId>::iterator last() {
		return this->empty() ? this->end() : (--(this->end()));
	}

	uint16_t tiefe() {
		return d;
	}

	std::deque<GenId> get_last(uint16_t n) {
		auto s = (n >= this->size()) ? 0 : (this->size() - n);
		return std::deque<GenId>(this->begin() + s, this->end());
	}

private:

	uint16_t d;
};

}

#endif /* UTILS_ACK_BACKLOG_H_ */
