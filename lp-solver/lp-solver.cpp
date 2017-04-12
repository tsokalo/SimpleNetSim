/*
 * LPSolver.cpp
 *
 *  Created on: 02.09.2016
 *      Author: tsokalo
 */

#include "lp-solver.h"
#include "utils/log.h"

namespace lps {


LPSolver::LPSolver(Objectives o, Constraints c, Bounds b) {
	SIM_LOG(LPSOLVER_LOG, "Initializing the solver..");
	m_o.swap(o);
	m_c.swap(c);
	m_b.swap(b);
	si = new OsiClpSolverInterface;
	ConstructTask();
}

LPSolver::~LPSolver() {
	delete si;
}

void LPSolver::ConstructTask() {
	double *objective = &m_o[0];
	double *col_lb = &m_b.first[0];
	double *col_ub = &m_b.second[0];

	double *row_lb = new double[m_c.size() + 1];
	double *row_ub = new double[m_c.size() + 1];

	CoinPackedMatrix *matrix = new CoinPackedMatrix(false, 0, 0);
	matrix->setDimensions(0, m_o.size());

	for (uint16_t i = 0; i < m_c.size(); i++) {
		CoinPackedVector row;
		for (uint16_t j = 0; j < m_c.at(i).size(); j++) {
			row.insert(j, m_c.at(i).at(j));
		}
		row_lb[i] = 0;
		row_ub[i] = si->getInfinity();
		matrix->appendRow(row);
	}

	//
	// sum of all t must be equal 1
	//
	CoinPackedVector row;
	for (uint16_t j = 0; j < m_c.at(0).size(); j++) {
		row.insert(j, 1);
	}
	row_lb[m_c.size()] = 1.0;
	row_ub[m_c.size()] = 1.0;
	matrix->appendRow(row);

	si->loadProblem(*matrix, col_lb, col_ub, objective, row_lb, row_ub);

	//write the MPS file to a file called test.mps
	si->writeMps("test", "mps", -1);
	si->readMps("test", "mps");
}

void LPSolver::SolveTask() {
	m_s.clear();
	// Solve the (relaxation of the) problem

	si->setStuff(0.000001, 0.000001);
	si->initialSolve();
	si->branchAndBound();
	si->setLogLevel(0);

	// Check the solution
	if (si->isProvenOptimal()) {
		SIM_LOG(LPSOLVER_LOG, "Found optimal solution!");
		SIM_LOG(LPSOLVER_LOG, "Objective value is " << si->getObjValue ());
		m_objective = si->getObjValue();
		int n = si->getNumCols();
		const double* solution = si->getColSolution();

		// We can then print the solution or could examine it.
		for (int i = 0; i < n; ++i) {
			SIM_LOG(LPSOLVER_LOG, si->getColName (i) << " = " << solution[i]);
			m_s.push_back(solution[i]);
		}
	}
	else {
		SIM_LOG(LPSOLVER_LOG, "Didn't find optimal solution.");
		// Could then check other status functions.
	}
}

}

