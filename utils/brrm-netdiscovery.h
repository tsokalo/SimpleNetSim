/*
 * brr-netdiscovery.h
 *
 *  Created on: 07.01.2017
 *      Author: tsokalo
 */

#ifndef BRRMNETDISCOVERY_H_
#define BRRMNETDISCOVERY_H_

#include "brrm-feedback.h"

namespace ncr {

struct NetDiscoveryMInfo: public FeedbackMInfo {
	NetDiscoveryMInfo() :
			FeedbackMInfo() {
		type = FeedbackInfo::NET_DISC;
	}

	NetDiscoveryMInfo(FeedbackMInfo& other, ttl_t ttl) {
		FeedbackMInfo::operator=(other);
		type = FeedbackInfo::NET_DISC;
		this->ttl = ttl;
	}

	NetDiscoveryMInfo(const NetDiscoveryInfo &other) {
		FeedbackMInfo::operator=(*static_cast<const FeedbackInfo*>(&other));
	}


	/*
	 * Serialize and Deserialize are inherited
	 */
};

}
#endif /* BRRMNETDISCOVERY_H_ */
