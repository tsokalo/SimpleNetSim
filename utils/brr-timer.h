/*
 * brr-timer.h
 *
 *  Created on: 06.02.2018
 *      Author: tsokalo
 */

#ifndef BRR_TIMER_H_
#define BRR_TIMER_H_

namespace ncr{

struct BrrTimer
{
	BrrTimer()
	{
		t = 0;
	}
	void tic()
	{
		t = (t != 0) ? t - 1 : 0;
	}
	void start(uint32_t t)
	{
		this->t = t;
	}
	void stop()
	{
		t = 0;
	}
	bool is_running()
	{
		return t != 0;
	}
	uint32_t t;
};
}


#endif /* BRR_TIMER_H_ */
