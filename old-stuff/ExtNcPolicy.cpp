/*
 * ExtNcPolicy.cpp
 *
 *  Created on: Jul 15, 2016
 *      Author: tsokalo
 */

#include "ext-nc-policy.h"
#include <assert.h>
#include <random>

ExtNcPolicy::ExtNcPolicy (std::vector<std::shared_ptr<CommNode> > nodes, int16_t src, int16_t dst, int16_t gs,
        NcPolicyType policyType) :
  NcPolicy (nodes, src, dst, gs, policyType)
{
  // TODO Auto-generated constructor stub

}

ExtNcPolicy::~ExtNcPolicy ()
{
  // TODO Auto-generated destructor stub
}

int16_t
ExtNcPolicy::get_next_sender (NcPolicyType policyType)
{
  assert(policyType == ANTiCS_E_NC_POLICY_TYPE);

  std::vector<uint16_t> ix;
  for (uint16_t i = 0; i < m_nodes.size (); i++)
    {
      if (m_nodes.at (i)->GetNodeType () == DESTINATION_NODE_TYPE) continue;
      if (m_nodes.at (i)->IsActive ()) ix.push_back (i);
    }
  std::uniform_int_distribution<> dis (0, ix.size () - 1);
  uint16_t curr_sender_index = dis (m_generator);
  return ix.at (curr_sender_index);
}
