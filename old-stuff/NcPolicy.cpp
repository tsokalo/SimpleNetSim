/*
 * NcPolicy.cpp
 *
 *  Created on: 03.01.2016
 *      Author: tsokalo
 */

#include "NcPolicy.h"
#include <algorithm>
#include <chrono>

#include <array>
#include <math.h>

#define ROUNDING_LEVEL      0.00000001
#define DEEQUALIZING_LEVEL  ROUNDING_LEVEL * 100
#define CALC_ACCURACY       0.001
bool
isequal (double v1, double v2)
{
  return (fabs (v1 - v2) < ROUNDING_LEVEL);
}
bool
isequalzero (double v1)
{
  return (fabs (v1) < ROUNDING_LEVEL);
}
bool
isgreater (double v1, double v2)
{
  return (v1 - v2 > ROUNDING_LEVEL && !isequal (v1, v2));
}
bool
isless (double v1, double v2)
{
  return (v1 - v2 < ROUNDING_LEVEL && !isequal (v1, v2));
}

NcPolicy::NcPolicy (std::vector<std::shared_ptr<CommNode> > nodes, int16_t src, int16_t dst, int16_t gs,
        NcPolicyType policyType) :
  m_nodes (nodes), m_src (src), m_dst (dst), m_gs (gs), m_policyType (policyType)
{
  m_p.resize (m_nodes.size (), 0);
  m_p.at (m_dst) = 1;
  m_curr = 0;
  m_transmitted.resize (m_nodes.size (), false);

  typedef std::chrono::high_resolution_clock myclock;
  myclock::time_point beginning = myclock::now ();
  myclock::duration d = myclock::now () - beginning;
  uint8_t seed_v = d.count () + (seed_corrector++);

  m_generator.seed (seed_v);

  update_priorities ();
  auto plan = calc_tdm_access_plan ();;
  SIM_LOG(SC_POL_LOG, "Objective: " << m_p.at(m_src));
  SIM_LOG(SC_POL_LOG, "Objective solution: ");if(SC_POL_LOG)for(auto p : plan)std::cout << p.second << " ";
  if(SC_POL_LOG)std::cout << std::endl;

  create_transmission_sequence ();
  calc_theoretical_values ();
  if (policyType == PLAYNCOOL_NC_POLICY_TYPE) calc_for_playncool ();
}

NcPolicy::~NcPolicy ()
{

}
void
NcPolicy::update_priorities ()
{
  std::vector<double> p_old (m_p.begin (), m_p.end ()), p_new (m_p.size ());

  double change = 0;
  do
    {
      std::vector<bool> updated (m_nodes.size (), false);

      //
      // function update_priority() launches recursively not one instance of itself but a bunch of instances:
      // each node starts update_priority() for each of its in-coming edges
      //
      m_acBuf.push_back (std::bind (&NcPolicy::update_priority, this, m_dst, &p_old, &p_new, &updated));

      while (!m_acBuf.empty ())
        {
          (*(m_acBuf.begin ())) ();
          m_acBuf.pop_front ();
        }
      std::copy (p_new.begin (), p_new.end (), p_old.begin ());
      change = std::accumulate (p_old.begin (), p_old.end (), 0.0) - std::accumulate (m_p.begin (), m_p.end (), 0.0);
      deequalize_priorities (p_old);
      std::copy (p_old.begin (), p_old.end (), m_p.begin ());

      print_priorities ();
      SIM_LOG (NC_POLICY_LOG, "Change level: " << change);
    }
  while (fabs (change) > CALC_ACCURACY);

  for (auto node : m_nodes)for (auto edge : node->GetOuts ())edge->SetMarked (false);;
}
void
NcPolicy::update_priority (int16_t nodeId, std::vector<double> *p_old, std::vector<double> *p_new, std::vector<bool> *updated)
{
  if (updated->at (nodeId)) return;

  SIM_LOG(NC_POLICY_LOG, "Updating priority of node " << nodeId);
  SIM_LOG(NC_POLICY_LOG, "Using the following priorities: ");
  for (uint16_t i = 0; i < m_p.size (); i++)
    {
      SIM_LOG(NC_POLICY_LOG, "Node " << i << " has priority: " << p_old->at (i));
    }

  if (nodeId == m_dst)
    {
      p_new->at (nodeId) = 1;
    }
  else
    {
      p_new->at (nodeId) = calc_priority (nodeId, *p_old);
    }
  updated->at (nodeId) = true;
  SIM_LOG(NC_POLICY_LOG, "Changed priority of node " << nodeId << " to " << p_new->at (nodeId));

  uint16_t groupSize = 0;
for (auto i_edge : m_nodes.at (nodeId)->GetIns ())
  {
    if (!updated->at (i_edge->v_))
      {
        m_acBuf.push_back (std::bind (&NcPolicy::update_priority, this, i_edge->v_, p_old, p_new, updated));
        groupSize++;
      }
  }
}
void
NcPolicy::update_unique_dof ()
{

}
bool
NcPolicy::use_edge (uint16_t n1, uint16_t n2, std::vector<double> *p_old)
{
  assert(n1 < m_nodes.size() && n2 < m_nodes.size());
  return (isgreater (p_old->at (n1), p_old->at (n2)) && !isequalzero (p_old->at (n1)));
}
int16_t
NcPolicy::get_winner (uint16_t node_i)
{
  int16_t winner_node = -1;
  double highest_priority = 0;
  for (auto edge : m_nodes.at (node_i)->GetOuts ())
    {
      if (m_transmitted.at (edge->v_)) continue;
      if(std::find(m_trans_seq.begin(), m_trans_seq.end(), edge->v_) == m_trans_seq.end())continue;
      SIM_LOG(
              NC_POLICY_LOG,"On edge between " << node_i << " and " << edge->v_ << (edge->GetLossProcess ()->IsLost () ? " loss" : " success") << ", receiver priority: " << m_p.at (edge->v_));
      if (!edge->GetLossProcess ()->IsLost ())
        {
          if(highest_priority < m_p.at(edge->v_))
            {
              highest_priority = m_p.at(edge->v_);
              winner_node = edge->v_;
            }
        }
    }
  return winner_node;
}
int16_t
NcPolicy::get_next_sender (NcPolicyType policyType)
{
  switch (policyType)
    {
  case ANTiCS_NC_POLICY_TYPE:
    {
      m_transmitted.at (m_trans_seq.at (m_curr)) = true;
      return m_trans_seq.at (m_curr++);
    }
  case SENDALWAYS_NC_POLICY_TYPE:
    {
      std::vector<uint16_t> ix;
      for (auto i = 0;
              i < m_transmitted.size ();
              i++)
                {
                  if (m_transmitted.at (i)) continue;
                  if (m_nodes.at (i)->GetNodeType () == DESTINATION_NODE_TYPE) continue;
                  if (m_nodes.at (i)->GetDof () > 0) ix.push_back (i);
                }
              std::uniform_int_distribution<> dis (0, ix.size () - 1);
              uint16_t curr_sender_index = dis (m_generator);
              return ix.at (curr_sender_index);
            }
          case PLAYNCOOL_NC_POLICY_TYPE:
            {
              auto src = std::find_if (m_nodes.begin(), m_nodes.end(), [&](node_ptr a)
                        {
                          return (a->GetNodeType() == SOURCE_NODE_TYPE);
                        });;

              SIM_LOG(NC_POLICY_LOG, "Relay starts when source sent: " << m_r << ", source already sent: " << (*src)->GetTotalSentSim ());
              if (m_r > (*src)->GetTotalSentSim ())
                {
                  return std::distance (std::begin (m_nodes), src);
                }
              else
                {
                  auto relay = std::find_if (m_nodes.begin(), m_nodes.end(), [&](node_ptr a)
                            {
                              return (a->GetNodeType() == RELAY_NODE_TYPE);
                            });;
                  std::uniform_int_distribution<> dis (0, 1);
                  return (dis (m_generator) == 0) ? std::distance (std::begin (m_nodes), src) : std::distance (std::begin (
                          m_nodes), relay);
                }
            }
          case ANTiCS_E_NC_POLICY_TYPE:
            {
              std::vector<uint16_t> ix;
              for (auto i = 0;
                      i < m_nodes.size ();
i                      ++)
                        {
                          if (!m_nodes.at (i)->IsActive()) continue;
                          if (m_nodes.at (i)->GetNodeType () == DESTINATION_NODE_TYPE)continue;
                          ix.push_back (i);
                        }
                      assert(!ix.empty());
                      std::uniform_int_distribution<> dis (0, ix.size () - 1);
                      uint16_t curr_sender_index = dis (m_generator);
                      return ix.at (curr_sender_index);
                    }
                }
              return -1;
            }
int16_t
NcPolicy::get_total_dof ()
{
  int16_t dof = 0;
  for (auto node_id : m_trans_seq)
    {
      auto node = m_nodes.at(node_id);
      if (!m_transmitted.at (node_id))
        {
          SIM_LOG(NC_POLICY_LOG, "Node: " << node_id << " has unique DOF: " << node->GetUniqueDof ());
        }
      if (!m_transmitted.at (node->GetId ())) dof += node->GetUniqueDof ();
    }

  return dof;
}
void
NcPolicy::calc_unique_dof_estimations (int16_t vr, uint32_t sent)
{
  for (auto edge : m_nodes.at(vr)->GetOuts())
    {
      if(m_p.at(vr) > m_p.at(edge->v_))
        {
          //          m_nodes.at(edge->v_)->AddUniqueDofEstimator(vr, 0, -1);
          continue;
        }

      double loss_prob = 1;
      for (auto edge2 : m_nodes.at(vr)->GetOuts())if(m_p.at(edge->v_) < m_p.at(edge2->v_) && edge->v_ != edge2->v_)loss_prob *= edge2->GetLossProcess()->GetMean();;

      double l = edge->GetLossProcess()->GetMean();
      m_nodes.at(edge->v_)->AddUniqueDofEstimator(vr, (double)sent * (1 - l) * loss_prob, -1);
    }

  //  double dof = 0;
  //  for (auto edge : m_nodes.at(vr)->GetOuts())if(m_p.at(vr) < m_p.at(edge->v_))dof += m_nodes.at(edge->v_)->GetDof (vr);;
  //  m_nodes.at (vr)->AddUniqueDofEstimator (vr, -1, dof);

  for (auto edge : m_nodes.at(vr)->GetOuts())if(m_p.at(vr) < m_p.at(edge->v_))m_nodes.at (edge->v_)->AddUniqueDofEstimator (vr, -1, m_nodes.at(edge->v_)->GetDof (vr));;
}
Edges
NcPolicy::sort_edges (Edges edges)
{
  std ::sort (edges.begin (), edges.end (), [&](Edge_ptr a, Edge_ptr b)
            {
              return m_p.at (a->v_) > m_p.at (b->v_);
            });;
  for (auto edge : edges)
    {
      SIM_LOG(NC_POLICY_LOG, "Edge owner: " << edge->v_ << " has priority " << m_p.at (edge->v_));
    }

  return edges;
}
void
NcPolicy::print_priorities ()
{
  for (uint16_t i = 0; i < m_p.size (); i++)
    {
      SIM_LOG(NC_POLICY_LOG, "Node " << i << " has priority: " << m_p.at (i));
    }
}
void
NcPolicy::filter_senders (node_ptr node)
{
  if (node->GetNodeType () == DESTINATION_NODE_TYPE) return;
  if (node->GetOuts ().empty ()) return;
  std::vector<uint16_t> temp;
  for (auto edge : node->GetOuts()) if(m_p.at(edge->v_) > m_p.at(node->GetId()))temp.push_back(edge->v_);;

  for (auto id : temp)if((std::find (m_trans_seq.begin(), m_trans_seq.end(), id) == m_trans_seq.end()))m_trans_seq.push_back(id);;

  for (auto id : temp)filter_senders(m_nodes.at(id));;
}
void
NcPolicy::create_transmission_sequence ()
{
  //
  // we ensure that no pair of nodes has the same priority
  // which allows us to be more sure in having the same transmission sequence in different iterations;
  // otherwise the std sort function can reassemble it
  //
  for (uint16_t i = 0; i < m_p.size (); i++)
    {
      for (uint16_t j = i + 1; j < m_p.size (); j++)
        {
          if (m_p.at (i) == m_p.at (j)) m_p.at (j) += 0.0000001;
        }
    }

  auto src = *std::find_if (m_nodes.begin(), m_nodes.end(), [&](node_ptr a)
            {
              return (a->GetNodeType() == SOURCE_NODE_TYPE);
            });;
  m_trans_seq.clear ();
  m_trans_seq .push_back (src->GetId ());

  filter_senders( src);

  std ::sort (m_trans_seq.begin (), m_trans_seq.end (), [&](int16_t a, int16_t b)
            {
              return m_p.at (a) < m_p.at (b);
            });;

  SIM_LOG(NC_POLICY_LOG, "Transmission sequence: ");

  for (auto i : m_trans_seq)
    {
      SIM_LOG(NC_POLICY_LOG, "Node: " << i << " has priority " << m_p.at (i));
    };;
}
void
NcPolicy::calc_theoretical_values ()
{
  auto node = m_nodes.at (get_next_sender (ANTiCS_NC_POLICY_TYPE));
  bool finished = false;
  do
    {
      if (node->GetNodeType () == DESTINATION_NODE_TYPE) finished = true;
      double totalSentCalc = 0, uniqueRcvdCalc = 0;
      if (node->GetNodeType () == SOURCE_NODE_TYPE)
        {
          uniqueRcvdCalc = m_gs;
        }
      else
        {
          for (auto edge_in : node->GetIns())
            {
              //
              // we do not count the packets received from the nodes with higher priorities
              // because the group of the nodes with higher priorities should already have
              // the complete DOF
              //
              if(m_p.at(node->GetId()) < m_p.at(edge_in->v_)) continue;
              double loss_prob = 1;
              for (auto edge_out : m_nodes.at(edge_in->v_)->GetOuts())
                {
                  //
                  // we count only that packet received by the nodes with higher priority
                  //
                  SIM_LOG(NC_POLICY_LOG, "Calculating received individual for node " << node->GetId() << ". Nodes " << edge_in->v_ << " and " << edge_out->v_
                          << " have priorities " << m_p.at(edge_in->v_) << " and " << m_p.at(edge_out->v_));
                  if(m_p.at(node->GetId()) > m_p.at(edge_out->v_)) continue;
                  if(edge_out->v_ == node->GetId()) continue;
                  SIM_LOG(NC_POLICY_LOG, "Calculating received individual for node " << node->GetId() << ". Loss probability on out edge with " << edge_out->v_
                          << " owner: " << edge_out->GetLossProcess()->GetMean());
                  loss_prob *= edge_out->GetLossProcess()->GetMean();
                }
              //              assert(m_nodes.at(edge_in->v_)->GetUniqueRcvd() > 0);
              SIM_LOG(NC_POLICY_LOG, "Calculating received individual for node " << node->GetId() << ". Total sent by the owner of the in-edge " << edge_in->v_
                      << " is: " << m_nodes.at(edge_in->v_)->GetTotalSent());
              uniqueRcvdCalc += m_nodes.at(edge_in->v_)->GetTotalSent() * (1 - edge_in->GetLossProcess()->GetMean()) * loss_prob;
            };;
          //          assert(round(uniqueRcvdCalc) <= m_gs);
        }
      SIM_LOG(NC_POLICY_LOG, "Received individual for node " << node->GetId() << ": " << uniqueRcvdCalc);
      if (node->GetNodeType () != DESTINATION_NODE_TYPE)
        {
          double loss_prob = 1;
          for (auto edge : node->GetOuts())
            {
              //
              // we count only that packet received by the nodes with higher priority
              //
              SIM_LOG(NC_POLICY_LOG, "Calculating total sent for node " << node->GetId() << ". Nodes " << edge->v_ << " and " << node->GetId()
                      << " have priorities " << m_p.at(edge->v_) << " and " << m_p.at(node->GetId()));
              if(m_p.at(edge->v_) < m_p.at(node->GetId())) continue;
              SIM_LOG(NC_POLICY_LOG, "Calculating total sent for node " << node->GetId() << ". Loss probability on in-edge with " << edge->v_
                      << " owner: " << edge->GetLossProcess()->GetMean());
              loss_prob *= edge->GetLossProcess()->GetMean();
            }
          totalSentCalc = uniqueRcvdCalc / (1 - loss_prob);
          SIM_LOG(NC_POLICY_LOG, "Sending total for node " << node->GetId() << ": " << totalSentCalc);
        }
      node->SetCalcVals (totalSentCalc, uniqueRcvdCalc);
      if (node->GetNodeType () != DESTINATION_NODE_TYPE) node = m_nodes.at (get_next_sender (ANTiCS_NC_POLICY_TYPE));
      //      else assert(round(uniqueRcvdCalc) == m_gs);
    }
  while (node->GetNodeType () != DESTINATION_NODE_TYPE || !finished);;

  m_transmitted.clear ();
  m_transmitted.resize (m_nodes.size (), false);
  m_curr = 0;
}
void
NcPolicy::calc_for_playncool ()
{
  assert(m_nodes.size() == 3);
  auto src = *std::find_if (m_nodes.begin(), m_nodes.end(), [&](node_ptr a)
            {
              return (a->GetNodeType() == SOURCE_NODE_TYPE);
            });;

  auto dst = *std::find_if (m_nodes.begin(), m_nodes.end(), [&](node_ptr a)
            {
              return (a->GetNodeType() == DESTINATION_NODE_TYPE);
            });;

  auto relay = *std::find_if (m_nodes.begin(), m_nodes.end(), [&](node_ptr a)
            {
              return (a->GetNodeType() == RELAY_NODE_TYPE);
            });;

  SIM_LOG(NC_POLICY_LOG, "Source ID: " << src->GetId() << ", relay ID: " << relay->GetId() << ", destination ID: " << dst->GetId());

  Edge_ptr src_relay_edge, src_dst_edge, relay_dst_edge;
  for(auto edge : src->GetOuts()) if(edge->v_ == relay->GetId())src_relay_edge = edge;
  for(auto edge : src->GetOuts()) if(edge->v_ == dst->GetId())src_dst_edge = edge;
  for(auto edge : relay->GetOuts()) if(edge->v_ == dst->GetId())relay_dst_edge = edge;

  double e1 = src_relay_edge->GetLossProcess ()->GetMean ();
  double e2 = relay_dst_edge->GetLossProcess ()->GetMean ();
  double e3 = src_dst_edge->GetLossProcess ()->GetMean ();

  SIM_LOG(NC_POLICY_LOG, "e1: " << e1 << ", e2: " << e2 << ", e3: " << e3);

  m_r = (double) m_gs / ((1 - e3) + ((1 - e1) * e3 * (2 - e2 - e3)) / (1 - e2 - e3 + e1 * e3));
  double k = m_r * (1 - e1) * e3 / (1 - e2 - e3 + e1 * e3);
  SIM_LOG(NC_POLICY_LOG, "r: " << m_r << ", k: " << k << ", k+r: " << k + m_r);
}
double
NcPolicy::calc_priority (uint16_t id, std::vector<double> p_old)
{
  SIM_LOG(SC_POL_LOG, "======>>>>>>>>>>>> Calculate priority for node " << id);
  Edges outs = sort_edges (m_nodes.at (id)->GetOuts ());
  assert(!outs.empty());
  std::map<uint16_t, double> datarates;

  double a = 1, b = 1, c = 0;
  datarates[id] = 1;
  a *= datarates[id];
  double group_e = 1;
  for (auto edge : outs)
    {
      //
      // ignore uninitialized tail nodes and tail nodes with smaller priority than of the node id
      //
      if(isgreater(p_old[edge->v_], p_old[id]) && !isequalzero(p_old[edge->v_]))
        {
          group_e *= edge->GetLossProcess()->GetMean();
          b *= p_old[edge->v_];
          SIM_LOG(SC_POL_LOG, "Count edge (" << id << "," << edge->v_ << ") for a and b. e = "
                  << edge->GetLossProcess()->GetMean() << ", p(" << edge->v_ << ") = " << p_old[edge->v_]
                  << ", p(" << id << ") = " << p_old[id]);
        }
      else
        {
          SIM_LOG(SC_POL_LOG, "Skip edge (" << id << "," << edge->v_ << ") for a and b. p(" << edge->v_ << ") = " << p_old[edge->v_]
                  << ", p(" << id << ") = " << p_old[id]);
        }
    }
  a *= (1 - group_e);

  auto y = [&](Edge_ptr edge_v)->double
    {
      if(edge_v->v_ == m_dst)return 0;

      double e = 1 - edge_v->GetLossProcess()->GetMean();
      SIM_LOG(SC_POL_LOG, "Base count edge (" << id << "," << edge_v->v_ << ") for y. e = "
              << edge_v->GetLossProcess()->GetMean());

      for (auto edge : outs)
        {
          //
          // ignore uninitialized tail nodes and tail nodes with smaller priority than of the node id
          //
          if(isgreater(p_old[edge->v_], p_old[edge_v->v_]) && !isequalzero(p_old[edge->v_]))
            {
              SIM_LOG(SC_POL_LOG, "Count edge (" << id << "," << edge->v_ << ") for y. e = "
                      << edge->GetLossProcess()->GetMean() << ", p(" << edge->v_ << ") = " << p_old[edge->v_]
                      << ", p(" << edge_v->v_ << ") = " << p_old[edge_v->v_]);
              e *= edge->GetLossProcess()->GetMean();
            }
        }
      return e;
    };;

  auto p_prod = [&](Edge_ptr edge_v)->double
    {
      double r = 1;
      for (auto edge : outs)
        {
          if(edge_v->v_ != edge->v_ && !isequalzero(p_old[edge->v_]))
            {
              r *= p_old[edge->v_];
              SIM_LOG(SC_POL_LOG, "Count node " << edge->v_ << " for p_prod. p(" << edge->v_ << ") = " << p_old[edge->v_]
                      << ", p(" << edge_v->v_ << ") = " << p_old[edge_v->v_]);
            }
        }
      return r;
    };;

  for (auto edge : outs)
    {
      if(isgreater(p_old[edge->v_], p_old[id]) && !isequalzero(p_old[edge->v_]) && edge->v_ != m_dst)
        {
          SIM_LOG(SC_POL_LOG, "|>>>------->>> Count edge (" << id << "," << edge->v_ << ") for c. p("
                  << edge->v_ << ") = " << p_old[edge->v_] << ", p(" << id << ") = " << p_old[id]);
          double yy = y(edge);
          double pp = p_prod(edge);
          c += yy * pp;
          SIM_LOG(SC_POL_LOG, "|<<<-------<<< Count edge (" << id << "," << edge->v_ << ") for c. p("
                  << edge->v_ << ") = " << p_old[edge->v_] << ", p(" << id << ") = " << p_old[id]
                  << ", y = " << yy << ", p_prod = " << pp);
        }
      else
        {
          SIM_LOG(SC_POL_LOG, "|>>>-------<<< Skip edge (" << id << "," << edge->v_ << ") for c. p("
                  << edge->v_ << ") = " << p_old[edge->v_] << ", p(" << id << ") = " << p_old[id]);
        }
    };;

  double p = a * b / (b + c);
  SIM_LOG(SC_POL_LOG, "======<<<<<<<<<<<< Calculate priority for node " << id << " : a = " << a << ", b = " << b
          << ", c = " << c << ", p = " << p);
  return p;
}
std::map<uint16_t, double>
NcPolicy::calc_tdm_access_plan ()
{
  std::map<uint16_t, double> plan;

  double group_e = 1;
  auto id = m_src;
  double src_datarate = 1;
  for (auto edge : get_node(m_src)->GetOuts())
    {
      //
      // ignore uninitialized tail nodes and tail nodes with smaller priority than of the node id
      //
      if(isgreater(m_p[edge->v_], m_p[id]) && !isequalzero(m_p[edge->v_]))
        {
          group_e *= edge->GetLossProcess()->GetMean();

          SIM_LOG(SC_POL_LOG, "Count edge (" << id << "," << edge->v_ << ") t(src). e = "
                  << edge->GetLossProcess()->GetMean() << ", p(" << edge->v_ << ") = " << m_p[edge->v_]
                  << ", p(" << id << ") = " << m_p[id]);
        }
      else
        {
          SIM_LOG(SC_POL_LOG, "Skip edge (" << id << "," << edge->v_ << ") for t(src). p(" << edge->v_ << ") = " << m_p[edge->v_]
                  << ", p(" << id << ") = " << m_p[id]);
        }
    }
  plan[m_src] = 1 / (src_datarate * (1 - group_e));

  m_acBuf.push_back (std::bind (&NcPolicy::calc_tdm_recursive, this, m_src, get_node (m_src)->GetOuts (), &plan));

  while (!m_acBuf.empty ())
    {
      (*(m_acBuf.begin ())) ();
      m_acBuf.pop_front ();
      SIM_LOG(SC_POL_LOG, "There are " << m_acBuf.size () << " actions in buffer to be done");
    }

  for(auto p : plan)
    {
      SIM_LOG(SC_POL_LOG, "Node " << p.first << " time = " << p.second);
    }
  double s = 0;
  for(auto p : plan)s+=p.second;
  for(auto &p : plan)p.second /= s;

  for(auto p : plan)
    {
      SIM_LOG(SC_POL_LOG, "Node " << p.first << " share = " << p.second);
    }

  return plan;
}
void
NcPolicy::calc_tdm_recursive (uint16_t id, Edges outs, std::map<uint16_t, double> *plan)
{
  auto y = [&](Edge_ptr edge_v)->double
    {
      if(edge_v->v_ == m_dst)return 0;

      double e = 1 - edge_v->GetLossProcess()->GetMean();
      SIM_LOG(SC_POL_LOG, "Base count edge (" << id << "," << edge_v->v_ << ") for y. e = "
              << edge_v->GetLossProcess()->GetMean());

      for (auto edge : get_node(id)->GetOuts())
        {
          //
          // ignore uninitialized tail nodes and tail nodes with smaller priority than of the node id
          //
          if(isgreater(m_p[edge->v_], m_p[edge_v->v_]) && !isequalzero(m_p[edge->v_]))
            {
              SIM_LOG(SC_POL_LOG, "Count edge (" << id << "," << edge->v_ << ") for y .. e . e = "
                      << edge->GetLossProcess()->GetMean() << ", p(" << edge->v_ << ") = " << m_p[edge->v_]
                      << ", p(" << edge_v->v_ << ") = " << m_p[edge_v->v_]);
              e *= edge->GetLossProcess()->GetMean();
            }
        }

      double p = 1;
      for (auto edge : get_node(id)->GetOuts())
        {
          //
          // ignore uninitialized tail nodes and tail nodes with smaller priority than of the node id
          //
          if(isgreater(m_p[edge->v_], m_p[id]) && !isequalzero(m_p[edge->v_]))
            {
              SIM_LOG(SC_POL_LOG, "Count edge (" << id << "," << edge->v_ << ") for y .. p . e = "
                      << edge->GetLossProcess()->GetMean() << ", p(" << edge->v_ << ") = " << m_p[edge->v_]
                      << ", p(" << edge_v->v_ << ") = " << m_p[edge_v->v_]);
              p *= edge->GetLossProcess()->GetMean();
            }
        }
      return e / (1 - p);
    };;

  SIM_LOG(SC_POL_LOG, "ENTER IT");for(auto edge : outs)
    {
      if(edge->v_ == m_dst)
        {
          plan->insert(plan->end(), std::pair<uint16_t, double>(m_dst, 0));
        }
      else
        {
          //
          // skip nodes, for which the access time is already calculated
          // and the nodes with zero priority
          //

          if(plan->find(edge->v_) == plan->end() && !isequalzero(m_p.at(edge->v_)))
            {
              plan->insert(plan->end(), std::pair<uint16_t, double>(edge->v_, y(edge) / m_p.at(edge->v_)));
              m_acBuf.push_back (std::bind (&NcPolicy::calc_tdm_recursive, this, edge->v_, get_node(edge->v_)->GetOuts(), plan));
            }
        }
    };;
}
std::shared_ptr<CommNode>
NcPolicy::get_node (uint16_t id)
{
  for(auto node : m_nodes)
    {
      if(node->GetId() == id)return node;
    };;
  assert(0);
}
void
NcPolicy::deequalize_priorities (std::vector<double> &p)
{
  SIM_LOG(SC_POL_LOG, "De-equalizing priorities");
  double s = std::accumulate (p.begin (), p.end (), 0.0);
  for (uint16_t i = 0; i < p.size (); i++)
    for (uint16_t j = 0; j < p.size (); j++)
      if (i != j && isequalzero (p.at (i) - p.at (j)) && !isequalzero (p.at (j)))
        {
          SIM_LOG(SC_POL_LOG, "De-equalizing priority for node " << j << " with current priority " << p.at (j));
          p.at (j) += DEEQUALIZING_LEVEL;
        }
      else
        {
          SIM_LOG(SC_POL_LOG, "p[" << i << "] = " << p.at (i) << ", p[" << j << "] = " << p.at (j)
                  << " " << (i != j)
                  << " " << isequalzero (p.at (i) - p.at (j))
                  << " " << !isequalzero(p.at (j)));
        }
  if (!isequalzero (s - std::accumulate (p.begin (), p.end (), 0.0))) deequalize_priorities (p);

  for (uint16_t i = 0; i < p.size (); i++)
    {
      SIM_LOG(SC_POL_LOG, "Priority of node " << i << " is " << p.at(i));
    }
}
