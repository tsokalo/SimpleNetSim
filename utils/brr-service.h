/*
 * brr-service.h
 *
 *  Created on: 07.09.2017
 *      Author: tsokalo
 */

#ifndef UTILS_BRR_SERVICE_H_
#define UTILS_BRR_SERVICE_H_

#include "utils/service-messtype.h"
#include <assert.h>

namespace ncr {
/*
 * it is possible to run only one service at a time
 */
struct BrrService {

	enum BrrServiceStatus {
		INITIALIZED, STARTED, PLANNED_FOR_REPETITION, REPEATED, FINISHED_NORMAL, FINISHED_ASSIGNED_HIGHER, NONE
	};

	BrrService() {
		status = NONE;
		want_start_service = false;
	}

	bool admit(ServiceMessType::ServiceMessType_ t) {
		//
		// the new service will not be admitted if the currently running service has the higher priority
		//
		return (!messType.is_higher_prior(t));
	}
	bool init(ServiceMessType::ServiceMessType_ t) {

		bool ret = false;

		//
		// if no more need to run the currently running service
		//
		if (!want_start_service && messType == t) stop();
		//
		// if want to start a new service
		//
		if (want_start_service && messType != t) {
			if (messType.assign(t)) {
				set_status(INITIALIZED);
				ret = true;
			} else {
				assert(0); // because of calling admit beforehand
			}
		}
		//
		// if want to re-send the service message of the currently running service
		//
		if (want_start_service && messType == t) {
			ret = need_repeat();
		}

		want_start_service = false;
		return ret;
	}

	void set_want_start_service(bool v) {
		want_start_service = v;
	}

	bool ready_to_start() {
		return (status == INITIALIZED || status == PLANNED_FOR_REPETITION);
	}

	void start() {

		if (this->status == PLANNED_FOR_REPETITION) {
			set_status(REPEATED);
		}
		if (this->status == INITIALIZED) {
			set_status(STARTED);
		}
	}
	ServiceMessType get_type() {
		return messType;
	}
	void stop() {
		set_status(NONE);
		messType.assign(ServiceMessType::NONE);
	}
	void set_status(BrrServiceStatus status) {

		switch (status) {
		case INITIALIZED: {
			assert(this->status == NONE || this->status == FINISHED_NORMAL || this->status == FINISHED_ASSIGNED_HIGHER);
			break;
		}
		case STARTED: {
			assert(this->status == INITIALIZED);
			break;
		}
		case PLANNED_FOR_REPETITION: {
			assert(this->status == STARTED || this->status == PLANNED_FOR_REPETITION || this->status == REPEATED);
			break;
		}
		case REPEATED: {
			assert(this->status == PLANNED_FOR_REPETITION);
			break;
		}
		case FINISHED_NORMAL:
		case FINISHED_ASSIGNED_HIGHER: {
			assert(this->status == STARTED || this->status == PLANNED_FOR_REPETITION || this->status == REPEATED);
			break;
		}
		case NONE: {
			break;
		}
		default: {
			assert(0);
		}
		}
		this->status = status;
	}
	void set_repeat(bool b) {
		if (b) set_status(PLANNED_FOR_REPETITION);
	}
	bool need_repeat() {
		return (status == PLANNED_FOR_REPETITION);
	}

private:
	/*
	 * type of the last service message that was sent;
	 * this variable is set to NONE only when the satisfying reaction of the up-/downstream nodes;
	 * the service message with higher priority can rewrite this variable
	 */
	ServiceMessType messType;
	bool want_start_service;
	BrrServiceStatus status;

}
;
}

#endif /* UTILS_BRR_SERVICE_H_ */
