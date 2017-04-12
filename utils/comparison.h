/*
 * comparison.h
 *
 *  Created on: 21.10.2016
 *      Author: tsokalo
 */

#ifndef COMPARISON_H_
#define COMPARISON_H_

#define PRECISION_	0.0000001
#include <cmath>
#include <limits>

namespace ncr {

inline bool leq(double lhs, double rhs) {
	return (lhs - PRECISION_ < rhs);
}
inline bool geq(double lhs, double rhs) {
	return (lhs + PRECISION_ > rhs);
}
inline bool eq(double lhs, double rhs) {
	return (std::abs(lhs - rhs) < PRECISION_);
}
inline bool gr(double lhs, double rhs) {
	return (!leq(lhs, rhs));
}
inline bool le(double lhs, double rhs) {
	return (!geq(lhs, rhs));
}
inline bool neq(double lhs, double rhs) {
	return !eq(lhs, rhs);
}
inline bool eqzero(double lhs) {
	return eq(lhs, 0);
}
}//ncr

#endif /* COMPARISON_H_ */
