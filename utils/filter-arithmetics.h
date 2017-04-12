/*
 * filter-arithmetics.h
 *
 *  Created on: 02.10.2016
 *      Author: tsokalo
 */

#ifndef FILTERARITHMETICS_H_
#define FILTERARITHMETICS_H_

#include "ssn.h"
#include "filter.h"

// CODER_INFO_FILTER_DEPTH should be equal or greater the generation size
#define CODER_INFO_FILTER_DEPTH          1000
#define LIN_DEP_FILTER_DEPTH      		 16

namespace ncr {

typedef AveBinaryFilter<CODER_INFO_FILTER_DEPTH> RcvMap;
typedef AveBinaryFilter<LIN_DEP_FILTER_DEPTH> LinDepMap;

class FilterArithmetics {

public:
	FilterArithmetics() {
		m_synced = false;
		m_minSize = 10;
	}
	virtual ~FilterArithmetics() {
	}

	void add(RcvMap f) {
		m_synced = false;
		m_lps.push_back(f);
	}

	RcvMap do_and() {
		assert(m_synced);
		m_result->set_all(1);
		for(auto f : m_lps) (*m_result) &= f;;
		return *m_result;
	}
	RcvMap do_or() {
		assert(m_synced);
		m_result->set_all(0);
		for(auto f : m_lps) (*m_result) |= f;;
		return *m_result;
	}
	RcvMap do_xor() {
		assert(m_synced);
		m_result->set_all(0);
		for(auto f : m_lps) (*m_result) ^= f;;
		return *m_result;
	}

	uint32_t count() {
		return m_lps.size();
	}

	bool check_sync()
	{
		do_sync();
		return m_synced;
	}

private:

	void do_sync() {

		if (m_lps.empty()) return;
		SIM_LOG (FILTER_LOG, "Adjust start SN");

		std::vector<RcvMap> lps;
		for(auto f : m_lps)if(f.get_num_vals() >= m_minSize)lps.push_back(f);
		m_lps.swap(lps);
		if (m_lps.empty()) return;

		ssn_t max_start_sn;
		for(auto f : m_lps)if(f.get_start_sn() > max_start_sn)max_start_sn=f.get_start_sn();;

		for (auto &f : m_lps)
		{
			SIM_LOG (FILTER_LOG, "Before: " << f);
			f.adjust_start_sn(max_start_sn);
			SIM_LOG (FILTER_LOG, "After: " << f);
		};;

		SIM_LOG (FILTER_LOG, "Adjust number of values");
		std::size_t min_size = std::numeric_limits<std::size_t>::max();
		for(auto f : m_lps)if(f.get_num_vals() < min_size)min_size=f.get_num_vals();;

		for (auto &f : m_lps)
		{
			SIM_LOG (FILTER_LOG, "Before: " << f);
			f.adjust_num_vals(min_size);;
			SIM_LOG (FILTER_LOG, "After: " << f);
		};;
		m_synced = true;
		m_result.reset();
		m_result = std::shared_ptr<RcvMap>(new RcvMap(max_start_sn, min_size));
		return;
	}

	std::vector<RcvMap> m_lps;
	bool m_synced;
	std::shared_ptr<RcvMap> m_result;
	std::size_t m_minSize;
};

void TestFilterArithmetics();
}//ncr

#endif /* FILTERARITHMETICS_H_ */
