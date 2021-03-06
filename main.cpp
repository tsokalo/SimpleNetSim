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
#include <math.h>
#include <iterator>
#include <string>

//#include <storage/storage.hpp>
//#include <kodo_rlnc/full_vector_codes.hpp>

#include "header.h"
#include "network/comm-net.h"
#include "utils/utils.h"
#include "routing-rules/exor-solver.h"
#include "routing-rules/srp-solver.h"
#include "test/test.h"
#include "utils/sim-parameters.h"
#include "utils/files-dirs.h"
#include "utils/mcs-80211n.h"

namespace ncr {

void ReadScenario(std::shared_ptr<CommNet> &net, SimParameters sp, std::string path, std::string folder_with_traces) {

	SIM_ASSERT_MSG(IsFileCreated(path), "Topology file not found!");
	//
	// read topology file
	//
	std::ifstream infile(path, std::ios_base::in);

	std::string line, temp, src, dst;
	std::map<std::string, std::vector<std::string> > graph_names;

	infile >> temp >> src;
	infile >> temp >> dst;
	infile >> temp;
	std::cout << src << " " << dst << std::endl;

	while (!infile.eof()) {
		std::string sender, receivers_str;
		std::vector<std::string> receivers;
		infile >> sender >> temp >> receivers_str;
		std::string receiver;
		for (auto letter : receivers_str) {
			if (letter == '\n') continue;
			if (letter == ',' && !receiver.empty()) {
				receivers.push_back(receiver);
				receiver.clear();
			} else {
				receiver.push_back(letter);
			}
		}
		if (!receiver.empty()) receivers.push_back(receiver);
		if (!sender.empty()) graph_names[sender] = receivers;
	}
	infile.close();

	for (auto e : graph_names) {
		std::cout << "Sender " << e.first << " has following receivers: ";
		for (auto r : e.second)
			std::cout << r << ", ";
		std::cout << std::endl;
	}

	//
	// make enumerated node list
	//
	std::vector<std::string> names;
	names.push_back(src);
	for (auto e : graph_names) {
		auto name = e.first;
		if (name != src && name != dst) names.push_back(name);
	}
	names.push_back(dst);

	auto get_id = [&](std::string name)
	{
		auto it = std::find(names.begin(), names.end(), name);
		assert(it != names.end());
		return std::distance(names.begin(), it);
	};
	auto get_trace_filename = [&](std::string s, std::string r)
	{
		auto files = FindFile(folder_with_traces, s + "-" + r);
		assert(files.size() == 1);
		return files.at(0);
	};
	auto get_mcs = [](std::string trace_file_name)
	{
		std::string sep = "MCS";
		std::size_t pos = trace_file_name.find(sep);
		std::string str = trace_file_name.substr (pos+sep.size());
		return std::stoi (str,nullptr, 0);
	};

	uint16_t numNodes = graph_names.size() + 1;
	net = std::shared_ptr<CommNet>(new CommNet(numNodes, sp));
	for (auto e : graph_names) {
		auto s_name = e.first;
		auto s_id = get_id(s_name);
		for (auto r_name : e.second) {
			auto r_id = get_id(r_name);
			auto trace_filename = get_trace_filename(s_name, r_name);
			std::cout << "connecting " << s_name << " with id " << s_id << " and " << r_name << " with id " << r_id << " using trace file " << trace_filename
					<< std::endl;
			net->ConnectNodes(s_id, r_id, trace_filename);
			std::cout << "Node " << s_id << " sends with MCS " << get_mcs(trace_filename) << " corresponding to rate " << mcs_rates.at(get_mcs(trace_filename)) << std::endl;
			net->GetNode(s_id)->SetTxDatarate(mcs_rates.at(get_mcs(trace_filename))*1000000);
		}
	}
	net->GetNode(get_id(dst))->SetTxDatarate(mcs_rates.at(13)*1000000);

	net->SetDestination(get_id(dst));
	net->Configure();
	net->PrintNet();
}


void CreateScenarioA(std::shared_ptr<CommNet> &net, SimParameters sp) {
	net = std::shared_ptr<CommNet>(new CommNet(5, sp));
	net->ConnectNodes(0, 1, 0.28);
	net->ConnectNodes(0, 2, 0.01);
	net->ConnectNodes(0, 3, 0.1);
	net->ConnectNodes(1, 3, 0.19);
	net->ConnectNodes(2, 3, 0.11);
	net->ConnectNodes(1, 4, 0.37);

	net->SetDestination(4);
	net->Configure();
	net->PrintNet();
}

void CreateTriangleScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	net = std::shared_ptr<CommNet>(new CommNet(3, sp));
	net->ConnectNodes(0, 1, 0.2);
	net->ConnectNodes(0, 2, 0.6);
	net->ConnectNodes(1, 2, 0.3);
//	net->SetDestination(1);
	net->SetDestination(2);
	net->Configure();
	net->PrintNet();
}

void CreateSquareScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	net = std::shared_ptr<CommNet>(new CommNet(4, sp));
	net->ConnectNodes(0, 1, 0.1);
	net->ConnectNodes(0, 2, 0.2);
	net->ConnectNodes(1, 3, 0.2);
	net->ConnectNodes(2, 3, 0.1);
	net->SetDestination(3);
	net->Configure();
	net->PrintNet();
}

void CreateBigSquareScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	net = std::shared_ptr<CommNet>(new CommNet(9, sp));
	net->ConnectNodes(0, 1, 0.2);
	net->ConnectNodes(0, 3, 0.1);
	net->ConnectNodes(1, 2, 0.1);
	net->ConnectNodes(1, 4, 0.1);
	net->ConnectNodes(2, 5, 0.2);
	net->ConnectNodes(3, 4, 0.2);
	net->ConnectNodes(3, 6, 0.2);
	net->ConnectNodes(4, 5, 0.1);
	net->ConnectNodes(4, 7, 0.2);
	net->ConnectNodes(5, 8, 0.1);
	net->ConnectNodes(6, 7, 0.2);
	net->ConnectNodes(7, 8, 0.1);
	net->SetDestination(8);
	net->Configure();
	net->PrintNet();
}

void CreateAutoSquareScenario(std::shared_ptr<CommNet> &net, SimParameters sp, uint16_t dim) {
	net = std::shared_ptr<CommNet>(new CommNet(dim * dim, sp));

	double e1 = 0.6, e2 = 0.6;
	for (uint16_t i = 0; i < dim; i++) {
		for (uint16_t j = 0; j < dim - 1; j++) {
			net->ConnectNodes(i * dim + j, i * dim + j + 1, ((i + j) % 2 == 0) ? e2 : e1);

		}
	}
	for (uint16_t i = 0; i < dim - 1; i++) {
		for (uint16_t j = 0; j < dim; j++) {
			net->ConnectNodes(i * dim + j, i * dim + j + dim, ((i + j) % 2 == 0) ? e1 : e2);
		}
	}
	net->SetSource(0);
	net->SetDestination(dim * dim - 1);
	net->Configure();
	net->PrintNet();
}

void CreateBetaSquareScenario(std::shared_ptr<CommNet> &net, SimParameters sp, uint16_t dim) {
	net = std::shared_ptr<CommNet>(new CommNet(dim * dim, sp));

	double e1 = 0.1, e2 = 0.6;
	for (uint16_t i = 0; i < dim; i++) {
		for (uint16_t j = 0; j < dim - 1; j++) {
			net->ConnectNodes(i * dim + j, i * dim + j + 1, e1 + e2 * (1 - (float) std::abs(j - i) / (float) dim));

		}
	}
	for (uint16_t i = 0; i < dim - 1; i++) {
		for (uint16_t j = 0; j < dim; j++) {
			net->ConnectNodes(i * dim + j, i * dim + j + dim, e1 + e2 * (1 - (float) std::abs(j - i) / (float) dim));
		}
	}
	net->SetSource(0);
	net->SetDestination(dim * dim - 1);
	net->Configure();
	net->PrintNet();
}

void CreateNoCScenario(std::shared_ptr<CommNet> &net, uint16_t NbrHops, SimParameters sp) {
	uint16_t num_nodes = pow(NbrHops + 1, 2.0);
	net = std::shared_ptr<CommNet>(new CommNet(num_nodes, sp));
	double e1 = 0.1;
	double e2 = 0.2;
	/*	uint16_t ProhSet[]={NbrHops,2*NbrHops+1,3*NbrHops+2};*/
	uint16_t m2 = 2 * NbrHops + 1;
	uint16_t m3 = 3 * NbrHops + 2;
	uint16_t p = num_nodes - NbrHops - 1;
	for (uint16_t i = 0; i < num_nodes - 1; i++) {
		if (i != NbrHops && i != m2 && i != m3) net->ConnectNodes(i, i + 1, e1);
		if (i < p) net->ConnectNodes(i, i + NbrHops + 1, e2);
	}
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
	net->SetSource(0);
//	net->SetDestination(1);
//	net->SetDestination(2);
	net->SetDestination(3);
	net->Configure();
	net->PrintNet();
}

void CreateReducedDiamondScenario(std::shared_ptr<CommNet> &net, SimParameters sp) {
	net = std::shared_ptr<CommNet>(new CommNet(4, sp));
	net->ConnectNodes(0, 1, 0.2);
	net->ConnectNodes(0, 2, 0.4);
	net->ConnectNodes(1, 3, 0);
	net->ConnectNodes(2, 3, 0);
	net->SetSource(0);
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
	} else {
		m = ProgMode(atoi(argv[1]));
		if (m == EVAL_MODE && argc > 2) {

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

	if (m == RUN_MODE || m == EVAL_MODE) {

//	CreateAutoSquareScenario(net, sim_par, 3);
//	CreateBetaSquareScenario(net, sim_par, 3);

//	CreateBigSquareScenario(net, sim_par);
//	CreateSquareScenario(net, sim_par);
//		CreateStackScenario(net, 1, sim_par);
//	CreateTriangleScenario(net, sim_par);
//	CreateNoCScenario(net, 2, sim_par);
//	CreateDiamondScenario(net, sim_par);
//	CreateBigMeshScenario(net, sim_par);
//	CreateUmbrellaScenario(net, sim_par);

//		std::string topology = subpath + "Topologies/Topology3.txt";
//		std::cout << "Looking for topology " << topology << std::endl;
//		std::string folder_with_traces = subpath + "Topologies/traces";
//		ReadScenario(net, sim_par, topology, folder_with_traces);
		CreateScenarioA(net, sim_par);
//	return 0;

	}

	if (m == RUN_MODE) {
		RemoveDirectory(folder);
		CreateDirectory(folder);
		std::cout << folder << std::endl;
		net->EnableLog(folder);
		net->Run(sim_par.simDuration);
	} else if (m == EVAL_MODE) {
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

		auto without_dst = get_node_ids(net->GetDst());
		auto without_src = get_node_ids(net->GetSrc());

		//
		// plot priorities; for all given nodes on one plot
		//
		PlotPriorities(net->GetNodes().size(), net->GetDstIds(), lb, subpath, useSns);

		//
		// plot input filters; for each given node a plot with filters for each input edge
		//
		PlotInputFilters(net->GetNodes().size(), net->GetDstIds(), lb, subpath);

		//
		// plot loss ratios of output edges; for each given node a plot with ratios for each output edge
		//
		PlotLossRatios(without_dst, lb, subpath);

		//
		// plot coalition sizes; for all given nodes on one plot
		//
		PlotCoalitions(net->GetNodes().size(), net->GetDstIds(), lb, subpath, f, useSns);

		//
		// plot coding rates; for all given nodes on one plot
		//
		PlotCodingRates(net->GetNodes().size(), net->GetDstIds(), lb, subpath, f);

		//
		// plot retransmission requests; for all given nodes on one plot
		//
		PlotRetransmissionRequests(lb, subpath, sim_par.warmup, sim_par.simDuration - sim_par.warmdown);

		//
		// plot sending statistics; for all given nodes on one plot
		//
		ExOrSolver exOrSolver(net);
		exOrSolver.Calc();
		SrpSolver srpSolver(net);
		srpSolver.Calc();

		auto plan_orp = exOrSolver.CalcTdmAccessPlan();
		auto plan_srp = srpSolver.CalcTdmAccessPlan();

		//
		// plot proportion of feedback/network discovery/excessive redundant packets to all sent packets
		//
		PlotResourceWaste(lb, subpath, exOrSolver.GetOptChannelUses(), sim_par.warmup, sim_par.simDuration - sim_par.warmdown);

		PlotEfficiencyDetails(lb, subpath, sim_par.warmup, sim_par.simDuration - sim_par.warmdown, sim_par.symbolSize, sim_par.genSize);

		std::cout << "Optimal plan: " << plan_orp << ", SRP plan" << plan_srp << std::endl;
		std::cout << "Optimal d: " << exOrSolver.GetOptChannelUses() * net->GetNodes().at(net->GetSrc())->GetDatarate() << ", SRP d: "
				<< srpSolver.GetOptChannelUses() * net->GetNodes().at(net->GetSrc())->GetDatarate() << std::endl;
		PlotSendingStatistics(lb, subpath, plan_orp, sim_par.warmup, sim_par.simDuration - sim_par.warmdown);

		//
		// plot the maximum achievable data rate, the achieved data rate and the maximum achievable data rate with RP-S
		//
		std::map<UanAddress, Datarate> d;
		for (auto node : net->GetNodes())
			d[node->GetId()] = node->GetTxDatarate();
		PlotRates(lb, subpath, exOrSolver.GetOptChannelUses() * net->GetNodes().at(net->GetSrc())->GetDatarate(),
				srpSolver.GetOptChannelUses() * net->GetNodes().at(net->GetSrc())->GetDatarate(), d, sim_par.warmup, sim_par.simDuration - sim_par.warmdown,
				sim_par.simDuration, sim_par.GetInLine(), sim_par.genSize);
		PlotRanks(lb, subpath, sim_par.warmup, sim_par.simDuration - sim_par.warmdown);

		PlotRatesPerDst(lb, subpath, net->GetDstIds(), d, sim_par.warmup, sim_par.simDuration - sim_par.warmdown);

		CreateArqInfoCvs(lb, subpath);
		//
		// analyze stability of the source priority
		//
		PlotSrcPriorStability(lb, subpath, exOrSolver.GetOptChannelUses(), net->GetSrc());
		//
		// analyze stability of the reception data rate at the destination
		//
		PlotOutputStability(lb, subpath, exOrSolver.GetOptChannelUses(), net->GetDst());

	} else if (m == TEST_MODE) {

		std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
		std::cout << "TEST MODE" << std::endl;
		std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;

//		TestRedundancy();
//		TestFeedbackAccuracy2();
//		std::string path = "/home/tsokalo/Dokumente/4_Publications/ArqNcFeedback/projects/sim/res1000_new_last.cvs";
//		EvaluteFeedbackAccuracy(path);
//		TestCcackSimple();
//		for (uint16_t i = 0; i < 20; i++)
//			TestCcackSimpleMonteCarlo();
//		TestCcack2Relay();
		TestOutOfOrder();
	}

	std::cout << std::endl << "Finished successfully" << std::endl;
	return 0;
}


