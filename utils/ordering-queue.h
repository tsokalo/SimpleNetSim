/*
 * ordering-queue.h
 *
 *  Created on: 17.02.2017
 *      Author: tsokalo
 */

#ifndef UTILS_ORDERING_QUEUE_H_
#define UTILS_ORDERING_QUEUE_H_

#include <stdint.h>
#include <vector>
#include <deque>

#include "ssn.h"

namespace ncr {

template<uint16_t N>
class OrderingQueue {

	typedef Ssn<uint16_t, N> l_ssn_t;
	typedef std::vector<l_ssn_t> queue_t;
	typedef std::deque<uint16_t> ssns_list;

public:

	OrderingQueue(uint32_t maxSize) :
			m_maxSize(maxSize) {

		m_lastReady = N - 1;

	}
	~OrderingQueue() {

	}
	void insert(ssns_list ssns) {

		for (auto ssn : ssns) {
			auto l_ssn = l_ssn_t(ssn);
			//
			// either insert in-between
			//
			auto it = m_queue.begin();
			//			if (it != m_queue.end())if (l_ssn_t(*it) > l_ssn_t(l_ssn)) continue;
			while (it != m_queue.end()) {
				if (*it == l_ssn) break;
				if (*it > l_ssn) {
					m_queue.insert(it, l_ssn);
					break;
				}
				it++;
			}
			//
			// or insert at end
			//
			if (it == m_queue.end()) {
				m_queue.push_back(l_ssn);
			}
		}
		//
		// adjust the length
		//
		if (m_queue.size() > m_maxSize) {
			auto to_erase = m_queue.size() - m_maxSize;
			m_queue.erase(m_queue.begin(), m_queue.begin() + to_erase);
			m_lastReady = this->first();
			m_lastReady--;
		}
	}
	ssns_list get() {

		ssns_list ssns;
		//		std::cout << "All sns: ";
		//		for(auto el : m_queue)std::cout << el << " ";
		//		std::cout << std::endl;

		//		if (!m_queue.empty()) {
		//
		//			auto it_b = m_queue.begin(), it_e = m_queue.begin();
		//
		//			while (it_e != m_queue.end()) {
		//				auto v = *it_e;
		//				ssns.push_back(v.val());
		//				it_e++;
		//				v++;
		//				if(it_e == m_queue.end())break;
		//				if (v != *it_e) break;
		//			}
		//			//
		//			// leave the last element of the continues sequence
		//			//
		//			assert(!ssns.empty());
		//			ssns.pop_back();
		//			it_e--;
		//
		//			m_queue.erase(it_b, it_e);
		//		}
		if (!m_queue.empty()) {

			auto v = m_lastReady;
			auto it_b = m_queue.begin(), it_e = m_queue.begin();

			while (it_e != m_queue.end()) {

				v++;
				if (v == *it_e) {
					ssns.push_back(v.val());
					m_lastReady = v.val();
					it_e++;
				}
				else {
					break;
				}
			}

			if (it_b != it_e) m_queue.erase(it_b, it_e);
		}
		return ssns;
	}

	friend std::ostream& operator<<(std::ostream& o, OrderingQueue<N>& m) {
//		o << "Max size: " << m.m_maxSize << " [";
//		for (auto v : m.m_queue)
//			o << v.val() << " ";
//		o << "]";
//
		o << "[" << m.first() << ",...," << m.last() << "]";
		o << ", last ready: " << m.m_lastReady.val();
		return o;
	}

protected:

	uint16_t first() {
		return (m_queue.empty()) ? 0 : m_queue.begin()->val();
	}
	uint16_t last() {
		return (m_queue.empty()) ? 0 : m_queue.at(m_queue.size() - 1).val();
	}

	uint32_t m_maxSize;
	queue_t m_queue;
	l_ssn_t m_lastReady;
};

}

#endif /* UTILS_ORDERING_QUEUE_H_ */
