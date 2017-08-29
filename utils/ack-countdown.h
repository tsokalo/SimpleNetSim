/*
 * ack-countdown.h
 *
 *  Created on: 29.08.2017
 *      Author: tsokalo
 */

#ifndef UTILS_ACK_COUNTDOWN_H_
#define UTILS_ACK_COUNTDOWN_H_

#include <deque>
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

		if(counters.find(genId) == counters.end())counters[genId] += maxCount;
		AckBacklog::add(genId);
	}
	void remove(GenId genId) {

		counters.erase(genId);
		AckBacklog::remove(genId);
	}

	void tic() {
		std::vector<GenId> gids;
		for(auto &c : counters)
		{
			c.second--;
			if(c.second == 0)gids.push_back(c.first);
		}
		for(auto g : gids) this->remove(g);
	}

private:
	uint16_t maxCount;
	std::map<GenId, uint16_t> counters;
};

}

#endif /* UTILS_ACK_COUNTDOWN_H_ */
