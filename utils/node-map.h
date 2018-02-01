/*
 * node-map.h
 *
 *  Created on: 01.11.2016
 *      Author: tsokalo
 */

#ifndef NODEMAP_H_
#define NODEMAP_H_

#include <iostream>

#include "header.h"

namespace ncr {

typedef std::pair<UanAddress, priority_t> NodeDesc;
typedef std::deque<NodeDesc> node_map_base_t;

struct node_map_t {

	typedef std::deque<NodeDesc> node_map_base_t;
	typedef node_map_base_t::iterator node_map_it;

	//
	// adds the new node ordered in the map by the priority value (descending order)
	//
	void add(UanAddress addr, priority_t p) {

		auto it_o = find(addr);
		// if already have
		if (it_o != v.end()) {
			// check if the new priority value changes the sequence
			bool b = false, e = false;

			if (it_o == v.begin()) {
				b = true;
			} else if ((it_o - 1)->second >= p) {
				b = true;
			}

			if (it_o == v.end()) {
				e = true;
			} else if (p >= (it_o + 1)->second) {
				e = true;
			}

			if (b && e) {
				// just update the priority
				it_o->second = p;
				return;
			} else {
				// erase the entry and find the new position
				v.erase(it_o, it_o + 1);
			}
		}

		auto s = v.size();
		for (node_map_it it = v.begin(); it != v.end(); it++) {
			if (p >= it->second) {
				v.emplace(it, addr, p);
				break;
			}
		}
		if (s == v.size()) v.emplace_back(addr, p);
	}
	void add(NodeDesc d) {
		this->add(d.first, d.second);
	}
	void remove(UanAddress a) {
		for (node_map_it it = v.begin(); it != v.end(); it++) {
			if (it->first == a) {
				v.erase(it);
				break;
			}
		}
	}
	priority_t at(UanAddress a) {
		for (node_map_it it = v.begin(); it != v.end(); it++) {
			if (it->first == a) {
				return it->second;
			}
		}
		assert(0);
	}
	void resize(uint16_t s) {
		if (s < v.size()) v.resize(s);
	}
	void remove(NodeDesc d) {
		this->remove(d.first);
	}
	void clear() {
		v.clear();
	}
	uint16_t size() {
		return v.size();
	}
	node_map_it begin() {
		return v.begin();
	}
	node_map_it end() {
		return v.end();
	}
	bool empty() {
		return v.empty();
	}

	friend std::ostream&
	operator<<(std::ostream& os, node_map_t& l) {
		os << "<";
		for (node_map_it it = l.begin(); it != l.end(); it++) {
			os << "[" << it->first << "," << it->second << "]";
		}
		os << ">";
		return os;
	}
	node_map_it find(UanAddress a) {
		for (node_map_it it = v.begin(); it != v.end(); it++) {
			if (it->first == a) {
				return it;
			}
		}
		return v.end();
	}
	bool is_in(UanAddress a) {
		for (node_map_it it = v.begin(); it != v.end(); it++) {
			if (it->first == a) {
				return true;
			}
		}
		return false;
	}

private:

	node_map_base_t v;

};
}	//ncr

#endif /* NODEMAP_H_ */
