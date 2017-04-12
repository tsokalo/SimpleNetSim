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
	NetDiscoveryInfo() {
		netDiscovery = true;
	}

	NetDiscoveryInfo(FeedbackInfo& other, ttl_t ttl) {
		this->addr = other.addr;
		this->p = other.p;
		this->rcvMap = other.rcvMap;
		this->rrInfo = other.rrInfo;
		this->netDiscovery = true;
		this->ttl = ttl;
		this->ackInfo = other.ackInfo;
	}

	/*
	 * Serialize and Deserialize are inherited
	 */
};

}
#endif /* BRRNETDISCOVERY_H_ */
