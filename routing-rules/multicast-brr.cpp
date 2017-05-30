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

	for (auto dst : destAddresses) {

		NodeType t = (ownAddress == dst) ? DESTINATION_NODE_TYPE : type;
		assert(!(t == DESTINATION_NODE_TYPE && type == SOURCE_NODE_TYPE));

		assert(m_brr.find(dst) == m_brr.end());
		m_brr[dst] = routing_rules_ptr(new NcRoutingRules(ownAddress, t, dst, sp));
		m_lead = dst;
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
void MulticastBrr::RcvHeaderInfo(HeaderInfo l) {
	for (auto brr : m_brr)
		brr.second->RcvHeaderInfo(l);
}
void MulticastBrr::RcvFeedbackInfo(FeedbackInfo l) {
	for (auto brr : m_brr)
		brr.second->RcvFeedbackInfo(l);
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
//
void MulticastBrr::AddToCoalition(UanAddress addr) {
	for (auto brr : m_brr)
		brr.second->AddToCoalition(addr);
}
void MulticastBrr::SetSendingRate(Datarate d) {
	for (auto brr : m_brr)
		brr.second->SetSendingRate(d);
}

/*
 * OUTPUTS
 */
TxPlan MulticastBrr::GetTxPlan() {
	TxPlan txPlan;
	for (auto brr : m_brr) {
		auto t = brr.second->GetTxPlan();
		for (auto item : t) {
			auto gid = item.first;
			auto new_item = item.second;
			auto old_item_it = txPlan.find(gid);
			if (old_item_it != txPlan.end()) {
				old_item_it->second.num_all = (new_item.num_all > old_item_it->second.num_all) ? new_item.num_all : old_item_it->second.num_all;
				old_item_it->second.all_prev_acked = new_item.all_prev_acked ? true : old_item_it->second.all_prev_acked;
			} else {
				txPlan[gid] = new_item;
			}
		}
	}
	return txPlan;
}
BrrMHeader MulticastBrr::GetHeader(TxPlan txPlan, FeedbackInfo f) {

	BrrMHeader brrHeader(m_brr.begin()->second->GetHeader(txPlan, f));
	//
	// find the smallest coding rate
	//
	double cr = std::numeric_limits<double>::max();
	UanAddress win_node = 0;
	for (auto brr_it : m_brr) {
		auto brr = brr_it.second;
		auto dst = brr_it.first;
		auto l_cr = brr->GetCodingRate();
		cr = (cr > l_cr) ? l_cr : cr;
		win_node = (cr > l_cr) ? dst : win_node;
	}
	for (auto brr_it : m_brr) {
		auto brr = brr_it.second;
		auto dst = brr_it.first;
		auto bH = brr->GetHeader(txPlan, f);
		//
		// save the priority for each destination in the header
		//
		brrHeader.h.p[dst] = bH.h.p;
		//
		// adjust the filtering coefficients according to the actual amount of the sent information
		//
		for (auto pf_ : bH.h.pf)
			brrHeader.h.pf[pf_.first] = pf_.second * brr->GetCodingRate() / cr;

		brrHeader.f.p[dst] = bH.f.p;
	}

	auto brrh = m_brr[win_node]->GetHeader(txPlan, f);
	brrHeader.f.rcvMap = brrh.f.rcvMap;
	brrHeader.f.rrInfo = brrh.f.rrInfo;
	brrHeader.f.netDiscovery = brrh.f.netDiscovery;
	brrHeader.f.ttl = brrh.f.ttl;
	brrHeader.f.ackInfo = brrh.f.ackInfo;
	brrHeader.f.updated = true;

	return brrHeader;
}
FeedbackInfo MulticastBrr::GetFeedbackInfo() {
	return m_brr[m_lead]->GetFeedbackInfo();
}
FeedbackInfo MulticastBrr::GetRetransRequestInfo(ttl_t ttl) {
	return m_brr[m_lead]->GetRetransRequestInfo(ttl);
}
HeaderInfo MulticastBrr::GetHeaderInfo() {
	return m_brr[m_lead]->GetHeaderInfo();
}
HeaderInfo MulticastBrr::GetHeaderInfo(TxPlan txPlan) {
	return m_brr[m_lead]->GetHeaderInfo(txPlan);
}
NetDiscoveryInfo MulticastBrr::GetNetDiscoveryInfo(ttl_t ttl) {
	return m_brr[m_lead]->GetNetDiscoveryInfo(ttl);
}
UanAddress MulticastBrr::GetSinkVertex() {
	return m_brr[m_lead]->GetSinkVertex();
}
//
bool MulticastBrr::NeedGen() {
	return m_brr[m_lead]->NeedGen();
}
uint32_t MulticastBrr::GetNumGreedyGen() {
	return m_brr[m_lead]->GetNumGreedyGen();
}
bool MulticastBrr::MaySendData(double dr) {
	return m_brr[m_lead]->MaySendData(dr);
}
bool MulticastBrr::MaySendFeedback() {
	return m_brr[m_lead]->MaySendFeedback();
}
bool MulticastBrr::MaySendNetDiscovery(ttl_t ttl) {
	return m_brr[m_lead]->MaySendNetDiscovery(ttl);
}
// retransmission requests
bool MulticastBrr::MaySendRetransRequest(std::map<GenId, uint32_t> ranks, UanAddress id, GenId genId, bool all_prev_acked) {
	return m_brr[m_lead]->MaySendRetransRequest(ranks, id, genId, all_prev_acked);
}
bool MulticastBrr::ProcessRetransRequest(FeedbackInfo fb) {
	return m_brr[m_lead]->ProcessRetransRequest(fb);
}
bool MulticastBrr::HasRetransRequest(FeedbackInfo fb) {
	return m_brr[m_lead]->HasRetransRequest(fb);
}
void MulticastBrr::ResetRetransInfo() {
	return m_brr[m_lead]->ResetRetransInfo();
}
void MulticastBrr::UpdateRetransRequest() {
	return m_brr[m_lead]->UpdateRetransRequest();
}
//
uint32_t MulticastBrr::GetGenBufSize(uint32_t maxPkts) {
	return m_brr[m_lead]->GetGenBufSize(maxPkts);
}
uint32_t MulticastBrr::GetAmountTxData() {
	return m_brr[m_lead]->GetAmountTxData();
}
uint16_t MulticastBrr::GetAckBacklogSize() {
	return m_brr[m_lead]->GetAckBacklogSize();
}
uint16_t MulticastBrr::GetCoalitionSize() {
	return m_brr[m_lead]->GetCoalitionSize();
}
void MulticastBrr::FindLeadingDst()
{

}
}
