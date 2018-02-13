/*
 * mcs-datarate.h
 *
 *  Created on: 13.02.2018
 *      Author: tsokalo
 */

#ifndef UTILS_MCS_DATARATE_H_
#define UTILS_MCS_DATARATE_H_

#include <map>

namespace ncr {

std::map<uint16_t, double> mcs_rates = { { 0, 6.5 }, { 1, 13 }, { 2, 19.5 }, { 3, 26 }, { 4, 39 }, { 5, 52 }, { 6, 58.5 }, { 7, 65 }, { 8, 13 }, { 9, 26 }, { 10, 39 },
		{ 11, 52 }, { 12, 78 }, { 13, 104 }, { 14, 117 }, { 15, 130 } };

}

#endif /* UTILS_MCS_DATARATE_H_ */
