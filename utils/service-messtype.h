/*
 * service-messtype.h
 *
 *  Created on: 07.09.2017
 *      Author: tsokalo
 */

#ifndef UTILS_SERVICE_MESSTYPE_H_
#define UTILS_SERVICE_MESSTYPE_H_

#include <assert.h>
#include <stdint.h>

namespace ncr {

struct ServiceMessType {

public:

	ServiceMessType() {
		t = NONE;
	}
	/*
	 * the service message types are listed in the descending order of their priorities
	 * the messages with higher priority will be sent first
	 */
	enum ServiceMessType_ {
		/*
		 * replication of network discovery
		 */
		REP_NET_DISC,
		/*
		 * network discovery
		 */
		NET_DISC,
		/*
		 * response to the end-to-end ACK request; will be forwarded in direction to the source
		 */
		RESP_ETE_ACK,
		/*
		 * request for the end-to-end ACK; the direct neighbors respond to it if they have ACK for the requested generation(s);
		 * otherwise forward it in direction of the destination
		 */
		REQ_ETE_ACK,
		/*
		 * replication of retransmission request
		 */
		REP_REQ_RETRANS,
		/*
		 * retransmission request
		 */
		REQ_RETRANS,
		/*
		 * response to the point-to-point ACK request; behaves as a regular feedback; this response is not forwarded
		 */
		RESP_PTP_ACK,
		/*
		 * request for the point-to-point ACK; only the neighbors of the sender respond to it; this request is not forwarded
		 */
		REQ_PTP_ACK,
		/*
		 * general feedback information
		 */
		REGULAR,
		/*
		 * non-initialized
		 */
		NONE
	};

	bool assign(const ServiceMessType& other) {
		return assign(other.t);
	}
	bool assign(const ServiceMessType_& other) {
		//
		// the higher priority corresponds to the smaller ServiceMessType_ value
		// rewrite the message type only if it is of higher priority or NONE
		//
		if (this->t >= other || other == NONE) {
			this->t = other;
			return true;
		}
		return false;
	}

	void copy(const ServiceMessType& other) {
		this->t = other.t;
	}

	bool assign_if(const ServiceMessType_& c, const ServiceMessType_& other) {

		if (this->t != NONE) if (this->t != c) return false;
		return this->assign(other);
	}

	ServiceMessType& operator=(const uint16_t& other) {
		this->t = ServiceMessType_(other);
		return *this;
	}

	bool is_higher_prior(const ServiceMessType& other) {
		return is_higher_prior(other.t);
	}

	bool is_higher_prior(const ServiceMessType_& t) {
		return (this->t < t);
	}

	inline friend bool operator==(const ServiceMessType &a, const ServiceMessType &b) {
		return a.t == b.t;
	}
	inline friend bool operator!=(const ServiceMessType &a, const ServiceMessType &b) {
		return a.t != b.t;
	}

	inline friend bool operator==(const ServiceMessType &a, const ServiceMessType_ &b) {
		return a.t == b;
	}
	inline friend bool operator!=(const ServiceMessType &a, const ServiceMessType_ &b) {
		return a.t != b;
	}
	friend std::ostream& operator<<(std::ostream& o, ServiceMessType_& m) {
		switch (m) {
		case REP_NET_DISC: {
			o << "REP_NET_DISC";
			break;
		}
		case NET_DISC: {
			o << "NET_DISC";
			break;
		}
		case RESP_ETE_ACK: {
			o << "RESP_ETE_ACK";
			break;
		}
		case REQ_ETE_ACK: {
			o << "REQ_ETE_ACK";
			break;
		}
		case REP_REQ_RETRANS: {
			o << "REP_REQ_RETRANS";
			break;
		}
		case REQ_RETRANS: {
			o << "REQ_RETRANS";
			break;
		}
		case RESP_PTP_ACK: {
			o << "RESP_PTP_ACK";
			break;
		}
		case REQ_PTP_ACK: {
			o << "REQ_PTP_ACK";
			break;
		}
		case REGULAR: {
			o << "REGULAR";
			break;
		}
		case NONE: {
			o << "NONE";
			break;
		}
		}
		return o;
	}
	friend std::ostream& operator<<(std::ostream& o, ServiceMessType& m) {

		o << m.t;
		return o;
	}

	uint16_t GetAsInt() {
		return (uint16_t) t;
	}

	static MessType ConvertToMessType(ServiceMessType type) {
		if (type == REGULAR || type == REQ_PTP_ACK || type == REQ_ETE_ACK || type == RESP_PTP_ACK || type == RESP_ETE_ACK) return FEEDBACK_MSG_TYPE;
		if (type == NET_DISC || type == REP_NET_DISC) return NETDISC_MSG_TYPE;
		if (type == REQ_RETRANS || type == REP_REQ_RETRANS) return RETRANS_REQUEST_MSG_TYPE;

		assert(0);

		return NONE_MSG_TYPE;
	}

protected:
	/*
	 * to be used only by friend classes/structures
	 */
	ServiceMessType& operator=(const ServiceMessType& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->t = other.t;
		}
		return *this;
	}

private:

	ServiceMessType_ t;
};
}

#endif /* UTILS_SERVICE_MESSTYPE_H_ */
