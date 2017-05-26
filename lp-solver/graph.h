/*
 * ************************************************************************
 */
#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <utility>
#include <iostream>
#include <map>
#include <assert.h>

#include <list>
#include "lp-solver-header.h"
/*
 * source is the node with index 0 and the destination is the node with index m_numNodes-1
 */

namespace lps {

class Graph {
public:

	Graph(uint16_t numNodes, uint16_t s, uint16_t d);
	void
	AddEdge(uint16_t u, uint16_t v, double l);

	void Evaluate();

	EPaths
	GetPaths();
	Cutsets
	GetAllCutSets();

	Constraints GetConstraints();
	Objectives GetObjectives();
	Bounds GetBounds();
	uint16_t GetNumNodes();

private:

	void
	SearchPaths(Paths &paths, uint16_t, uint16_t, bool[], uint16_t[], uint16_t &);
	void
	SearchCutsets();
	void
	ConstructM();

	std::vector<uint16_t> m_nodeIds;
	uint16_t m_numNodes;
	std::list<uint16_t> *adj;
	uint16_t s;
	uint16_t d;
	std::map<Edge, uint16_t> m_edges;
	std::map<Edge, double> m_l;
	Constraints m_M;
	Cutsets m_cutsets;
};

}

#endif
