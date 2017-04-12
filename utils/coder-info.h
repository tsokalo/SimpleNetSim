/*
 * coder-info.h
 *
 *  Created on: 18.01.2017
 *      Author: tsokalo
 */

#ifndef UTILS_CODER_INFO_H_
#define UTILS_CODER_INFO_H_

#include "header.h"
#include <stdint.h>

namespace ncr {

struct CoderInfo {
	CoderInfo() {
		rank = 0;
		genSize = 0;
	}
	CoderInfo(const CoderInfo &arg) {
		rank = arg.rank;
		genSize = arg.genSize;
		seen = arg.seen;
		decoded = arg.decoded;
	}
	CoderInfo(uint16_t rank, uint16_t genSize, SeenMap seen, DecodedMap decoded) {
		this->rank = rank;
		this->genSize = genSize;
		this->seen.swap(seen);
		this->decoded.swap(decoded);
	}
	CoderInfo&
	operator=(CoderInfo arg) {
		rank = arg.rank;
		genSize = arg.genSize;
		seen.swap(arg.seen);
		decoded.swap(arg.decoded);
		return *this;
	}
	uint16_t rank;
	uint16_t genSize;
	SeenMap seen;
	DecodedMap decoded;
};

}

#endif /* UTILS_CODER_INFO_H_ */
