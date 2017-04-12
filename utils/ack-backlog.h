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
struct AckBacklog {

	typedef std::deque<GenId> buf_t;
	typedef buf_t::iterator ack_it;
public:
	AckBacklog(uint16_t deepness) {
		this->deepness = deepness;
		SIM_ASSERT_MSG(deepness < (MAX_GEN_SSN >> 1), "Deepness on SRC equals 4*ACK_WIN_SIZE (4 * m_sp.numGen), MAX_GEN_SSN: " << MAX_GEN_SSN);
	}
	void add(GenId genId) {

		if(is_in(genId))return;

		auto it = bl.begin();
		while (it != bl.end()) {
			if (gen_ssn_t(*it) > gen_ssn_t(genId)) {
				bl.insert(it, genId);
				break;
			}
			it++;
		}
		if (it == bl.end()) bl.push_back(genId);
		if (bl.size() > deepness) bl.pop_front();
	}
	void remove(GenId genId)
	{
		auto it = std::find(bl.begin(), bl.end(), genId);
		if(it != bl.end())
		{
			bl.erase(it, it + 1);
		}
	}

	bool is_in(GenId genId) {
		return (std::find(bl.begin(), bl.end(), genId) != bl.end());
	}

	ack_it begin() {
		return bl.begin();
	}

	ack_it end() {
		return bl.end();
	}
	ack_it last() {
		return bl.empty() ? bl.end() : (--(bl.end()));
	}

	bool empty() {
		return bl.empty();
	}
	uint16_t size() {
		return deepness;
	}

	buf_t get_last(uint16_t n) {
		return (n >= bl.size()) ? bl : buf_t(bl.begin() + (bl.size() - n), bl.end());
	}

private:

	uint16_t deepness;
	buf_t bl;
};

}

#endif /* UTILS_ACK_BACKLOG_H_ */
