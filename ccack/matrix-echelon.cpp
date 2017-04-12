/*
 * matrix-echelon.cpp
 *
 *  Created on: 18.01.2017
 *      Author: tsokalo
 */

#include "matrix-echelon.h"

namespace ncr {

// Swap rows i and k of a matrix A
// Note that due to the reference, both dimensions are preserved for
// built-in arrays

void swap_rows(CodingMatrix& A, std::size_t i, std::size_t k) {

	typedef std::size_t index_type;

	// check indices
	assert(0 <= i);
	assert(i <= A.size() - 1);

	assert(0 <= k);
	assert(k <= A.size() - 1);

	for (index_type col = 0; col <= A.begin()->size() - 1; ++col)
		std::swap(A[i][col], A[k][col]);
}

// divide row i of matrix A by v

void divide_row(CodingMatrix& A, std::size_t i, uint8_t v, gf_actions_ptr gf) {

	typedef std::size_t index_type;

	assert(0 <= i);
	assert(i <= A.size() - 1);

	assert(v != 0);

	for (index_type col = 0; col <= A.begin()->size() - 1; ++col)
		A[i][col] = gf->Divide(A[i][col], v);
}

// in matrix A, add v times row k to row i

void add_multiple_row(CodingMatrix& A, std::size_t i, std::size_t k, uint8_t v, gf_actions_ptr gf) {

	typedef std::size_t index_type;

	assert(0 <= i);
	assert(i <= A.size() - 1);

	assert(0 <= k);
	assert(k <= A.size() - 1);

	for (index_type col = 0; col <= A.begin()->size() - 1; ++col)
		A[i][col] = gf->Add(A[i][col], gf->Multiply(v, A[k][col]));
}

void check_row_echelon_form(CodingMatrix &A) {
	auto find_ident_column = [](CodingMatrix A, uint16_t i)->uint16_t
	{
		for (uint16_t j = 0; j < A.at(i).size(); j++)
		{
			bool search_more = false;
			if(A.at(i).at(j) != 1)
			{
				search_more = true;
			}
			else
			{
				for (uint16_t k = 0; k < A.size(); k++)
				{
					if(k != i && A.at(k).at(j) != 0)
					{
						search_more = true;
						break;
					}
				}
			}
			if(!search_more)return j;
		}
		assert(0);
	};
	auto swap_columns = [](CodingMatrix &A, uint16_t i, uint16_t j)
	{
		for (uint16_t k = 0; k < A.size(); k++)
		{
			std::swap(A[k][i], A[k][j]);
		}
	};
	for (uint16_t i = 0; i < A.size(); i++) {
		if (A.at(i).at(i) != 1) {
			auto j = find_ident_column(A, i);
			swap_columns(A, i, j);
		}
	}
}

// convert A to reduced row echelon form

void to_reduced_row_echelon_form(CodingMatrix& A, gf_actions_ptr gf) {

	assert(!A.empty());
	assert(!A.begin()->empty());

	typedef std::size_t index_type;

	index_type lead = 0;

	auto reduce_to_echelon = [&]()
	{
		for (index_type row = 0; row <= A.size() - 1; ++row) {
			if (lead > A.begin()->size() - 1) return;
			index_type i = row;
			while (A[i][lead] == 0) {
				++i;
				if (i > A.size() - 1) {
					i = row;
					++lead;
					if (lead > A.begin()->size() - 1) return;
				}
			}
			if (i != row) swap_rows(A, i, row);
			divide_row(A, row, A[row][lead], gf);
			for (i = 0; i <= A.size() - 1; ++i) {
				if (i != row) add_multiple_row(A, i, row, gf->Subtract(0, A[i][lead]), gf);
			}
		}
	};

	reduce_to_echelon();

	//
	// remove rows with zeros only
	//
	auto a_it = A.begin();
	while (a_it != A.end()) {
		bool all_zeros = true;
		for (auto v : *a_it) {
			if (v != 0) {
				all_zeros = false;
				break;
			}
		}
		if (all_zeros) {
			A.erase(a_it, a_it + 1);
		}
		else {
			a_it++;
		}
	}
}
}
