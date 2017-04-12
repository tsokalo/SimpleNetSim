/*
 * LPSolver.h
 *
 *  Created on: 02.09.2016
 *      Author: tsokalo
 */

#ifndef LPSOLVER_H_
#define LPSOLVER_H_

#include <iostream>
#include "OsiClpSolverInterface.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinPackedVector.hpp"
#include "lp-solver-header.h"

namespace lps
{

class LPSolver
{
public:
  LPSolver (Objectives o, Constraints c, Bounds b);
  virtual
  ~LPSolver ();

  void
  SolveTask ();

  Solution GetSolution()
  {
    return m_s;
  }
  double GetObjectiveValue()
  {
    return -m_objective;
  }

private:

  void
  ConstructTask ();

  Objectives m_o;
  Constraints m_c;
  Bounds m_b;
  OsiClpSolverInterface *si;
  Solution m_s;
  double m_objective;
};

}

#endif /* LPSOLVER_H_ */
