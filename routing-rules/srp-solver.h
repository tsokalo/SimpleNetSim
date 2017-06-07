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
#include "lp-solver/graph.h"
#include "routing-rules/exor-solver.h"

#include "lp-solver/lp-solver.h"
#include "network/comm-net.h"
#include "utils/log.h"

namespace ncr {

class SrpSolver: public ExOrSolver {

	typedef std::shared_ptr<CommNet> comm_net_ptr;
	typedef std::shared_ptr<lps::Graph> graph_ptr;

public:
	SrpSolver(comm_net_ptr commNet) :
			ExOrSolver(commNet) {

	}
	virtual ~SrpSolver() {

	}

	TdmAccessPlan CalcTdmAccessPlan() {

		SrpSolver::DoJob();

		TdmAccessPlan plan;
		for (uint16_t i = 0; i < m_commNet->GetNodes().size(); i++)
			plan[i] = m_optSolution[i];
		return plan;
	}

private:

	void DoJob() {

		SIM_LOG(EXOR_SOLVER_LOG, "Start job");

		using namespace lps;

		uint16_t numDest = 0;

		//
		// define constraints
		//
		std::map<uint16_t, Constraints> constraints_map;
		auto src = std::function<UanAddress()>([this] {for (auto n : m_commNet->GetNodes())if (n->GetNodeType() == SOURCE_NODE_TYPE)return n->GetId();})();

		uint16_t N = 0;
		auto dsts = m_commNet->GetDstIds();
		// constraints for time variables defined by cuts
		for (auto node : m_commNet->GetNodes()) {
			if (std::find(dsts.begin(), dsts.end(), node->GetId()) != dsts.end()) {
				auto dst = node->GetId();
				std::cout << "Constructing the graph using destination " << dst << std::endl;
				graph_ptr graph = ConstructGraph(src, dst);
				if (N < graph->GetNumNodes())
					N = graph->GetNumNodes();
				graph->Evaluate();
				auto cutsets = graph->GetAllCutSets();
				auto c = graph->GetConstraints();
				constraints_map[numDest] = c;
				numDest++;
			}
		}

		// constraints for data rate variables
		Constraints constraints;
		for (auto constraint_d : constraints_map) {
			for (auto constraint : constraint_d.second) {
				constraint.insert(constraint.end(), numDest, 0);
				constraint.at(constraint.size() - numDest + constraint_d.first) = -1;
				constraints.push_back(constraint);
			}
		}
		auto n = constraints.size();

		// additional constraint for time variables: sum is equal one
		std::vector<double> c(N, 1);
		c.insert(c.end(), numDest, 0);
		constraints.push_back(c);

		// additional constraint for data rate variables: all should be equal
		for (uint16_t i = 0; i < numDest; i++)
			for (uint16_t j = i + 1; j < numDest; j++) {
				std::vector<double> c(N + numDest, 0);
				c.at(N + i) = 1;
				c.at(N + j) = -1;
				constraints.push_back(c);
			}
		auto k = constraints.size() - n - 1;

		//
		// define free variable of the constraints
		//
		Bounds cBounds(std::vector<double>(n, 0), std::vector<double>(n, std::numeric_limits<double>::max()));
		// for the sum of all ts
		cBounds.first.insert(cBounds.first.end(), 1, 1);
		cBounds.second.insert(cBounds.second.end(), 1, 1);
		// for difference of all rates
		cBounds.first.insert(cBounds.first.end(), k, 0);
		cBounds.second.insert(cBounds.second.end(), k, 0);

		//
		// define objectives
		//
		Objectives objectives(N, 0);
		objectives.insert(objectives.end(), numDest, 1);

		//
		// define bounds
		//
		// for time variables
		Bounds bounds(std::vector<double>(N, 0), std::vector<double>(N, 1));
		// for data rates
		bounds.first.insert(bounds.first.end(), numDest, 0);
		bounds.second.insert(bounds.second.end(), numDest, std::numeric_limits<double>::max());

		std::cout << "Constraints: " << std::endl;
		for (auto constraint : constraints) {
			for (auto c : constraint)
				std::cout << c << " ";
			std::cout << std::endl;
		}

		std::cout << "Objectives: " << std::endl;
		for (auto o : objectives) {
			std::cout << o << " ";
		}
		std::cout << std::endl;

		std::cout << "Variable bounds: " << std::endl;
		std::cout << "LB: ";
		for (auto b : bounds.first) {
			std::cout << b << " ";
		}
		std::cout << std::endl;
		std::cout << "UB: ";
		for (auto b : bounds.second) {
			std::cout << b << " ";
		}
		std::cout << std::endl;

		std::cout << "Constraints bounds: " << std::endl;
		std::cout << "LB: ";
		for (auto b : cBounds.first) {
			std::cout << b << " ";
		}
		std::cout << std::endl;
		std::cout << "UB: ";
		for (auto b : cBounds.second) {
			std::cout << b << " ";
		}
		std::cout << std::endl;

		LPSolver solver(objectives, constraints, bounds, cBounds);
		solver.SolveTask();
		auto solution = solver.GetSolution();
		m_optObjective = solver.GetObjectiveValue();
		for (uint16_t j = 0; j < solution.size() - 1; j++) {
			m_optSolution[j] = solution.at(j);
		}
		SIM_LOG(EXOR_SOLVER_LOG, "Job finished successfully");
	}

	graph_ptr ConstructGraph(UanAddress s, UanAddress d) {

		auto construct_full_graph = [this](UanAddress s, UanAddress d) {
			graph_ptr graph = graph_ptr(new lps::Graph(m_commNet->GetNodes().size(), s, d));

			for (auto node : m_commNet->GetNodes()) {
				auto edges = node->GetOuts();
				for (auto edge : edges) {
					graph->AddEdge(node->GetId(), edge->v_, edge->GetLossProcess()->GetMean());
				}
			}
			return graph;
		};

		auto get_path_cost = [this](lps::EPath path) {

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
				if (eq(l, 1.0))
				continue;
				v += 1 / (1 - l) / m_commNet->GetNode(e.from)->GetDatarate();
				if (GOD_VIEW)
				std::cout << "Edge<" << e.from << "," << e.to << "> : " << m_commNet->GetNode(e.from)->GetDatarate() * (1 - l) << " / ";
			}
			if (GOD_VIEW)
			std::cout << std::endl;
			return 1 / v;
		};

		auto paths = construct_full_graph(s, d)->GetPaths();

		double max_rate = 0;
		lps::EPath p;

		for (auto path : paths) {

			double v = get_path_cost(path);
			if (max_rate < v) {
				max_rate = v;
				p = path;
			}
		}

		graph_ptr graph = graph_ptr(new lps::Graph(m_commNet->GetNodes().size(), s, d));

		for (auto node : m_commNet->GetNodes()) {
			auto edges = node->GetOuts();
			for (auto edge : edges) {
				for (auto ep : p) {
					if (node->GetId() == ep.from && edge->v_ == ep.to) {
						graph->AddEdge(node->GetId(), edge->v_, edge->GetLossProcess()->GetMean());
						break;
					}
				}
			}
		}

		return graph;
	}
};

}

#endif /* SRPSOLVER_H_ */
