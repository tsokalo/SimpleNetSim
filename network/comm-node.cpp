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

	m_nodeType = NO_NODE_TYPE;
	m_simulator = simulator;
	m_sp = sp;
}
CommNode::~CommNode() {

}
void CommNode::SetLogCallback(add_log_func addLog) {
	m_brr->SetLogCallback(addLog);
}
void CommNode::Configure(NodeType type, std::vector<UanAddress> dst) {
	m_nodeType = type;
//	m_sp.numGen = (m_nodeType == SOURCE_NODE_TYPE) ? 2 * m_sp.numGen : m_sp.numGen;

	m_brr = routing_rules_ptr(new MulticastBrr(m_id, m_nodeType, dst, m_sp));

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
	} else {
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
	std::cout << "Ins: ";
	for (auto i : m_ins)
		std::cout << "(" << i->v_ << " -> " << m_id << " : " << i->GetLossProcess()->GetMean() << ") ";
	std::cout << std::endl;
	std::cout << "Out: ";
	for (auto i : m_outs)
		std::cout << "(" << i->v_ << " <- " << m_id << " : " << i->GetLossProcess()->GetMean() << ") ";
	std::cout << std::endl;
}

UanAddress CommNode::GetId() {
	return m_id;
}

//
// let all nodes connected to output edges here the transmission
//
NcPacket CommNode::DoBroadcast() {

	SIM_LOG_FUNC_N(COMM_NODE_LOG, m_id);
	auto plan_broadcast =
			[this](NcPacket f, MessType m)
			{
				auto notify_sending = std::bind(&MulticastBrr::NotifySending, m_brr);
				for (auto i : m_outs) m_simulator->Schedule(std::bind(&Edge::Transmit, i, std::placeholders::_1), f, (i->v_ == m_outs.at(0)->v_), m, notify_sending);;
			};

	NcPacket pkt;

	if (m_brr->MaySendServiceMessage()) {
		//
		// Sending the packet with the service message
		//
		pkt.SetFeedback(m_brr->GetServiceMessage());
		pkt.SetHeader(m_brr->GetHeaderInfo());
		auto type = pkt.GetFeedback().type;

		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " sends service message " << type.GetAsInt() << ", TTL: " << pkt.GetFeedback().ttl);
		plan_broadcast(pkt, ServiceMessType::ConvertToMessType(type));

	} else {
		//
		// Sending the packet with data
		//
		TxPlan plan = m_brr->GetTxPlan();
		assert(!plan.empty());
		//
		// in this implementation we send one data symbol per the broadcast event; so we take only one
		//
		auto item_it = plan.begin_orig_order();
		GenId genId = item_it->first;
		TxPlanItem planItem = item_it->second;
		assert(planItem.num_all != 0);
		planItem.num_all = 1;
		TxPlan planI;
		planI[genId] = planItem;

		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << ": for generation " << genId << " number of packets in plan: " << planItem.num_all);

		m_brr->UpdateSent(genId, 1);
		auto header = m_brr->GetHeaderInfo(planI);
		pkt.SetHeader(header);

		if (m_nodeType == SOURCE_NODE_TYPE) {
			pkt.SetData(m_encQueue->get_coded(genId));
		} else {
			pkt.SetData(m_decQueue->get_coded(genId));
		}
#ifdef HASH_VECTOR_FEEDBACK_ART
		m_brr->AddSentCcack(genId, ExtractCodingVector(pkt.GetData(), m_sp.genSize));
#endif

		plan_broadcast(pkt, DATA_MSG_TYPE);
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

	SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " receives from " << input->v_);

	m_brr->ProcessHeaderInfo(pkt.GetHeader());

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
		} else {
			SIM_LOG(COMM_NODE_LOG, "Node " << m_id << ", receiving linear dependent packet");
			m_brr->UpdateRcvd(genId, input->v_, true);
		}
#ifdef HASH_VECTOR_FEEDBACK_ART
		m_brr->AddRcvdCcack(genId, ExtractCodingVector(pkt.GetData(), m_sp.genSize));
#endif
		m_brr->UpdateRetransRequestInfo(m_decQueue->get_ranks(), input->v_, genId, plan_item.all_prev_acked);

	} else {

		NcPacket pkt;

		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " receive feedback symbol");
		auto f = pkt.GetFeedback();
		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " TTL has " << (f.ttl != 0 ? "not expired" : "expired"));

		m_brr->ProcessServiceMessage(f);

		if (m_nodeType == SOURCE_NODE_TYPE) {
			if (m_brr->NeedGen()) {
				m_trafGen->Start(m_sp.genSize);
			}
		}
	}

}
bool CommNode::DoIwannaSend() {

	SIM_LOG_FUNC(COMM_NODE_LOG);

	return m_brr->MaySend();
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
//		assert(txPlan.size() == 1);
		assert(txPlan.size() > 0);
		auto genId = txPlan.begin()->first;

		m_brr->UpdateLoss(genId, input->v_);
	} else {
		SIM_LOG(COMM_NODE_LOG, "Node " << m_id << " loosing feedback symbol - dropping information");
	}
}
}		//ncr
