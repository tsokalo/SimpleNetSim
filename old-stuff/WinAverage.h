/*
 * WinAverage.h
 *
 *  Created on: Dec 15, 2015
 *      Author: tsokalo
 */

#ifndef WINAVERAGE_H_
#define WINAVERAGE_H_


#include <deque>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <memory>
#include <map>
#include <functional>
#include <assert.h>
#include <string.h>

#include "header.h"

class WinAverage
{
public:
  WinAverage (uint16_t winSize) :
    m_winSize (winSize), m_w (m_winSize, 1), m_wType (NO_WEIGHT_COEFS)
  {
    m_lostCount = 0;
    m_nlostCount = 0;
    m_lossRatio = 1;
  }
  WinAverage (uint16_t winSize, WeightCoefs wType, double coef) :
    m_winSize (winSize), m_w (m_winSize, 0), m_wType (wType)
  {
    if (m_wType == LIN_LAST_MOST_WEIGHT_COEFS)
      {
        for (uint16_t i = 0; i < m_w.size (); i++)
          m_w.at (i) = 1 + coef / 2 - (1 - coef) * (double) i / (double) (m_winSize - 1);
      }
    if (m_wType == LIN_FIRST_MOST_WEIGHT_COEFS)
      {
        for (uint16_t i = 0; i < m_w.size (); i++)
          m_w.at (i) = 1 + coef / 2 - (1 - coef) * (double) (m_winSize - 1 - i) / (double) (m_winSize - 1);
      }
  }
  ~WinAverage ()
  {
  }

  //
  // val == true if the packet is lost
  //
  void
  add_val (bool val)
  {
    (val) ? m_lostCount++ : m_nlostCount++;
  }

  int32_t
  get_actual_loss ()
  {
    return m_lostCount;
  }
  //
  // returns an estimation of the packet loss basing on the history of losses
  //
  double
  get_loss_ratio ()
  {
    return m_lossRatio;
  }
  void
  update_stats ()
  {
    if (m_nlostCount + m_lostCount > 0) add_val_to_history ((double) (m_lostCount) / ((double) m_nlostCount
            + (double) m_lostCount));
    m_nlostCount = 0;
    m_lostCount = 0;

    if (m_vals.empty ()) return;
    double ave = 0;
    std::deque<double>::iterator it = m_w.begin ();
    for (auto i: m_vals) ave += i * (*(it++));
    m_lossRatio = ave / ((m_vals.size () < m_winSize) ? (double) m_vals.size () : (double) m_winSize);
  }

private:

  void
  add_val_to_history (double val)
  {
    m_vals.push_back (val);
    if (m_vals.size () > m_winSize) m_vals.pop_front ();

  }

  double m_lossRatio;
  uint32_t m_lostCount;
  uint32_t m_nlostCount;
  uint16_t m_winSize;
  std::deque<double> m_vals;
  std::deque<double> m_w;
  WeightCoefs m_wType;
};


#endif /* WINAVERAGE_H_ */
