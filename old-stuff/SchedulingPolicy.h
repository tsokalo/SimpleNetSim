/*
 * SchedulingPolicy.h
 *
 *  Created on: Jul 14, 2016
 *      Author: tsokalo
 */

#ifndef SCHEDULINGPOLICY_H_
#define SCHEDULINGPOLICY_H_

#include <functional>
#include <random>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <tuple>
#include <functional>

#include "filter.h"
#include "file-helper.h"

struct NodeInfo
{
  NodeInfo () :
    id (0), p (1)
  {
  }
  NodeInfo (uint16_t id, double p)
  {
    this->id = id;
    this->p = p;
  }

  NodeInfo&
  operator= (NodeInfo a)
  {
    this->id = a.id;
    this->p = a.p;
    return *this;
  }

  uint16_t id;
  double p;
};

bool
is_equal (double v1, double v2);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Use case 1
 *
 * 1. if data originates on me (I'm the source node), I will call RcvFwrdFeedback(me, 0, generated_packets)
 *
 * 2. after that I call GetNumToSend (), which tells me an amount of data together with coding redundancy
 * I use this value to understand that I have something to send and I will try to get the channel access
 *
 * 3. When I get the channel access I send as much data as suits in my transmission slot, which can be less
 * than returned by GetNumToSend(). Afterwards I call RcvAck(sent_packets) to indicate I did actually sent
 * It allows me to mentioned, how much is still remaining to be sent.
 *
 *
 * Use case 2
 *
 * 1. if I'm a helper or the destination, I will call RcvFwrdFeedback(me, a, rcvd_packets) after receiving any data;
 * the value "a" I take from the packet header; the value "rcvd_packets" is the total number
 * of the correctly received packets (both innovative and not innovative).
 *
 * Then I proceed to the steps 2 and 3 in the use case 1
 *
 */
class SchedulingPolicy
{
  typedef std::unordered_map<uint16_t, double> node_list;
  typedef std::unordered_map<uint16_t, AveBinaryFilter<64> > loss_rate_matrix;
  typedef std::map<uint16_t, AveBinaryFilter<64> > lindep_rate_matrix;

  typedef std::tuple<double, double, uint16_t> FwrdFeedback;
  // src id, priority, loss rate, nl rate, num rcvd
  typedef std::tuple<uint16_t, double, double, double, uint16_t> bckwrd_info_item;
  typedef std::vector<bckwrd_info_item> bckwrd_info;
  typedef std::tuple<uint16_t, double, bckwrd_info> BckwrdFeedback;
  typedef std::tuple<uint16_t, double, uint32_t> RetransmissionRequest;
  typedef std::shared_ptr<FileHelper> file_helper_ptr;

public:

  SchedulingPolicy ();
  virtual
  ~SchedulingPolicy ();

  /*
   * initialization
   */
  void
  SetNode (NodeInfo node, uint32_t num = 0);
  void
  SetMainRouteAffiliation (NodeInfo prevHopNode, NodeInfo nextHopNode, bool belongToMainPath);
  void
  Initialize ();
  void
  EnableFileLog (std::string folder, std::function<uint64_t
  ()> getEventId)
  {
    m_resPath = folder;
    m_getEventId = getEventId;
  }

  /*
   * Forward feedback
   */
  FwrdFeedback
  GetFwrdFeedback ();
  bool
  RcvFwrdFeedback (NodeInfo src, double a, int16_t numRcvd, int16_t numLost, int16_t numL = 0);

  /*
   * Backward feedback
   */
  bool
  ShouldSendBckwrdFeedback ();
  BckwrdFeedback
  GetBckwrdFeedback ();
  void
  RcvBckwrdFeedback (uint16_t src, double p, bckwrd_info info, uint32_t missing_symbols);
  void
  RcvAck (uint32_t num, uint32_t missing_symbols);

  /*
   * Sending strategy
   */
  void
  UpdateSendStrategy ();
  uint32_t
  GetNumToSend ();
  void
  MarkSent (uint16_t num);

  /*
   * Retransmission strategy
   */
  bool
  NeedRetransmission ();
  RetransmissionRequest
  GetRetransmissionRequest ();
  void
  RcvRetransmissionRequest (RetransmissionRequest r);

private:

  /*
   * number of packets to send considering only the main route
   */
  double
  GetNumToSendOrig ();

  /*
   * number of packets to send considering the mesh routing policy
   */
  double
  GetNumToSendMesh ();

  void
  AddNode (NodeInfo node);

  /*
   * Estimation of own priority
   */
  void
  UpdatePriority (NodeInfo node);
  double
  CalcPriority (node_list coalition);

  /*
   * Formation of own coalition
   */
  void
  UpdateCoalition ();
  void
  SortCoalition (node_list &coalition);
  void
  ShowCoalition (node_list l, std::string message);

  node_list m_pList;
  node_list m_coalition;
  uint16_t m_nextHopId;
  uint16_t m_prevHopId;
  uint16_t m_id;
  bool m_belongMainPath;

  //
  // own calculation:
  // values from X node to Me
  //
  loss_rate_matrix m_e;
  lindep_rate_matrix m_l;
  double m_meshUniqueInfo;
  double m_origUniqueInfo;
  double m_meshSentInfo;
  double m_origSentInfo;
  uint32_t m_missing;
  std::map<uint16_t, uint16_t> m_rcvd;
  uint32_t m_numSent;
  bool m_acked;

  AveFloatingFilter<10> m_alpha;
  AveFloatingFilter<10> m_beta;

  //
  // information from feedbacks:
  // values from Me and Y(s) to X node
  //
  std::map<int16_t, double> m_a;
  std::map<uint16_t, std::map<int16_t, double> > m_f_e;
  std::map<uint16_t, std::map<int16_t, double> > m_f_l;

  //
  // log files
  //
  std::string m_resPath;
  file_helper_ptr m_alphaFile;
  file_helper_ptr m_betaFile;
  file_helper_ptr m_meshInfoFile;
  file_helper_ptr m_origInfoFile;
  std::map<int16_t, file_helper_ptr> m_eFile;
  std::map<int16_t, file_helper_ptr> m_lFile;
  std::function<uint64_t
  ()> m_getEventId;

  /*
   * if threshold_num_tosend equals 1, there is no limitation issued by packet trains
   */
static constexpr double threshold_num_tosend = 1;
/*
 * if stat_ratio equals 0, only the original routing is used
 */
static constexpr double stat_ratio = 0.5;
/*
 * with m_b equal 0 each node with higher priority than I have will be added to the coalition
 */
static constexpr double m_b = 0.01;
};

#endif /* SCHEDULINGPOLICY_H_ */

