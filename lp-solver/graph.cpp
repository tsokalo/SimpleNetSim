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

Graph::Graph(uint16_t V) {

	this->V = V;
	this->s = 0;
	this->d = V - 1;
	adj = new std::list<uint16_t>[V];
}

void Graph::AddEdge(uint16_t u, uint16_t v, double l) {

	adj[u].push_back(v);
	m_edges[Edge(u, v)] = 1;
	m_l[Edge(u, v)] = l;
	SIM_LOG(GRAPH_LOG, "Set " << Edge (u, v) << ": " << m_l[Edge (u, v)] );
}
void Graph::Evaluate() {

	SearchCutsets();
	ConstructM();
}
EPaths Graph::GetPaths() {

	// Mark all the vertices as not visited
	bool *visited = new bool[V];

	// Create an array to store paths
	uint16_t *path = new uint16_t[V];
	uint16_t path_index = 0; // Initialize path[] as empty

	// Initialize all vertices as not visited
	for (uint16_t i = 0; i < V; i++)
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

			if (GRAPH_LOG) std::cout << path[i] << " ";
		}
		if (GRAPH_LOG) std::cout << std::endl;
	}
	else // If current vertex is not destination
	{
		// Recur for all the vertices adjacent to current vertex
		std::list<uint16_t>::iterator i;
		for (i = adj[u].begin(); i != adj[u].end(); ++i)
			if (!visited[*i]) SearchPaths(paths, *i, d, visited, path, path_index);
	}

	// Remove current vertex from path[] and mark it as unvisited
	path_index--;
	visited[u] = false;
}
void Graph::SearchCutsets() {
	auto paths = GetPaths();
	m_cutsets.clear();

	if (GRAPH_LOG) {
		for(auto path : paths)
		{
			std::cout << "Path -> ";
			for(auto e : path)std::cout << e;
			std::cout << std::endl;
		};;
	}

	auto get_left_side = [](uint16_t V)
	{
		std::vector<std::vector<uint16_t> > combs_l;

		std::function<void(uint16_t, uint16_t, std::vector<uint16_t>)> show;
		show = [&show, &combs_l](uint16_t c, uint16_t n, std::vector<uint16_t> v)
		{
			for (uint16_t i = c; i < n; i++) {

				v.push_back(i);
				combs_l.push_back(v);
				show(i + 1,n,v);
				v.pop_back();
			}
		};;

		//
		// form left side of the cut (including the source)
		//
		show(1, V - 1, std::vector<uint16_t>());
		for(auto &comb : combs_l)comb.push_back(0);

		combs_l.push_back(std::vector<uint16_t>(1,0));

		return combs_l;
	};

	auto get_right_side = [](uint16_t V, std::vector<std::vector<uint16_t> > combs_l)
	{
		std::vector<std::vector<uint16_t> > combs_r;
		//
		// form right side of the cut (including the destination)
		//
		for(auto comb : combs_l)
		{
			std::vector<uint16_t> v;
			for (uint16_t i = 1; i < V; i++)
			{
				if(std::find(comb.begin(), comb.end(), i) == comb.end())v.push_back(i);
			}
			comb.push_back(V - 1);
			combs_r.push_back(v);
		}
		return combs_r;
	};

	auto combs_l = get_left_side(V);
	auto combs_r = get_right_side(V, combs_l);

	auto cl_it = combs_l.begin();
	auto cr_it = combs_r.begin();

	while (cl_it != combs_l.end()) {

		Cutset cutset;
		for (auto c1 : *cl_it)for(auto c2 : *cr_it)if(m_edges[Edge(c1, c2)] == 1)cutset.push_back(Edge(c1, c2));;
		m_cutsets.push_back(cutset);
		cl_it++;
		cr_it++;
	}

//	for (uint16_t k = 1; k < V; k++) {
//		Cutset cutset;
//		std::vector<uint16_t> cut_l, cut_r;
//
//		//
//		// form two disjoint cuts
//		//
//		for (uint16_t i = s; i < k; i++)
//			cut_l.push_back(i);
//		for (uint16_t i = k; i <= d; i++)
//			cut_r.push_back(i);
//
//		//
//		// collect all existing edges between these cuts
//		//
//		for (auto c1 : cut_l)for(auto c2 : cut_r)if(m_edges[Edge(c1, c2)] == 1)cutset.push_back(Edge(c1, c2));;
//		m_cutsets.push_back(cutset);
//	}

	if (GRAPH_LOG) {
		for (auto cutset : m_cutsets)
		{
			std::cout << "Cutset: ";
			for(auto e : cutset) std::cout << e;
			std::cout << std::endl;
		};;
	}
}
Cutsets Graph::GetAllCutSets() {
	return m_cutsets;
}
void Graph::ConstructM() {

	m_M.resize(m_cutsets.size(), std::vector<double>(V, 0));

	for (uint16_t i = 0; i < m_cutsets.size(); i++) {
		auto cutset = m_cutsets.at(i);;

		for (auto e1 : cutset)
		{
			double l = 1;
			for(auto e2 : cutset)
			{
				if(e1.from == e2.from)l *= m_l[e2];
				SIM_LOG(GRAPH_LOG, e2 << ": " << m_l[e2] );
			}
			m_M.at(i).at(e1.from) = 1 - l;
		};;
	};;

	for (uint16_t j = 0; j < m_M.size(); j++) {
		for (uint16_t k = 0; k < m_M.at(j).size(); k++) {
			SIM_LOG(GRAPH_LOG, "m_M[" << j << "][" << k << "]: " << m_M[j][k] );
		}
	}
}
Constraints Graph::GetConstraints(uint16_t i) {

	assert(i < m_M.size());
	Constraints M(m_M.size());

	auto m_it = M.begin(), mm_it = m_M.begin();
	while (mm_it != m_M.end()) {
		m_it->insert(m_it->begin(), mm_it->begin(), mm_it->end());
		m_it++;
		mm_it++;
	}

	SIM_LOG(GRAPH_LOG, "Constraints: " );
	if (GRAPH_LOG) {
		for (auto c : M)
		{
			std::cout << "Cut: ";
			for(auto o : c)std::cout << o << " ";
			std::cout << std::endl;
		};;
	}

	for (uint16_t j = 0; j < M.size(); j++)
		for (uint16_t k = 0; k < M.at(j).size(); k++) {
			if (j == i) continue;
			M.at(j).at(k) -= M.at(i).at(k);
		}

	M.erase(M.begin() + i, M.begin() + i + 1);

	if (GRAPH_LOG) {
		std::cout << "Constraints: " << std::endl;

		for (auto c : M)
		{
			std::cout << "Cut: ";
			for(auto o : c)std::cout << o << " ";
			std::cout << std::endl;
		};;
	}
	return M;
}
Bounds Graph::GetBounds(uint16_t i) {
	assert(i < m_M.size());

	auto bounds = Bounds (std::vector<double> (m_M.begin ()->size (), 0), std::vector<double> (m_M.begin ()->size (), 1));
	if (GRAPH_LOG) {

		std::cout << "Lower bounds: " << std::endl;
		for(auto l : bounds.first)std::cout << l << " ";
		std::cout << std::endl;

		std::cout << "Upper bounds: " << std::endl;
		for(auto u : bounds.second)std::cout << u << " ";
		std::cout << std::endl;
	}
	return Bounds(std::vector<double>(m_M.begin()->size(), 0), std::vector<double>(m_M.begin()->size(), 1));
}

}
