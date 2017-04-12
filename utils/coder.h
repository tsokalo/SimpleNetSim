// Copyright Steinwurf ApS 2011.
// Distributed under the "STEINWURF RESEARCH LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing
/*
 * coder.h
 *
 *  Created on: 29.10.2016
 *      Author: tsokalo
 */

#ifndef CODER_H_
#define CODER_H_

#include <algorithm>
#include <ctime>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

#include <storage/split.hpp>

#include <kodo_rlnc/full_vector_codes.hpp>
#include <kodo_rlnc/on_the_fly_codes.hpp>
#include <kodo_core/set_systematic_on.hpp>
#include <kodo_core/set_systematic_off.hpp>
#include <kodo_core/is_systematic_on.hpp>

#include <kodo_core/set_trace_callback.hpp>
#include <kodo_core/has_set_trace_callback.hpp>


#include "header.h"
#include "ssn.h"
#include "feedback-estimator.h"
#include "coder-help-info.h"
#include "utils/coding-vector.h"

#ifndef FULL_VECTOR
#define ON_THE_FLY
#endif

namespace ncr {

typedef fifi::binary4 fifi_field;

#ifdef FULL_VECTOR
typedef kodo_rlnc::full_vector_encoder<fifi_field> encoder;
typedef kodo_rlnc::full_vector_decoder<fifi_field> decoder;
#else
typedef kodo_rlnc::on_the_fly_encoder<fifi_field> encoder;
typedef kodo_rlnc::on_the_fly_decoder<fifi_field> decoder;
#endif

typedef std::shared_ptr<encoder> encoder_ptr;
typedef std::shared_ptr<decoder> decoder_ptr;

struct coder_overhead {

	static uint16_t get(uint32_t genSize)
	{
		encoder::factory encFactory(genSize, 1);
		auto enc = encFactory.build();
		return (enc->payload_size() - 1);
	}

};

template<class T>
class coder_queue {

	typedef std::unordered_map<GenId, T> CoderMap;

public:
	coder_queue(uint32_t symbolSize) {
		m_symbolSize = symbolSize;
	}
	T at(GenId genId) {
		return m_coders.at(genId);
	}
	bool find(GenId genId) {
		return (m_coders.find(genId) != m_coders.end());
	}

	T last() {
		return m_coders.at(m_ids.at(m_ids.size() - 1));
	}
	NcSymbol get_coded(GenId genId) {
		assert(find(genId));
		NcSymbol symb(m_coders.at(genId)->payload_size(), 0);
		m_coders.at(genId)->write_payload(symb.data());
		//		for(auto c : symb) std::cout << (uint16_t)c << "\t";
		//		std::cout << std::endl;
		return symb;
	}
	uint32_t rank(GenId genId) {
		return (find(genId) ? m_coders.at(genId)->rank() : 0);
	}

	std::map<GenId, uint32_t> get_ranks() {
		std::map<GenId, uint32_t> ranks;
		for (auto id : m_ids) {
			assert(find(id));
			ranks[id] = m_coders.at(id)->rank();
			if (ranks[id] == 0) ranks.erase(id);
		}
		return ranks;
	}

	virtual CodingMatrix get_coding_matrix(GenId genId)=0;

	virtual CoderHelpInfo get_help_info_m(CodingMatrix m, GenId genId) = 0;
	virtual CoderHelpInfo get_help_info_c(CoderInfo c, GenId genId) = 0;

	CoderHelpInfo get_help_info(CodingMatrix m, CoderInfo c, GenId genId) {
		return (!m.empty()) ? get_help_info_m(m, genId) : ((!c.seen.empty()) ? get_help_info_c(c, genId) : CoderHelpInfo());
	}

protected:

	void pop_front() {
		if (m_coders.empty()) return;
		m_coders.erase(*m_ids.begin());
		m_ids.pop_front();
	}

	uint16_t custom_rank(CodingMatrix m) {
		uint16_t r = 0;
		for (uint16_t i = 0; i < m.size(); i++) {
			if (m.at(i).at(i) == 1) {
				r++;
				//
				// check column
				//
				for (uint16_t j = 0; j < m.size(); j++) {
					if (j == i) continue;
					if (m.at(j).at(i) != 0) {
						r--;
						break;
					}
				}
			}
		}
		return r;
	}

	CoderMap m_coders;
	std::deque<GenId> m_ids;
	uint32_t m_symbolSize;
	uint16_t m_numGen;
	uint32_t m_genSize;
	gen_ssn_t m_lastGenId;
};

class encoder_queue: public coder_queue<encoder_ptr> {

	typedef std::function<void(GenId)> notify_enque_func;

public:
	encoder_queue(uint16_t numGen, uint32_t genSize, uint32_t symbolSize) :
			coder_queue<encoder_ptr>(symbolSize), m_encFactory(genSize, symbolSize) {

		m_numGen = numGen;
		m_genSize = genSize;
	}

	void enque(NcSymbol symb) {

		SIM_LOG(CODER_LOG, "Enqueue symbol with size " << symb.size());

		assert(symb.size() == m_symbolSize);
		this->push_back(symb);
	}

	void set_notify_callback(notify_enque_func c) {
		m_notifyEnqueue = c;
	}

	CoderHelpInfo get_help_info_m(CodingMatrix m, GenId genId) {

		CoderHelpInfo h;

		assert(!m.empty());
		assert(m_genSize >= m.size());

		h.origRank = custom_rank(m);
		assert(h.origRank > 0);
		h.finRank = find(genId) ? m_genSize : h.origRank;

		return h;
	}
	CoderHelpInfo get_help_info_c(CoderInfo c, GenId genId) {
		CoderHelpInfo h;

		assert(m_genSize == c.genSize);

		h.origRank = c.rank;
		assert(h.origRank > 0);
		h.finRank = find(genId) ? m_genSize : h.origRank;

		return h;
	}

	CodingMatrix get_coding_matrix(GenId genId) {
		assert(0);
		return CodingMatrix();
	}

private:

	void push_back(NcSymbol symb) {

		auto add_coder = [this]()
		{
			m_lastGenId++;
			auto enc = m_encFactory.build();
#ifdef FULL_VECTOR
				kodo_core::set_systematic_off(*enc);
#endif
				m_coders[m_lastGenId.val()] = enc;
				if (m_coders.size() > m_numGen) pop_front();
				m_ids.push_back(m_lastGenId.val());

				SIM_LOG(CODER_LOG, "Add coder for generator with ID: " << m_lastGenId.val());
			};
		;

		if (m_coders.empty()) add_coder();
		encoder_ptr c = this->last();
		auto rank = c->rank();
		if (rank == m_genSize) {
			add_coder();
			c = this->last();
			rank = c->rank();
		}
		c->set_const_symbol(rank, storage::storage(symb));
		if (m_notifyEnqueue) m_notifyEnqueue(m_lastGenId.val());

		SIM_LOG(CODER_LOG, "New rank: " << rank + 1);
	}

	encoder::factory m_encFactory;
	notify_enque_func m_notifyEnqueue;
};

class decoder_queue: public coder_queue<decoder_ptr> {

public:
	decoder_queue(uint16_t numGen, uint32_t genSize, uint32_t symbolSize) :
			coder_queue<decoder_ptr>(symbolSize), m_decFactory(genSize, symbolSize) {

		m_numGen = numGen;
		m_genSize = genSize;
	}

	void enque(NcSymbol symb, GenId genId) {

		SIM_LOG(CODER_LOG, "Enqueue symbol with size " << symb.size() << ", generation ID: " << genId);

		this->add(symb, genId);
	}

	//
	// get only those uncoded symbols, which were not copied yet
	//
	std::vector<OrigSymbol> get_uncoded() {
		std::vector<OrigSymbol> s;
		auto it = m_coders.begin();
		while (it != m_coders.end()) {

			GenId genId = it->first;

			for (uint32_t i = 0; i < m_genSize; i++) {

				if (m_coders.at(genId)->is_symbol_uncoded(i)) {

					if (m_forwardMap[genId].find(i) == m_forwardMap[genId].end()) {

						s.push_back(std::vector<uint8_t>(m_symbolSize, 0));
						it->second->copy_from_symbol(i, storage::storage(s.at(s.size() - 1)));
						m_forwardMap[genId][i] = true;
						SIM_LOG(CODER_LOG, "New uncoded. Generation ID: " << genId << ", index: " << i);
					}
					else {
						//						SIM_LOG(CODER_LOG, "Previously uncoded. Generation ID: " << genId << ", index: " << i);
					}
				}
			}
			it++;
		}

		return s;
	}

	CodingMatrix get_coding_matrix(GenId genId) {
		assert(find(genId));
		assert(m_coders.at(genId)->rank() != 0);

		// is_symbol_partially_decoded
		CodingMatrix m;
		CodingVector symbol_coefficients(m_genSize);
		for (uint16_t i = 0; i < m_coders.at(genId)->coefficient_vector_size(); i++) {
			uint8_t* coef_vector = m_coders.at(genId)->coefficient_vector_data(i);
			symbol_coefficients.assign(coef_vector, coef_vector + m_genSize);
			if (!(m_coders.at(genId)->is_symbol_uncoded(i) || m_coders.at(genId)->is_symbol_partially_decoded(i))) {
				for (auto &v : symbol_coefficients)
					v = 0;
				;
			}
			m.push_back(symbol_coefficients);
			//						for(auto c : symbol_coefficients)std::cout << (uint16_t)c << "\t";
			//						std::cout << std::endl;
		}

		//				std::cout << "Cutrom rank:" << custom_rank(m) << std::endl;

		return m;
	}

	CoderInfo get_coder_info(GenId genId) {
		assert(find(genId));
		assert(m_coders.at(genId)->rank() != 0);

		auto dec = m_coders.at(genId);

		DecodedMap dm;
		for (uint32_t i = 0; i < m_genSize; ++i)
			dm.push_back(dec->is_symbol_uncoded(i));
		SeenMap sm;
		for (uint32_t i = 0; i < m_genSize; ++i)
			sm.push_back(dec->is_symbol_pivot(i));
		return CoderInfo(dec->rank(), m_genSize, sm, dm);
	}

	CoderHelpInfo get_help_info_m(CodingMatrix m, GenId genId) {

		CoderHelpInfo h;

		assert(!m.empty());
		assert(m_genSize >= m.size());

		auto dec = m_decFactory.build();
		//
		// read all external coding coefficients
		//
		std::vector<uint8_t> fake_symbol(dec->symbol_size());
		for (auto s : m) {
			//						for(auto c : s)std::cout << (uint16_t)c << "\t";
			//						std::cout << std::endl;
			dec->read_symbol(fake_symbol.data(), s.data());
		}

		h.origRank = custom_rank(m);
		//				std::cout << "GenID " << genId << ", orig rank " << h.origRank << std::endl;
		assert(h.origRank > 0);

		if (find(genId)) {
			//
			// read all own coding coefficients
			//
			std::vector<uint8_t> s(m_genSize);

			auto dec_o = m_coders.at(genId);

			for (uint16_t i = 0; i < dec_o->coefficient_vector_size(); i++) {
				uint8_t* coef_vector = dec_o->coefficient_vector_data(i);
				s.assign(coef_vector, coef_vector + m_genSize);
				dec->read_symbol(fake_symbol.data(), s.data());
			}

			CodingVector symbol_coefficients(m_genSize);
			for (uint16_t i = 0; i < dec->coefficient_vector_size(); i++) {
				uint8_t* coef_vector = dec->coefficient_vector_data(i);
				symbol_coefficients.assign(coef_vector, coef_vector + m_genSize);
				h.m.push_back(symbol_coefficients);
				//				for(auto c : symbol_coefficients)std::cout << (uint16_t)c << "\t";
				//				std::cout << std::endl;
			}

			h.finRank = custom_rank(h.m);

			//			std::cout << "GenID " << genId << ", fin rank " << h.finRank << std::endl;
		}
		else {
			h.finRank = h.origRank;
			h.m = m;
		}

		return h;
	}
	CoderHelpInfo get_help_info_c(CoderInfo remote_info, GenId genId) {

		assert(remote_info.rank > 0);
		assert(remote_info.seen.size() == m_genSize);
		assert(remote_info.decoded.size() == m_genSize);

		CoderHelpInfo h;
		if (find(genId)) {
			FeedbackEstimator est(get_coder_info(genId), remote_info);
			h.c = est.GetMergedCoderInfo();
			h.origRank = remote_info.rank;
			h.finRank = h.c.rank;
		}
		else {
			h.origRank = remote_info.rank;
			h.finRank = remote_info.rank;
			h.c = remote_info;
		}
		return h;
	}

private:

	void add(NcSymbol symb, GenId genId) {

		assert(!symb.empty());
		if (this->find(genId)) {
			auto dec = this->at(genId);
			dec->read_payload(symb.data());
		}
		else {
			m_coders[genId] = m_decFactory.build();
			//			auto it = m_ids.begin();
			//			for (; it != m_ids.end(); it++) {
			//				if (gen_ssn_t(*it) < genId) {
			//					m_ids.insert(it, genId);
			//					break;
			//				}
			//			}
			//			if (it == m_ids.end())
			m_ids.push_back(genId);
			if (m_coders.size() > m_numGen) {
				auto id = *m_ids.begin();
				if (m_forwardMap.find(id) != m_forwardMap.end()) m_forwardMap.erase(id);
				pop_front();
			}
			if (this->find(genId)) {
				auto dec = this->at(genId);
				dec->read_payload(symb.data());
			}
		}
	}

	decoder::factory m_decFactory;
	std::map<GenId, std::map<uint32_t, bool> > m_forwardMap;

};

}			//ncr

#endif /* CODER_H_ */
