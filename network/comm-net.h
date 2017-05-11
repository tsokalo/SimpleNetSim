/*
 * CommNet.h
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#ifndef COMMNET_H_
#define COMMNET_H_

#include "comm-node.h"
#include "routing-rules/god-view-routing-rules.h"
#include "utils/logger.h"
#include "simulator.h"
#include "assert.h"
#include <iostream>
#include <random>

#include "utils/sim-parameters.h"

namespace ncr
{

  class GodViewRoutingRules;

  /*
   * Here one-source-destination scenario with mesh network in between is analyzed
   */
  class CommNet
  {
    typedef std::shared_ptr<CommNode> node_ptr;
    typedef std::shared_ptr<Simulator> simulator_ptr;
    typedef std::shared_ptr<Logger> logger_ptr;
  public:
    CommNet (uint16_t numNodes, SimParameters sp);
    virtual
    ~CommNet ();
    /*
     *  between the src and dst we create two edges; output edges have shared parts with input edges
     *  double e1 is the packet loss ratio on the edge (src, dst)
     *  double e2 is the packet loss ratio on the edge (dst, src)
     */
    void
    ConnectNodes (UanAddress src, UanAddress dst, double e1, double e2 = -1);

    void
    ConnectNodesDirected (UanAddress src, UanAddress dst, double e);

    void
    PrintNet ();

    void
	Configure();
    /*
     * One cycle consists of:
     * 1. sender selection
     * 2. sending a message and receiving this message by all nodes
     *
     * Parameter "cycles" - the number of cycles - determines the simulation duration
     */
    void
    Run (int64_t cycles);

    /*
     * only one source is possible
     */
    void
    SetSource (UanAddress i);
    /*
     * multiple destinations are possible; set each of them individually
     */
    void
    SetDestination (UanAddress i);

    node_ptr
    GetNode (UanAddress i)
    {
      assert((uint16_t )i < m_nodes.size ());
      return m_nodes.at (i);
    }
    std::vector<node_ptr>
    GetNodes ()
    {
      return m_nodes;
    }
    UanAddress
    GetSrc ()
    {
      return m_src;
    }
    UanAddress
    GetDst ()
    {
      assert(!m_dst.empty ());
      return m_dst.at (0);
    }
    UanAddress
    GetDst (uint16_t i)
    {
      assert(m_dst.size () > i);
      return m_dst.at (i);
    }

    void
    DoBroadcast (node_ptr sender);

    void
    EnableLog (std::string path);

  private:
    node_ptr
    SelectSender ();

    logger_ptr m_logger;

    std::vector<node_ptr> m_nodes;
    UanAddress m_src;
    std::vector<UanAddress> m_dst;

    std::random_device m_rd;
    std::mt19937 m_gen;

    simulator_ptr m_simulator;
    SimParameters m_sp;
  };
}

#endif /* COMMNET_H_ */
