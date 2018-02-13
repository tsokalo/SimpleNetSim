/*
 * brr-feedback.h
 *
 *  Created on: 07.01.2017
 *      Author: tsokalo
 */

#ifndef BRRMFEEDBACK_H_
#define BRRMFEEDBACK_H_

#include <sstream>
#include <string.h>

#include "header.h"
#include "utils/brr-retrans-request.h"
#include "utils/ack-info.h"
#include "brr-feedback.h"

namespace ncr {

struct FeedbackMInfo: public FeedbackInfo {

	FeedbackMInfo() :
			FeedbackInfo() {
	}
	FeedbackMInfo(const FeedbackInfo &other) {
		FeedbackInfo::operator=(other);
	}
	FeedbackMInfo(const FeedbackMInfo &other) {
		FeedbackMInfo::operator=(other);
	}

	FeedbackMInfo& operator=(const FeedbackMInfo& other) {
		if (this != &other) { // self-assignment check expected

			FeedbackInfo::operator=(other);
			this->ps = other.ps;
		}
		return *this;
	}
	FeedbackMInfo& operator=(const FeedbackInfo& other) {
		FeedbackInfo::operator=(other);
		return *this;
	}

	void Reset() {
		FeedbackInfo::Reset();
		this->ps.clear();
	}

	void Serialize(std::stringstream &ss) {

		FeedbackInfo::Serialize(ss);
		ss << (uint16_t) ps.size() << DELIMITER;
		for (auto p_ : ps)
			ss << p_.first << DELIMITER << p_.second << DELIMITER;
	}

	void Deserialize(std::stringstream &ss) {

		FeedbackInfo::Deserialize(ss);
		double v;
		uint16_t n;
		ss >> n;
		ps.clear();
		for (uint16_t i = 0; i < ps.size(); i++) {
			UanAddress a;
			ss >> a;
			ss >> v;
			ps[a] = v;
		}
	}

	/*
	 * we use string conversion for serialization in the simulator
	 * in real applications, the data will be much compressed with careful bit to bit conversion
	 * GetSerializedSize() give the header size for real applications
	 */
	uint32_t GetSerializedSize() {
		uint32_t ssize = 0;

		for (auto v : ps) {
			ssize += 1; //v.first - sink address
			ssize += 2; //v.second.val() - priority value (conversion from double to int16_t)
		}
		ssize += 1; // number of items in ps list

		ssize += FeedbackInfo::GetSerializedSize();

		return ssize;
	}

	std::unordered_map<UanAddress, priority_t> ps;
};

}
#endif /* BRRMFEEDBACK_H_ */
