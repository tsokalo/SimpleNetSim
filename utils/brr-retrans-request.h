/*
 * brr-retrans-request.h
 *
 *  Created on: 07.01.2017
 *      Author: tsokalo
 */

#ifndef BRRRETRANSREQUEST_H_
#define BRRRETRANSREQUEST_H_

#include <sstream>
#include <vector>
#include <string.h>
#include "header.h"
#include "utils/coder-info.h"
#include "utils/bit-set.h"
#include "utils/coding-vector.h"
#include "coder-help-info.h"

namespace ncr {

struct DecoderDetails {
	// either
	CodingMatrix codingCoefs;
	// or
	CoderInfo coderInfo;
	//or
	/*
	 * feedback with CCACK
	 */
	CodingVector hashVector;

	void Serialize(std::stringstream &ss) {

		/////////////////////////////////////////////////////////////////////////
		auto haveCodingMatrix = (!codingCoefs.empty());
		//
		// we do not support the serializatio of coding matrices in derivatives of ANChOR;
		// nevertheless, we provide the sample code that can do this job
		//
		assert(!haveCodingMatrix);

		ss << (uint8_t) haveCodingMatrix << DELIMITER;
		if (haveCodingMatrix) {
			ss << (uint16_t) codingCoefs.size() << DELIMITER;

			if (codingCoefs.size() != 0) {
				ss << (uint16_t) codingCoefs.begin()->size() << DELIMITER;
				for (auto v : codingCoefs) {
					ss << std::string((const char*) v.data(), codingCoefs.begin()->size());
				};;
			}
		}
		/////////////////////////////////////////////////////////////////////////
		auto haveCoderInfo = (!coderInfo.seen.empty());

		ss << (uint8_t) haveCoderInfo << DELIMITER;
		if (haveCoderInfo) {
			ss << coderInfo.rank << DELIMITER;
			ss << coderInfo.genSize << DELIMITER;

			bitset b;
			for (auto v : coderInfo.seen)
				b.add(v);
			ss << b.to_string() << DELIMITER;
			b.clear();
			for (auto v : coderInfo.decoded)
				b.add(v);
			ss << b.to_string() << DELIMITER;
		}
		/////////////////////////////////////////////////////////////////////////
		auto haveHashVector = (!hashVector.empty());

		ss << (uint8_t) haveHashVector << DELIMITER;
		if (haveHashVector) {
			ss << (uint16_t) hashVector.size() << DELIMITER;
			ss << hashVector.to_string() << DELIMITER;
		}
	}

	void Deserialize(std::stringstream &ss) {

		/////////////////////////////////////////////////////////////////////////
		bool haveCodingMatrix = false;
		uint8_t w;
		ss >> w;
		haveCodingMatrix = w;
		if (haveCodingMatrix) {
			codingCoefs.clear();

			uint16_t n, m;
			ss >> n;

			if (n != 0) {
				ss >> m;
				std::string d;
				char *buffer = new char[(uint32_t) n * m + 1];
				ss.read(buffer, 1);
				ss.read(buffer, (uint32_t) n * m);

				CodingVector v(m);
				for (uint16_t i = 0; i < n; i++) {

					v.assign(buffer + m * i, buffer + m * (i + 1));
					codingCoefs.push_back(v);
				}
				delete[] buffer;
			}
		}
		/////////////////////////////////////////////////////////////////////////
		bool haveCoderInfo = false;
		ss >> w;
		haveCoderInfo = w;
		if (haveCoderInfo) {
			ss >> coderInfo.rank;
			ss >> coderInfo.genSize;

			bitset b;
			auto n = b.get_raw_string_size(coderInfo.genSize);

			std::string str(n, 0);
			ss.read((char *) str.data(), 1);
			ss.read((char *) str.data(), n);

			b.from_string(str, coderInfo.genSize);
			coderInfo.seen.clear();
			coderInfo.seen = b.get_vals();

			ss.read((char *) str.data(), 1);
			ss.read((char *) str.data(), n);

			b.from_string(str, coderInfo.genSize);
			coderInfo.decoded.clear();
			coderInfo.decoded = b.get_vals();
		}
		/////////////////////////////////////////////////////////////////////////
		bool haveHashVector = false;
		ss >> w;
		haveHashVector = w;
		if (haveHashVector) {
			uint16_t s = 0;
			ss >> s;
			std::string str;
			ss >> str;
			hashVector.from_string(str);
		}
	}

	DecoderDetails& operator=(const DecoderDetails& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->codingCoefs = other.codingCoefs;
			this->coderInfo = other.coderInfo;
			this->hashVector = other.hashVector;
		}
		return *this;
	}
	DecoderDetails& operator=(const CoderHelpInfo& other) // copy assignment
			{

		this->codingCoefs = other.m;
		this->coderInfo = other.c;
		this->hashVector = other.hashVec;

		return *this;
	}
	/*
	 * we use string conversion for serialization in the simulator
	 * in real applications, the data will be much compressed with careful bit to bit conversion
	 * GetSerializedSize() give the header size for real applications
	 */
	uint32_t GetSerializedSize() {
		uint32_t ssize = 0;

		ssize += 2; // generation size
		for(auto v: codingCoefs)
		{
			ssize += v.size(); // size of the coding vector
		}

		ssize += ceil(coderInfo.genSize / 8); // map of seen
		ssize += ceil(coderInfo.genSize / 8); // map of decoded
		// coderInfo.rank can be obtained from the map of seen
		// coderInfor.genSize is already specified for the coding matrix size

		ssize += hashVector.size();

		return ssize;
	}

};

struct RetransRequestInfo: public std::map<GenId, DecoderDetails> {

	RetransRequestInfo() {
		forwarder = 0;
	}

	RetransRequestInfo& operator=(const RetransRequestInfo& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->clear();
			this->insert(other.begin(), other.end());
			this->forwarder = other.forwarder;
		}
		return *this;
	}

	void Serialize(std::stringstream &ss) {

		ss << forwarder << DELIMITER;
		ss << (uint16_t) this->size() << DELIMITER;
		for (auto m : *this) {
			ss << m.first << DELIMITER;
			m.second.Serialize(ss);
		};;
	}

	void Deserialize(std::stringstream &ss) {

		ss >> forwarder;
		uint16_t n;
		ss >> n;
		DecoderDetails decDet;
		for (uint16_t i = 0; i < n; i++) {
			GenId genId;
			ss >> genId;
			decDet.Deserialize(ss);
			this->operator [](genId) = decDet;
		}
	}

	/*
	 * we use string conversion for serialization in the simulator
	 * in real applications, the data will be much compressed with careful bit to bit conversion
	 * GetSerializedSize() give the header size for real applications
	 */
	uint32_t GetSerializedSize() {
		uint32_t ssize = 0;

		ssize += 1; // forwarder address

		ssize += 1; // number of items in the list
		for (auto m : *this) {
			ssize += 1; // generation ID
			ssize += m.second.GetSerializedSize();
		}

		return ssize;
	}

	/*
	 * retransmission request forwarder
	 */
	UanAddress forwarder;

};

}

#endif /* BRRRETRANSREQUEST_H_ */
