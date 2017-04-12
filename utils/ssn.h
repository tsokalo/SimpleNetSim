/*
 * ssn.h
 *
 *  Created on: 02.10.2016
 *      Author: tsokalo
 */

#ifndef SSN_H_
#define SSN_H_

#include <iostream>
#include <assert.h>

#define MAX_GEN_SSN	10000
#define MAX_SYM_SSN	(1<<15)

namespace ncr {

/*
 * N is the cycle length of sequence numbers, i.e N-1 - the biggest SN
 *
 * Keep N strict twice greater than the maximum expected distance between two
 * sequence number to be compared with a view to avoid ambiguity
 *
 * T is the type of sequence number variable
 */
template<class T, T N>
struct Ssn {
public:
	Ssn() {
		v = 0;
	}
	Ssn(T v) {
		this->v = v;
	}
	Ssn<T, N>& operator=(const Ssn<T, N>& other) // copy assignment
	{
		if (this != &other) { // self-assignment check expected
			this->v = other.v;
		}
		return *this;
	}
	inline Ssn& operator++(int) {
		v = (v == N - 1) ? 0 : (v + 1);
		return *this;
	}
	inline Ssn& operator++() {
		v = (v == N - 1) ? 0 : (v + 1);
		return *this;
	}
	inline Ssn& operator--(int) {
		v = (v == 0) ? N - 1 : (v - 1);
		return *this;
	}
	inline Ssn& operator--() {
		v = (v == 0) ? N - 1 : (v - 1);
		return *this;
	}
	friend std::ostream& operator<<(std::ostream& o, Ssn<T, N> const& a) {
		o << a.v;
		return o;
	}
	friend std::istream& operator>>(std::istream& o, Ssn<T, N> & a) {
		o >> a.v;
		return o;
	}
	inline friend bool operator<(const Ssn<T, N>& lhs, const Ssn<T, N>& rhs) {
		return (dist(lhs, rhs) < dist(rhs, lhs));
	}
	inline friend bool operator>(const Ssn<T, N>& lhs, const Ssn<T, N>& rhs) {
		return (dist(lhs, rhs) > dist(rhs, lhs));
	}
	inline friend bool operator<=(const Ssn<T, N>& lhs, const Ssn<T, N>& rhs) {
		return (dist(lhs, rhs) < dist(rhs, lhs) || lhs.v == rhs.v);
	}
	inline friend bool operator>=(const Ssn<T, N>& lhs, const Ssn<T, N>& rhs) {
		return (dist(lhs, rhs) > dist(rhs, lhs) || lhs.v == rhs.v);
	}
	inline friend bool operator==(const Ssn<T, N>& lhs, const Ssn<T, N>& rhs) {
		return (lhs.v == rhs.v);
	}
	inline friend bool operator!=(const Ssn<T, N>& lhs, const Ssn<T, N>& rhs) {
		return (lhs.v != rhs.v);
	}
	T* data() {
		return &v;
	}
	uint16_t size() {
		return sizeof(T);
	}
	T val() {
		return v;
	}
	static T get_distance(T& lhs, T& rhs) {
		return dist(Ssn<T, N> (lhs), Ssn<T, N> (rhs));
	}
	static T rotate(T& lhs, T l) {
		assert(l <= N);
		return (lhs + l >= N) ? (lhs + l - N) : (lhs + l);
	}
	static T rotate_back(T& lhs, T l) {
		assert(l <= N);
		l = N - l;
		return rotate(lhs, l);
	}
protected:

	/*
	 * Example:
	 * r = 7 - rhs
	 * l = 4 - lsh
	 * | = 0 = 12 - cycle end
	 *
	 *  >   .   .
	 *    r       .
	 *  .           .
	 *  .           |
	 *    l       .
	 *      .   .   <
	 *
	 * Here r > l, so return simply r - l = 3
	 *
	 * Return: distance from left to right
	 */

	static inline T dist(const Ssn<T, N>& lhs, const Ssn<T, N>& rhs) {
		return (rhs.v >= lhs.v) ? (rhs.v - lhs.v) : (N - lhs.v + rhs.v);
	}
	T v;
};

typedef uint16_t GenId;
typedef Ssn<GenId, MAX_GEN_SSN> gen_ssn_t;
typedef Ssn<uint16_t, MAX_SYM_SSN> symb_ssn_t;

}//ncr

#endif /* SSN_H_ */
