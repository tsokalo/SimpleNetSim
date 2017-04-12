/*
 * Ccack.h
 *
 *  Created on: 12.01.2017
 *      Author: tsokalo
 *
 * Using:
 *
 * Efficient network-coding-based opportunistic routing through cumulative coded acknowledgments
 * Koutsonikolas, Dimitrios, Wang, Chih Chun, Hu, Y. Charlie
 * IEEE/ACM Transactions on Networking
 */

#ifndef CCACK_CCACK_H_
#define CCACK_CCACK_H_

#include <deque>
#include <stdint.h>
#include <memory>
#include <limits>

#include "header.h"
#include "ccack-vector-buffer.h"
#include "hash-matrix.h"
#include "utils/coding-vector.h"

namespace ncr {

class Ccack {

	typedef std::shared_ptr<CcackVectorBuffer> ccack_vec_ptr;
	typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;

public:

	Ccack(uint16_t genSize, hash_matrix_set_ptr hashMatrixSet);
	virtual ~Ccack();

	/*
	 * inputs
	 */
	void SaveRcv(CodingVector vec);
	void SaveSnt(CodingVector vec);

	/*
	 * outputs
	 */
	CodingVector GetHashVector();
	void RcvHashVector(CodingVector v);
	uint16_t GetHeardSymbNum();
	CodingMatrix GetHeardSymb();

	void Reset();

private:

	CodingMatrix CollectRxVectorsForAck();

	ccack_vec_ptr m_brx;
	ccack_vec_ptr m_btx;
	hash_coder::factory m_decFactory;

	hash_matrix_set_ptr m_hashMatrixSet;

	uint16_t m_genSize;
	uint16_t m_levels;

	std::random_device m_rd;
	std::mt19937 m_gen;
	std::uniform_int_distribution<> m_dis;

	gf_actions_ptr m_gf;
};
}
#endif /* CCACK_CCACK_H_ */
