/*
 * CommNode.h
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#ifndef COMMNODE_H_
#define COMMNODE_H_

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
#include "edge.h"
#include "routing-rules/multicast-brr.h"
#include "utils/coder.h"
#include "traffic/traffic-generator.h"
#include "traffic/traffic-sink.h"
#include "simulator.h"
#include "utils/sim-parameters.h"

namespace ncr {

typedef std::map<UanAddress, EdgeId> node_ids;

class CommNode {

	typedef std::shared_ptr<MulticastBrr> routing_rules_ptr;
	typedef std::shared_ptr<TrafficGenerator> traf_gen_ptr;
	typedef std::shared_ptr<TrafficSink> traf_sink_ptr;
	typedef std::shared_ptr<encoder_queue> encoder_queue_ptr;
	typedef std::shared_ptr<decoder_queue> decoder_queue_ptr;
	typedef std::shared_ptr<Simulator> simulator_ptr;
	typedef std::function<void(MessType)> set_msg_type_func;

public:
	CommNode(UanAddress id, simulator_ptr simulator, SimParameters sp);
	virtual
	~CommNode();
	void
	SetLogCallback(add_log_func addLog);
	void
	Configure(NodeType type, std::vector<UanAddress> dst);

	Edge_ptr
	CreateInputEdge(UanAddress src_id, double e);
	Edge_ptr
	CreateInputEdge(UanAddress src_id, std::string traceFile);
	Edge_ptr
	CreateOutputEdge(UanAddress dst_id, Edge_ptr input);
	Edge_ptr
	GetEdge(UanAddress src_id, UanAddress dst_id);
	void
	PrintEdges();
	EdgesIn GetIns() {
		return m_ins;
	}
	EdgesOut GetOuts() {
		return m_outs;
	}

	UanAddress
	GetId();

	//
	// let all nodes connected to output edges here the transmission
	//
	NcPacket
	DoBroadcast();
	//
	// it will be automatically called with input edges when the transmission is triggered by any output edges
	// of other nodes, when the input edge of the current node coincides with the output edge of the other node
	//
	void
	Receive(Edge * input, NcPacket symb);
	//
	// if a packet erasure is identified, the packet is dropped but the information about the packet loss
	// is used by the node to keep the history of erasures
	//
	void
	NotifyLoss(Edge * input, NcPacket symb);

	//
	// checks if there is any outstanding data to be sent
	//
	bool DoIwannaSend();

	Datarate GetDatarate() {
		return m_sp.apiRate;
	}
	NodeType GetNodeType() {
		return m_nodeType;
	}

	void SetMessTypeCallback(set_msg_type_func f);

	void EnableCcack(hash_matrix_set_ptr hashMatrixSet);

private:

	void NotifyGen(GenId genId);

	EdgesIn m_ins;
	EdgesOut m_outs;

	node_ids m_in_ids;
	node_ids m_out_ids;
	UanAddress m_id;

	NodeType m_nodeType;

	std::default_random_engine m_generator;
	std::uniform_real_distribution<double> m_distribution;

	routing_rules_ptr m_brr;

	traf_gen_ptr m_trafGen;
	traf_sink_ptr m_trafSink;
	encoder_queue_ptr m_encQueue;
	decoder_queue_ptr m_decQueue;

	simulator_ptr m_simulator;
	SimParameters m_sp;

	get_rank_func m_getRank;

};
}	//ncr

#endif /* COMMNODE_H_ */
