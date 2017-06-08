/*
 * GodViewRoutingRules.h
 *
 *  Created on: 27.10.2016
 *      Author: tsokalo
 */

#ifndef GODVIEWROUTINGRULES_H_
#define GODVIEWROUTINGRULES_H_

#include "network/comm-net.h"
#include "network/edge.h"
#include "lp-solver/graph.h"
#include "utils/comparison.h"
#include "header.h"

#include <stdint.h>
#include <functional>
#include <memory>
#include <deque>
#include <vector>
#include <fstream>

namespace ncr {

class CommNet;

struct TreeDesc {
	TreeDesc() {
		maxRate = -1 * (PRECISION_) * 10;
	}
	TreeDesc operator=(const TreeDesc & rhs) {
		if (this == &rhs) return *this; // calls copy constructor SimpleCircle(*this)
		bestPath.clear();
		bestPath.insert(bestPath.begin(), rhs.bestPath.begin(), rhs.bestPath.end());
		paths.clear();
		paths.insert(paths.begin(), rhs.paths.begin(), rhs.paths.end());
		maxRate = rhs.maxRate;
		return *this; // calls copy constructor
	}
	friend bool operator<(const TreeDesc& l, const TreeDesc& r) {
		return l.maxRate < r.maxRate;
	}
	friend bool operator>(const TreeDesc& l, const TreeDesc& r) {
		return l.maxRate > r.maxRate;
	}
	friend std::ostream&
	operator<<(std::ostream& os, const TreeDesc& l) {
		os << "[";
		for (auto i : l.bestPath) {
			os << i.from << "->" << i.to << ", ";
		}
		os << l.maxRate;
		os << "]";

		return os;
	}
	lps::EPath bestPath;
	lps::EPaths paths;
	double maxRate;
};

class GodViewRoutingRules {

	typedef std::shared_ptr<CommNet> comm_net_ptr;
	typedef std::function<void()> Action;
	typedef std::deque<Action> ActionBuffer;
	typedef std::vector<priority_t> priorities_t;
	typedef std::shared_ptr<lps::Graph> graph_ptr;

public:

	GodViewRoutingRules(comm_net_ptr commNet);
	virtual
	~GodViewRoutingRules();

	/*
	 * returns the optimal average number of channel uses (data packets sent) per one new (not a copy of previously received) packet
	 * received by the destination
	 *
	 * consider single sending data rate for all senders
	 */
	double GetOptChannelUses();

	/*
	 * if the indivisible time slot value approaches zero, the number of optimal TDM access plans approaches infinity
	 *
	 * The function below gives one of them
	 */
	TdmAccessPlan CalcTdmAccessPlan();

	/*
	 * get the upper bound of achievable data rate
	 */
	double GetOptDatarate();
	/*
	 * get highest achievable data rate with single-path routing
	 */
	double GetSinglePathDatarate();

	TreeDesc GetTreeDesc(UanAddress s, UanAddress d);

private:

	void UpdatePriorities();
	void UpdatePriority(int16_t nodeId, priorities_t *p_old, priorities_t *p_new, std::vector<bool> *updated);
	void DeequalizePriorities(priorities_t &p);
	void PrintPriorities();
	Edges SortEdges(Edges edges);
	priority_t CalcPriority(UanAddress id, priorities_t p_old);

	graph_ptr ConstructGraph(UanAddress s, UanAddress d);
	double GetPathCost(lps::EPath path);

//	void CalcTdmRecursive(UanAddress id, TdmAccessPlan &plan, std::map<UanAddress, bool> checkStatus);

	comm_net_ptr m_commNet;
	priorities_t m_p;
	std::map<UanAddress, Datarate> m_d;
	ActionBuffer m_acBuf;
	std::deque<int16_t> m_acBufGroupSizes;
	uint64_t m_t;
	MessType m_msgType;
	LogBank m_logBank;

	std::ofstream m_outFile;

};
}

#endif /* GODVIEWROUTINGRULES_H_ */
