/*
 * SchedulingPolicy.cpp
 *
 *  Created on: Jul 14, 2016
 *      Author: tsokalo
 */

#include "scheduling-policy.h"
#include <assert.h>
#include <sstream>
#include <iostream>
#include <cmath>
#include <math.h>
#include <map>

bool
is_equal (double x, double y)
{
  return (std::abs (x - y) < 0.0000001);
}

SchedulingPolicy::SchedulingPolicy ()
{
  m_meshUniqueInfo = 0;
  m_origUniqueInfo = 0;
  m_meshSentInfo = 0;
  m_origSentInfo = 0;
  m_resPath.clear ();
  m_numSent = 0;
  m_acked = false;
  m_missing = 0;
}

SchedulingPolicy::~SchedulingPolicy ()
{

}

void
SchedulingPolicy::SetNode (NodeInfo node, uint32_t num)
{
  m_id = node.id;
  AddNode (node);

  m_meshUniqueInfo = num;
  m_origUniqueInfo = num;
}

void
SchedulingPolicy::SetMainRouteAffiliation (NodeInfo prevHopNode, NodeInfo nextHopNode, bool belongToMainPath)
{
  m_prevHopId = prevHopNode.id;
  m_nextHopId = nextHopNode.id;
  AddNode (prevHopNode);
  AddNode (nextHopNode);
  m_belongMainPath = belongToMainPath;
}

void
SchedulingPolicy::Initialize ()
{
  assert (m_pList.size () > 1 && m_pList.size () <= 3);

  m_pList[m_id] = 0;
  if (m_belongMainPath) m_pList[m_id] = 1;
  m_pList[m_prevHopId] = 1;
  m_pList[m_nextHopId] = 1;

  if (m_belongMainPath) m_e[m_prevHopId].set_default (0);
  if (m_belongMainPath) m_l[m_prevHopId].set_default (0);
  if (m_belongMainPath) m_f_e[m_id][m_nextHopId] = 0;
  if (m_belongMainPath) m_f_l[m_id][m_nextHopId] = 0;

  UpdateCoalition ();

  if (!m_resPath.empty ())
    {
      m_alphaFile = file_helper_ptr (new FileHelper (LogInfo (m_id, "alpha"), m_resPath));
      m_betaFile = file_helper_ptr (new FileHelper (LogInfo (m_id, "beta"), m_resPath));
      m_meshInfoFile = file_helper_ptr (new FileHelper (LogInfo (m_id, "mesh_info"), m_resPath));
      m_origInfoFile = file_helper_ptr (new FileHelper (LogInfo (m_id, "orig_info"), m_resPath));
      std::map<int16_t, file_helper_ptr> m_eFile;
      std::map<int16_t, file_helper_ptr> m_lFile;
    }
}

//
// src node sends data to dst node and dst node have calculated e;
// now dst node sends the feedback containing e and I receive this feedback;
// I may be the src node and may be not
//
void
SchedulingPolicy::RcvBckwrdFeedback (uint16_t src, double p, bckwrd_info info, uint32_t missing_symbols)
{
  assert(src != m_id);

  AddNode (NodeInfo (src, p));

  std::stringstream ss;
  for (auto i : info)
    {
      uint16_t dst = std::get<0>(i);
      AddNode (NodeInfo(dst, std::get<1>(i)));
      m_f_e[dst][src] = std::get<2>(i);
      m_f_l[dst][src] = std::get<3>(i);
      if(dst == m_id && !m_resPath.empty())
        {
          if(!m_eFile[src])m_eFile.at(src) = file_helper_ptr (new FileHelper (LogInfo (m_id, "packet_loss", src), m_resPath));
          if(!m_lFile[src])m_lFile.at(src) = file_helper_ptr (new FileHelper (LogInfo (m_id, "lindep_loss", src), m_resPath));
          m_eFile.at(src)->Write(m_getEventId(), m_f_e[m_id][src]);
          m_lFile.at(src)->Write(m_getEventId(), m_f_l[m_id][src]);
        }
      if(dst == m_id) RcvAck(std::get<4>(i), missing_symbols);

      ss << "<" << std::get<0>(i) << "," << std::get<1>(i) << "," << std::get<2>(i) << "," << std::get<3>(i) << "," << std::get<4>(i) << "> ";
    };;
  SIM_LOG(SC_POL_LOG, "Node " << m_id << " reading backward feedback: [" << src << "," << p << "," << ss.str() << "," << missing_symbols << "]");

  UpdatePriority (NodeInfo (src, p));
  for(auto i : info)
    {
      UpdatePriority (NodeInfo(std::get<0>(i), std::get<1>(i)));
    }

  UpdateCoalition ();
}
//
// src node sends data to some dst node (doesn't actually matter whom exactly)
// and add the value "a" to the packet header
//
bool
SchedulingPolicy::RcvFwrdFeedback (NodeInfo src, double a, int16_t numRcvd, int16_t numLost, int16_t numL)
{
  SIM_LOG(SC_POL_LOG, "Node " << m_id << " read forward feedback: [" << src.id << "," << src.p << "," << a << "," << numRcvd << ","
          << numLost << "," << numL << "]");
  AddNode (src);

  m_a[src.id] = a;

  assert(numLost + numL <= numRcvd);
  numRcvd -= (numLost + numL);
  m_e[src.id].add (numRcvd, 0);
  m_l[src.id].add (numRcvd, 0);
  m_e[src.id].add (numLost, 1);
  m_l[src.id].add (numL, 1);
  m_e[src.id].update ();
  m_l[src.id].update ();

  if (src.p < m_pList.at (m_id) || is_equal (src.p, m_pList.at (m_id)))
    {
      SIM_LOG(SC_POL_LOG, "Node " << m_id << " receives forward feedback from the node with lower or equal priority: [" << src.id << "," << src.p
              << "," << m_id << "," << m_pList.at(m_id) << "]");
      return is_equal (src.p, m_pList.at (m_id));
    }

  m_rcvd[src.id] += numRcvd;

  double v = numRcvd;
  if (src.id == m_prevHopId) m_origUniqueInfo += v;
  m_meshUniqueInfo += (is_equal (m_e.at (src.id).val (), 0)) ? v : v * m_a.at (src.id) / m_e.at (src.id).val ();

  SIM_LOG(SC_POL_LOG, "Node " << m_id << " updated values: [" << src.id << "," << m_e.at(src.id).val()
          << "," << m_origUniqueInfo << "," << m_meshUniqueInfo << "]");
  return true;
}
void
SchedulingPolicy::MarkSent (uint16_t num)
{
  assert(num == 1);

  if (!m_acked)
    {
      //
      // make ACK guess
      //
      double v = m_numSent;

      m_meshSentInfo += (is_equal (m_beta.val (), 0)) ? 0 : v / m_beta.val ();
      m_origSentInfo += (is_equal (m_alpha.val (), 0)) ? 0 : v / m_alpha.val ();

      SIM_LOG(SC_POL_LOG, "Node " << m_id << " guess ACK: [" << num << "," << m_origUniqueInfo << "," <<
              m_meshUniqueInfo << "," << m_origSentInfo << "," << m_meshSentInfo << "]");
    }
  m_numSent = num;
  m_acked = false;
  SIM_LOG(SC_POL_LOG, "Node " << m_id << " mark sent: [" << num << "]");
}
void
SchedulingPolicy::UpdateSendStrategy ()
{
  double numOrig = GetNumToSendOrig (), numMesh = GetNumToSendMesh ();
  double remainingMesh = (m_meshUniqueInfo > m_meshSentInfo) ? m_meshUniqueInfo - m_meshSentInfo : 0;
  double remainingOrig = (m_origUniqueInfo > m_origSentInfo) ? m_origUniqueInfo - m_origSentInfo : 0;
  double new_alpha = (is_equal (remainingMesh, 0)) ? 0 : numMesh / remainingMesh;
  double new_beta = (is_equal (remainingOrig, 0)) ? 0 : numOrig / remainingOrig;
  if (!is_equal (new_alpha, 0)) m_alpha.add (new_alpha);
  if (!is_equal (new_beta, 0)) m_beta.add (new_beta);

  if (!m_resPath.empty ())
    {
      m_alphaFile->Write (m_getEventId (), m_alpha.val ());
      m_betaFile->Write (m_getEventId (), m_beta.val ());
    }

  SIM_LOG(SC_POL_LOG, "Node " << m_id << " num to send: [" << m_origUniqueInfo << "," << m_meshUniqueInfo << ","
          << m_origSentInfo << "," << m_meshSentInfo << "," << numOrig << "," << numMesh << ","
          << remainingOrig << "," << remainingMesh << "," << stat_ratio
          << "," << new_alpha << "," << new_beta << "," << m_alpha.val ()<< "," << m_beta.val () << "," <<
          stat_ratio * m_beta.val () * m_origUniqueInfo + (1 - stat_ratio) * m_alpha.val () * m_meshUniqueInfo << "]");
}
uint32_t
SchedulingPolicy::GetNumToSend ()
{
  return stat_ratio * m_beta.val () * m_origUniqueInfo + (1 - stat_ratio) * m_alpha.val () * m_meshUniqueInfo;
}

//
// num includes the coding redundancy
//
void
SchedulingPolicy::RcvAck (uint32_t num, uint32_t missing_symbols)
{
  //
  // TODO: use not number of the received packets but the receive mask
  //
  m_acked = true;

  double v = num;
  m_meshSentInfo += v;
  m_origSentInfo += v;
  double remainingMesh = (m_meshUniqueInfo > m_meshSentInfo) ? m_meshUniqueInfo - m_meshSentInfo : 0;
  double remainingOrig = (m_origUniqueInfo > m_origSentInfo) ? m_origUniqueInfo - m_origSentInfo : 0;

  if ((remainingMesh < missing_symbols && !is_equal (remainingMesh, missing_symbols)))
    {
      m_missing = (m_meshSentInfo < missing_symbols - remainingMesh) ? missing_symbols - remainingMesh - m_meshSentInfo : 0;
      m_meshSentInfo = (m_meshSentInfo < missing_symbols - remainingMesh) ? 0 : m_meshSentInfo - (missing_symbols
              - remainingMesh);
    }

  if ((remainingOrig < missing_symbols && !is_equal (remainingOrig, missing_symbols)))
    {
      m_origSentInfo = (m_origSentInfo < missing_symbols - remainingOrig) ? 0 : m_origSentInfo - (missing_symbols
              - remainingOrig);
    }

  if (!m_resPath.empty ())
    {
      m_meshInfoFile->Write (m_getEventId (), m_meshUniqueInfo, m_meshSentInfo, m_missing);
      m_origInfoFile->Write (m_getEventId (), m_origUniqueInfo, m_origSentInfo, m_missing);
    }

  SIM_LOG(SC_POL_LOG, "Node " << m_id << " receive ACK: [" << num << "," << m_origUniqueInfo << "," << m_meshUniqueInfo
          << "," << m_origSentInfo << "," << m_meshSentInfo<< "]");
}

SchedulingPolicy::FwrdFeedback
SchedulingPolicy::GetFwrdFeedback ()
{
  double a = 1;
  for (auto c_node : m_coalition)a *= m_f_e.at(m_id).at(c_node.first);;
  SIM_LOG(SC_POL_LOG, "Node " << m_id << " writing forward feedback: [" << m_id << "," << m_pList.at (m_id) << "," << a << "]");
  return FwrdFeedback (a, m_pList.at (m_id), m_id);
}
SchedulingPolicy::BckwrdFeedback
SchedulingPolicy::GetBckwrdFeedback ()
{
  //
  // for all the nodes known by the current node and not present in its coalition
  // i.e. having lower priority than the current node
  //
  bckwrd_info info_v;
  std::stringstream ss;
  for (auto node : m_pList)
    {
      if(node.first == m_id)continue;
      if(m_coalition.count(node.first) != 0)continue;
      //
      // add info for only those nodes, who have lower priority then me
      //
      ss << "<" << node.first << "," << node.second << "," << m_e.at(node.first).val() << "," << m_l.at(node.first).val() << "," << m_rcvd[node.first] << "> ";
      info_v.push_back(bckwrd_info_item(node.first, node.second, m_e.at(node.first).val(), m_l.at(node.first).val(), m_rcvd[node.first]));
      //
      // we send the feedback only once
      // if smbd does not get it, it's not our problem)))
      //
      m_rcvd[node.first] = 0;
    };;

  SIM_LOG(SC_POL_LOG, "Node " << m_id << " writing backward feedback: [" << m_id << "," << m_pList.at (m_id) << "," << ss.str() << "]");
  return BckwrdFeedback (m_id, m_pList.at (m_id), info_v);
}
bool
SchedulingPolicy::ShouldSendBckwrdFeedback ()
{
  return true;
}
bool
SchedulingPolicy::NeedRetransmission ()
{
  return (!is_equal (m_missing, 0));
}
SchedulingPolicy::RetransmissionRequest
SchedulingPolicy::GetRetransmissionRequest ()
{
  return RetransmissionRequest (m_id, m_pList.at (m_id), m_missing);
}
void
SchedulingPolicy::RcvRetransmissionRequest (RetransmissionRequest r)
{
  double remainingMesh = (m_meshUniqueInfo > m_meshSentInfo) ? m_meshUniqueInfo - m_meshSentInfo : 0;
  double remainingOrig = (m_origUniqueInfo > m_origSentInfo) ? m_origUniqueInfo - m_origSentInfo : 0;

  double missing_symbols = std::get<2> (r);
  if ((remainingMesh < missing_symbols && !is_equal (remainingMesh, missing_symbols)))
    {
      m_missing = (m_meshSentInfo < missing_symbols - remainingMesh) ? missing_symbols - remainingMesh - m_meshSentInfo : 0;
      m_meshSentInfo = (m_meshSentInfo < missing_symbols - remainingMesh) ? 0 : m_meshSentInfo - (missing_symbols
              - remainingMesh);
    }

  if ((remainingOrig < missing_symbols && !is_equal (remainingOrig, missing_symbols)))
    {
      m_origSentInfo = (m_origSentInfo < missing_symbols - remainingOrig) ? 0 : m_origSentInfo - (missing_symbols
              - remainingOrig);
    }

}
double
SchedulingPolicy::GetNumToSendOrig ()
{
  double remainingOrig = (m_origUniqueInfo > m_origSentInfo) ? m_origUniqueInfo - m_origSentInfo : 0;
  return (is_equal (m_f_e[m_id][m_nextHopId], 1)) ? 0 : (((m_belongMainPath) ? (remainingOrig / (1 - m_f_e[m_id][m_nextHopId]))
          : 0));
}
double
SchedulingPolicy::GetNumToSendMesh ()
{
  double num_to_send = (m_meshUniqueInfo > m_meshSentInfo) ? m_meshUniqueInfo - m_meshSentInfo : 0;
  double p = 1;
  for(auto c_node : m_coalition)p *= m_f_e[m_id][c_node.first];
  num_to_send = (is_equal (p, 1)) ? num_to_send : num_to_send / (1 - p);

  return (num_to_send < threshold_num_tosend) ? 0 : num_to_send;
}

void
SchedulingPolicy::AddNode (NodeInfo node)
{
  m_pList[node.id] = node.p;
}

void
SchedulingPolicy::UpdateCoalition ()
{
  ShowCoalition (m_coalition, "old coalition");
  m_coalition.clear ();
  if (m_nextHopId != m_id) m_coalition[m_nextHopId] = m_pList[m_nextHopId];

  for (auto node : m_pList)
    {
      if(node.first == m_nextHopId)continue;
      if(node.first == m_id)continue;
      //
      // ignore the nodes, which cannot help at all
      //
      if(node.second <= m_pList.at(m_id))continue;

      //
      // ignore some nodes, which cannot help significantly
      //
      node_list new_coalition = m_coalition;
      new_coalition[node.first] = node.second;
      SortCoalition(new_coalition);
      double new_priority = CalcPriority (new_coalition);

      ShowCoalition(new_coalition, "sorted temporary coalition");
      SIM_LOG(SC_POL_LOG, "Node " << m_id << ": [" << m_pList[m_id] << "," << new_priority << "," << m_b << "]");

      if(new_priority - m_pList.at(m_id) > m_b) m_coalition = new_coalition;
    };;

  if (!m_coalition.empty ()) m_pList.at (m_id) = CalcPriority (m_coalition);

  ShowCoalition (m_coalition, "sorted new coalition");
}

void
SchedulingPolicy::UpdatePriority (NodeInfo node)
{
  if (node.id != m_id)
    {
      m_pList.at (node.id) = node.p;
    }
  else
    {
      //
      // assume the nodes in the coalition are already sorted by priority
      //
      if (m_coalition.empty ())
        {
          SIM_LOG(SC_POL_LOG, "Node " << m_id << " has no coalition. The priority will not be updated");
          return;
        }
      m_pList.at (m_id) = CalcPriority (m_coalition);

      SIM_LOG(SC_POL_LOG, "Node " << m_id << " updated own priority: [" << m_pList[m_id] << "]");
    }
}

double
SchedulingPolicy::CalcPriority (node_list coalition)
{
  std::map<uint16_t, double> datarates;

  double a = 1, b = 1, c = 0;
  datarates[m_id] = 1;
  a *= datarates[m_id];
  double group_e = 1;
  for (auto node : coalition)
    {
      if(std::isgreater(m_pList[node.first], m_pList[m_id]))group_e *= m_f_e[m_id][node.first];
      b *= m_pList[node.first];
    }
  a *= (1 - group_e);

  auto y = [&](uint16_t v_id)->double
    {
      double e = 1 - m_f_e[m_id][v_id];
      for (auto node : coalition)
      if(std::isgreater(m_pList[node.first], m_pList[v_id]))
      e *= m_f_e[m_id][node.first];
      return e;
    };;

  auto p_prod = [&](uint16_t v_id)->double
    {
      double r = 1;
      for (auto node : coalition)
      if(std::isgreater(m_pList[node.first], m_pList[m_id]) && v_id != node.first)
      r *= m_pList[node.first];
      return r;
    };;

  for (auto node : coalition)
    {
      if(std::isgreater(m_pList[m_id], m_pList[node.first]))continue;
      c += y(node.first) * p_prod(node.first);
    };;
//  SIM_LOG(SC_POL_LOG, "Node " << m_id << ": [" << m_pList[node.first] << "," << m_f_e[m_id][node.first] << "," << group_e << "," << p << "]");
  m_pList[m_id] = a * b / (b + a * c);

  return m_pList[m_id];
}

void
SchedulingPolicy::SortCoalition (node_list &coalition)
{
  //
  // sort the nodes in the coalition in descending order of the priority
  //
  std::map<double, uint16_t> temp;
  for(auto v : coalition) temp[v.second] = v.first;;
  coalition.clear ();
  for(auto v : temp) coalition[v.second] = v.first;;
}
void
SchedulingPolicy::ShowCoalition (node_list l, std::string message)
{
  if (SC_POL_LOG)
    {
      std::cout << "Node " << m_id << " " << message << ": ";
      if (l.empty ()) std::cout << "empty";
      for (auto node : l) std::cout << node.first << " -> " << node.second << " :: ";;
      std::cout << std::endl;
    }
}
