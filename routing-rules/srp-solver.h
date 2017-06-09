/*
 * SrpSolver.h
 *
 *  Created on: 23.11.2016
 *      Author: tsokalo
 */

#ifndef SRPSOLVER_H_
#define SRPSOLVER_H_

#include <iostream>
#include <map>
#include <memory>
#include <algorithm>

#include "lp-solver/graph.h"
#include "lp-solver/lp-solver-header.h"
#include "routing-rules/exor-solver.h"
#include "routing-rules/god-view-routing-rules.h"

#include "lp-solver/lp-solver.h"
#include "network/comm-net.h"
#include "utils/log.h"
#include "utils/comparison.h"

namespace ncr {

/*
 * This impelementation uses the algorithm from:
 * Hoang Lan Nguyen, Uyen Trang Nguyen,
 * "Algorithms for Bandwidth Efficient Multicast Routing in Multi-channel Multi-radio Wireless Mesh Networks",
 * IEEE WCNC 2011 - Network Algorithms
 */

class SrpSolver: public ExOrSolver {

	typedef std::shared_ptr<CommNet> comm_net_ptr;
	typedef std::shared_ptr<lps::Graph> graph_ptr;

	struct node_set: public std::vector<UanAddress> {
		void add(UanAddress v) {
			if (std::find(this->begin(), this->end(), v) == this->end()) this->push_back(v);
		}

		void add(lps::EPath p) {
			for (auto e : p) {
				add(e.from);
				add(e.to);
			}
		}

		friend std::ostream&
		operator<<(std::ostream& os, const node_set& l) {
			os << "[";
			for (auto i : l)
				os << i << ", ";
			os << "]";
			return os;
		}
		node_set&
		operator=(const std::vector<UanAddress>& other) // copy assignment
				{
			this->clear();
			for (auto v : other)
				this->push_back(v);
			return *this;
		}
	};
	struct edge_set: public std::vector<lps::Edge> {
		void add(lps::Edge e) {
			for (auto e_ : *this)
				if (e_ == e) return;
			this->push_back(e);
		}
		void add(lps::EPath p) {
			for (auto e : p)
				add(e);
		}
		friend std::ostream&
		operator<<(std::ostream& os, const edge_set& l) {
			os << "[";
			for (auto i : l) {
				os << i.from << "->" << i.to << " | ";
			}
			os << "]";
			return os;
		}
	};
	struct weight_set: public std::map<UanAddress, std::map<UanAddress, double> > {
		friend std::ostream&
		operator<<(std::ostream& os, const weight_set& l) {
			os << "[";
			for (auto i : l) {
				for (auto j : i.second) {
					os << i.first << "->" << j.first << ":" << j.second << " | ";
				}
			}
			os << "]";
			return os;
		}
	};

public:
	SrpSolver(comm_net_ptr commNet) :
			ExOrSolver(commNet) {

	}
	virtual ~SrpSolver() {

	}

	void Calc() {
		SrpSolver::DoJob();
	}

	TdmAccessPlan CalcTdmAccessPlan() {

		TdmAccessPlan plan;
		for(auto n : m_commNet->GetNodes())
			plan[n->GetId()] = m_optSolution[n->GetId()];
		return plan;
	}

private:

	void DoJob() {

		SIM_LOG(EXOR_SOLVER_LOG, "Start job");

		using namespace lps;

		ConstructGraph();
		std::cout << "Initial weights: " << m_w << std::endl;
		m_dst = m_commNet->GetDstIds();
		std::cout << "Initial DSTs: " << m_dst << std::endl;
		//
		// resultant graph will consist of node vt and edges et
		//
		node_set vt;
		edge_set et;
		for (auto node : m_commNet->GetNodes()) {
			if (node->GetNodeType() == SOURCE_NODE_TYPE) vt.add(node->GetId());
		}
		assert(vt.size() == 1);


		while (!m_dst.empty())		//-----> start main cycle
		{
			//
			// find shortest path between vt and m_dst
			//
			TreeDesc solution;
			UanAddress dst;
			GodViewRoutingRules gvrr(m_commNet);
			for (auto v : vt) {
				for (auto d : m_dst) {
					auto solution_ = gvrr.GetTreeDesc(v, d);
					std::cout << "Solution between " << v << " and " << d << ": " << solution_ << std::endl;
					if (solution_ > solution) {
						solution = solution_;
						dst = d;
					}
				}
			}

			std::cout << "Found solution: " << solution << std::endl;
			//
			// save the intermediate result
			//
			vt.add(solution.bestPath);
			et.add(solution.bestPath);

			m_dst.erase(std::remove(m_dst.begin(), m_dst.end(), dst), m_dst.end());
			//
			// update weights considering the broadcast channel
			//
			for (auto e : solution.bestPath) {
				//
				// for all sinks of e.from
				//
				for (auto &r : m_w[e.from]) {
					auto sink = r.first;
					if (sink == e.to) continue;
					if(std::find(vt.begin(), vt.end(), sink) != vt.end())continue;
					r.second = (r.second > m_w[e.from][e.to]) ? r.second - m_w[e.from][e.to] : PRECISION_;
					std::cout << "Change the loss ratio on (" << e.from << "," << sink << ") (old) "
							<< m_commNet->GetNode(e.from)->GetEdge(e.from, sink)->GetLossProcess()->GetMean() << " get " << 1 - 1 / r.second << std::endl;
					m_commNet->GetNode(e.from)->GetEdge(e.from, sink)->GetLossProcess()->SetMean(1 - 1 / r.second);
				}
			}
			std::cout << "Updated weights " << m_w << std::endl;

		}			//-----> finish main cycle

		std::cout << "Using the set of edges " << et << std::endl;
		//
		// do final evaluation
		//
		m_optObjective = 0;
		m_optSolution.clear();
		for (auto e : et) {
			m_optObjective += m_w[e.from][e.to];
			m_optSolution[e.from] += m_w[e.from][e.to];
		}

		for (auto &os : m_optSolution)
			os.second /= m_optObjective;
		m_optObjective = 1 / m_optObjective * m_commNet->GetDstIds().size();

		SIM_LOG(EXOR_SOLVER_LOG, "Job finished successfully");
	}

	void ConstructGraph() {

		for (auto node : m_commNet->GetNodes()) {
			auto edges = node->GetOuts();
			auto s = node->GetId();
			for (auto edge : edges) {
				auto r = edge->v_;
				auto w = edge->GetLossProcess()->GetMean();
				assert(!eq(w, 1));
				m_w[s][r] = 1 / (1 - w);
			}
		}
	}

	weight_set m_w;
	node_set m_dst;
};

}

#endif /* SRPSOLVER_H_ */
