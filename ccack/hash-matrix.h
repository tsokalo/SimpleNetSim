/*
 * hash-matrix.h
 *
 *  Created on: 12.01.2017
 *      Author: tsokalo
 */

#ifndef CCACK_HASH_MATRIX_H_
#define CCACK_HASH_MATRIX_H_

#include <stdint.h>
#include <random>
#include <memory>
#include <algorithm>
#include <string.h>

#include <storage/split.hpp>
#include <kodo_rlnc/on_the_fly_codes.hpp>

#include "utils/coder.h"

namespace ncr {

typedef kodo_rlnc::on_the_fly_decoder<fifi_field> hash_coder;

/****************************************************************************/
struct HashMatrix {
	/*
	 * m_genSize - generation size
	 * fs - field size
	 *
	 * hash matrix is the diagonal matrix
	 * all diagonal elements must be non-zero
	 */
	HashMatrix(uint16_t genSize, uint16_t fs) :
			m_gen(m_rd()), m_dis(1, (1 << fs) - 1) {

		this->m_genSize = genSize;
		m_v.resize(m_genSize);
	}

	void Generate() {

		for (uint16_t i = 0; i < m_v.size(); i++) {
			m_v.at(i) = m_dis(m_gen);
		}
	}

	std::vector<uint8_t> Get() {
		return m_v;
	}

private:

	uint16_t m_genSize;
	std::random_device m_rd;
	std::mt19937 m_gen;
	std::uniform_int_distribution<> m_dis;

	std::vector<uint8_t> m_v;
};
/****************************************************************************/
struct HashMatrixSet {

	typedef std::shared_ptr<HashMatrix> hash_matrix_ptr;

	HashMatrixSet(uint16_t n, uint16_t genSize, uint16_t fs) :
			m_decFactory(n, 1) {

		m_fs = fs;
		m_ms.resize(n);
		this->m_genSize = genSize;
		for (auto &m : m_ms) {
			m = hash_matrix_ptr(new HashMatrix(genSize, fs));
		}

		Generate();
	}

	uint16_t GetLevels() {
		return m_ms.size();
	}

	std::vector<hash_matrix_ptr> GetMatrixSet() {
		return m_ms;
	}

	uint16_t GetFieldSize()
	{
		return m_fs;
	}

private:

	void Generate() {

//		do {
			for (auto &m : m_ms) {
				m->Generate();
			}
//		} while (!Validate());
	}

	bool Validate() {

		auto rs = GetAllCombinations(m_genSize, m_ms.size());

		for (auto r : rs) {

			auto cm = ConstructCheckMatrix(r);
			auto dec = m_decFactory.build();
			std::vector<uint8_t> fake_symbol(dec->symbol_size());
			for (auto s : cm) {
				dec->read_symbol(fake_symbol.data(), s.data());
			}
			if (dec->rank() != m_ms.size()) return false;
		}
		return true;
	}
	std::vector<std::vector<uint8_t> > ConstructCheckMatrix(std::vector<uint16_t> is) {

		std::vector<std::vector<uint8_t> > all;

		for (auto m : m_ms) {

			std::vector<uint8_t> sel;
			std::vector<uint8_t> ful = m->Get();
			for (auto i : is) {
				assert(i < ful.size());
				sel.push_back(ful.at(i));
			}
			all.push_back(sel);
		}
		return all;
	}

	/*
	 * using code from http://rosettacode.org/wiki/GetAllCombinationsinations#C.2B.2B
	 *
	 * use integers [0, N - 1] and produces GetAllCombinationsinations of size K
	 */
	std::vector<std::vector<uint16_t> > GetAllCombinations(uint16_t n, uint16_t k) {

		std::string bitmask(k, 1); // K leading 1's
		bitmask.resize(n, 0); // N-K trailing 0's

		std::vector<std::vector<uint16_t> > r;

		do {
			std::vector<uint16_t> t;
			for (uint16_t i = 0; i < n; ++i) {
				if (bitmask[i]) t.push_back(i);
			}
			r.push_back(t);
		} while (std::prev_permutation(bitmask.begin(), bitmask.end()));

		return r;
	}

	uint16_t m_genSize;
	uint16_t m_fs;
	std::vector<hash_matrix_ptr> m_ms;
	/*
	 * just to check the rank
	 */
	hash_coder::factory m_decFactory;
};

typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;

}

#endif /* CCACK_HASH_MATRIX_H_ */
