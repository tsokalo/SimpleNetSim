/*
 * CommNet.cpp
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#include <functional>
#include "comm-net.h"
#include "utils/nc-packet.h"
#include "ccack/hash-matrix.h"

namespace ncr {

CommNet::CommNet(uint16_t numNodes, SimParameters sp) :
		m_gen(m_rd()) {
	m_simulator = simulator_ptr(new Simulator());
	m_sp = sp;

	m_nodes.resize(numNodes);
	std::vector<std::shared_ptr<CommNode> >::iterator it = m_nodes.begin();
	for (auto& i : m_nodes) {
		i = std::shared_ptr<CommNode>(new CommNode(std::distance(m_nodes.begin(), it++), m_simulator, m_sp));
	};;

	SetSource(SRC_UANADDRESS);
}
//CommNet::CommNet(CommNet *net) :
//		m_gen(m_rd()) {
//	m_simulator = simulator_ptr(new Simulator());
//	m_sp = net->GetSimParameters();
//
//	m_nodes.resize(net->GetNodes().size());
//	std::vector<std::shared_ptr<CommNode> >::iterator it = m_nodes.begin();
//	for (auto& i : m_nodes) {
//		i = std::shared_ptr<CommNode>(new CommNode(std::distance(m_nodes.begin(), it++), m_simulator, m_sp));
//	};;
//
//	SetSource(SRC_UANADDRESS);
//	for (auto n : net->GetNodes()) {
//		for (auto e : n->GetOuts()) {
//			ConnectNodes(n->GetId(), e->v_, e->GetLossProcess()->GetMean(), e->reverse_edge_->GetLossProcess()->GetMean());
//		}
//	}
//	for (auto dst : net->GetDstIds()) {
//		SetDestination(dst);
//	}
//	Configure();
//}
CommNet::~CommNet() {

}
/*
 *  between the src and dst we create two edges; output edges have shared parts with input edges
 *  double e1 is the packet loss ratio on the edge (dst, src)
 *  double e2 is the packet loss ratio on the edge (src, dst)
 */
void CommNet::ConnectNodes(UanAddress src, UanAddress dst, double e1, double e2) {

	if (eq(e2, -1)) e2 = e1;
	assert(m_nodes.size() > (uint16_t )src && m_nodes.size() > (uint16_t )dst && src != dst && m_nodes.at(src) && m_nodes.at(dst));

	std::shared_ptr<Edge> fromSrc = m_nodes.at(src)->CreateOutputEdge(dst, m_nodes.at(dst)->CreateInputEdge(src, e1));
	std::shared_ptr<Edge> fromDst = m_nodes.at(dst)->CreateOutputEdge(src, m_nodes.at(src)->CreateInputEdge(dst, e2));
	fromSrc->reverse_edge_ = fromDst;
	fromDst->reverse_edge_ = fromSrc;
}
void
CommNet::ConnectNodes(UanAddress src, UanAddress dst, std::string traceFile)
{
	assert(m_nodes.size() > (uint16_t )src && m_nodes.size() > (uint16_t )dst && src != dst && m_nodes.at(src) && m_nodes.at(dst));

	std::shared_ptr<Edge> fromSrc = m_nodes.at(src)->CreateOutputEdge(dst, m_nodes.at(dst)->CreateInputEdge(src, traceFile));
	std::shared_ptr<Edge> fromDst = m_nodes.at(dst)->CreateOutputEdge(src, m_nodes.at(src)->CreateInputEdge(dst, traceFile));
	fromSrc->reverse_edge_ = fromDst;
	fromDst->reverse_edge_ = fromSrc;
}
void CommNet::ConnectNodesDirected(UanAddress src, UanAddress dst, double e) {
	assert(m_nodes.size() > (uint16_t )src && m_nodes.size() > (uint16_t )dst && src != dst && m_nodes.at(src) && m_nodes.at(dst));

	m_nodes.at(src)->CreateOutputEdge(dst, m_nodes.at(dst)->CreateInputEdge(src, e));
}
void CommNet::PrintNet() {
	for (auto i : m_nodes) {
		std::cout << ">> Node " << i->GetId() << std::endl;
		i->PrintEdges();
	};;
}

void CommNet::Configure() {

	if (m_dst.empty()) {
		std::cout << "Setting default DST " << m_nodes.size() - 1 << std::endl;
		SetDestination(m_nodes.size() - 1);
	}

	for (uint16_t j = 0; j < m_nodes.size(); j++) {
		if (m_nodes.at(j)->GetId() == m_src) m_nodes.at(j)->Configure(SOURCE_NODE_TYPE, m_dst);
		else m_nodes.at(j)->Configure(RELAY_NODE_TYPE, m_dst);
	}

}
void CommNet::Run(int64_t cycles) {

#ifdef HASH_VECTOR_FEEDBACK_ART
	//
	// each node should get the same set of hash matrices
	//
	hash_matrix_set_ptr hashMatrixSet = hash_matrix_set_ptr(new HashMatrixSet(m_sp.ccackLevels, m_sp.genSize, m_sp.fieldSize));
#endif

	if (m_logger) {
		for (auto& i : m_nodes) {
			i->SetLogCallback(std::bind(&Logger::AddLog, m_logger, std::placeholders::_1, std::placeholders::_2));
#ifdef HASH_VECTOR_FEEDBACK_ART
			i->EnableCcack(hashMatrixSet);
#endif
		};;
	}

	int64_t i = 0;
	while (((m_logger) ? m_logger->GetLogCounter() : i++) < cycles) {
//		PrintProgress(cycles, i);
		DoBroadcast(SelectSender());
		m_simulator->Execute();
//		if (m_logger) m_logger->IncTime();
	}
}
/*
 * only one source is possible
 */
void CommNet::SetSource(UanAddress i) {
	assert(m_nodes.size() > (uint16_t )i);
	m_src = i;
}
/*
 * only one destination is possible
 */
void CommNet::SetDestination(UanAddress i) {
	assert(m_nodes.size() > (uint16_t )i);
	assert(m_src != i);
	m_dst.push_back(i);
}
void CommNet::DoBroadcast(node_ptr sender) {

	NcPacket symb = sender->DoBroadcast();
}

void CommNet::EnableLog(std::string path) {
	m_logger = logger_ptr(new Logger(path));
	m_simulator->SetIncTimeCallback(std::bind(&Logger::IncTime, m_logger, std::placeholders::_1));
	auto f = std::bind(&Logger::SetMessType, m_logger, std::placeholders::_1);
	m_simulator->SetMessTypeCallback(f);
	for (auto &n : m_nodes)
		n->SetMessTypeCallback(f);
	;

}
CommNet::node_ptr CommNet::SelectSender() {
	std::vector<uint16_t> v;
	uint16_t i = 0;
	do {
//		std::cout << "Selecting sender.." << std::endl;
		v.clear();
		for (std::vector<node_ptr>::iterator it = m_nodes.begin(); it != m_nodes.end(); it++) {
//			if ((*it)->GetId() == m_dst) continue;
			if ((*it)->DoIwannaSend()) v.push_back(std::distance(m_nodes.begin(), it));
		}
		assert(i++ < 1000);
	} while (v.empty());

	std::uniform_int_distribution<> dis(0, v.size() - 1);
	return m_nodes.at(v.at(dis(m_gen)));
}
} //ncr

