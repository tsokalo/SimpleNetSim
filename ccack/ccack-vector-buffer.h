/*
 * ccack-vector-buffer.h
 *
 *  Created on: 12.01.2017
 *      Author: tsokalo
 */

#ifndef CCACK_CCACK_VECTOR_BUFFER_H_
#define CCACK_CCACK_VECTOR_BUFFER_H_

#include <functional>

#include "hash-matrix.h"
#include "matrix-operations.h"
#include "utils/log.h"

namespace ncr {

struct CcackVectorBuffer {

	typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;

	CcackVectorBuffer(uint16_t maxSize) {
		m_maxSize = maxSize;
	}

	void add(CodingVector v) {
		m_vecsNH.push_back(v);
		m_useCount.push_back(0);
		if (m_vecsNH.size() > m_maxSize) {
			m_vecsNH.erase(m_vecsNH.begin(), m_vecsNH.begin() + 1);
			m_useCount.pop_front();
		}
	}

	//
	// v: the null-space vector of the third vertex
	//
	void mark_heard(CodingVector v, hash_matrix_set_ptr hashMatrixSet, gf_actions_ptr gf) {

		if (m_vecsH.empty() && m_vecsNH.empty()) return;
		if (CCACK_LOG) {
			std::cout << "Before heard" << std::endl;
			for (auto v : m_vecsH)
			std::cout << v << " ";
			std::cout << std::endl;
			std::cout << "Before not heard" << std::endl;
			for (auto v : m_vecsNH)
			std::cout << v << " ";
			std::cout << std::endl;
		}

		auto hashMatrixSetRaw = hashMatrixSet->GetMatrixSet();

		auto check_heard = [&](CodingVector w)
		{
			for (auto m : hashMatrixSetRaw) {
				auto res = MultiplyVectorOnDiagonalMatrix(w, m->Get(), gf);
				SIM_LOG(CCACK_LOG, "Multi by diag matrix " << w << "x" << CodingVector(m->Get()) << "=" << res);
				SIM_LOG(CCACK_LOG, "Multi with hash vec " << res << "x" << v << "=" << (int16_t)InnerProduct(res, v, gf));
				if (InnerProduct(res, v, gf) != 0)return false;
			}
			return true;
		};

		auto it = m_vecsNH.begin();
		while (it != m_vecsNH.end()) {

			if (check_heard(*it)) {
				m_vecsH.push_back(*it);
				uint16_t d = std::distance(m_vecsNH.begin(), it);
				assert(d < m_useCount.size());
				auto u_it = m_useCount.begin() + d;
				m_useCount.erase(u_it, u_it + 1);
				m_vecsNH.erase(it, it + 1);
			}
			else {
				it++;
			}
		}
		if (CCACK_LOG) {
			std::cout << "After heard" << std::endl;
			for (auto v : m_vecsH)
			std::cout << v << " ";
			std::cout << std::endl;
			std::cout << "After not heard" << std::endl;
			for (auto v : m_vecsNH)
			std::cout << v << " ";
			std::cout << std::endl;
		}
	}

	//
	// return the non-heard vector with the smallest use count
	//
	CodingVector get_next() {

		assert(!m_useCount.empty());
		uint16_t i = 0;
		uint16_t min_c = std::numeric_limits<uint16_t>::max();
		for (uint16_t j = 0; j < m_useCount.size(); j++) {
			if (min_c > m_useCount.at(j)) {
				i = j;
				min_c = m_useCount.at(j);
			}
		}
		assert(i < m_vecsNH.size());
		m_useCount.at(i)++;

		return m_vecsNH .at(i);
	}

	uint16_t size() {
		return m_vecsNH.size();
	}

	bool empty() {
		return m_vecsNH.empty();
	}

	CodingMatrix GetHeard() {

		return m_vecsH;
	}
	CodingMatrix GetAll() {

		CodingMatrix all;
		all.insert(all.end(), m_vecsH.begin(), m_vecsH.end());
		all.insert(all.end(), m_vecsNH.begin(), m_vecsNH.end());
		return all;
	}

	void reset()
	{
		m_vecsNH.insert(m_vecsNH.end(), m_vecsH.begin(), m_vecsH.end());
		m_vecsH.clear();
		m_useCount.clear();
		m_useCount.resize(m_vecsNH.size(), 0);
	}

private:

	CodingMatrix m_vecsH;
	CodingMatrix m_vecsNH;
	std::deque<uint16_t> m_useCount;
	uint16_t m_maxSize;
};

}

#endif /* CCACK_CCACK_VECTOR_BUFFER_H_ */
