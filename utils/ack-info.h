/*
 * ack-info.h
 *
 *  Created on: 24.02.2017
 *      Author: tsokalo
 */

#ifndef UTILS_ACK_INFO_H_
#define UTILS_ACK_INFO_H_

#include <iostream>
#include <map>

#include "ssn.h"

namespace ncr {
struct AckInfo: public std::map<GenId, bool> {

	AckInfo() {
		rxWinEnd = MAX_GEN_SSN;
	}

	void Serialize(std::stringstream &ss) {

		ss << (uint16_t) this->size() << DELIMITER;
		for (auto a : *this)
			ss << a.first << DELIMITER << (uint8_t) a.second << DELIMITER;

		ss << rxWinEnd << DELIMITER;
	}

	void Deserialize(std::stringstream &ss) {

		uint16_t n;
		this->clear();
		ss >> n;
		for (uint16_t i = 0; i < n; i++) {
			GenId g;
			ss >> g;
			uint8_t v;
			ss >> v;
			this->operator[](g) = v;
		}
		ss >> rxWinEnd;
	}

	friend std::ostream& operator<<(std::ostream& o, AckInfo& m) {
		o << "Size: " << m.size() << ", last GID " << m.rxWinEnd << " [";
		for (auto v : m)
			o << v.first << ":" << (uint16_t) v.second << " ";
		o << "]";
		return o;
	}
	/*
	 * we use string conversion for serialization in the simulator
	 * in real applications, the data will be much compressed with careful bit to bit conversion
	 * GetSerializedSize() give the header size for real applications
	 */
	uint32_t GetSerializedSize() {
		uint32_t ssize = 0;

		ssize += 1; // number of items in the list
		for (auto a : *this) {
			ssize += 2; //a.first - generation ID
			ssize += 1; //a.second - ACK/NACK
		}

		ssize += 2; // rxWinEnd - generation ID

		return ssize;
	}

	/*
	 * indicates the latest generation in the TX buffer; the vertex may already ACK this generation
	 */
	GenId rxWinEnd;

};
}

#endif /* UTILS_ACK_INFO_H_ */
