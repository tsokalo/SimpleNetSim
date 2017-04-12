/*
 * long-bit-set.h
 *
 *  Created on: 18.01.2017
 *      Author: tsokalo
 */

#ifndef UTILS_BIT_SET_H_
#define UTILS_BIT_SET_H_

#include <vector>
#include <iostream>
#include <assert.h>
#include <string.h>

#define BITSET_KEY	'k'

namespace ncr {

struct bitset {

	bitset() {

	}
	bitset(std::string str) {
		from_full_string(str);
	}
	void add(bool v) {
		vals.push_back(v);
	}
	void set(bool v, uint32_t pos) {
		assert(pos < vals.size());
		vals.at(pos) = v;
	}
	bool at(uint32_t pos) {
		assert(pos < vals.size());
		return vals.at(pos);
	}
	uint32_t size() {
		return vals.size();
	}
	void clear() {
		vals.clear();
	}

	std::string to_string() {

		auto s = vals.size();
		auto n = (s >> 3);
		n = (n << 3 < s) ? n + 1 : n;
		std::string str(n, 0);
		uint32_t c = 0;
		for (auto v : vals) {
			auto i = (c >> 3);
			assert(i < str.size());
			auto j = c - (i << 3);
			str[i] |= (v << j);
			c++;
		}

		str.push_back(BITSET_KEY);
		return str;
	}
	void from_string(std::string str, uint32_t n) {

		assert((str.size() << 3) >= n);
		str.pop_back();

		vals.clear();
		for (uint32_t c = 0; c < n; c++) {
			auto i = (c >> 3);
			assert(i < str.size());
			auto j = c - (i << 3);
			vals.push_back(str[i] & (1 << j));
		}
	}
	uint16_t get_raw_string_size(uint32_t s = 0) {
		if (s == 0) s = vals.size();
		uint32_t n = (s >> 3);
		n = (n << 3 < s) ? n + 1 : n;
		n++;// BITSET_KEY
		return n;
	}

	void from_full_string(std::string str) {
		vals.clear();
		for (auto s : str)
			vals.push_back((s == '1') ? true : false);
	}
	std::string to_full_string() {
		std::stringstream ss;
		for (auto v : vals)
			ss << v;
		return ss.str();
	}

	friend std::ostream&
	operator<<(std::ostream& os, bitset& l) {

		std::cout << "<";
		for (auto v : l.get_vals())
			std::cout << v;
		std::cout << ">";
		return os;
	}

	std::vector<bool> get_vals() {
		return vals;
	}

	double get_ratio() {
		uint32_t c = 0;
		for (auto v : vals)
			if (v) c++;
		return (double) c / (double) vals.size();
	}
private:

	std::vector<bool> vals;
};

}

#endif /* UTILS_BIT_SET_H_ */
