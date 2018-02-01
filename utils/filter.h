/*
 * filter.h
 *
 *  Created on: 02.10.2016
 *      Author: tsokalo
 */

#ifndef FILTER_H_
#define FILTER_H_

#include <functional>
#include <random>
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include <algorithm>
#include <tuple>
#include <bitset>
#include <assert.h>
#include <iostream>
#include <sstream>

#include <boost/math/distributions/chi_squared.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/algorithm/minmax_element.hpp>

#include "ssn.h"
#include "log.h"
#include "bit-set.h"

#define SN_CYCLE_LENGTH        40

namespace ncr {

typedef Ssn<uint32_t, SN_CYCLE_LENGTH> ssn_t;

/*
 * N is the size of bit set. It works as a FIFO queue
 */
template<uint32_t N>
class BinaryFilter {
public:
	BinaryFilter() {
		m_filtered = 0;
		m_default = 1;
		m_is_ready = false;
		m_pos = 0;
		m_is_init = false;
		assert(m_vals.size() >= 16);
	}
	BinaryFilter(ssn_t startSn, uint32_t num_vals) {
		m_filtered = 0;
		m_default = 1;
		m_is_ready = false;
		m_pos = 0;
		m_is_init = false;
		assert(m_vals.size() >= 16);
		m_startSn = startSn;
		SIM_LOG(FILTER_LOG, "Configuring filter: " << startSn << ", " << num_vals);
		add(num_vals, 0);
		SIM_LOG(FILTER_LOG, "" << *this);
	}
	~BinaryFilter() {
	}
	bool is_init() {
		return m_is_init;
	}
	void fill(bool v) {
		add(m_vals.size(), v);
	}
	void adjust_start_sn(ssn_t startSn) {
		while (m_startSn != startSn) {
			if (m_pos == 0) {
				m_startSn = startSn;
				break;
			}
			m_vals >>= 1;
			m_startSn++;
			m_pos--;
		}
		m_filtered = 0;
		m_is_ready = false;
		if (m_pos == 0) m_is_init = false;
	}
	ssn_t get_start_sn() {
		return m_startSn;
	}
	uint32_t get_num_vals() {
		return m_pos;
	}
	void adjust_num_vals(uint32_t num) {
		while (m_pos != num) {
			if (m_pos == 0) {
				break;
			}
			m_vals.set(m_pos - 1, 0);
			m_pos--;
		}
	}
	void set_default(double v) {
		m_default = v;
	}
	bool operator[](uint32_t k) {
		assert(k < m_vals.size());
		return m_vals[k];
	}

	/*
	 * AND
	 */
	BinaryFilter<N>& operator&=(const BinaryFilter<N>& rhs) {
		SIM_LOG(FILTER_LOG, "&= " << *this << " : " << rhs);
		assert(m_is_init && rhs.m_is_init);
		assert(m_pos == rhs.m_pos);
		assert(m_startSn == rhs.m_startSn);
		m_vals &= rhs.m_vals;
		return *this;
	}
	/*
	 * OR
	 */
	BinaryFilter<N>& operator|=(const BinaryFilter<N>& rhs) {
		SIM_LOG(FILTER_LOG, "&= " << *this << " : " << rhs);
		assert(m_is_init && rhs.m_is_init);
		assert(m_pos == rhs.m_pos);
		assert(m_startSn == rhs.m_startSn);
		m_vals |= rhs.m_vals;
		return *this;
	}
	/*
	 * XOR
	 */
	BinaryFilter<N>& operator^=(const BinaryFilter<N>& rhs) {
		SIM_LOG(FILTER_LOG, "&= " << *this << " : " << rhs);
		assert(m_is_init && rhs.m_is_init);
		assert(m_pos == rhs.m_pos);
		assert(m_startSn == rhs.m_startSn);
		m_vals ^= rhs.m_vals;
		return *this;
	}
	friend std::ostream& operator<<(std::ostream& o, BinaryFilter<N> const& a) {
		o << a.m_vals.to_string() << " <" << a.m_startSn << ", " << a.m_pos << ">";
		return o;
	}
	void add(bool val) {
		m_is_init = true;
		if (m_pos == m_vals.size()) {
			m_pos--;
			m_vals >>= 1;
			m_startSn++;
		}
		(val) ? m_vals.set(m_pos) : m_vals.reset(m_pos);

		m_pos++;

		SIM_LOG(FILTER_LOG, "Added " << val << ", result: " << *this);
	}
	void add(int16_t num, bool val) {
		while (num-- > 0)
			add(val);
	}
	void update() {
		if (is_ready()) apply();
	}
	double val() {
		return (m_is_ready) ? m_filtered : m_default;
	}
	//
	// value without accuracy guarantees
	//
	double val_unrel() {
		if (m_is_init) apply();
		return m_filtered;
	}
	void reset() {
		for (size_t i = 0; i < m_vals.size(); i++) {
			m_vals.reset(i);
		}
	}
	void set_all(bool v) {
		for (uint32_t i = 0; i < m_pos; i++) {
			m_vals.set(i, v);
		}
	}
	virtual bool is_ready() {
		m_is_ready = m_pos * 2 > m_vals.size();
		return m_is_ready;
	}

	std::string Serialize() {
		std::stringstream ss;
//		std::cout << "SERIALIZE: Receive map: " << *this << std::endl;

		ss << m_pos << '\t';
		ss << m_startSn.val() << '\t';

		bitset b;
		for (uint32_t i = 0; i < N; i++)
			b.add(m_vals[i]);
		auto str = b.to_string();
		assert(!str.empty());

		ss << str << '\t';

		return ss.str();
	}

	void Deserialize(std::stringstream &ss) {

		m_vals.reset();
		m_pos = 0;

		auto v = m_startSn.val();
		ss >> m_pos;
		ss >> v;
		m_startSn = v;

		bitset b;
		auto n = b.get_raw_string_size(N);

		std::string str(n, 0);
		ss.read((char *) str.data(), 1);
		ss.read((char *) str.data(), n);

		b.from_string(str, N);
		for (uint32_t i = 0; i < N; i++)
			add(b.at(i));

		m_is_init = true;
		m_is_ready = false;

//		std::cout << "DESERIALIZE: Receive map: " << *this << std::endl;

	}

	void Print() {
		std::cout << m_vals.to_string() << std::endl;
	}

protected:

	virtual void
	apply() = 0;

	double m_filtered;
	std::bitset<N> m_vals;
	uint32_t m_pos;
	bool m_is_init;
	bool m_is_ready;
	double m_default;
	ssn_t m_startSn;
};

template<uint32_t N>
class AveBinaryFilter: public BinaryFilter<N> {
	using BinaryFilter<N>::m_filtered;
	using BinaryFilter<N>::m_vals;
	using BinaryFilter<N>::m_pos;
	using BinaryFilter<N>::m_is_ready;

public:
	AveBinaryFilter() {
		m_num_batches_bits = 4;
	}

	AveBinaryFilter(ssn_t startSn, uint32_t num_vals) :
			BinaryFilter<N>(startSn, num_vals) {
		m_num_batches_bits = 4;
	}

	bool is_ready() {
		SIM_LOG(FILTER_LOG, "current position: " << m_pos << " max size: " << m_vals.size ());
		if (m_pos < m_vals.size()) return false;

		uint32_t batch_size = m_vals.size() >> m_num_batches_bits, j = 0;
		std::vector<double> v;

		SIM_LOG(FILTER_LOG, m_vals.to_string () << " batch size: " << batch_size);

		while (j++ < (uint32_t)(1 << m_num_batches_bits) - 1) {
			auto temp_vals = ((m_vals << (batch_size * j)) >> (m_vals.size() - batch_size));
			v.push_back((double) temp_vals.count() / (double) batch_size);
			SIM_LOG(FILTER_LOG, temp_vals.to_string () << " -> " << (double) temp_vals.count () / (double) batch_size);
		}

		double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
		double stdev = std::sqrt(std::inner_product(v.begin(), v.end(), v.begin(), 0.0) / v.size() - mean * mean);

		SIM_LOG(FILTER_LOG, "mean: " << mean << " stdev: " << stdev);

		m_is_ready = stdev < 0.2;
		return m_is_ready;
	}

private:

	void apply() {
		if (m_pos != 0) {
			m_filtered = (double) m_vals.count() / (double) ((m_pos != m_vals.size()) ? m_pos : m_vals.size());
		}
	}
	uint32_t m_num_batches_bits;
};

}	//ncr
#endif /* FILTER_H_ */
