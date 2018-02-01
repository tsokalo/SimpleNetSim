/*
 * Ccack.cpp
 *
 *  Created on: 12.01.2017
 *      Author: tsokalo
 */

#include "ccack.h"
#include <iostream>
#include "matrix-operations.h"
#include "matrix-echelon.h"
#include "utils/log.h"

namespace ncr {

Ccack::Ccack(uint16_t genSize, hash_matrix_set_ptr hashMatrixSet) :
		m_decFactory(genSize, 1), m_gen(m_rd()), m_dis(1, (1 << hashMatrixSet->GetFieldSize()) - 1) {

	m_genSize = genSize;
	m_hashMatrixSet = hashMatrixSet;
	m_levels = m_hashMatrixSet->GetLevels();

	m_brx = ccack_vec_ptr(new CcackVectorBuffer(10 * genSize));
	m_btx = ccack_vec_ptr(new CcackVectorBuffer(10 * genSize));

	m_gf = gf_actions_ptr(new GfActions());
}

Ccack::~Ccack() {

}

void Ccack::SaveRcv(CodingVector vec) {
	m_brx->add(vec);
}
void Ccack::SaveSnt(CodingVector vec) {
	m_btx->add(vec);
}
CodingVector Ccack::GetHashVector() {

	auto print_vec = [](std::string str, std::vector<uint8_t> vec)
	{
		if(CCACK_LOG)
		{
			std::cout << str << ": ";
			for(auto v : vec)std::cout << (int16_t)v << " ";
			std::cout << std::endl;
		}
	};

	//
	// select RX coding vectors that were ACKed the least number of times
	// size (at most): (m_genSize/m_levels - 1) x m_genSize
	//
	auto rxVecs = CollectRxVectorsForAck();

	for (auto v : rxVecs) {
		print_vec("Using Rx vector", v);
	}

	if (rxVecs.empty()) return CodingVector();
	//
	// get the set of hash matrices;
	// size (exact): m_genSize x m_genSize
	//
	auto hashMatrixSetRaw = m_hashMatrixSet->GetMatrixSet();

	for (auto m : hashMatrixSetRaw) {
		print_vec("Hash matrix diagonal", m->Get());
	}
	//
	// compute products of the RX matrix and each hash matrix
	// then concatenate all resulting matrices
	// size (at most): (m_genSize - m_levels) x m_genSize
	//
	CodingMatrix phis;
	for (auto m : hashMatrixSetRaw) {
		auto res = MultiplyMatrixOnDiagonalMatrix(rxVecs, m->Get(), m_gf);
		phis.insert(phis.begin(), res.begin(), res.end());
	}
	for (auto v : phis) {
		print_vec("phis vector", v);
	}

	//
	// transform phis to the echelon form
	// have m_levels degrees of freedom;
	//
	CodingMatrix phis_echelon = phis;

	to_reduced_row_echelon_form(phis_echelon, m_gf);

	for (auto v : phis_echelon) {
		print_vec("phis echelon vector", v);
	}

	//
	// form the basis of the null space
	// there are m_levels vectors in the basis
	//
	CodingMatrix nullSpaceBasis = FormNullSpace(phis_echelon, m_gf);

	for (auto v : nullSpaceBasis) {
		print_vec("Null space basis vector", v);
	}

	//
	// generate random coefficients
	//
	uint16_t dof = m_genSize - phis_echelon.size();
	CodingVector vec(dof);
	for (auto &v : vec)
		v = m_dis(m_gen);

	print_vec("Random coefficients", vec);

	vec = MultiplyMatrixOnVector(nullSpaceBasis, vec, m_gf, true);

	print_vec("Hash vector", vec);

	TestHashVector(vec, phis, m_gf);

	return vec;
}
void Ccack::RcvHashVector(CodingVector v) {

	if (v.empty()) return;
	m_brx->mark_heard(v, m_hashMatrixSet, m_gf);
	m_btx->mark_heard(v, m_hashMatrixSet, m_gf);
}
uint16_t Ccack::GetHeardSymbNum() {

	auto dec = m_decFactory.build();
	auto heardR = m_brx->GetHeard();
	auto heardT = m_btx->GetHeard();
	if (!heardT.empty()) heardR.insert(heardR.end(), heardT.begin(), heardT.end());
	std::vector<uint8_t> fake_symbol(dec->symbol_size());
	for (auto s : heardR)
		dec->read_symbol(fake_symbol.data(), s.data());
	return dec->rank();
}
CodingMatrix Ccack::GetHeardSymb() {
	auto heardR = m_brx->GetHeard();
	auto heardT = m_btx->GetHeard();
	if (!heardT.empty()) heardR.insert(heardR.end(), heardT.begin(), heardT.end());
	return heardR;
}
void Ccack::Reset() {
	m_brx->reset();
	m_btx->reset();
}
CodingMatrix Ccack::CollectRxVectorsForAck() {

	CodingMatrix s;
	if (m_brx->empty()) return s;

	auto temp = floor((double) m_genSize / (double) m_levels) - 1;
	assert(temp > 0);
	auto max = (temp < m_brx->size()) ? temp : m_brx->size();

	auto dec = m_decFactory.build();
	std::vector<uint8_t> fake_symbol(dec->symbol_size());
	auto rank = 0;

	while (max != 0) {
		auto v = m_brx->get_next();
		auto u = v;
		dec->read_symbol(fake_symbol.data(), v.data());
		auto new_rank = dec->rank();
		if (new_rank > rank) {
			s.push_back(u);
			rank = new_rank;
		}
		max--;
	}

//	assert(!s.empty());

	return s;
}

}
