/*
 * UniPriorCalc.cpp
 *
 *  Created on: 27.10.2016
 *      Author: tsokalo
 */

#include "god-view-routing-rules.h"
#include "utils/comparison.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "lp-solver/lp-solver-header.h"

#include <cmath>

#define DEEQUALIZING_LEVEL  PRECISION_ * 100
#define CALC_ACCURACY       0.00001

namespace ncr {

GodViewRoutingRules::GodViewRoutingRules(comm_net_ptr commNet) {
	m_commNet = commNet;

	CalculatePriorities();
}
virtual GodViewRoutingRules::~GodViewRoutingRules() {

}
void GodViewRoutingRules::CalculatePriorities() {
	std::vector<UanAddress> dsts = m_commNet->GetDstIds();
	for (auto node : m_commNet->GetNodes()) {
		if (node->GetNodeType() == DESTINATION_NODE_TYPE) {
			dsts.push_back(node->GetId());
			node->SetNodeType(RELAY_NODE_TYPE);
		}
	}

	for (auto dst : dsts) {
		CalculateUnicastPriorities(dst);
	}

	CalculateMulticastPriorities();
}

void GodViewRoutingRules::CalculateUnicastPriorities(UanAddress dst) {

	for (auto node : m_commNet->GetNodes()) {

		double p;
		//
		// ...
		//

		auto src = node->GetId();
		m_p[dst][src] = p;
	}
}
void GodViewRoutingRules::CalculateMulticastPriorities() {

	auto dsts = m_commNet->GetDstIds();
	for (auto node : m_commNet->GetNodes()) {

		auto id = node->GetId();
		//
		// form the set of destinations without including myself
		//
		auto dsts_prime = dsts;
		auto it = std::find(dsts_prime.begin(), dsts_prime.end(), id);
		if (it != dsts_prime.end()) dsts_prime.erase(it, it + 1);
		//
		// form the set of cooperating nodes for each destination
		//
		std::map<UanAddress, node_map_t> coalitions;
		for (auto edge : node->GetOuts()) {
			auto neighbor = edge->v_;
			assert(neighbor != node);
			for (auto dst : dsts_prime) {
				if (m_p[dst][id] < m_p[dst][neighbor]) coalitions[dst].add(neighbor, m_p[dst][neighbor]);
			}
		}

		//
		// The nodes from all coalitions for each destination
		//
		node_map_t big_coalition;

		for (auto edge : node->GetOuts()) {

			auto neighbor = edge->v_;

			for (auto coalition_p : coalitions) {
				auto dst = coalition_p.first;
				auto coalition = coalition_p.second;
				if (coalition.find(neighbor) != coalition.end()) {
					big_coalition.add(neighbor, m_p[dst][neighbor]);
				}
			}
		}

		auto a = [&](node_map_t coalition)->double
		{
			double v = 1;
			for (auto edge : node->GetOuts()) {

				auto neighbor = edge->v_;
				if(!coalition.is_in(neighbor))continue;
				v *= edge->GetLossProcess()->GetMean();
			}
			return 1 - v;
		};

		auto multi_a = [&]()->double
		{
			double v = 1;
			for (auto coalition_p : coalitions) {
				auto dst = coalition_p.first;
				auto coalition = coalition_p.second;
				double w = a(coalition);
				v = v > w ? w : v;
			}
			return v;
		};

		auto b = [&](node_map_t coalition, UanAddress neighbor, UanAddress dst)->double
		{
			double v = 1;
			for (auto edge : node->GetOuts()) {

				auto neighbor2 = edge->v_;
				if(!coalition.is_in(neighbor2))continue;
				if(neighbor == neighbor2)v *= (1 - edge->GetLossProcess()->GetMean());

				if(m_p[dst][neighbor2] < m_p[dst][neighbor])continue;
				v *= edge->GetLossProcess()->GetMean();
			}
			return v;
		};

		auto multi_b = [&](UanAddress neighbor)->double
		{
			double v = 0;
			for (auto coalition_p : coalitions) {
				auto dst = coalition_p.first;
				auto coalition = coalition_p.second;
				double w = b(coalition, neighbor, dst)/m_p[dst][neighbor];
				v = v < w ? w : v;
			}
			return v;
		};

		double t = 1;
		for (auto it : big_coalition) {
			t += multi_b(id, it.first, big_coalition, subset_dsts);
		}
		m_multi_p[id] = multi_a() / t;
	}
}

UniPriorCalc::UniPriorCalc(comm_net_ptr commNet) {

	m_commNet = commNet;
	m_p.resize(m_commNet->GetNodes().size(), 0);
	m_p[m_commNet->GetDst()] = DESTINATION_PRIORITY;
	for (auto node : m_commNet->GetNodes())
		m_d[node->GetId()] = node->GetDatarate();
	;
}

UniPriorCalc::~UniPriorCalc() {

}
double UniPriorCalc::GetOptChannelUses() {
	UpdatePriorities();
	PrintPriorities();
	return m_p.at(m_commNet->GetSrc()).val() / m_d.at(m_commNet->GetSrc());
}

void UniPriorCalc::UpdatePriorities() {

	priorities_t p_old(m_p.begin(), m_p.end()), p_new(m_p.size());

	double change = 0;
	do {
		std::vector<bool> updated(m_commNet->GetNodes().size(), false);

		//
		// function UpdatePriority() launches recursively not one instance of itself but a bunch of instances:
		// each node starts UpdatePriority() for each of its in-coming edges
		//
		m_acBuf.push_back(std::bind(&UniPriorCalc::UpdatePriority, this, m_commNet->GetDst(), &p_old, &p_new, &updated));

		while (!m_acBuf.empty()) {
			(*(m_acBuf.begin()))();
			m_acBuf.pop_front();
		}
		std::copy(p_new.begin(), p_new.end(), p_old.begin());

		auto p_old_it = p_old.begin();
		auto p_it = m_p.begin();
		while (p_old_it != p_old.end()) {
			if (*p_it != DESTINATION_PRIORITY && *p_old_it != DESTINATION_PRIORITY) change += fabs(*p_it - *p_old_it);
			p_old_it++;
			p_it++;
		}
		change /= m_d[m_commNet->GetSrc()];
		change /= (double) m_d.size();

		DeequalizePriorities(p_old);
		std::copy(p_old.begin(), p_old.end(), m_p.begin());

		PrintPriorities();
		SIM_LOG(GOD_VIEW, "Change level: " << change);

	} while (fabs(change) > CALC_ACCURACY);

	for (auto node : m_commNet->GetNodes())
		for (auto edge : node->GetOuts())
			edge->SetMarked(false);
	;
}
void UniPriorCalc::UpdatePriority(int16_t nodeId, priorities_t *p_old, priorities_t *p_new, std::vector<bool> *updated) {
	if (updated->at(nodeId)) return;

	SIM_LOG(GOD_VIEW, "Updating priority of node " << nodeId);
	SIM_LOG(GOD_VIEW, "Using the following priorities: ");
	for (uint16_t i = 0; i < m_p.size(); i++) {
		SIM_LOG(GOD_VIEW, "Node " << i << " has priority: " << p_old->at (i));
	}

	p_new->at(nodeId) = CalcPriority(nodeId, *p_old);

	updated->at(nodeId) = true;
	SIM_LOG(GOD_VIEW, "Changed priority of node " << nodeId << " to " << p_new->at (nodeId));

	uint16_t groupSize = 0;
	for (auto i_edge : m_commNet->GetNodes().at(nodeId)->GetIns()) {
		if (!updated->at(i_edge->v_)) {
			m_acBuf.push_back(std::bind(&UniPriorCalc::UpdatePriority, this, i_edge->v_, p_old, p_new, updated));
			groupSize++;
		}
	};;
}
void UniPriorCalc::DeequalizePriorities(priorities_t &p) {
	//	SIM_LOG(GOD_VIEW, "De-equalizing priorities");
	//	double s = std::accumulate(p.begin(), p.end(), 0.0);
	//	for (uint16_t i = 0; i < p.size(); i++)
	//		for (uint16_t j = 0; j < p.size(); j++)
	//			if (i != j && eqzero(p.at(i) - p.at(j)) && !eqzero(p.at(j))) {
	//				SIM_LOG(GOD_VIEW, "De-equalizing priority for node " << j << " with current priority " << p.at (j));
	//				p.at(j) += DEEQUALIZING_LEVEL;
	//			}
	//			else {
	//				SIM_LOG(GOD_VIEW, "p[" << i << "] = " << p.at (i) << ", p[" << j << "] = " << p.at (j)
	//						<< " " << (i != j)
	//						<< " " << eqzero (p.at (i) - p.at (j))
	//						<< " " << !eqzero(p.at (j)));
	//			}
	//	if (!eqzero(s - std::accumulate(p.begin(), p.end(), 0.0))) DeequalizePriorities(p);
	//
	//	for (uint16_t i = 0; i < p.size(); i++) {
	//		SIM_LOG(GOD_VIEW, "Priority of node " << i << " is " << p.at(i));
	//	}
}
void UniPriorCalc::PrintPriorities() {
	for (uint16_t i = 0; i < m_p.size(); i++) {
		SIM_LOG(GOD_VIEW, "Node " << i << " has priority: " << m_p.at (i));
	}
}
Edges UniPriorCalc::SortEdges(Edges edges) {
	std::sort(edges.begin(), edges.end(), [&](Edge_ptr a, Edge_ptr b)
	{
		return m_p.at (a->v_) > m_p.at (b->v_);
	});
	;
	//	for (auto edge : edges)
	//	{
	//		SIM_LOG(GOD_VIEW, "Edge owner: " << edge->v_ << " has priority " << m_p.at (edge->v_));
	//	}

	return edges;
}
priority_t UniPriorCalc::CalcPriority(UanAddress id, priorities_t p_old) {

	SIM_LOG(GOD_VIEW, "======>>>>>>>>>>>> Calculate priority for node " << id);
	SIM_LOG_FUNC(GOD_VIEW);

	if (m_commNet->GetDst() == id) {
		return DESTINATION_PRIORITY;
	}

	auto a = [&](comm_net_ptr commNet)->double
	{
		double prod_e = 1;
		Edges outs = SortEdges(commNet->GetNodes().at(id)->GetOuts());

		for (auto edge : outs)
		{
			if(p_old[id] >= p_old[edge->v_])break;
			prod_e *= edge->GetLossProcess()->GetMean();
			SIM_LOG_NP(GOD_VIEW, id, p_old[id], "it->first " << edge->v_ << ", prod_e " << prod_e);
		}

		SIM_LOG_NP(GOD_VIEW, id, p_old[id], "a coefficient: prod_e = " << prod_e << ", m_d[id] = " << m_d[id]);
		return m_d[id] * (1 - prod_e);
	};
	;

	auto b = [&](UanAddress u, comm_net_ptr commNet)->double
	{
		if(u == commNet->GetDst())return 0;

		double prod_e = 1;
		double loss_on_e = 0;
		Edges outs = SortEdges(commNet->GetNodes().at(id)->GetOuts());

		for (auto edge : outs)
		{
			if(edge->v_ == u)
			{
				loss_on_e = edge->GetLossProcess()->GetMean();
				break;
			}
			assert(p_old.at(edge->v_) >= p_old.at(u));

			prod_e *= edge->GetLossProcess()->GetMean();
		}

		double p = m_d[id] * (1 - loss_on_e) * prod_e;
		SIM_LOG_NP(GOD_VIEW, id, p_old[id], "output node " << u << ", b coefficient: prod_e = " << prod_e << ", loss on e = "
				<< loss_on_e << ", m_d[id] = " << m_d[id] << ", p = " << p);

		return p;
	};
	;

	double denominator = 1;

	Edges outs = SortEdges(m_commNet->GetNodes().at(id)->GetOuts());
	for (auto edge : outs) {
		if (p_old[id] >= p_old[edge->v_]) break;
		double bb = b(edge->v_, m_commNet);
		denominator += bb / p_old[edge->v_];
		SIM_LOG_NP(GOD_VIEW, id, p_old[id], "sink node " << edge->v_ << ", priority " << p_old[edge->v_] << ", b coefficient " << bb);
	}

	double aa = a(m_commNet);
	double new_p = aa / denominator;
	SIM_LOG_NP(GOD_VIEW, id, new_p, "a coefficient: " << aa << ", old priority: " << p_old[id]);

	SIM_LOG(GOD_VIEW, "======<<<<<<<<<<<< Calculate priority for node " << id);

	return new_p;
}
TdmAccessPlan UniPriorCalc::CalcTdmAccessPlan() {

	//
	// check nodes, which consist in the coalition of at least one another node
	// the not checked nodes should not send any data
	//
	std::map<UanAddress, bool> cs;
	std::function<void(UanAddress)> check;
	check = [this, &check, &cs](UanAddress v)
	{
		if(cs.find(v) != cs.end()) return;
		cs[v] = true;
		SIM_LOG(GOD_VIEW, "check " << v);
		for (auto edge : m_commNet->GetNode(v)->GetOuts())
		{
			if(m_p[edge->v_] > m_p[v])
			{
				check(edge->v_);
			}
		}
	};
	;

	//
	// Calculate coding redundancy rate
	//
	auto c = [this](UanAddress v)->double
	{
		double prod_e = 1;
		Edges outs = SortEdges(m_commNet->GetNodes().at(v)->GetOuts());

		for (auto edge : outs)
		{
			if(m_p[v] >= m_p[edge->v_])break;
			prod_e *= edge->GetLossProcess()->GetMean();
		}

		SIM_LOG(GOD_VIEW, "c[" << v << "] = " << (1 - prod_e));
		return (1 - prod_e);
	};
	;

	//
	// Calculate filtering probability
	//
	auto pf = [this](UanAddress v, UanAddress u)->double
	{
		if(u == m_commNet->GetDst())return 0;

		double prod_e = 1;
		double loss_on_e = 0;
		Edges outs = SortEdges(m_commNet->GetNodes().at(v)->GetOuts());

		for (auto edge : outs)
		{
			if(edge->v_ == u)
			{
				loss_on_e = edge->GetLossProcess()->GetMean();
				break;
			}
			assert(m_p.at(edge->v_) >= m_p.at(u));

			prod_e *= edge->GetLossProcess()->GetMean();
		}

		double p = (1 - loss_on_e) * prod_e;
		SIM_LOG(GOD_VIEW, "pf[" << v << "," << u << "] = " << p);

		return p;
	};
	;

	//
	// sort output edges of the given node in ascending order of the edge sinks
	// and remove those edges, whose sink have smaller or equal priority to the one of the given node
	//
	auto get_special_set = [this](UanAddress v)->Edges
	{
		auto edges = m_commNet->GetNode(v)->GetOuts();

		std ::sort (edges.begin (), edges.end (), [&](Edge_ptr a, Edge_ptr b)
				{
					return m_p.at (a->v_) < m_p.at (b->v_);
				});;

		for(Edges::iterator edge_it = edges.begin(); edge_it != edges.end(); )
		{
			UanAddress u = (*edge_it)->v_;
			if(m_p.at(v) >= m_p.at(u))
			{
				edges.erase(edge_it, edge_it + 1);
			}
			else
			{
				edge_it++;
			}
		}
		for(auto edge : edges)
		{
			UanAddress u = edge->v_;
			SIM_LOG(GOD_VIEW, "edge<" << v << "," << u << ">: p[" << u << "] = " << m_p[u]);
		}

		return edges;
	};
	;

	//
	// calculate the message size to be sent by each node
	//
	typedef std::map<UanAddress, double> amount_data_t;
	std::map<UanAddress, bool> calculated;
	std::function<void(amount_data_t *n, UanAddress v)> calc_n;
	calc_n = [this, &calc_n, &cs, &get_special_set, &pf, &c, &calculated](amount_data_t *n, UanAddress v)
	{
		if(calculated.find(v) != calculated.end())return;

		calculated[v] = true;

		SIM_LOG(GOD_VIEW, "calculate n for sinks of edges of node " << v);

		auto edges = get_special_set(v);
		for(auto edge : edges)
		{
			UanAddress u = edge->v_;
			if(cs.find(u) != cs.end())
			{
				SIM_LOG(GOD_VIEW, "calculate n for sink node " << u);

				if(u == m_commNet->GetDst())
				{
					(*n)[u] = 0;
					continue;
				}
				(*n)[u] += (*n)[v] * pf(v, u) / c(u);
				SIM_LOG(GOD_VIEW, "n[" << u << "] = " << (*n)[u]);
				m_acBuf.push_back(std::bind(calc_n, n, u));
			}
		}
	};
	;

	//
	// DO THE JOB
	//
	check(m_commNet->GetSrc());

	amount_data_t n;
	UanAddress v = m_commNet->GetSrc();
	n[v] = 1 / c(v);
	SIM_LOG(GOD_VIEW, "n[" << v << "] = " << n[v]);
	m_acBuf.push_back(std::bind(calc_n, &n, v));

	while (!m_acBuf.empty()) {
		(*(m_acBuf.begin()))();
		m_acBuf.pop_front();
		SIM_LOG(GOD_VIEW, "There are " << m_acBuf.size () << " actions in buffer to be done");
	}

	//
	// calculate the TDM access plan
	//
	TdmAccessPlan plan;
	for (auto a : n) {
		plan[a.first] = a.second / m_d.at(a.first);
	}

	for (auto p : plan) {
		SIM_LOG(GOD_VIEW, "Node " << p.first << " time = " << p.second);
	}
	double s = 0;
	for (auto p : plan)
		s += p.second;
	;
	SIM_LOG(GOD_VIEW, "s =  " << s << " 1/p(vs) " << 1/m_p[v]);
	for (auto &p : plan)
		p.second /= s;

	for (auto p : plan) {
		SIM_LOG(GOD_VIEW, "Node " << p.first << " share = " << p.second);
	}

	return plan;
}

double UniPriorCalc::GetOptDatarate() {
	UpdatePriorities();

	return m_p.at(m_commNet->GetSrc()).val();
}

double UniPriorCalc::GetSinglePathDatarate() {

//	auto src = std::function<UanAddress()>([this] {for (auto n : m_commNet->GetNodes())if (n->GetNodeType() == SOURCE_NODE_TYPE)return n->GetId();})();
//
//	std::map<UanAddress, std::map<UanAddress, TreeDesc> > m;
//
//	for (auto node : m_commNet->GetNodes()) {
//		if (node->GetNodeType() == DESTINATION_NODE_TYPE) {
//			auto dst = node->GetId();
//			for (auto sender : m_commNet->GetNodes()) {
//				m[sender][dst] = GetTreeDesc(sender->GetId(), dst);
//			}
//		}
//	}
//
//	auto neighbour = [&](UanAddress sender, UanAddress dst) {
//		return m[sender][dst].bestPath.begin()->to;
//	};
//
//	auto is_edge_in_path = [](lps::Edge edge, lps::EPath path) {
//		for(auto e : path)if(e == edge)return true;
//		return false;
//	};
//
//
//
//
//	auto proceed = [&](UanAddress sender) {
//
//		std::vector<UanAddress> bestPath;
//		bestPath.push_back(sender);
//
//		//
//		// find the slowest connection
//		//
//			auto find_slowest_path = [&](std::vector<UanAddress> dsts) {
//				TreeDesc u_desc;
//				UanAddress slowestDst = -1;
//				for(auto dst : dsts)
//				{
//					if(m[sender][dst].maxRate < u_desc.maxRate)
//					{
//						u_desc = m[sender][dst];
//						slowestDst = dst;
//					}
//				}
//				return u_desc;
//			};
//
//			std::vector<UanAddress> dsts;
//			for(auto desc_it : m[sender]) dsts.push_back(desc_it.first);
//
//			do
//			{
//				auto edge = *(find_slowest_path(dsts).bestPath.begin());
//				bestPath.push_back(edge.to);
//				//
//				// spawn the new propagation path if the selected edge does not belong to at least one another DST
//				//
//				dsts.clear();
//				for(auto desc_it : m[sender])
//				{
//					if(!is_edge_in_path(edge, desc_it.second.paths))
//					{
//						dsts.push_back(desc_it.first);
//					}
//				}
//
//
//			}while(!dsts.empty());
//
//		};

	double max_rate = 0;

	return max_rate;
}
UniPriorCalc::graph_ptr UniPriorCalc::ConstructGraph(UanAddress s, UanAddress d) {

	graph_ptr graph = graph_ptr(new lps::Graph(m_commNet->GetNodes().size(), s, d));

	for (auto node : m_commNet->GetNodes()) {
		auto edges = node->GetOuts();
		for (auto edge : edges) {
			graph->AddEdge(node->GetId(), edge->v_, edge->GetLossProcess()->GetMean());
		}
	}

	return graph;
}

TreeDesc UniPriorCalc::GetTreeDesc(UanAddress s, UanAddress d) {

	TreeDesc td;

	auto graph = ConstructGraph(s, d);
	td.paths = graph->GetPaths();

	double max_rate = 0;
	lps::EPath p;

	for (auto path : td.paths) {

		double v = GetPathCost(path);
		if (td.maxRate < v) {
			td.maxRate = v;
			td.bestPath = path;
		}
	}

	SIM_LOG(GOD_VIEW, "Optimal single-path routing data rate " << s << "->" << d << ": " << td.maxRate);

	return td;
}

double UniPriorCalc::GetPathCost(lps::EPath path) {

	auto get_loss_ratio = [this](UanAddress from, UanAddress to)->double
	{
		auto node = m_commNet->GetNode(from);
		auto edges = node->GetOuts();
		for(auto edge : edges)
		{
			if(edge->v_ == to)
			{
				return edge->GetLossProcess()->GetMean();
			}
		}
		assert(0);
	};

	double v = 0;
	for (auto e : path) {

		auto l = get_loss_ratio(e.from, e.to);
		if (eq(l, 1.0)) continue;
		v += 1 / (1 - l) / m_commNet->GetNode(e.from)->GetDatarate();
		if (GOD_VIEW) std::cout << "Edge<" << e.from << "," << e.to << "> : " << m_commNet->GetNode(e.from)->GetDatarate() * (1 - l) << " / ";
	}
	if (GOD_VIEW) std::cout << std::endl;
	return 1 / v;

}
}	//ncr
