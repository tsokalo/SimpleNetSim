/*
 * matrix-operations.h
 *
 *  Created on: 12.01.2017
 *      Author: tsokalo
 */

#ifndef MATRIXOPERATIONS_H_
#define MATRIXOPERATIONS_H_

#include <stdint.h>
#include <vector>
#include <assert.h>
#include <iostream>

#include "header.h"

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
//#include "galois-field/galois-field.h"
#include <fifi/full_table.hpp>

#include "utils/coder.h"
#include "utils/coding-vector.h"

namespace ncr {

typedef std::vector<uint8_t> MatrixDiagonal;

/*
 * http://www.partow.net/projects/galois/index.html
 */
struct GfActions {

//	typedef std::shared_ptr<galois::GaloisField> gf_ptr;

	GfActions() {
//		//		/*
//		//		 p(x) = 1x^8+1x^7+0x^6+0x^5+0x^4+0x^3+1x^2+1x^1+1x^0
//		//		 1    1    0    0    0    0    1    1    1
//		//		 */
//		//		unsigned int poly[9] = { 1, 1, 1, 0, 0, 0, 0, 1, 1 };
//		/*
//		 p(x) = X^8+X^4+X^3+X^2+1
//		 1    0    0    0    1    1    1    0    1
//		 */
//		unsigned int poly[9] = { 1, 0, 0, 0, 1, 1, 1, 0, 1 };
//		/*
//		 A Galois Field of type GF(2^8)
//		 */
//		m_gf = gf_ptr(new galois::GaloisField(8, poly));
	}

	uint8_t Add(uint8_t a, uint8_t b) {
		return m_fta.add(a, b);
//		return m_gf->add(a, b);
	}

	uint8_t Subtract(uint8_t a, uint8_t b) {
		return m_fta.subtract(a, b);
//		return m_gf->sub(a, b);
	}

	uint8_t Multiply(uint8_t a, uint8_t b) {
		return m_fta.multiply(a,b);
//		return m_gf->mul(a, b);
	}
	uint8_t Divide(uint8_t a, uint8_t b) {
		return m_fta.divide(a,b);
//		return m_gf->div(a, b);
	}

private:

//	gf_ptr m_gf;
	fifi::full_table<fifi_field> m_fta;

};

typedef std::shared_ptr<GfActions> gf_actions_ptr;

/*
 * inner product value should have a sign
 */
int8_t InnerProduct(CodingVector v, CodingVector u, gf_actions_ptr gf);
CodingMatrix MultiplyMatrixOnDiagonalMatrix(CodingMatrix v, MatrixDiagonal u, gf_actions_ptr gf);
CodingVector MultiplyVectorOnDiagonalMatrix(CodingVector v, MatrixDiagonal u, gf_actions_ptr gf);
CodingMatrix Transpond(CodingMatrix v);
CodingVector MultiplyMatrixOnVector(CodingMatrix v, CodingVector u, gf_actions_ptr gf, bool transpond);
void TestHashVector(CodingVector v, CodingMatrix rxVecs, gf_actions_ptr gf);
CodingMatrix FormNullSpace(CodingMatrix m, gf_actions_ptr gf);

}
#endif /* MATRIXOPERATIONS_H_ */
