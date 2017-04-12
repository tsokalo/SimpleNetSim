/*
 * feedback-estimator.h
 *
 *  Created on: 18.01.2017
 *      Author: tsokalo
 */

#ifndef UTILS_FEEDBACK_ESTIMATOR_H_
#define UTILS_FEEDBACK_ESTIMATOR_H_

#include <vector>
#include <map>
#include <stdint.h>
#include <assert.h>
#include "utils/coder-info.h"

namespace ncr {

class FeedbackEstimator {
public:
	FeedbackEstimator(CoderInfo l, CoderInfo r) :
			local(l), remote(r) {

		assert(l.genSize == r.genSize);
		assert(l.rank <= l.genSize);
		assert(r.rank <= r.genSize);
		this->genSize = l.genSize;
	}
	double GetN() {
		double low_bound = GetLowBound();
		double top_bound = GetTopBound();
		return (low_bound > top_bound) ? low_bound : (low_bound + 0.5 * (top_bound - low_bound));
	}
	double GetP() {
		double top_bound = GetTopBound();
		top_bound /= (double) genSize;
		return top_bound * (1 - 1 * top_bound) * genSize;
	}

	uint32_t GetLowBound() {
		int32_t minRank = local.rank - remote.rank;
		assert(remote.seen.size() == local.seen.size());
		int32_t minLocal = 0;
		auto r_it = remote.seen.begin();
		auto l_it = local.seen.begin();
		while (l_it != local.seen.end()) {
			if (*l_it && !(*r_it)) minLocal++;
			l_it++;
			r_it++;
		}
		return (minRank < minLocal) ? minLocal : minRank;
	}
	uint32_t GetTopBound() {
		int32_t maxRemote = remote.genSize - remote.rank;
		assert(remote.decoded.size() == local.seen.size());
		uint32_t n = 0;
		auto r_it = remote.decoded.begin();
		auto l_it = local.seen.begin();
		while (l_it != local.seen.end()) {
			if (*l_it && *r_it) n++;
			l_it++;
			r_it++;
		}
		int32_t maxLocal = local.rank - n;
		return (maxRemote < maxLocal) ? maxRemote : maxLocal;
	}
	CoderInfo GetMergedCoderInfo() {

		CoderInfo merged;
		merged.genSize = genSize;
		merged.rank = remote.rank + ceil(GetN());
		merged.rank = (merged.rank > merged.genSize) ? merged.genSize : merged.rank;
		merged.seen.resize(genSize);
		merged.decoded.resize(genSize);

		auto l_it = local.seen.begin();
		auto r_it = remote.seen.begin();
		auto m_it = merged.seen.begin();
		while (l_it != local.seen.end()) {
			*m_it = (*l_it || *r_it);
			l_it++;
			r_it++;
			m_it++;
		}
		l_it = local.decoded.begin();
		r_it = remote.decoded.begin();
		m_it = merged.decoded.begin();
		while (l_it != local.decoded.end()) {
			*m_it = (*l_it || *r_it);
			l_it++;
			r_it++;
			m_it++;
		}

		return merged;
	}
private:
	CoderInfo local;
	CoderInfo remote;
	uint32_t genSize;

};
}

#endif /* UTILS_FEEDBACK_ESTIMATOR_H_ */
