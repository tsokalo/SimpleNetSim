/*
 * brr-service.h
 *
 *  Created on: 07.09.2017
 *      Author: tsokalo
 */

#ifndef UTILS_BRR_SERVICE_H_
#define UTILS_BRR_SERVICE_H_

#include "utils/service-messtype.h"
#include "utils/log.h"
#include <assert.h>
#include <iostream>

#define INVALID_TIMER_VALUE	-1

namespace ncr {
/*
 * it is possible to run only one service at a time
 */
struct BrrService {

	enum BrrServiceStatus {
		INITIALIZED, STARTED, PLANNED_FOR_REPETITION, REPEATED, FINISHED_NORMAL, FINISHED_ASSIGNED_HIGHER, NONE
	};
	typedef std::map<uint16_t, int16_t> Timer;

	BrrService() {
		status = NONE;
		want_start_service = false;
		timerPeriod[ServiceMessType::REP_NET_DISC] = INVALID_TIMER_VALUE;
		timerPeriod[ServiceMessType::NET_DISC] = INVALID_TIMER_VALUE;
		timerPeriod[ServiceMessType::RESP_ETE_ACK] = INVALID_TIMER_VALUE;
		timerPeriod[ServiceMessType::REQ_ETE_ACK] = 3;
		timerPeriod[ServiceMessType::REP_REQ_RETRANS] = INVALID_TIMER_VALUE;
		timerPeriod[ServiceMessType::REQ_RETRANS] = 3;
		timerPeriod[ServiceMessType::RESP_PTP_ACK] = INVALID_TIMER_VALUE;
		timerPeriod[ServiceMessType::REQ_PTP_ACK] = 3;
		timerPeriod[ServiceMessType::REGULAR] = INVALID_TIMER_VALUE;
		timerPeriod[ServiceMessType::NONE] = INVALID_TIMER_VALUE;
		timer = INVALID_TIMER_VALUE;
	}

	bool admit(ServiceMessType::ServiceMessType_ t) {

		SIM_LOG_FUNC(BRR_LOG);

		messTypeAdmitted.assign(ServiceMessType::NONE);
		want_start_service = false;
		//
		// the new service will not be admitted if the currently running service has the higher priority
		//
		bool b = (!messType.is_higher_prior(t));
		if (b) messTypeAdmitted.assign(t);
		SIM_LOG(BRR_LOG, "Current service " << messType.GetAsInt() << " , new service " << (uint16_t) t << ": admit " << b);

		return b;
	}
	bool init() {

		SIM_LOG_FUNC(BRR_LOG);

		SIM_ASSERT_MSG(messTypeAdmitted != ServiceMessType::NONE, "Call admit before init / init cannot be called if admit was successful");

		bool ret = false;

		if(messType == messTypeAdmitted && this->is_running())return false;
		//
		// if no more need to run the currently running service
		//
		if (!want_start_service && messType == messTypeAdmitted) stop();
		//
		// if want to start a new service
		//
		if (want_start_service && messType != messTypeAdmitted) {
			if (messType.assign(messTypeAdmitted)) {
				timer = INVALID_TIMER_VALUE;
				set_status(FINISHED_ASSIGNED_HIGHER);
				set_status(INITIALIZED);
				ret = true;
			} else {
				assert(0); // because of calling admit beforehand
			}
		} else {
			//
			// if want to re-send the service message of the currently running service
			// or send it the first time if it was not sent yet (status INITIALIZED)
			//
			if (want_start_service && messType == messTypeAdmitted) {
				ret = ready_to_start();
			}
		}

		if (ret) timer = timerPeriod[messType.GetAsInt()]; // reset the timer

		SIM_LOG(BRR_LOG,
				"Current service " << messType.GetAsInt() << ", admitted service " << messTypeAdmitted.GetAsInt() << ": case " << want_start_service << "," << (messType == messTypeAdmitted) << ", ret " << ret << ", timer " << timer);

		messTypeAdmitted.assign(ServiceMessType::NONE);
		want_start_service = false;
		return ret;
	}
	void tic() {

		SIM_LOG_FUNC(BRR_LOG);

		timer = (is_timeout() || timer == INVALID_TIMER_VALUE) ? timer : timer - 1;
		if (is_timeout()) set_repeat(true);
	}

	bool is_timeout() {
		return (timer == 0);
	}
	bool is_running()
	{
		SIM_LOG_FUNC(BRR_LOG);

		return (status == INITIALIZED || status == STARTED || status == PLANNED_FOR_REPETITION || status == REPEATED) && !is_timeout();
	}

	void set_want_start_service(bool v) {

		SIM_LOG_FUNC(BRR_LOG);
		SIM_LOG(BRR_LOG, "Current service " << messType.GetAsInt() << " want start " << v);
		want_start_service = v;
	}

	bool ready_to_start() {

		SIM_LOG_FUNC(BRR_LOG);

		return (status == INITIALIZED || status == PLANNED_FOR_REPETITION);
	}

	void start() {

		SIM_LOG_FUNC(BRR_LOG);

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

		SIM_LOG_FUNC(BRR_LOG);

		timer = INVALID_TIMER_VALUE;
		set_status(FINISHED_NORMAL);
		messType.assign(ServiceMessType::NONE);
		messTypeAdmitted.assign(ServiceMessType::NONE);
		want_start_service = false;
	}
	void stop_if(ServiceMessType::ServiceMessType_ t) {

		SIM_LOG_FUNC(BRR_LOG);

		if (messType == t) {
			stop();
		}
	}
	void set_status(BrrServiceStatus status) {

		SIM_LOG_FUNC(BRR_LOG);

		SIM_LOG(BRR_LOG, "Current service " << messType.GetAsInt() << " , current status " << (uint16_t) this->status << ", new status " << (uint16_t) status);

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
			if (this->status == INITIALIZED) break;
			assert(this->status == STARTED || this->status == PLANNED_FOR_REPETITION || this->status == REPEATED);
			break;
		}
		case REPEATED: {
			assert(this->status == PLANNED_FOR_REPETITION);
			break;
		}
		case FINISHED_NORMAL:
		case FINISHED_ASSIGNED_HIGHER: {
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

		SIM_LOG_FUNC(BRR_LOG);

		SIM_LOG(BRR_LOG, "Current service " << messType.GetAsInt() << " , current status " << (uint16_t) this->status << ", set repeat " << b);

		if (b) set_status(PLANNED_FOR_REPETITION);
	}
	bool need_repeat() {

		SIM_LOG_FUNC(BRR_LOG);

		return (status == PLANNED_FOR_REPETITION);
	}

	friend std::ostream& operator<<(std::ostream& o, BrrService& m) {

		o << "Service: " << m.messType.GetAsInt() << " / " << (uint16_t) m.status << " / " << m.messTypeAdmitted.GetAsInt() << " / " << m.want_start_service;
		return o;
	}

private:
	/*
	 * type of the last service message that was sent;
	 * this variable is set to NONE only when the satisfying reaction of the up-/downstream nodes;
	 * the service message with higher priority can rewrite this variable
	 */
	ServiceMessType messType;
	ServiceMessType messTypeAdmitted;
	bool want_start_service;
	BrrServiceStatus status;
	Timer timerPeriod;
	int16_t timer;

}
;
}

#endif /* UTILS_BRR_SERVICE_H_ */
