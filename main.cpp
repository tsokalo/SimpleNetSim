/*
 * main.cpp
 *
 *  Created on: Dec 1, 2015
 *      Author: tsokalo
 */
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <vector>
#include <memory>

#include <storage/storage.hpp>
#include <kodo_rlnc/full_vector_codes.hpp>

#include "header.h"
#include "network/comm-net.h"
#include "utils/utils.h"
#include "routing-rules/exor-solver.h"
#include "routing-rules/srp-solver.h"
#include "test/test.h"
#include "utils/sim-parameters.h"


namespace ncr {
void CreateTriangleScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	net = std::shared_ptr<CommNet>(new CommNet(3, sp));
        net->ConnectNodes(0, 1, 0.1);
        net->ConnectNodes(0, 2, 0.606);
        net->ConnectNodes(1, 2, 0.3);
        net->Configure();
        net->PrintNet();
}

void CreateKrishnaScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
        net = std::shared_ptr<CommNet>(new CommNet(4, sp));
    	net->ConnectNodes(0, 1, 0.1);
    	net->ConnectNodes(0, 2, 0.5);
//    	net->ConnectNodes(1, 2, 0.2, 0.3);
    	net->ConnectNodes(1, 3, 0.5);
    	net->ConnectNodes(2, 3, 0.1);
//    	net->SetDestination(1);
    	net->SetDestination(2);
    	net->SetDestination(3);
        net->Configure();
        net->PrintNet();
}


void CreateDiamondScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	net = std::shared_ptr<CommNet>(new CommNet(4, sp));
	net->ConnectNodes(0, 1, 0.1);
	net->ConnectNodes(0, 2, 0.5);
	net->ConnectNodes(1, 2, 0.2, 0.3);
	net->ConnectNodes(1, 3, 0.5);
	net->ConnectNodes(2, 3, 0.1);
	net->SetDestination(1);
	net->SetDestination(2);
	net->SetDestination(3);
	net->Configure();
	net->PrintNet();
}

void CreateStackScenario(std::shared_ptr<CommNet> &net, uint16_t deepness, SimParameters sp) {

	uint16_t num_nodes = deepness + 2;
	net = std::shared_ptr<CommNet>(new CommNet(num_nodes, sp));
	double e1 = 0.1;
	double e2 = 0.5;
	for (uint16_t i = 2; i < num_nodes; i++) {
		net->ConnectNodes(i - 2, i - 1, e1, e1);
		net->ConnectNodes(i - 2, i, e2, e2);
	}
	net->ConnectNodes(num_nodes - 2, num_nodes - 1, e1, e1);
	net->Configure();

	net->PrintNet();
}

void CreateFullMeshScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	uint16_t numNodes = 9;
	net = std::shared_ptr<CommNet>(new CommNet(numNodes, sp));
	for (uint16_t i = 0; i < numNodes; i++) {
		for (uint16_t j = i + 1; j < numNodes; j++) {
			net->ConnectNodes(i, j, 0.1 + 1 / (double) ((numNodes - i) * j), 0.1 + 1 / (double) ((numNodes - i) * j));
		}
	}
	net->Configure();

	net->PrintNet();
}

void CreateBigMeshScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	uint16_t numNodes = 10;
	net = std::shared_ptr<CommNet>(new CommNet(numNodes, sp));

	net->ConnectNodes(0, 1, 0.2);
	net->ConnectNodes(0, 2, 0.2);
	net->ConnectNodes(0, 3, 0.2);

	net->ConnectNodes(1, 6, 0.2);
	net->ConnectNodes(1, 4, 0.2);

	net->ConnectNodes(2, 4, 0.2);
	net->ConnectNodes(2, 5, 0.2);

	net->ConnectNodes(3, 5, 0.2);
	net->ConnectNodes(3, 8, 0.2);

	net->ConnectNodes(4, 6, 0.2);
	net->ConnectNodes(4, 7, 0.2);

	net->ConnectNodes(5, 7, 0.2);
	net->ConnectNodes(5, 8, 0.2);

	net->ConnectNodes(6, 9, 0.2);

	net->ConnectNodes(7, 9, 0.2);

	net->ConnectNodes(8, 9, 0.2);
	net->Configure();

	net->PrintNet();
}

void CreateUmbrellaScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	net = std::shared_ptr<CommNet>(new CommNet(10, sp));
	for (uint16_t i = 1; i < 9; i++) {
		net->ConnectNodes(0, i, 0.7);
		net->ConnectNodes(i, 9, 0.2);
	}
	net->Configure();
	net->PrintNet();
}

}

int main(int argc, char *argv[]) {

	using namespace ncr;

	std::cout << "Program start" << std::endl;

	std::string path = argv[0];
	size_t position = path.rfind("/build");
	std::string subpath = path.substr(0, position + 1);
	if (subpath.empty()) {
		std::cout << "Please, give the full path" << std::endl;
		exit(1);
	}
	std::string folder = subpath + "Results/";

	bool useSns = true;
	ProgMode m;
	if (argc < 2) {
		m = EVAL_MODE;
	}
	else {
		m = ProgMode(atoi(argv[1]));
		if(m == EVAL_MODE && argc > 2)
		{

			std::cout << "Using results folder " << argv[2] << std::endl;
			folder = argv[2];
			useSns = false;
		}
	}

	std::shared_ptr<CommNet> net;

	SimParameters sim_par(subpath + GetSimParamFileName());

	//
	// using default parameters
	//
//	CreateKrishnaScenario(net, sim_par);
//	CreateStackScenario(net, 4, sim_par);
	CreateTriangleScenario(net, sim_par);
//	CreateDiamondScenario(net, sim_par);
//	CreateBigMeshScenario(net, sim_par);
//	CreateUmbrellaScenario(net, sim_par);


	if (m == RUN_MODE) {
		RemoveDirectory(folder);
		CreateDirectory(folder);
		std::cout << folder << std::endl;
		net->EnableLog(folder);
		net->Run(20000);
	}
	else if (m == EVAL_MODE) {
		std::string f = folder + GetLogFileName();
		std::cout << "Using file " << f << std::endl;
		LogBank lb = ReadLogBank(f);

		//
		// get all node IDs except the one specified
		//
		auto get_node_ids = [&](UanAddress id)
		{
			std::vector<UanAddress> ids;
			auto nodes = net->GetNodes();
			for(auto node : nodes)ids.push_back(node->GetId());
			auto it = std::find(ids.begin(), ids.end(), id);
			if(it != ids.end())ids.erase(it, it + 1);
			return ids;
		};
		;

		auto without_dst = get_node_ids(net->GetDst());
		auto without_src = get_node_ids(net->GetSrc());

//		//
//		// plot priorities; for all given nodes on one plot
//		//
//		PlotPriorities(without_dst, lb, subpath, useSns);
//
//		//
//		// plot input filters; for each given node a plot with filters for each input edge
//		//
//		PlotInputFilters(without_src, lb, subpath);
//
//		//
//		// plot loss ratios of output edges; for each given node a plot with ratios for each output edge
//		//
//		PlotLossRatios(without_dst, lb, subpath);
//
//		//
//		// plot coalition sizes; for all given nodes on one plot
//		//
//		PlotCoalitions(without_dst, lb, subpath, f, useSns);
//
//		//
//		// plot coding rates; for all given nodes on one plot
//		//
//		PlotCodingRates(without_dst, lb, subpath, f);

		//
		// plot retransmission requests; for all given nodes on one plot
		//
		PlotRetransmissionRequests(lb, subpath);

		//
		// plot proportion of feedback/network discovery/excessive redundant packets to all sent packets
		//
		GodViewRoutingRules godView(net);
		godView.GetOptChannelUses();
		godView.CalcTdmAccessPlan();
		PlotResourceWaste(lb, subpath, godView.GetOptChannelUses());
//
		//
		// plot sending statistics; for all given nodes on one plot
		//
//		ExOrSolver exOrSolver(net);
		SrpSolver srpSolver(net);
//		PlotSendingStatistics(without_dst, lb, subpath, godView.CalcTdmAccessPlan(), exOrSolver.CalcTdmAccessPlan());

		//
		// plot the maximum achievable data rate, the achieved data rate and the maximum achievable data rate with RP-S
		//
		std::map<UanAddress, Datarate> d;
		for (auto node : net->GetNodes())
			d[node->GetId()] = node->GetDatarate();
		PlotRates(lb, subpath, godView.GetOptDatarate(), godView.GetSinglePathDatarate(), d);

		//
		// analyze stability of the source priority
		//
		PlotSrcPriorStability(lb, subpath, godView.GetOptDatarate(), net->GetSrc());
		//
		// analyze stability of the reception data rate at the destination
		//
		PlotOutputStability(lb, subpath, godView.GetOptChannelUses(), net->GetDst());

	}
	else if (m == TEST_MODE) {
//		TestChannelCapacityStack(folder);
		TestBitSet();
	}

	std::cout << "Finished successfully" << std::endl;
	return 0;
}

