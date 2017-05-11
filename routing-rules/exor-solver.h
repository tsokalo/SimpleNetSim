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
		m_commNet = commNet;

		DoJob();
	}
	virtual ~ExOrSolver() {

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
		return m_optSolution;
	}

private:

	void DoJob() {

		SIM_LOG(EXOR_SOLVER_LOG, "Start job");
		graph_ptr graph = ConstructGraph();
		graph->Evaluate();

		auto cutsets = graph->GetAllCutSets();
		std::vector<lps::Solution> solutions;
		std::vector<double> objectiveValues;
		for (uint16_t i = 0; i < cutsets.size(); i++) {

			SIM_LOG(1, "Solving for cut " << i);
			lps::LPSolver solver(graph->GetObjectives(i), graph->GetConstraints(i), graph->GetBounds(i));
			solver.SolveTask();
			solutions.push_back(solver.GetSolution());
			objectiveValues.push_back(solver.GetObjectiveValue());
		}

		auto i = std::distance(objectiveValues.begin(), std::max_element(objectiveValues.begin(), objectiveValues.end()));
		;

		SIM_LOG(1, "Optimal objective: " << objectiveValues.at(i));
		m_optObjective = objectiveValues.at(i);

		for (uint16_t j = 0; j < solutions.at(i).size(); j++) {
			m_optSolution[j] = solutions.at(i).at(j);
		}

		if (1) {
			std::cout << "Optimal solution: ";
			for (auto s : solutions.at(i))
				std::cout << s << " ";
			std::cout << std::endl;
		}
		SIM_LOG(EXOR_SOLVER_LOG, "Job finished successfully");
	}

	graph_ptr ConstructGraph() {

		UanAddress s, d;
		for (auto node : m_commNet->GetNodes()) {
			if (node->GetNodeType() == DESTINATION_NODE_TYPE)
				d = node->GetId();
			if (node->GetNodeType() == SOURCE_NODE_TYPE)
				s = node->GetId();
		}

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
