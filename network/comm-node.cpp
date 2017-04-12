/*
 * CommNode.cpp
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#include "comm-node.h"
#include "utils/utils.h"
#include "utils/nc-packet.h"
#include "utils/brr-pkt-header.h"
#include <chrono>

using namespace std::placeholders;

namespace ncr {

uint16_t seed_corrector = 0;

CommNode::CommNode(UanAddress id, simulator_ptr simulator, SimParameters sp) :
		m_id(id), m_distribution(0.0, 1.0) {

	typedef std::chrono::high_resolution_clock myclock;
	myclock::time_point beginning = myclock::now();
	myclock::duration d = myclock::now() - beginning;
	uint8_t seed_v = d.count() + (seed_corrector++);
	m_generator.seed(seed_v);

	m_simulator = simulator;
	m_sp = sp;
}
CommNode::~CommNode() {

}
void CommNode::SetLogCallback(add_log_func addLog) {
	m_brr->SetLogCallback(addLog);
}
void CommNode::Configure(NodeType type, UanAddress dst) {
	m_nodeType = type;
	m_sp.numGen = (m_nodeType == SOURCE_NODE_TYPE) ? 2 * m_sp.numGen : m_sp.numGen;

	m_brr = routing_rules_ptr(new NcRoutingRules(m_id, m_nodeType, dst, m_sp));

	if (m_nodeType == SOURCE_NODE_TYPE) {
		m_encQueue = encoder_queue_ptr(new encoder_queue(m_sp.numGen, m_sp.genSize, m_sp.symbolSize));
		m_encQueue->set_notify_callback(std::bind(&CommNode::NotifyGen, this, std::placeholders::_1));
		auto send_func = std::bind(&encoder_queue::enque, m_encQueue, std::placeholders::_1);
		m_trafGen = traf_gen_ptr(new TrafficGenerator(send_func, m_sp.symbolSize, m_sp.apiRate));
		m_getRank = std::bind(&encoder_queue::rank, m_encQueue, std::placeholders::_1);
		m_brr->SetGetRankCallback(m_getRank);
		m_brr->SetCoderHelpInfoCallback(
				std::bind(&encoder_queue::get_help_info, m_encQueue, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		//
		// fill the encoder buffer
		//
		m_trafGen->Start(m_sp.genSize * (m_sp.numGenBuffering + 1));

		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " type " << m_nodeType);
	}
	else {
		m_decQueue = decoder_queue_ptr(new decoder_queue(m_sp.numGen, m_sp.genSize, m_sp.symbolSize));
		m_getRank = std::bind(&decoder_queue::rank, m_decQueue, std::placeholders::_1);
		m_brr->SetGetRankCallback(m_getRank);
		m_brr->SetGetCodingMatrixCallback(std::bind(&decoder_queue::get_coding_matrix, m_decQueue, std::placeholders::_1));
		m_brr->SetGetCoderInfoCallback(std::bind(&decoder_queue::get_coder_info, m_decQueue, std::placeholders::_1));
		m_brr->SetCoderHelpInfoCallback(
				std::bind(&decoder_queue::get_help_info, m_decQueue, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		m_trafSink = traf_sink_ptr(new TrafficSink());
		m_brr->SetReceiveAppCallback(std::bind(&TrafficSink::Receive, m_trafSink, std::placeholders::_1));

		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " type " << m_nodeType);
	}
}

Edge_ptr CommNode::CreateInputEdge(int16_t src_id, double e) {
	assert(src_id != m_id);
	Edge_ptr edge = Edge_ptr(new Edge(src_id, std::bind(&CommNode::Receive, this, std::placeholders::_1, std::placeholders::_2)));
	edge->SetNotifyLoss(std::bind(&CommNode::NotifyLoss, this, std::placeholders::_1, std::placeholders::_2));
	edge->SetLossProcess(std::shared_ptr<BernoulliLossProcess>(new BernoulliLossProcess(e)));

	m_ins.push_back(edge);
	m_in_ids[edge->v_] = m_ins.size() - 1;
	return edge;
}
Edge_ptr CommNode::CreateOutputEdge(int16_t dst_id, Edge_ptr input) {
	assert(dst_id != m_id);

	Edge_ptr edge = Edge_ptr(new Edge(std::bind(&Edge::Receive, input, std::placeholders::_1), dst_id));
	edge->SetLossProcess(input->GetLossProcess());

	m_outs.push_back(edge);
	m_out_ids[edge->v_] = m_outs.size() - 1;
	return edge;
}

Edge_ptr CommNode::GetEdge(int16_t src_id, int16_t dst_id) {
	return (src_id == m_id) ? m_outs.at(m_out_ids[dst_id]) : m_ins.at(m_in_ids[src_id]);
}

void CommNode::PrintEdges() {
	if (COMM_NODE_LOG) {
		std::cout << "Ins: ";
		for (auto i : m_ins)
			std::cout << "(" << i->v_ << " -> " << m_id << " : " << i->GetLossProcess()->GetMean() << " , " << i->GetLossProcess()->IsLost() << ") ";
		std::cout << std::endl;
		std::cout << "Out: ";
		for (auto i : m_outs)
			std::cout << "(" << i->v_ << " <- " << m_id << " : " << i->GetLossProcess()->GetMean() << " , " << i->GetLossProcess()->IsLost() << ") ";
		std::cout << std::endl;
	}
}

UanAddress CommNode::GetId() {
	return m_id;
}

//
// let all nodes connected to output edges here the transmission
//
NcPacket CommNode::DoBroadcast() {

	SIM_LOG_FUNC_N(COMM_NODE_LOG, m_id);

	assert(m_nodeType != DESTINATION_NODE_TYPE);

	TxPlan plan = m_brr->GetTxPlan();
	assert(!plan.empty());

//	auto it = plan.begin();
//	std::cout << "Node " << m_id << ": Tx plan: ";
//	for (; it != plan.end(); it++) {
//		std::cout << "<" << it->first << " - " << it->second.num_all << " - " << m_getRank(it->first) << "> ";
//	}
//	std::cout << std::endl;

	auto item_it = plan.begin_orig_order();

	assert(item_it != plan.end());

	GenId genId = item_it->first;
	TxPlanItem planItem = item_it->second;

	assert(planItem.num_all != 0);

	SIM_LOG(COMM_NODE_LOG, "Node " << m_id << ": for generation " << genId << " number of packets in plan: " << planItem.num_all);

	m_brr->UpdateSent(genId, 1);

	planItem.num_all = 1;
	TxPlan planI;
	planI[genId] = planItem;

	HeaderInfo header = m_brr->GetHeaderInfo(planI);

	NcPacket pkt;
	if (m_nodeType == SOURCE_NODE_TYPE) {
		pkt.SetData(m_encQueue->get_coded(genId));
	}
	else {
		pkt.SetData(m_decQueue->get_coded(genId));
	}
#ifdef HASH_VECTOR_FEEDBACK_ART
	m_brr->AddSentCcack(genId, ExtractCodingVector(pkt.GetData(), m_sp.genSize));
#endif
	pkt.SetHeader(header);
	auto notify_sending = std::bind(&NcRoutingRules::NotifySending, m_brr);
	for (auto &i : m_outs)
		m_simulator->Schedule(std::bind(&Edge::Transmit, i, std::placeholders::_1), pkt, (i->v_ == m_outs.at(0)->v_), DATA_MSG_TYPE, notify_sending);
	;

	if (m_nodeType == SOURCE_NODE_TYPE) {
		if (m_brr->NeedGen()) {
			m_trafGen->Start(m_sp.genSize);
		}
	}

	return pkt;
}

//
// it will be automatically called with input edges when the transmission is triggered by any output edges
// of other nodes, when the input edge of the current node coincides with the output edge of the other node
//
void CommNode::Receive(Edge* input, NcPacket pkt) {

	SIM_LOG_FUNC(COMM_NODE_LOG);

	assert(!m_outs.empty());

	auto plan_broadcast =
			[this](NcPacket f, MessType m)
			{
				auto notify_sending = std::bind(&NcRoutingRules::NotifySending, m_brr);
				for (auto i : m_outs) m_simulator->Schedule(std::bind(&Edge::Transmit, i, std::placeholders::_1), f, (i->v_ == m_outs.at(0)->v_), m, notify_sending);;
			};
	;

	SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " receives from " << input->v_);

	m_brr->RcvHeaderInfo(pkt.GetHeader());

	if (!pkt.IsFeedbackSymbol()) {

		auto txPlan = pkt.GetHeader().txPlan;
		//
		// special for this simulator
		//
		assert(txPlan.size() == 1);
		GenId genId = txPlan.begin()->first;
		auto plan_item = txPlan.begin()->second;

		if (m_nodeType == SOURCE_NODE_TYPE) return;

		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " receives packet from generation " << genId);

		auto rank = m_decQueue->rank(genId);
		m_decQueue->enque(pkt.GetData(), genId);

		if (rank < m_decQueue->rank(genId)) {
			m_brr->UpdateRcvd(genId, input->v_, m_decQueue->get_uncoded());
		}
		else {
			SIM_LOG(COMM_NODE_LOG, "Node " << m_id << ", receiving linear dependent packet");
			m_brr->UpdateRcvd(genId, input->v_, true);
		}
#ifdef HASH_VECTOR_FEEDBACK_ART
		m_brr->AddRcvdCcack(genId, ExtractCodingVector(pkt.GetData(), m_sp.genSize));
#endif
		NcPacket feedback;

		//
		// --->
		//
		if (m_brr->MaySendNetDiscovery()) {
			SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " sends the network discovery message with maximum TTL");
			feedback.SetHeader(m_brr->GetHeaderInfo());
			feedback.SetFeedback(m_brr->GetNetDiscoveryInfo());
			plan_broadcast(feedback, NETDISC_MSG_TYPE);
		}
		else {
			SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " refuses to send the network discovery message");

			//
			// --->
			//
			if (m_brr->MaySendRetransRequest(m_decQueue->get_ranks(), input->v_, genId, plan_item.all_prev_acked)) {
				SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " sends the retransmission request");
				feedback.SetHeader(m_brr->GetHeaderInfo());
				feedback.SetFeedback(m_brr->GetRetransRequestInfo());
				plan_broadcast(feedback, RETRANS_REQUEST_MSG_TYPE);
			}
			else {
				SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " refuses to send the retransmission request");

				//
				// --->
				//
				if (m_brr->MaySendFeedback()) {
					SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " sends the feedback");
					feedback.SetHeader(m_brr->GetHeaderInfo());
					feedback.SetFeedback(m_brr->GetFeedbackInfo());
					plan_broadcast(feedback, FEEDBACK_MSG_TYPE);
				}
				else {
					SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " refuses to send the feedback");
				}
			}
		}
	}
	else {

		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " receive feedback symbol");
		m_brr->RcvFeedbackInfo(pkt.GetFeedback());

		if (pkt.GetFeedback().ttl != 0) {

			//
			// --->
			//
			if (pkt.GetFeedback().netDiscovery) {

				SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " receive the network discovery message with TTL " << pkt.GetFeedback().ttl);

				if (m_brr->MaySendNetDiscovery(pkt.GetFeedback().ttl)) {
					SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " sends the network discovery message with TTL " << pkt.GetFeedback().ttl - 1);
					NcPacket feedback;
					feedback.SetHeader(m_brr->GetHeaderInfo());
					feedback.SetFeedback(m_brr->GetNetDiscoveryInfo(pkt.GetFeedback().ttl - 1));
					plan_broadcast(feedback, NETDISC_MSG_TYPE);
				}
				else {
					SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " refuses to send the network discovery message");
				}
			}
			else {
				SIM_LOG(COMM_NODE_LOG, "Node " << m_id << ". Network discovery flag is not set");

				if (m_brr->HasRetransRequest(pkt.GetFeedback())) {

					SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " processing retransission request");

					//
					// --->
					//
					if (m_brr->ProcessRetransRequest(pkt.GetFeedback())) {
						if (m_nodeType != SOURCE_NODE_TYPE) {

							SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " forward retransmission request. TTL " << pkt.GetFeedback().ttl);

							NcPacket feedback;
							feedback.SetHeader(m_brr->GetHeaderInfo());
							feedback.SetFeedback(m_brr->GetRetransRequestInfo(pkt.GetFeedback().ttl - 1));
							//							m_brr->ResetRetransInfo();
							plan_broadcast(feedback, RETRANS_REQUEST_MSG_TYPE);
						}
						else {
							SIM_LOG(COMM_NODE_LOG, "Node " << m_id << ". The source does not forward retransmission requests");
						}
					}
					else {
						SIM_LOG(COMM_NODE_LOG, "Node " << m_id << ". Retransmission request is either not set or I should not forward it");
					}
				}
				else {
					SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " there is no retransmission request");
				}
			}

		}
		else {
			SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " TTL has expired");
		}
	}
}
bool CommNode::DoIwannaSend() {

	SIM_LOG_FUNC(COMM_NODE_LOG);

	return m_brr->MaySendData();
//
//	if (!m_brr->MaySendData()) return false;
//
//	TxPlan txPlan = m_brr->GetTxPlan();
//	auto accumulate = [](TxPlan plan)->uint32_t
//	{
//		uint32_t sum = 0;
//		for(auto item : plan)
//		{
//			sum += item.second.num_all;
//		}
//		return sum;
//	};
//
//	return (accumulate(txPlan) > 0);
}
void CommNode::SetMessTypeCallback(set_msg_type_func f) {
	if (m_trafSink) m_trafSink->SetMessTypeCallback(f);
}

void CommNode::EnableCcack(hash_matrix_set_ptr hashMatrixSet) {
	m_brr->EnableCcack(hashMatrixSet);
}

void CommNode::NotifyGen(GenId genId) {

	SIM_LOG_FUNC(COMM_NODE_LOG);

	m_brr->UpdateRcvd(genId, m_id);
}
void CommNode::NotifyLoss(Edge * input, NcPacket pkt) {

	SIM_LOG_FUNC(COMM_NODE_LOG);

	SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " symbol loss is notified");

	if (!pkt.IsFeedbackSymbol()) {

		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " loosing information symbol");
		//
		// special for this simulator
		//
		auto txPlan = pkt.GetHeader().txPlan;
		assert(txPlan.size() == 1);
		auto genId = txPlan.begin()->first;

		m_brr->UpdateLoss(genId, input->v_);
	}
	else {
		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " loosing feedback symbol - dropping information");
	}
}
}		//ncr
