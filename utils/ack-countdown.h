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
			counters[genId] = maxCount;
			c_in_mem.push_back(genId);
			if (c_in_mem.size() > max_in_mem) c_in_mem.pop_front();
			AckBacklog::add(genId);
		}
	}
	void remove(GenId genId) {

		if (std::find(c_in_mem.begin(), c_in_mem.end(), genId) != c_in_mem.end()) {
			counters.erase(genId);
			AckBacklog::remove(genId);
		}
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

	void reset(GenId genId) {
		counters[genId] = maxCount;
	}

private:
	uint16_t maxCount;
	std::map<GenId, uint16_t> counters;

	std::deque<GenId> c_in_mem;
	static const uint16_t max_in_mem = 300;
};

struct AckHistory {

	typedef std::shared_ptr<AckCountDown> ack_count_ptr;

public:
	AckHistory(uint16_t maxCount) {
		this->maxCount = maxCount;
	}

	void add(UanAddress addr, GenId genId) {
		if (h.find(addr) != h.end()) {
			h[addr]->reset(genId);
		} else {
			h.emplace(addr, ack_count_ptr(new AckCountDown(1000, maxCount)));
			h[addr]->add(genId);
		}
	}
	void tic() {
		for (auto v : h)
			v.second->tic();
	}
	bool is_in(UanAddress addr, GenId genId) {
		if (h.find(addr) == h.end()) return false;
		return h[addr]->is_in(genId);
	}
private:

	uint16_t maxCount;
	std::map<UanAddress, ack_count_ptr> h;
};

}

#endif /* UTILS_ACK_COUNTDOWN_H_ */
