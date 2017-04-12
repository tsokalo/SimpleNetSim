/*
 * NcPacket.h
 *
 *  Created on: 03.01.2016
 *      Author: tsokalo
 */

#ifndef NCBUFFER_H_
#define NCBUFFER_H_

#include <deque>
#include <stdint.h>
#include <algorithm>
#include <tuple>
#include <vector>
#include "header.h"
#include "utils/brr-pkt-header.h"
#include "utils/brr-feedback.h"

namespace ncr {

class NcPacket {
public:
	NcPacket() {
		m_isFeedbackSymbol = false;
	}
	NcPacket(NcSymbol symb) {
		this->SetData(symb);
		m_isFeedbackSymbol = false;
	}
	virtual ~NcPacket() {
	}
	void SetData(NcSymbol symb) {
		m_symb.swap(symb);
	}
	NcSymbol GetData() {
		return m_symb;
	}
	void SetHeader(HeaderInfo header) {
		m_header = header;
	}
	HeaderInfo GetHeader() {
		return m_header;
	}
	bool IsFeedbackSymbol()
	{
		return m_isFeedbackSymbol;
	}
	void SetFeedback(FeedbackInfo fi)
	{
		m_feedback = fi;
		m_isFeedbackSymbol = true;
	}
	FeedbackInfo GetFeedback()
	{
		return m_feedback;
	}

private:

	NcSymbol m_symb;
	HeaderInfo m_header;
	FeedbackInfo m_feedback;
	bool m_isFeedbackSymbol;
};
}

#endif /* NCBUFFER_H_ */
