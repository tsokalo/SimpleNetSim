/*
 * brr-header.h
 *
 *  Created on: 07.02.2017
 *      Author: tsokalo
 */

#ifndef BRRMHEADER_H_
#define BRRMHEADER_H_

#include "brr-header.h"
#include "brrm-pkt-header.h"
#include "brrm-feedback.h"

namespace ncr {
struct BrrMHeader {

	BrrMHeader() {
	}
	BrrMHeader(HeaderMInfo h, FeedbackMInfo f) {

		this->h = h;
		this->f = f;
	}
	BrrMHeader(BrrHeader brrh) {

		this->h = brrh.h;
		this->f = brrh.f;
	}

	HeaderMInfo h;
	FeedbackMInfo f;

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

#endif /* BRRMHEADER_H_ */
