/*
 * matrix-echelon.h
 *
 *  Created on: 13.01.2017
 *      Author: tsokalo
 */

#ifndef CCACK_MATRIX_ECHELON_H_
#define CCACK_MATRIX_ECHELON_H_

/*
 * using code from: https://rosettacode.org/wiki/Reduced_row_echelon_form
 */

#include <algorithm> // for std::swap
#include <cstddef>
#include <cassert>

#include "header.h"
#include "matrix-operations.h"

namespace ncr {

void swap_rows(CodingMatrix& A, std::size_t i, std::size_t k);
void divide_row(CodingMatrix& A, std::size_t i, uint8_t v, gf_actions_ptr gf);
void add_multiple_row(CodingMatrix& A, std::size_t i, std::size_t k, uint8_t v, gf_actions_ptr gf);
void check_row_echelon_form(CodingMatrix &A);
void to_reduced_row_echelon_form(CodingMatrix& A, gf_actions_ptr gf);

}

#endif /* CCACK_MATRIX_ECHELON_H_ */
