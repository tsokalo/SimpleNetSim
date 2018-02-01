/*
 * MulticastBrr.cpp
 *
 *  Created on: 11.05.2017
 *      Author: tsokalo
 */

#include "multicast-brr.h"
#include "brrm-header.h"

namespace ncr {

MulticastBrr::MulticastBrr(UanAddress ownAddress, NodeType type, std::vector<UanAddress> destAddresses, SimParameters sp) {

	m_id = ownAddress;

	for (auto dst : destAddresses) {

		SIM_LOG(BRR_LOG, "Create BRR instance for DST " << dst << ", I am " << m_id);

		NodeType t = (ownAddress == dst) ? DESTINATION_NODE_TYPE : type;
		assert(!(t == DESTINATION_NODE_TYPE && type == SOURCE_NODE_TYPE));

		assert(m_brr.find(dst) == m_brr.end());
		m_brr[dst] = routing_rules_ptr(new NcRoutingRules(ownAddress, t, dst, sp));
	}
}

MulticastBrr::~MulticastBrr() {

}

void MulticastBrr::SetLogCallback(add_log_func addLog) {
	for (auto brr : m_brr)
		brr.second->SetLogCallback(addLog);
}
void MulticastBrr::SetGetRankCallback(get_rank_func c) {
	for (auto brr : m_brr)
		brr.second->SetGetRankCallback(c);
}
void MulticastBrr::SetGetCodingMatrixCallback(get_coding_matrix_func c) {
	for (auto brr : m_brr)
		brr.second->SetGetCodingMatrixCallback(c);
}
void MulticastBrr::SetGetCoderInfoCallback(get_coder_info_func c) {
	for (auto brr : m_brr)
		brr.second->SetGetCoderInfoCallback(c);
}
void MulticastBrr::SetCoderHelpInfoCallback(get_help_info_func c) {
	for (auto brr : m_brr)
		brr.second->SetCoderHelpInfoCallback(c);
}
void MulticastBrr::SetReceiveAppCallback(receive_app_func c) {
	for (auto brr : m_brr)
		brr.second->SetReceiveAppCallback(c);
}

void MulticastBrr::EnableCcack(hash_matrix_set_ptr hashMatrixSet) {
	for (auto brr : m_brr)
		brr.second->EnableCcack(hashMatrixSet);
}

/*
 * INPUTS
 */
void MulticastBrr::ProcessHeaderInfo(HeaderMInfo l) {

	HeaderInfo h;
	h.addr = l.addr;
	h.pf = l.pf;
	h.txPlan = l.txPlan;

	for (auto p : l.p) {
		auto addr = p.first;
		assert(m_brr.find(addr) != m_brr.end());
		h.p = p.second;
		m_brr.at(addr)->ProcessHeaderInfo(h);
	}
}
void MulticastBrr::UpdateSent(GenId genId, uint32_t num, bool notify_sending) {
	for (auto brr : m_brr)
		brr.second->UpdateSent(genId, num, notify_sending);
}
void MulticastBrr::UpdateRcvd(GenId genId, UanAddress id, bool linDep) {
	for (auto brr : m_brr)
		brr.second->UpdateRcvd(genId, id, linDep);
}
void MulticastBrr::UpdateRcvd(GenId genId, UanAddress id, std::vector<OrigSymbol> v) {
	for (auto brr : m_brr)
		brr.second->UpdateRcvd(genId, id, v);
}
void MulticastBrr::UpdateLoss(GenId genId, UanAddress id) {
	for (auto brr : m_brr)
		brr.second->UpdateLoss(genId, id);
}
void MulticastBrr::NotifySending() {
	for (auto brr : m_brr)
		brr.second->NotifySending();
}
void MulticastBrr::AddSentCcack(GenId genId, CodingVector cv) {
	for (auto brr : m_brr)
		brr.second->AddSentCcack(genId, cv);
}
void MulticastBrr::AddRcvdCcack(GenId genId, CodingVector cv) {
	for (auto brr : m_brr)
		brr.second->AddRcvdCcack(genId, cv);
}
void MulticastBrr::AddToCoalition(UanAddress addr) {
	for (auto brr : m_brr)
		brr.second->AddToCoalition(addr);
}
void MulticastBrr::SetSendingRate(Datarate d) {
	for (auto brr : m_brr)
		brr.second->SetSendingRate(d);
}
void MulticastBrr::ProcessServiceMessage(FeedbackMInfo f) {
	for (auto p : f.ps) {
		auto dst = p.first;
		assert(m_brr.find(dst) != m_brr.end());
		f.p = p.second;
		SIM_LOG_N(BRRM_LOG, m_id, "Feedback source " << f.addr << " with priority " << f.p << " for DST " << dst);
		m_brr.at(dst)->ProcessServiceMessage(f);
	}
}
void MulticastBrr::CheckReqRetrans(UanAddress id, GenId genId, bool all_prev_acked)
{
	//
	// if at least for all destination we may send the retransmission request
	//
	for (auto brr_it : m_brr)
		brr_it.second->CheckReqRetrans(id, genId, all_prev_acked);
}
/*
 * OUTPUTS
 */
TxPlan MulticastBrr::GetTxPlan() {

	TxPlan txPlan;

#ifdef MAX_DATA_TXPLAN
	//
	// 1. send the maximum number of symbols in each generation among all destinations
	// 2. set all_prev_ack flag if it is set by at least one destination
	//
	for (auto brr : m_brr) {
		auto dst = brr.first;
		auto t = brr.second->GetTxPlan();

		for (auto item : t) {
			auto gid = item.first;
			auto new_item = item.second;

			m_trafficLoad[dst].add(new_item.num_all);

			auto old_item_it = txPlan.find(gid);
			if (old_item_it != txPlan.end()) {
				old_item_it->second.num_all = (new_item.num_all > old_item_it->second.num_all) ? new_item.num_all : old_item_it->second.num_all;
				old_item_it->second.all_prev_acked = new_item.all_prev_acked ? true : old_item_it->second.all_prev_acked;
			} else {
				txPlan[gid] = new_item;
			}
		}
	}
#endif

#ifdef MAX_TRAFFICLOAD_TXPLAN
	//
	// update traffic load filter
	//
	for (auto brr : m_brr) {
		auto dst = brr.first;
		auto t = brr.second->GetTxPlan();

		for (auto item : t) {
			auto gid = item.first;
			auto new_item = item.second;

			m_trafficLoad[dst].add(new_item.num_all);
		}
	}
	//
	// find the DST with the max traffic load
	//
	double tlmax = 0;
	UanAddress dstmax = 0;
	for (auto tl : m_trafficLoad) {
		auto newdst = tl.first;
		auto val = tl.second.val_unrel();
		dstmax = (tlmax < val) ? newdst : dstmax;
		tlmax = (tlmax < val) ? val : tlmax;
	}
	//
	// save the corresponding TX plan
	//
	txPlan = m_brr.at(dstmax)->GetTxPlan();
	//
	// add other TX items if not already present in the plan
	//
	for (auto brr : m_brr) {
		auto dst = brr.first;
		if (dst == dstmax)
		continue;

		auto t = brr.second->GetTxPlan();

		for (auto item : t) {
			auto gid = item.first;
			auto new_item = item.second;

			auto old_item_it = txPlan.find(gid);
			if (old_item_it == txPlan.end()) {
				txPlan[gid] = new_item;
			}
		}
	}

#endif

	SIM_LOG_N(BRRM_LOG, m_id, "TX plan " << txPlan);
	return txPlan;
}
BrrMHeader MulticastBrr::GetHeader(TxPlan txPlan, FeedbackMInfo f) {

	auto h = GetHeaderInfo(txPlan);
	f.addr = h.addr;
	f.ps = h.p;
	return BrrMHeader(h, f);
}

HeaderMInfo MulticastBrr::GetHeaderInfo() {

	HeaderMInfo header(m_brr.begin()->second->GetHeaderInfo());

#ifdef USE_MAX_FILTERING_COEFS
	for (auto brr_it : m_brr) {
		auto brr = brr_it.second;
		auto dst = brr_it.first;
		auto h = brr->GetHeaderInfo();
		//
		// save the priority for each destination in the header
		//
		header.p[dst] = h.p;
		//
		// adjust the filtering coefficients according to the actual amount of the sent information
		//
		for (auto pf_ : h.pf) {
			header.pf[pf_.first] = (header.pf[pf_.first] < pf_.second) ? pf_.second : header.pf[pf_.first];
		}
	}
#endif

#ifdef USE_MIN_FILTERING_COEFS
	for (auto brr_it : m_brr) {
		auto brr = brr_it.second;
		auto dst = brr_it.first;
		auto h = brr->GetHeaderInfo();
		//
		// save the priority for each destination in the header
		//
		header.p[dst] = h.p;
		//
		// adjust the filtering coefficients according to the actual amount of the sent information
		//
		for (auto pf_ : h.pf)
		{
			header.pf[pf_.first] = (header.pf[pf_.first] > pf_.second) ? pf_.second : header.pf[pf_.first];
		}
	}
#endif

#ifdef NORM_REST_FILTERING_COEFS

	//
	// find the maximum traffic load
	//
	double tlmax = 0;
	double cr = 1;

	for (auto tl : m_trafficLoad) {
		auto dst = tl.first;
		auto val = tl.second.val_unrel();
		auto newcr = m_brr.at(dst)->GetCodingRate();
		cr = (tlmax < val) ? newcr : cr;
		tlmax = (tlmax < val) ? val : tlmax;
	}

	for (auto brr_it : m_brr) {
		auto brr = brr_it.second;
		auto dst = brr_it.first;
		auto h = brr->GetHeaderInfo();
		//
		// save the priority for each destination in the header
		//
		header.p[dst] = h.p;
		//
		// adjust the filtering coefficients according to the actual amount of the sent information
		//
		for (auto pf_ : h.pf)
		{
			auto v = pf_.second * brr->GetCodingRate() / cr;
			header.pf[pf_.first] = (header.pf[pf_.first] < v) ? v : header.pf[pf_.first];
		}
	}
//
//	for (auto brr_it : m_brr) {
//		auto brr = brr_it.second;
//		auto dst = brr_it.first;
//		auto h = brr->GetHeaderInfo();
//
//		for (auto pf_ : h.pf)
//		header.pf[pf_.first] = pf_.second * brr->GetCodingRate() / cr;
//	}

#endif

#ifdef DEST_AWARE_FILTERING_COEFS

	double cr = 1;

	for (auto brr_it : m_brr) {
		auto brr = brr_it.second;
		auto dst = brr_it.first;

		auto newcr = brr->GetCodingRate();
		cr = (newcr > cr) ? newcr : cr;
	}
	cr = 1 / cr;

	for (auto brr_it : m_brr) {
		auto brr = brr_it.second;
		auto dst = brr_it.first;
		auto a = brr->GetInfoOnDsts();
		assert(leq(a,1));
		auto cr_ = 1 / brr->GetCodingRate();
		assert(leq(a,cr_));
//		std::cout << cr_ << "\t" << a << "\t" << cr << std::endl;
		assert(leq(cr_ - a , cr));
		auto h = brr->GetHeaderInfo();
		//
		// save the priority for each destination in the header
		//
		header.p[dst] = h.p;
		//
		// adjust the filtering coefficients according to the actual amount of the sent information
		//
		for (auto pf_ : h.pf)
		{
			auto v = (1 - (1 - pf_.second) * (cr_ - a) / cr);
			header.pf[pf_.first] = (header.pf[pf_.first] < v) ? v : header.pf[pf_.first];
//			std::cout << m_id << "\tSink: " << pf_.first << "\tDST: " << dst << "\tcr_: " << cr_ << "\ta: " << a << "\tcr: " << cr << "\tpfi: " << pf_.second << "\tv: " << v << "\thpfi: " << header.pf[pf_.first] << std::endl;
		}
//		std::cout << std::endl;
	}
//	std::cout << std::endl;
//	std::cout << std::endl;

#endif

	return header;
}
HeaderMInfo MulticastBrr::GetHeaderInfo(TxPlan txPlan) {
	auto header = GetHeaderInfo();
	header.txPlan = txPlan;
	return header;
}
FeedbackMInfo MulticastBrr::GetServiceMessage() {

	FeedbackMInfo f(m_brr.begin()->second->GetServiceMessage());
	for (auto brr_it : m_brr) {
		auto brr = brr_it.second;
		auto dst = brr_it.first;
		f.ps[dst] = brr->GetPriority();
	}
	return f;
}
bool MulticastBrr::NeedGen() {
	//
	// only if for all destination new data has to be generated
	//
	for (auto brr_it : m_brr)
		if (!brr_it.second->NeedGen()) return false;

	return true;
}
uint32_t MulticastBrr::GetFreeBufferSize() {

	//
	// select the minimum of that is required for each destination
	//
	uint32_t num = std::numeric_limits < uint32_t > ::max();
	for (auto brr_it : m_brr) {
		auto v = brr_it.second->GetFreeBufferSize();
		num = (num > v) ? v : num;
	}
	return num;
}
bool MulticastBrr::MaySend(double dr) {
	//
	// if at least for all destination we may send data
	//
	for (auto brr_it : m_brr)
	{
		if (brr_it.second->MaySend(dr)) return true;
	}
	return false;
}
bool MulticastBrr::MaySendData(double dr){
	//
	// if at least for all destination we may send data
	//
	for (auto brr_it : m_brr)
		if (brr_it.second->MaySendData(dr)) return true;

	return false;
}
bool MulticastBrr::MaySendServiceMessage(){
	//
	// if at least for all destination we may send data
	//
	for (auto brr_it : m_brr)
		if (brr_it.second->MaySendServiceMessage()) return true;

	return false;
}
void MulticastBrr::CreateRetransRequest() {
	for (auto brr_it : m_brr)
		brr_it.second->CreateRetransRequest();
}

void MulticastBrr::ResetRetransInfo() {
	for (auto brr_it : m_brr)
		brr_it.second->ResetRetransInfo();
}

uint32_t MulticastBrr::GetGenBufSize(uint32_t maxPkts) {
	//
	// select the minimum of that is required for each destination
	//
	uint32_t num = std::numeric_limits < uint32_t > ::max();
	for (auto brr_it : m_brr) {
		auto v = brr_it.second->GetGenBufSize(maxPkts);
		num = (num > v) ? v : num;
	}
	return num;
}
uint16_t MulticastBrr::GetAckBacklogSize() {
	//
	// select the minimum ACK backlog size
	//
	uint32_t num = std::numeric_limits < uint32_t > ::max();
	for (auto brr_it : m_brr) {
		auto v = brr_it.second->GetAckBacklogSize();
		num = (num > v) ? v : num;
	}
	return num;
}
std::map<UanAddress, uint16_t> MulticastBrr::GetCoalitionSize() {
	std::map < UanAddress, uint16_t > cs;
	for (auto brr_it : m_brr) {
		cs[brr_it.first] = brr_it.second->GetCoalitionSize();
	}
	return cs;
}

}
