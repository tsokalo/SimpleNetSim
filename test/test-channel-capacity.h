/*
 * test-channel-capacity.h
 *
 *  Created on: 30.01.2017
 *      Author: tsokalo
 */

#ifndef TEST_TEST_CHANNEL_CAPACITY_H_
#define TEST_TEST_CHANNEL_CAPACITY_H_

#include <fstream>
#include <string.h>
#include <functional>
#include <memory>

#include "header.h"
#include "network/comm-net.h"
#include "utils/utils.h"
#include "routing-rules/exor-solver.h"
#include "test/test.h"
#include "utils/sim-parameters.h"

namespace ncr {

void TestChannelCapacity(std::string folder) {

	auto create_scenario = [](std::shared_ptr<CommNet> &net, SimParameters sp, double e1, double e2, double e3) {
		net = std::shared_ptr<CommNet>(new CommNet(3, sp));
		net->ConnectNodes(0, 1, e1);
		net->ConnectNodes(0, 2, e3);
		net->ConnectNodes(1, 2, e2);
	};

	std::shared_ptr<CommNet> net;
	double e1 = 0.15, e2 = 0.05, e3 = 0.05;
	double step = 0.05, ceiling = 0.9;
	std::ofstream f("data.txt");

	while (e1 < ceiling) {
		e2 = 0.05;
		while (e2 < ceiling) {
			e3 = 0.05;
			while (e3 < ceiling) {

				create_scenario(net, SimParameters(), e1, e2, e3);
				//
				// evaluate simulation
				//
				GodViewRoutingRules godView(net);
				auto orp = godView.GetOptDatarate();
				auto lrp = godView.GetSinglePathDatarate();
				auto alpha = (orp - lrp) / lrp;

				f << e1 << "\t" << e2 << "\t" << e3 << "\t" << alpha << "\t" << std::endl;

				e3 += step;
			}
			e2 += step;
		}
		e1 += step;
	}
	f.close();
}

void TestChannelCapacityDiamond(std::string folder) {

	auto create_scenario = [](std::shared_ptr<CommNet> &net, SimParameters sp, double e1, double e2, double e3) {
		net = std::shared_ptr<CommNet>(new CommNet(4, sp));
		net->ConnectNodes(0, 1, e3);
		net->ConnectNodes(0, 2, 1-e3);
		net->ConnectNodes(1, 2, e1, e2);
		net->ConnectNodes(1, 3, 1-e3);
		net->ConnectNodes(2, 3, e3);
	};

	std::shared_ptr<CommNet> net;
	double e1 = 0.05, e2 = 0.05, e3 = 0.05;
	double step = 0.05, ceiling = 0.95;
	uint32_t m = (ceiling - e1) / step * (ceiling - e2) / step * (0.5 - e3) / step, c = 0;
	std::ofstream f("data.txt");

	while (e1 < ceiling) {
		e2 = 0.05;
		while (e2 < ceiling) {
			e3 = 0.05;
			while (e3 < 0.5) {
				c++;
				PrintProgress(m, c);
				create_scenario(net, SimParameters(), e1, e2, e3);

				//
				// evaluate simulation
				//
				GodViewRoutingRules godView(net);
				auto orp = godView.GetOptDatarate();
				auto lrp = godView.GetSinglePathDatarate();
				auto alpha = (orp - lrp) / lrp;

				f << e1 << "\t" << e2 << "\t" << e3 << "\t" << alpha << "\t" << std::endl;

				e3 += step;

			}
			e2 += step;
		}
		e1 += step;
	}
	f.close();
}

void TestChannelCapacityStack(std::string folder) {

	auto create_scenario = [](std::shared_ptr<CommNet> &net, SimParameters sp, double e1, double e2, uint16_t deepness) {
		uint16_t num_nodes = deepness + 2;
		net = std::shared_ptr<CommNet>(new CommNet(num_nodes, sp));
		for (uint16_t i = 2; i < num_nodes; i++) {
			net->ConnectNodes(i - 2, i - 1, e1, e1);
			net->ConnectNodes(i - 2, i, e2, e2);
		}
		net->ConnectNodes(num_nodes - 2, num_nodes - 1, e1, e1);
	};

	std::shared_ptr<CommNet> net;
	double e1 = 0.05, e2 = 0.05;
	double step = 0.05, ceiling = 0.95;
	uint16_t deepness = 1, max_deepness = 8;
	std::ofstream f("data.txt");

	while (e1 < ceiling) {
		e2 = 0.05;
		while (e2 < ceiling) {

			if (e2 < e1) {
				e2 += step;
				continue;
			}
			deepness = 1;
			while (deepness < max_deepness) {

				create_scenario(net, SimParameters(), e1, e2, deepness);

				//
				// evaluate simulation
				//
				GodViewRoutingRules godView(net);
				auto orp = godView.GetOptDatarate();
				auto lrp = godView.GetSinglePathDatarate();
				auto alpha = (orp - lrp) / lrp;

				f << e1 << "\t" << e2 << "\t" << deepness << "\t" << alpha << "\t" << std::endl;

				deepness += 2;

			}
			e2 += step;
		}
		e1 += step;
	}
	f.close();
}
}

#endif /* TEST_TEST_CHANNEL_CAPACITY_H_ */
