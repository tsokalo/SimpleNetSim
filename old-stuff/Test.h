/*
 * test.h
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#ifndef TEST_H_
#define TEST_H_

#include <iostream>
#include <random>
#include <vector>
#include <numeric>
#include <functional>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include "header.h"
#include "NcSymbol.h"

void
PrintSymbol(NcSymbol sym);
void
PrintState (MarkovState st);
double
p_lin_ind (uint16_t f_size, uint16_t rcv_dof, uint16_t Gs);
void
UpdateWithPolicy (Policy policy, MarkovState st, uint16_t Gs, Losses l, uint16_t k = 0, uint16_t s = 0);
void
MakeStep (Policy policy, MarkovState &st, TransProb p, uint16_t Gs, Losses l, std::default_random_engine &generator,
        std::uniform_real_distribution<double> &distribution, int16_t k = 0, uint16_t s = 0);
void
MakeTransition (Policy policy, MarkovState &st, TransProb p, double v, uint16_t Gs, Losses l, int16_t k = 0, uint16_t s = 0);
void
InitState (MarkovState &st);
double
GetTotalSentCalc (Policy policy, int16_t Gs, Losses l);
double
GetTotalSentCalcBestRoute (int16_t Gs, Losses l);
std::string
GetBestRoute (int16_t Gs, Losses l);
void
PrintState (MarkovState st);
long double
factorial_partial (uint16_t n, uint16_t k);
long double
factorial (uint16_t n);
long double
permutations (uint16_t n, uint16_t k);
long double
calc_prob_r (uint16_t n, uint16_t k, double p);
double
calc_datarate_eqv (uint16_t r, uint16_t ki, uint16_t k, uint16_t g, double e1, double e3);
template<typename T>
  T
  normal_pdf (T x, T m, T s);
std::pair<long double, long double>
binom_approx_normal (uint16_t n, uint16_t k, double p);
void
is_binom_approx_normal_adequate (uint16_t n, double p);
double
calc_completion_time (Losses l, uint16_t s, uint16_t Gs);
uint16_t
calc_overhead(double retrans_prob, double loss_prob, uint16_t want_to_deliver);
void
approx_binom_with_normal ();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
Test1 ();
void
Test2 ();
void
Test3 ();
void
Test4 ();
void
Test5 ();
void
Test6 ();
void
Test7 ();
void
TestFilter();
void
TestBrr();

#endif /* TEST_H_ */
