/*
 * brr-header.h
 *
 *  Created on: 07.02.2017
 *      Author: tsokalo
 */

#ifndef BRRHEADER_H_
#define BRRHEADER_H_

#include "brr-pkt-header.h"
#include "brr-feedback.h"

namespace ncr {
struct BrrHeader {

	BrrHeader() {
	}
	BrrHeader(HeaderInfo h, FeedbackInfo f) {
		this->h = h;
		this->f = f;
	}

	HeaderInfo h;
	FeedbackInfo f;

	std::string Serialize() {
		std::stringstream ss;
		h.Serialize(ss);
		f.Serialize(ss);
		return ss.str();
	}
	void Deserialize(std::string str) {
		std::stringstream ss(str);
		h.Deserialize(ss);
		f.Deserialize(ss);
	}
};
}

#endif /* BRRHEADER_H_ */
