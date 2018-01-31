/*
 * ExOrSolver.h
 *
 *  Created on: 23.11.2016
 *      Author: tsokalo
 */

#ifndef EXORSOLVER_H_
#define EXORSOLVER_H_

#include <iostream>
#include <map>
#include <memory>
#include "lp-solver/graph.h"

#include "lp-solver/lp-solver.h"
#include "network/comm-net.h"
#include "utils/log.h"

namespace ncr {

class ExOrSolver {

	typedef std::shared_ptr<CommNet> comm_net_ptr;
	typedef std::shared_ptr<lps::Graph> graph_ptr;

public:
	ExOrSolver(comm_net_ptr commNet) {
//		m_commNet = comm_net_ptr(new CommNet(commNet.get()));
		m_commNet = commNet;

	}
	virtual ~ExOrSolver() {

	}

	void Calc() {
		DoJob();
	}

	/*
	 * returns the channel capacity between the source and the destination
	 *
	 * considers single sending data rate for all senders
	 */
	double GetOptChannelUses() {
		return m_optObjective;
	}

	/*
	 * if the indivisible time slot value approaches zero, the number of optimal TDM access plans approaches infinity
	 *
	 * The function below gives one of them
	 */
	TdmAccessPlan CalcTdmAccessPlan() {

		TdmAccessPlan plan;
		for (auto n : m_commNet->GetNodes())
			plan[n->GetId()] = m_optSolution[n->GetId()];
		return plan;
	}

protected:

	void DoJob() {

		SIM_LOG(EXOR_SOLVER_LOG, "Start job");

		using namespace lps;

		uint16_t numDest = 0;

		//
		// define constraints
		//
		std::map<uint16_t, Constraints> constraints_map;
		auto src = std::function<UanAddress()>([this] {for (auto n : m_commNet->GetNodes())if (n->GetNodeType() == SOURCE_NODE_TYPE)return n->GetId();})();

		auto dsts = m_commNet->GetDstIds();
		// constraints for time variables defined by cuts
		for (auto node : m_commNet->GetNodes()) {
			if (std::find(dsts.begin(), dsts.end(), node->GetId()) != dsts.end()) {
				auto dst = node->GetId();
				std::cout << "Preparing constraints for destination " << dst << std::endl;
				graph_ptr graph = ConstructGraph(src, dst);
				graph->Evaluate();
				auto cutsets = graph->GetAllCutSets();
				auto c = graph->GetConstraints();
				constraints_map[numDest] = c;
				numDest++;


				auto obj = graph->GetObjectives();
				auto bs = graph->GetBounds();
				std::cout << "obj from graph: ";
				for(auto o : obj)
					std::cout << o << " ";
				std::cout << std::endl;
			}
		}
		std::cout << "Number of destinations " << numDest << std::endl;

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
		std::vector<double> c(m_commNet->GetNodes().size(), 1);
		c.insert(c.end(), numDest, 0);
		constraints.push_back(c);

		// additional constraint for data rate variables: all should be equal
		for (uint16_t i = 0; i < numDest; i++)
			for (uint16_t j = i + 1; j < numDest; j++) {
				std::vector<double> c(m_commNet->GetNodes().size() + numDest, 0);
				c.at(m_commNet->GetNodes().size() + i) = 1;
				c.at(m_commNet->GetNodes().size() + j) = -1;
				constraints.push_back(c);
			}
		auto k = constraints.size() - n - 1;

		//
		// define free variable of the constraints
		//
		Bounds cBounds(std::vector<double>(n, 0), std::vector<double>(n, std::numeric_limits<double>::max()));
		// for the sum of all ts
		cBounds.first.insert(cBounds.first.end(), 1, 0.999999);
		cBounds.second.insert(cBounds.second.end(), 1, 1.0000001);
		// for difference of all rates
		cBounds.first.insert(cBounds.first.end(), k, 0);
		cBounds.second.insert(cBounds.second.end(), k, 0);

		//
		// define objectives
		//
		Objectives objectives(m_commNet->GetNodes().size(), 0);
		objectives.insert(objectives.end(), numDest, 1);

		//
		// define bounds
		//
		// for time variables
		Bounds bounds(std::vector<double>(m_commNet->GetNodes().size(), 0), std::vector<double>(m_commNet->GetNodes().size(), 1));
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

		graph_ptr graph = graph_ptr(new lps::Graph(m_commNet->GetNodes().size(), s, d));

		for (auto node : m_commNet->GetNodes()) {
			auto edges = node->GetOuts();
			for (auto edge : edges) {
				graph->AddEdge(node->GetId(), edge->v_, edge->GetLossProcess()->GetMean());
			}
		}

		return graph;
	}

	comm_net_ptr m_commNet;
	double m_optObjective;
	std::map<uint16_t, double> m_optSolution;
};

}

#endif /* EXORSOLVER_H_ */
