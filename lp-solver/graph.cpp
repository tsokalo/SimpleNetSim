#include <vector>
#include <iostream>
#include <vector>
#include <iostream>
#include <utility>
#include <map>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cctype>
#include <functional>

#include "graph.h"
#include "utils/log.h"

namespace lps {

Graph::Graph(uint16_t numNodes, uint16_t s, uint16_t d) {

	this->m_numNodes = numNodes;
	this->s = s;
	this->d = d;
	adj = new std::list<uint16_t>[m_numNodes];

	SIM_LOG(GRAPH_LOG, "Create graph. Nodes: " << numNodes << ", SRC: " << s << ", DST: " << d);
}

void Graph::AddEdge(uint16_t u, uint16_t v, double l) {

	adj[u].push_back(v);
	m_edges[Edge(u, v)] = 1;
	m_l[Edge(u, v)] = l;
	SIM_LOG(GRAPH_LOG, "Set " << Edge (u, v) << ": " << m_l[Edge (u, v)]);

	if(std::find(m_nodeIds.begin(), m_nodeIds.end(), v) == m_nodeIds.end())m_nodeIds.push_back(v);
	if(std::find(m_nodeIds.begin(), m_nodeIds.end(), u) == m_nodeIds.end())m_nodeIds.push_back(u);
}
void Graph::Evaluate() {

//	assert(m_numNodes == m_nodeIds.size());
	SearchCutsets();
	ConstructM();
}
EPaths Graph::GetPaths() {

	// Mark all the vertices as not visited
	bool *visited = new bool[m_numNodes];

	// Create an array to store paths
	uint16_t *path = new uint16_t[m_numNodes];
	uint16_t path_index = 0; // Initialize path[] as empty

	// Initialize all vertices as not visited
	for (uint16_t i = 0; i < m_numNodes; i++)
		visited[i] = false;

	Paths paths;
	// Call the recursive helper function to pruint16_t all paths
	SearchPaths(paths, s, d, visited, path, path_index);

	EPaths e_paths(paths.size());
	for (uint16_t j = 0; j < paths.size(); j++) {
		auto path = paths.at(j);
		for (uint16_t i = 0; i < path.size() - 1; i++) {
			e_paths.at(j).push_back(Edge(path.at(i), path.at(i + 1)));
		}
	};;

	return e_paths;
}

// A recursive function to print all paths from 'u' to 'd'.
// visited[] keeps track of vertices in current path.
// path[] stores actual vertices and path_index is current
// index in path[]
void Graph::SearchPaths(Paths &paths, uint16_t u, uint16_t d, bool visited[], uint16_t path[], uint16_t &path_index) {

	// Mark the current node and store it in path[]
	visited[u] = true;
	path[path_index] = u;
	path_index++;

	// If current vertex is same as destination, then print
	// current path[]
	if (u == d) {
		paths.resize(paths.size() + 1);
		auto p = paths.end() - 1;
		for (uint16_t i = 0; i < path_index; i++) {
			p->push_back(path[i]);

			if (GRAPH_LOG)
				std::cout << path[i] << " ";
		}
		if (GRAPH_LOG)
			std::cout << std::endl;
	} else // If current vertex is not destination
	{
		// Recur for all the vertices adjacent to current vertex
		std::list<uint16_t>::iterator i;
		for (i = adj[u].begin(); i != adj[u].end(); ++i)
			if (!visited[*i])
				SearchPaths(paths, *i, d, visited, path, path_index);
	}

	// Remove current vertex from path[] and mark it as unvisited
	path_index--;
	visited[u] = false;
}
void Graph::SearchCutsets() {
	auto paths = GetPaths();
	m_cutsets.clear();

	if (GRAPH_LOG) {
		for (auto path : paths) {
			std::cout << "Path -> ";
			for (auto e : path)
				std::cout << e;
			std::cout << std::endl;
		};;
	}

	auto get_left_side = [this]()
	{
		typedef std::vector<uint16_t> id_l;
		std::vector<id_l> combs_l;

		std::function<void(id_l, id_l)> show;
		show = [&show, &combs_l](id_l ids, id_l v)
		{
			std::cout << "Show: ";
			for(auto c : ids)std::cout << c << " ";
			std::cout << std::endl;
			for (uint16_t i = 0; i < ids.size(); i++) {

				v.push_back(ids.at(i));
				combs_l.push_back(v);
				typedef std::function<id_l(id_l, uint16_t)> l_f;
				if(ids.size() > i + 1)show(l_f([](id_l is, uint16_t j) {is.erase(is.begin(), is.begin() + j + 1);return is;})(ids, i),v);
				v.pop_back();
			}
		};

		//
		// form left side of the cut (including the source)
		//
			std::vector<uint16_t> ids;
			for(auto id : m_nodeIds)if(id !=s && id != d)ids.push_back(id);

			show(ids, std::vector<uint16_t>());
			for(auto &comb : combs_l)comb.push_back(s);

			combs_l.push_back(std::vector<uint16_t>(1,s));

			return combs_l;
		};

	auto get_right_side = [this](std::vector<std::vector<uint16_t> > combs_l)
	{
		std::vector<std::vector<uint16_t> > combs_r;
		//
		// form right side of the cut (including the destination)
		//
			std::vector<uint16_t> ids;
			for(auto id : m_nodeIds)if(id !=s)ids.push_back(id);
			for(auto comb : combs_l)
			{
				std::vector<uint16_t> v;
				for (auto i : ids)
				{
					if(std::find(comb.begin(), comb.end(), i) == comb.end())v.push_back(i);
				}
				combs_r.push_back(v);
			}
			return combs_r;
		};

	auto combs_l = get_left_side();

	for (auto comb : combs_l) {
		for (auto c : comb)
			std::cout << c << " ";
		std::cout << std::endl;
	}

	auto combs_r = get_right_side(combs_l);

	auto cl_it = combs_l.begin();
	auto cr_it = combs_r.begin();

	while (cl_it != combs_l.end()) {

		Cutset cutset;
		for (auto c1 : *cl_it)
			for (auto c2 : *cr_it)
				if (m_edges[Edge(c1, c2)] == 1)
					cutset.push_back(Edge(c1, c2));

		m_cutsets.push_back(cutset);
		cl_it++;
		cr_it++;
	}

	if (GRAPH_LOG) {
		for (auto cutset : m_cutsets) {
			std::cout << "Cutset: ";
			for (auto e : cutset)
				std::cout << e;
			std::cout << std::endl;
		};;
	}
}
Cutsets Graph::GetAllCutSets() {
	return m_cutsets;
}
void Graph::ConstructM() {

	m_M.resize(m_cutsets.size(), std::vector<double>(m_numNodes, 0));

	for (uint16_t i = 0; i < m_cutsets.size(); i++) {
		auto cutset = m_cutsets.at(i);

		for (auto e1 : cutset) {
			double l = 1;
			for (auto e2 : cutset) {
				if (e1.from == e2.from)
					l *= m_l[e2];
				SIM_LOG(GRAPH_LOG, e2 << ": " << m_l[e2]);
			}
			m_M.at(i).at(e1.from) = (1 - l);
		};;
	};;

	for (uint16_t j = 0; j < m_M.size(); j++) {
		for (uint16_t k = 0; k < m_M.at(j).size(); k++) {
			SIM_LOG(GRAPH_LOG, "m_M[" << j << "][" << k << "]: " << m_M[j][k]);
		}
	}
}

Constraints Graph::GetConstraints() {

	return m_M;
}
Objectives Graph::GetObjectives() {
	auto obj = Objectives(m_numNodes, 0);
	return obj;
}
Bounds Graph::GetBounds() {

	auto bounds = Bounds(std::vector<double>(m_M.begin()->size(), 0), std::vector<double>(m_M.begin()->size(), 1));

	if (GRAPH_LOG) {

		std::cout << "Lower bounds: " << std::endl;
		for (auto l : bounds.first)
			std::cout << l << " ";
		std::cout << std::endl;

		std::cout << "Upper bounds: " << std::endl;
		for (auto u : bounds.second)
			std::cout << u << " ";
		std::cout << std::endl;
	}
	return bounds;
}

uint16_t Graph::GetNumNodes()
{
	return m_numNodes;
}

}
