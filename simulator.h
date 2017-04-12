/*
 * simulator.h
 *
 *  Created on: 01.11.2016
 *      Author: tsokalo
 */

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include <functional>
#include <deque>
#include <tuple>
#include <memory>
#include <stdlib.h>

#include "utils/nc-packet.h"
#include "utils/log.h"
#include "utils/utils.h"

namespace ncr {

class Simulator {

	typedef std::function<void(NcPacket)> Handler;
	/*
	 * Handler	- the function to be called
	 * NcPacket	- the parameter for the Handler
	 * bool		- indication of the first event in one batch
	 *
	 * Here each batch consists of a set of point-to-point transmission forming a broadcast transmission
	 * Per each broadcast transmission the God View counter should be incremented only once
	 */
	typedef std::function<void()> notify_sending_func;
	typedef std::tuple<Handler, NcPacket, bool, MessType, notify_sending_func> Event;
	typedef std::function<void(uint64_t)> inc_time_func;
	typedef std::function<void(MessType)> set_msg_type_func;


public:
	Simulator() {
	}
	~Simulator() {
	}

	void Schedule(Handler handler, NcPacket packet, bool n, MessType t, notify_sending_func f) {
		SIM_LOG_FUNC(SIMULATOR_LOG);
		m_events.push_back(Event(handler, packet, n, t, f));
	}

	void Execute() {

		while (!m_events.empty()) {
			SIM_LOG(SIMULATOR_LOG, "Remaining events: " << m_events.size());
			assert(m_incTime && m_setMessType);
			if (std::get<2>(*m_events.begin())) {
				auto m = std::get<3>(*m_events.begin());
				SIM_LOG(SIMULATOR_LOG, ">>>>>>>>>>>>>>>>>>>>> NEW BROADCAST EVENT: " << m << " <<<<<<<<<<<<<<<<<<<");
				//
				// do only once for the first edges in the set of out-coming edges: n=true
				//
				m_incTime(0);
				m_setMessType(m);
				std::get<4>(*m_events.begin())();
			}
			(std::get<0>(*m_events.begin()))(std::get<1>(*m_events.begin()));
			m_events.pop_front();
		}
	}

	void SetIncTimeCallback(inc_time_func f) {
		m_incTime = f;
	}
	void SetMessTypeCallback(set_msg_type_func f) {
		m_setMessType = f;
	}

private:
	std::deque<Event> m_events;

	inc_time_func m_incTime;
	set_msg_type_func m_setMessType;

};
}//ncr
#endif /* SIMULATOR_H_ */
