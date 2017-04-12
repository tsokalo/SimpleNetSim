/*
 * v-header.h
 *
 *  Created on: 10.01.2017
 *      Author: tsokalo
 */

#ifndef VHEADER_H_
#define VHEADER_H_

#include <cstdint>
#include <cstring>
#include <vector>
#include <assert.h>

namespace ncr {
template<class T>
struct header_value {

	static void append(std::vector<uint8_t> &vec, T v)
	{
		auto s = vec.size();
		vec.resize(s + sizeof(T));
		std::memcpy(vec.data() + s, &v, sizeof(T));
	}
	static T get(std::vector<uint8_t> &vec)
	{
		T v;
		assert(vec.size() >= sizeof(T));
		auto s = vec.size() - sizeof(T);
		std::memcpy(&v, vec.data() + s, sizeof(T));
		vec.resize(s);
		return v;
	}
};
}

#endif /* VHEADER_H_ */
