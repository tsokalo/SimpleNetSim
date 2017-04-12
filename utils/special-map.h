/*
 * special-map.h
 *
 *  Created on: 01.11.2016
 *      Author: tsokalo
 */

#ifndef SPECIALMAP_H_
#define SPECIALMAP_H_

#include <unordered_map>
#include <vector>

#include "header.h"
#include "ssn.h"
#include "ccack/ccack.h"

namespace ncr {

template<class Key, class T, class S>
struct special_map: public std::unordered_map<Key, T> {

	typedef typename std::unordered_map<Key, T>::iterator map_it;

	T& operator[](Key k) {
		auto it = std::find(m_keys.begin(), m_keys.end(), k);
		if (it == m_keys.end()) {
			auto it_k = m_keys.begin();
			for (; it_k != m_keys.end(); it_k++) {
				if (S(k) < S(*it_k)) {
					m_keys.insert(it_k, k);
					break;
				}
			}
			if (it_k == m_keys.end()) m_keys.push_back(k);
		}
		return this->std::unordered_map<Key, T>::operator[](k);
	}

	map_it begin_orig_order() {

		if (m_keys.empty()) return this->end();

		auto it = this->find(m_keys.at(0));
		;
		assert(it != this->end());
		return it;
	}

	map_it last_orig_order() {

		if (m_keys.empty()) return this->end();

		return this->find(m_keys.at(m_keys.size() - 1));
	}
	map_it next_orig_order(map_it it) {
		auto k = it->first;
		auto k_it = std::find(m_keys.begin(), m_keys.end(), k);
		assert(k_it != m_keys.end());
		k_it++;
		if (k_it == m_keys.end()) return this->end();
		return this->find(*k_it);
	}
	void erase(Key k) {

		auto it = std::find(m_keys.begin(), m_keys.end(), k);
		if (it != m_keys.end()) {
			m_keys.erase(it, it + 1);
			this->std::unordered_map<Key, T>::erase(k);
		}
	}

	bool is_in_tail(uint16_t tail_size, GenId genId) {

		tail_size = (tail_size > m_keys.size()) ? m_keys.size() : tail_size;

		auto rit = m_keys.rbegin();
		auto e = m_keys.rbegin() + tail_size;
		for (; rit != e; ++rit)
			if (*rit == genId) return true;
		return false;
	}

	void clear() {
		m_keys.clear();
		this->std::unordered_map<Key, T>::clear();
	}

	friend std::ostream& operator<<(std::ostream& o, special_map<Key, T, S> & m) {

		o << "[";
		auto p_it = m.begin_orig_order();
		while (p_it != m.end()) {
			o << "<" << p_it->first << "," << p_it->second << ">";
			p_it = m.next_orig_order(p_it);
		}
		o << "]";
		return o;
	}
	std::string print_ids() {
		std::stringstream ss;
		ss << "[";
		auto p_it = this->begin_orig_order();
		while (p_it != this->end()) {
			ss << p_it->first << " ";
			p_it = this->next_orig_order(p_it);
		}
		ss << "]";
		return ss.str();
	}

protected:

	std::vector<Key> m_keys;
};

typedef special_map<GenId, std::unordered_map<UanAddress, uint32_t>, gen_ssn_t> RcvNum;
typedef special_map<GenId, uint32_t, gen_ssn_t> SentNum;
typedef special_map<GenId, uint32_t, gen_ssn_t> CodedSeqId;
typedef special_map<GenId, double, gen_ssn_t> ForwardPlan;
typedef special_map<GenId, double, gen_ssn_t> FilteredPlan;
typedef special_map<GenId, double, gen_ssn_t> RetransmissionPlan;
typedef special_map<GenId, GenerationState, gen_ssn_t> GenStateMap;
typedef special_map<GenId, std::shared_ptr<Ccack>, gen_ssn_t> CcackMap;

} //ncr

#endif /* SPECIALMAP_H_ */
