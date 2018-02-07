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

	void add(GenId gid) {

		if (counters.find(gid) == counters.end()) {
			counters[gid] = maxCount;
			auto r_gid = AckBacklog::add(gid);
			if(r_gid != MAX_GEN_SSN + 1)counters.erase(r_gid);
		}
	}
	void remove(GenId genId) {

		if (counters.find(genId) != counters.end()) {
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

	friend std::ostream& operator<<(std::ostream& o, AckCountDown& m) {

		o << "(";
		for (auto v : m.counters) {
			o << v.first << ":" << v.second << ",";
		}
		o << ")";
		return o;
	}

private:
	uint16_t maxCount;
	std::map<GenId, uint16_t> counters;
};

struct BrrAckTimer {

	typedef std::shared_ptr<AckCountDown> ack_count_ptr;

public:
	BrrAckTimer(uint16_t d, uint16_t maxCount) {
		this->maxCount = maxCount;
		this->d = d;
	}

	void add(UanAddress addr, GenId genId) {
		if (h.find(addr) != h.end()) {
			h[addr]->reset(genId);
		} else {
			h.emplace(addr, ack_count_ptr(new AckCountDown(d, maxCount)));
			h[addr]->add(genId);
		}
	}
	void tic() {
		for (auto v : h)
			v.second->tic();
	}
	bool is_in(UanAddress addr, GenId genId) {
		if (h.empty() || h.find(addr) == h.end()) return false;
		return h[addr]->is_in(genId);
	}

	friend std::ostream& operator<<(std::ostream& o, BrrAckTimer& m) {

		o << "[";
		for (auto v : m.h) {
			o << v.first << " - " << *(v.second) << ",";
		}
		o << "]";
		return o;
	}

private:

	uint16_t maxCount;
	std::map<UanAddress, ack_count_ptr> h;
	uint16_t d;
};

}

#endif /* UTILS_ACK_COUNTDOWN_H_ */
