/*
 * ack-countdown.h
 *
 *  Created on: 29.08.2017
 *      Author: tsokalo
 */

#ifndef UTILS_ACK_COUNTDOWN_H_
#define UTILS_ACK_COUNTDOWN_H_

#include <deque>
#include <algorithm>
#include <assert.h>
#include "ack-backlog.h"

namespace ncr {

struct AckCountDown: public AckBacklog {

public:
	AckCountDown(uint16_t d, uint16_t maxCount) :
			AckBacklog(d) {
		this->maxCount = maxCount;
		assert(this->maxCount > 0);
	}

	void add(GenId genId) {

		if (std::find(c_in_mem.begin(), c_in_mem.end(), genId) == c_in_mem.end()) {
			counters[genId] += maxCount;
			c_in_mem.push_back(genId);
			if(c_in_mem.size() > max_in_mem)c_in_mem.pop_front();
			AckBacklog::add(genId);
		}
	}
	void remove(GenId genId) {

		counters.erase(genId);
		AckBacklog::remove(genId);
	}

	void tic() {
		std::vector<GenId> gids;
		for (auto &c : counters) {
			c.second--;
			if (c.second == 0) gids.push_back(c.first);
		}
		for (auto g : gids)
			this->remove(g);
	}

private:
	uint16_t maxCount;
	std::map<GenId, uint16_t> counters;

	std::deque<GenId> c_in_mem;
	static const uint16_t max_in_mem = 300;
};

}

#endif /* UTILS_ACK_COUNTDOWN_H_ */
