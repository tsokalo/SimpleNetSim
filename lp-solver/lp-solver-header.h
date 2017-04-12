/*
 * ************************************************************************
 */
#ifndef HEADER_H
#define HEADER_H

#include <vector>
#include <utility>
#include <iostream>

#include <list>

namespace lps
{

struct Edge
{
  Edge ()
  {
    from = 0;
    to = 0;
  }
  Edge (uint16_t from, uint16_t to)
  {
    this->from = from;
    this->to = to;
  }

  Edge
  operator= (const Edge & rhs)
  {
    if (this == &rhs) return *this;
    this->from = rhs.from;
    this->to = rhs.to;
    return *this;
  }
  bool
  operator< (const Edge& src) const
  {
    return (this->from * 1000 + this->to < src.from * 1000 + src.to);
  }
  bool
  operator== (const Edge &c)
  {
    return (this->from == c.from) && (this->to == c.to);
  }
  friend std::ostream&
  operator<< (std::ostream& out, const Edge& f)
  {
    return out << "(" << f.from << "," << f.to << ")";
  }

  uint16_t from;
  uint16_t to;
};

typedef std::vector<std::vector<uint16_t> > Paths;
typedef std::vector<Edge> EPath;
typedef std::vector<std::vector<Edge> > EPaths;
typedef std::vector<Edge> Cutset;
typedef std::vector<std::vector<Edge> > Cutsets;
typedef std::vector<double> Solution;

typedef std::vector<double> Objectives;
typedef std::pair<std::vector<double>, std::vector<double> > Bounds;
typedef std::vector<std::vector<double> > Constraints;
}

#endif
