/*
 * coder-help-info.h
 *
 *  Created on: 18.01.2017
 *      Author: tsokalo
 */

#ifndef UTILS_CODER_HELP_INFO_H_
#define UTILS_CODER_HELP_INFO_H_

#include "header.h"
#include "utils/coder-info.h"
#include "utils/coding-vector.h"

namespace ncr {

struct CoderHelpInfo {
	CoderHelpInfo() {
		finRank = 0;
		origRank = 0;
	}
	CoderHelpInfo(CodingMatrix m, CoderInfo c, CodingVector hashVec) {
		this->m = m;
		this->c = c;
		this->hashVec = hashVec;
		finRank = 0;
		origRank = 0;
	}
	CoderHelpInfo& operator=(const CoderHelpInfo& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->m = other.m;
			this->c = other.c;
			this->hashVec = other.hashVec;
			this->finRank = other.finRank;
			this->origRank = other.origRank;
			this->rcvd = other.rcvd;
			this->gotLinDep = other.gotLinDep;
		}
		return *this;
	}
	//either
	CodingMatrix m;
	//or
	CoderInfo c;
	//or
	CodingVector hashVec;

	uint32_t finRank;
	uint32_t origRank;

	std::map<uint16_t, uint16_t> rcvd;// <from node> <number>
	std::map<uint16_t, bool> gotLinDep;// <from node> <received a linear dependent packet>
};
}

#endif /* UTILS_CODER_HELP_INFO_H_ */
