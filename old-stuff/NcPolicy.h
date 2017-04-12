/*
 * NcPolicy.h
 *
 *  Created on: 03.01.2016
 *      Author: tsokalo
 */

#ifndef NCPOLICY_H_
#define NCPOLICY_H_

#include "CommNet.h"
#include <functional>
#include <random>

class CommNet;
typedef std::function<void
()> Action;
typedef std::deque<Action> ActionBuffer;

class NcPolicy
{
  typedef std::shared_ptr<CommNode> node_ptr;
  typedef std::vector<double>::iterator p_it;
public:
  NcPolicy (std::vector<std::shared_ptr<CommNode> > nodes, int16_t src, int16_t dst, int16_t gs, NcPolicyType policyType);
  virtual
  ~NcPolicy ();

  int16_t
  get_winner (uint16_t node_i);

  inline bool
  is_finished ()
  {
    return (m_curr == m_trans_seq.size () - 1 || m_nodes.at (m_dst)->CanDecode ());
  }

  int16_t
  get_next_sender (NcPolicyType policyType);
  int16_t
  get_total_dof ();
  void
  calc_unique_dof_estimations (int16_t vr, uint32_t sent);

protected:
  void
  update_priorities ();
  void
  update_priority (int16_t nodeId, std::vector<double> *p_old, std::vector<double> *p_new, std::vector<bool> *updated);
  void
  update_unique_dof ();

  /*
   * sort in descending oder of the priorities of the nodes hanging at end of the edges
   */
  Edges
  sort_edges (Edges edges);
  bool
  use_edge (uint16_t src, uint16_t dst, std::vector<double> *p_old);
  void
  print_priorities ();
  void
  filter_senders (node_ptr node);
  void
  create_transmission_sequence ();
  void
  calc_theoretical_values ();
  void
  calc_for_playncool ();

  double
  calc_priority (uint16_t id, std::vector<double> p_old);
  std::map<uint16_t, double>
  calc_tdm_access_plan();
  void
  calc_tdm_recursive(uint16_t id, Edges outs, std::map<uint16_t, double> *plan);
  std::shared_ptr<CommNode>
  get_node(uint16_t);
  void
  deequalize_priorities(std::vector<double> &p);

  std::vector<std::shared_ptr<CommNode> > m_nodes;
  std::vector<double> m_p;
  std::vector<int16_t> m_trans_seq;
  std::vector<bool> m_transmitted;
  uint16_t m_curr;
  int16_t m_src;
  int16_t m_dst;
  int16_t m_gs;

  ActionBuffer m_acBuf;
  std::deque<int16_t> m_acBufGroupSizes;

  NcPolicyType m_policyType;
  std::default_random_engine m_generator;

  double m_r;//for PlayNCool
};

#endif /* NCPOLICY_H_ */
