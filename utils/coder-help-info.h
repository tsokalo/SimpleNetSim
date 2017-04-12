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
	CoderHelpInfo& operator=(const CoderHelpInfo& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->m = other.m;
			this->c = other.c;
			this->hashVec = other.hashVec;
			this->finRank = other.finRank;
			this->origRank = other.origRank;
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
};
}

#endif /* UTILS_CODER_HELP_INFO_H_ */
