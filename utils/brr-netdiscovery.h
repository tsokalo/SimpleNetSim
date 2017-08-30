/*
 * brr-netdiscovery.h
 *
 *  Created on: 07.01.2017
 *      Author: tsokalo
 */

#ifndef BRRNETDISCOVERY_H_
#define BRRNETDISCOVERY_H_

#include "brr-feedback.h"

namespace ncr{

struct NetDiscoveryInfo: public FeedbackInfo {
	NetDiscoveryInfo() : FeedbackInfo() {
		type = FeedbackInfo::NET_DISC;
	}

	NetDiscoveryInfo(FeedbackInfo& other, ttl_t ttl) {
		FeedbackInfo::operator=(other);
		type = FeedbackInfo::NET_DISC;
		this->ttl = ttl;
	}

	/*
	 * Serialize and Deserialize are inherited
	 */
};

}
#endif /* BRRNETDISCOVERY_H_ */
