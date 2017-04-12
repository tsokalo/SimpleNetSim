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

namespace ncr {
template<class T>
struct vheader {

	static void append(std::vector<uint8_t> &vec, T v)
	{
		auto s = vec.size();
		vec.resize(s + sizeof(T));
		std::memcpy(vec.data()+s, &v, sizeof(T));
	}
	static T get(std::vector<uint8_t> &vec)
	{
		T v;
		auto new_s = vec.size() - sizeof(T);
		std::memcpy(&v, vec.data() + new_s, sizeof(T));
		vec.erase(vec.begin() + new_s, vec.end());
		return v;
	}
};
}

#endif /* VHEADER_H_ */
