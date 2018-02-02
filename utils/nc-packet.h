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
#include "utils/brrm-pkt-header.h"
#include "utils/brrm-feedback.h"

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
	void SetHeader(HeaderMInfo header) {
		m_header = header;
	}
	HeaderMInfo GetHeader() {
		return m_header;
	}
	bool IsServiceMessage()
	{
		return m_isFeedbackSymbol;
	}
	void SetFeedback(FeedbackMInfo fi)
	{
		m_feedback = fi;
		m_isFeedbackSymbol = true;
	}
	FeedbackMInfo GetFeedback()
	{
		return m_feedback;
	}

private:

	NcSymbol m_symb;
	HeaderMInfo m_header;
	FeedbackMInfo m_feedback;
	bool m_isFeedbackSymbol;
};
}

#endif /* NCBUFFER_H_ */
