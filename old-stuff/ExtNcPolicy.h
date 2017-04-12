/*
 * ExtNcPolicy.h
 *
 *  Created on: Jul 15, 2016
 *      Author: tsokalo
 */

#ifndef EXTNCPOLICY_H_
#define EXTNCPOLICY_H_

#include "NcPolicy.h"
#include "header.h"

class ExtNcPolicy : public NcPolicy
{
public:
  ExtNcPolicy (std::vector<std::shared_ptr<CommNode> > nodes, int16_t src, int16_t dst, int16_t gs, NcPolicyType policyType);
  virtual
  ~ExtNcPolicy ();

  inline bool
  is_finished ()
  {
    return (m_nodes.at (m_dst)->CanDecode ());
  }

  //
  // emulates channel access procedure
  //
  int16_t
  get_next_sender (NcPolicyType policyType);

private:

};

#endif /* EXTNCPOLICY_H_ */
