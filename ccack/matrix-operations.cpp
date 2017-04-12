/*
 * matrix-operations.cpp
 *
 *  Created on: 13.01.2017
 *      Author: tsokalo
 */

#include "matrix-operations.h"
#include "utils/log.h"

namespace ncr {

/*
 * inner product value should have a sign
 */
int8_t InnerProduct(CodingVector v, CodingVector u, gf_actions_ptr gf) {
	int8_t r = 0;
	assert(v.size() == u.size());
	for (uint16_t i = 0; i < v.size(); i++) {
		r = gf->Add(r, gf->Multiply(v.at(i), u.at(i)));
	}
	return r;
}
CodingMatrix MultiplyMatrixOnDiagonalMatrix(CodingMatrix v, MatrixDiagonal u, gf_actions_ptr gf) {

	CodingMatrix w;
	assert(!v.empty() && !u.empty());
	assert(u.size() == v.begin()->size());

	w.resize(v.size());
	for (uint16_t i = 0; i < v.size(); i++) {
		for (uint16_t j = 0; j < v.begin()->size(); j++) {
			w.at(i).push_back(gf->Multiply(v.at(i).at(j), u.at(j)));
		}
	}
	return w;
}
CodingVector MultiplyVectorOnDiagonalMatrix(CodingVector v, MatrixDiagonal u, gf_actions_ptr gf) {

	CodingVector w;
	assert(!v.empty() && !u.empty());
	assert(u.size() == v.size());

	for (uint16_t i = 0; i < v.size(); i++) {
		w.push_back(gf->Multiply(v.at(i), u.at(i)));
	}
	return w;
}
CodingMatrix Transpond(CodingMatrix v) {

	assert(!v.empty());

	CodingMatrix vv;

	for (uint16_t i = 0; i < v.begin()->size(); i++) {
		CodingVector t;
		for (uint16_t k = 0; k < v.size(); k++) {
			t.push_back(v.at(k).at(i));
		}
		vv.push_back(t);
	}

	assert(v.size() == vv.begin()->size());
	assert(vv.size() == v.begin()->size());
	return vv;
}
CodingVector MultiplyMatrixOnVector(CodingMatrix v, CodingVector u, gf_actions_ptr gf, bool transpond) {

	assert(!v.empty() && !u.empty());

	CodingVector w;
	CodingMatrix vv = (transpond) ? Transpond(v) : v;

	assert(vv.begin()->size() == u.size());

	for (auto e : vv) {
		w.push_back(InnerProduct(e, u, gf));
	};;

	return w;
}

void TestHashVector(CodingVector v, CodingMatrix rxVecs, gf_actions_ptr gf) {
	for (auto u : rxVecs) {
		SIM_LOG(CCACK_LOG, "product of " << u << "\tand\t" << v << "\tis\t" << (int16_t) InnerProduct(v, u, gf));
		assert(InnerProduct(v, u, gf) == 0);
	};;
}
CodingMatrix FormNullSpace(CodingMatrix m, gf_actions_ptr gf) {

	auto is_pivot = [](CodingMatrix A, uint16_t j, uint16_t &row)->bool
	{
		row = A.size();
		uint16_t ones = 0;
		for(uint16_t i = 0; i < A.size(); i++)
		{
			if(A.at(i).at(j) != 0 && A.at(i).at(j) != 1)return false;
			if(A.at(i).at(j) == 1)
			{
				ones++;
				row = i;
			}
		}
		if(ones != 1)return false;
		return true;
	};
//
//	//
//	// remove zero lines if any
//	//
//	for(auto )

	CodingMatrix nullSpaceBasis;
	auto pe = m.size();
	auto genSize = m.begin()->size();
	assert(genSize > pe);

	std::vector<bool> pivots(genSize, false);
	uint16_t row = -1;
	for (uint16_t j = 0; j < genSize; j++) {

		uint16_t expected_row = row + 1;
		pivots.at(j) = is_pivot(m, j, row);
		SIM_LOG(CCACK_LOG, "" << j << " is " << pivots.at(j) << " row " << row << " expected_row " << expected_row);
		if (pivots.at(j)) {
			assert(row != pe);
			if (row != expected_row) {
				pivots.at(j) = false;
				row = expected_row - 1;
			}
		}
		else {
			row = expected_row - 1;
		}
	}

	uint16_t c = 0;
	for (uint16_t j = 0; j < genSize; j++) {

		if (!pivots.at(j)) {

			CodingVector bVec(genSize, 0);
			uint16_t k = 0, p = 0;
			bool sp = false;
			for (uint16_t i = 0; i < genSize; i++) {
				if (pivots.at(i)) {
					assert(k < pe);
					bVec.at(i) = gf->Subtract(bVec.at(i), m.at(k).at(j));
					k++;
				}
				else {
					if (p == c && !sp) {
						bVec.at(i) = 1;
						c++;
						sp = true;
					}
					p++;
					continue;
				}
			}
			nullSpaceBasis.push_back(bVec);
		}
	}

	uint16_t dof = genSize - pe;
	if(nullSpaceBasis.size() != dof)
	{
		std::cout << nullSpaceBasis.size() << " <-> " << dof << std::endl;
		for(auto c : m)
				{
					std::cout << "ORIG: " << c << std::endl;
				}
		for(auto c : nullSpaceBasis)
		{
			std::cout << "NULL: " << c << std::endl;
		}
	}
	assert(nullSpaceBasis.size() == dof);
	return nullSpaceBasis;
}
}

