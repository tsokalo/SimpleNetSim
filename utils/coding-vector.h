/*
 * coding-vector.h
 *
 *  Created on: 24.02.2017
 *      Author: tsokalo
 */

#ifndef UTILS_CODING_VECTOR_H_
#define UTILS_CODING_VECTOR_H_

#include <vector>
#include <stdint.h>
#include <iostream>
#include <string.h>

namespace ncr
{
struct CodingVector: public std::vector<uint8_t> {
	CodingVector() {
	}
	CodingVector(uint32_t s) :
			std::vector<uint8_t>(s) {
	}
	CodingVector(uint32_t s, uint8_t v) :
			std::vector<uint8_t>(s, v) {
	}
	CodingVector(std::vector<uint8_t> v) :
			std::vector<uint8_t>(v) {
	}
	friend std::ostream&
	operator<<(std::ostream& os, const CodingVector& l) {
		os << "<";
		for (auto v : l)
			os << (uint16_t) v << " ";
		os << ">";
		return os;
	}
	std::string to_string() {
		std::string str;
		for (auto s : *this)
			str.push_back(s);
		return str;
	}
	void from_string(std::string str) {
		this->clear();
		for (auto s : str)
			this->push_back(s);
	}
};
typedef std::vector<CodingVector> CodingMatrix;
}

#endif /* UTILS_CODING_VECTOR_H_ */
