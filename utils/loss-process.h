/*
 * LossProcess.h
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#ifndef LOSSPROCESS_H_
#define LOSSPROCESS_H_

#include <chrono>
#include <random>
#include "header.h"

namespace ncr
{

class LossProcess
{
public:
  LossProcess () :
          m_lost (false)
  {
    typedef std::chrono::high_resolution_clock myclock;
    myclock::time_point beginning = myclock::now ();
    myclock::duration d = myclock::now () - beginning;
    uint8_t seed_v = d.count () + (seed_corrector++);

    m_generator.seed (seed_v);
  }

  virtual
  ~LossProcess ()
  {
  }

  virtual bool
  IsLost () = 0;

  virtual void
  Toss () = 0;

  virtual double
  GetMean () = 0;

protected:

  bool m_lost;
  std::default_random_engine m_generator;

};

class BernoulliLossProcess : public LossProcess
{
public:
  BernoulliLossProcess (double e) :
          LossProcess (), m_e (e), m_distribution (0.0, 1.0)
  {

  }

  virtual
  ~BernoulliLossProcess ()
  {
  }

  bool
  IsLost ()
  {
    return m_lost;
  }

  void
  Toss ()
  {
    m_lost = (m_distribution (m_generator) < m_e);
  }

  double
  GetMean ()
  {
    return m_e;
  }

private:

  double m_e;
  std::uniform_real_distribution<double> m_distribution;

};
}

#endif /* LOSSPROCESS_H_ */
