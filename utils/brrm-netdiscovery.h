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
		netDiscovery = true;
	}

	NetDiscoveryMInfo(FeedbackMInfo& other, ttl_t ttl) {
		this->addr = other.addr;
		this->p = other.p;
		this->rcvMap = other.rcvMap;
		this->rrInfo = other.rrInfo;
		this->netDiscovery = true;
		this->ttl = ttl;
		this->ackInfo = other.ackInfo;
	}

	NetDiscoveryMInfo(const NetDiscoveryInfo &other) {
		this->addr = other.addr;
		this->rcvMap = other.rcvMap;
		this->rrInfo = other.rrInfo;
		this->netDiscovery = other.netDiscovery;
		this->ttl = other.ttl;
		this->ackInfo = other.ackInfo;

		this->updated = true;
	}


	/*
	 * Serialize and Deserialize are inherited
	 */
};

}
#endif /* BRRMNETDISCOVERY_H_ */
