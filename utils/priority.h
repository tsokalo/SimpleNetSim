/*
 * ssn.h
 *
 *  Created on: 02.10.2016
 *      Author: tsokalo
 */

#ifndef PRIORITY_H
#define PRIORITY_H

#include <iostream>
#include <assert.h>
#include <iomanip>

#include "comparison.h"

namespace ncr {

#define DESTINATION_PRIORITY 			-1

/*
 * N - PRIORITY_HYSTERESIS_WIDTH
 * it's dependent on possible range of Priority values
 */
template<std::size_t N>
struct Priority {
public:
	Priority<N> () {
		v = 0;
	}
	Priority<N> (double v) {
		this->v = v;
	}
	Priority<N> (const Priority<N> &other) {
		this->v = other.v;
	}
	Priority<N>& operator=(const Priority<N>& other) // copy assignment
	{
		if (this != &other) { // self-assignment check expected
			this->v = other.v;
		}
		return *this;
	}
	Priority<N>& operator=(const double& other) {
		this->v = other;
		return *this;
	}
	Priority<N>& operator=(const float& other) {
		this->v = other;
		return *this;
	}
	Priority<N>& operator=(const int& other) {
		this->v = other;
		return *this;
	}
	Priority<N>& operator=(const long& other) {
		this->v = other;
		return *this;
	}

	friend std::ostream& operator<<(std::ostream& o, Priority<N> const& a) {
		if (eq(a.v, DESTINATION_PRIORITY)) {
			o << "DESTINATION_PRIORITY";
		}
		else {
			o << std::setprecision(6) << a.v;
		}
		return o;
	}

	inline friend bool operator==(const Priority<N>& lhs, const Priority<N>& rhs) {
		bool a = eq(lhs.v, DESTINATION_PRIORITY), b = eq(rhs.v, DESTINATION_PRIORITY);
		return (((std::abs(lhs.v - rhs.v) < N) && (!(a || b))) || (a * b));
	}
	inline friend bool operator<=(const Priority<N>& lhs, const Priority<N>& rhs) {
		bool a = eq(lhs.v, DESTINATION_PRIORITY), b = eq(rhs.v, DESTINATION_PRIORITY);
//		std::cout << "a " << a << ", b " << b << ", (rhs.v + N > lhs.v) " << (rhs.v + N > lhs.v)
//				<< ", !(a || b) " << !(a || b) << ", ((!a) * b) " << ((!a) * b)
//				<< ", ret " << (((rhs.v + N > lhs.v) && (!(a || b))) || ((!a) * b)) << std::endl;
		return (((rhs.v + N > lhs.v) && (!(a || b))) || ((!a) * b));
	}
	inline friend bool operator>=(const Priority<N>& lhs, const Priority<N>& rhs) {
		bool a = eq(lhs.v, DESTINATION_PRIORITY), b = eq(rhs.v, DESTINATION_PRIORITY);
//		std::cout << "a " << a << ", b " << b << ", (rhs.v - N < lhs.v) " << (rhs.v - N < lhs.v)
//				<< ", !(a || b) " << !(a || b) << ", (a * (!b)) " << (a * (!b))
//				<< ", ret " << (((rhs.v - N < lhs.v) && (!(a || b))) || (a * (!b))) << std::endl;
		return (((rhs.v - N < lhs.v) && (!(a || b))) || (a * (!b)));
	}

	inline friend bool operator<(const Priority<N>& lhs, const Priority<N>& rhs) {
		return !(lhs >= rhs);
	}
	inline friend bool operator>(const Priority<N>& lhs, const Priority<N>& rhs) {
		return !(lhs <= rhs);
	}

	inline friend bool operator!=(const Priority<N>& lhs, const Priority<N>& rhs) {
		return !(lhs == rhs);
	}

	friend double operator*(double lhs, const Priority<N>& rhs) {
		assert(neq(rhs.v, DESTINATION_PRIORITY) && neq(lhs, DESTINATION_PRIORITY));
		return lhs *= rhs.v;
	}
	friend double operator/(double lhs, const Priority<N>& rhs) {
		assert(neq(lhs, DESTINATION_PRIORITY));
		if (eq(rhs.v, DESTINATION_PRIORITY)) {
			return 0;
		}
		else {
			return lhs /= rhs.v;
		}
	}
	friend double operator*(Priority<N> lhs, const double& rhs) {
		assert(neq(lhs.v, DESTINATION_PRIORITY) && neq(rhs, DESTINATION_PRIORITY));
		return lhs.v *= rhs;
	}
	friend double operator/(Priority<N> lhs, const double& rhs) {
		assert(neq(lhs.v, DESTINATION_PRIORITY));
		if (eq(rhs, DESTINATION_PRIORITY)) {
			return 0;
		}
		else {
			return lhs.v /= rhs;
		}
	}
	friend double operator-(Priority<N> lhs, const Priority<N>& rhs) {
		assert(neq(rhs.v, DESTINATION_PRIORITY) && neq(lhs.v, DESTINATION_PRIORITY));
		return lhs.v -= rhs.v;
	}
	friend double operator-(Priority<N> lhs, const double& rhs) {
		assert(neq(lhs.v, DESTINATION_PRIORITY) && neq(rhs, DESTINATION_PRIORITY));
		return lhs.v -= rhs;
	}
	friend double operator+(Priority<N> lhs, const Priority<N>& rhs) {
		assert(neq(rhs.v, DESTINATION_PRIORITY) && neq(lhs.v, DESTINATION_PRIORITY));
		return lhs.v += rhs.v;
	}
	friend double operator+(Priority<N> lhs, const double& rhs) {
		assert(neq(lhs.v, DESTINATION_PRIORITY) && neq(rhs, DESTINATION_PRIORITY));
		return lhs.v += rhs;
	}

	double val() {
		return v;
	}
protected:

	double v;
};

}//ncr

#endif /* PRIORITY_H */
