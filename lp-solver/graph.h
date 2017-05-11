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

namespace lps
{

class Graph
{
public:

  Graph (uint16_t numNodes, uint16_t s, uint16_t d);
  void
  AddEdge (uint16_t u, uint16_t v, double l);

  void Evaluate();

  EPaths
  GetPaths ();
  Cutsets
  GetAllCutSets ();
  Constraints
  GetConstraints (uint16_t i);
  Objectives
  GetObjectives(uint16_t i)
  {
    assert(i < m_M.size());
    std::cout << "Objectives: ";
    for(auto o : m_M.at(i))std::cout << o << " ";
    std::cout << std::endl;
    return m_M.at(i);
//    return std::vector<double>(m_numNodes, 0);
  }
  Bounds
  GetBounds(uint16_t i);

private:

  void
  SearchPaths (Paths &paths, uint16_t, uint16_t, bool[], uint16_t[], uint16_t &);
  void
  SearchCutsets();
  void
  ConstructM ();


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
