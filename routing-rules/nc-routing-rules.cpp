/*
 * nc-routing-rules.cc
 *
 *  Created on: 03.10.2016
 *      Author: tsokalo
 */
#include "nc-routing-rules.h"
#include "utils/comparison.h"
#include "utils/ssn.h"
#include <assert.h>
#include <iterator>
#include <algorithm>

namespace ncr {

NcRoutingRules::NcRoutingRules(UanAddress ownAddress, NodeType type, UanAddress destAddress, SimParameters sp) :
		m_outdatedGens(2 * sp.numGen), m_outdatedGensInform(2 * sp.numGen, sp.ackMaxRetransNum), m_thresholdP(sp.sendRate / 10), m_gen(m_rd()), m_dis(0, 1) {

	m_sp = sp;
	//	assert((sp.sendRate / 5) >= SMALLEST_SENDER_PHY_DATA_RATE && (sp.sendRate / 50) <= SMALLEST_SENDER_PHY_DATA_RATE);

	m_id = ownAddress;
	m_dst = destAddress;
	m_nodeType = type;
	m_d = m_sp.sendRate;

	m_h.addr = m_id;
	m_f.addr = m_id;
	m_f.type.assign(ServiceMessType::NONE);
	m_inF[m_id] = 0;
	m_cr = 1;
	m_p = m_thresholdP;
	if (m_id == m_dst) m_p = DESTINATION_PRIORITY;
	m_h.p = m_p;
	m_f.p = m_p;
	m_f.ttl = -1;
	m_logItem.p = m_p;
	m_logItem.d = m_d;
	m_logItem.cr = m_cr;
	m_logItem.dst = m_dst;
	m_congControl = congestion_control_ptr(new CongestionControl(m_nodeType));
	m_fastFeedback = false;
	m_sent = 0;
	SetConnected(false);

	m_feedbackP = 0.0;
	m_netDiscP = 1.0;
	m_maxTtl = m_sp.maxTtl;
	SIM_LOG_NP(BRR_LOG, m_id, m_p, "Using TTL " << m_maxTtl);
	SIM_ASSERT_MSG(m_maxTtl >= m_id, "TTL and addressing problem. TTL " << m_maxTtl << ", own ID " << m_id);
	m_b = 0.1;
	m_numRr = rr_counter_ptr(new RetransRequestCounter(sp.numRr));
}
NcRoutingRules::~NcRoutingRules() {

}
void NcRoutingRules::SetLogCallback(add_log_func addLog) {
	m_addLog = addLog;
}
void NcRoutingRules::SetGetRankCallback(get_rank_func c) {
	m_getRank = c;
}
void NcRoutingRules::SetGetCodingMatrixCallback(get_coding_matrix_func c) {
	m_getCodingMatrix = c;
}
void NcRoutingRules::SetGetCoderInfoCallback(get_coder_info_func c) {
	m_getCoderInfo = c;
}
void NcRoutingRules::SetCoderHelpInfoCallback(get_help_info_func c) {
	m_getCoderHelpInfo = c;
}
void NcRoutingRules::SetReceiveAppCallback(receive_app_func c) {
	m_rcvApp = c;
}
void NcRoutingRules::EnableCcack(hash_matrix_set_ptr hashMatrixSet) {
	m_hashMatrixSet = hashMatrixSet;
}
//////////////////////////////////////////////////////////////////////

void NcRoutingRules::ProcessHeaderInfo(HeaderInfo l) {

	SIM_LOG_FUNC(BRR_LOG);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "address " << l.addr << ", p " << l.p);

	m_outputs.add(l.addr, l.p);

	if (l.p >= m_p) {

		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "add output edge: " << m_outputs);
		UpdateCoalition();
	}
	if (l.pf.find(m_id) != l.pf.end()) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "save input filter: " << l.pf.at(m_id));
		m_inF[l.addr] = l.pf.at(m_id);
		m_logItem.fp[l.addr] = m_inF[l.addr];
		if (m_addLog) m_addLog(m_logItem, m_id);
	}
}

void NcRoutingRules::UpdateSent(GenId genId, uint32_t num, bool notify_sending) {

	SIM_LOG_FUNC(BRR_LOG);

	m_sentNum[genId] += num;
	m_sent += num;

	if (notify_sending) {
		m_logItem.ns = num;
		m_logItem.gsn = genId;
		if (m_addLog) m_addLog(m_logItem, m_id);
		m_logItem.ns = 0;
	}

	DoUpdateForwardPlan();
}
void NcRoutingRules::NotifySending() {

	m_logItem.ns = 1;
	if (m_addLog) m_addLog(m_logItem, m_id);
	m_logItem.ns = 0;
}
void NcRoutingRules::AddSentCcack(GenId genId, CodingVector cv) {

	assert(m_hashMatrixSet);
	if (m_ccack.find(genId) == m_ccack.end()) {
		m_ccack[genId] = ccack_ptr(new Ccack(m_sp.genSize, m_hashMatrixSet));
	}
	m_ccack[genId]->SaveSnt(cv);

	SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "CCACK: save sent " << cv);
}
void NcRoutingRules::AddRcvdCcack(GenId genId, CodingVector cv) {

	assert(m_hashMatrixSet);
	if (m_ccack.find(genId) == m_ccack.end()) {
		m_ccack[genId] = ccack_ptr(new Ccack(m_sp.genSize, m_hashMatrixSet));
	}
	m_ccack[genId]->SaveRcv(cv);

	SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "CCACK: save received " << cv);
}
void NcRoutingRules::UpdateRcvd(GenId genId, UanAddress id, std::vector<OrigSymbol> v) {

	UpdateRcvd(genId, id);

	if (m_id == m_dst) {
		for (auto s : v) {
			if (m_rcvApp) {
				m_logItem.ssn = m_rcvApp(s);
				m_logItem.gsn = genId;
				SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Decoding original symbol with SSN " << m_logItem.ssn);
			}
			if (m_addLog) m_addLog(m_logItem, m_id);
		}
		m_logItem.ssn = 0;
	}
}
void NcRoutingRules::UpdateRcvd(GenId genId, UanAddress id, bool linDep) {

	SIM_LOG_FUNC(BRR_LOG);

	if (id != m_id) {
		SetConnected(true);
		ValidateReaction(genId, id);
		PlanExpectedReaction(genId, id);
	}
	uint32_t num = 1;

	m_inRcvMap[id].add(num, 0);

	if (linDep) {
		//
		// receive linearly dependent symbol
		//
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "from " << id << ", number " << num << ", LIN DEP!");
		SetAcks();
		m_inLinDepMap[id].add(num, 0);
//		if (m_coalition.find(id) == m_coalition.end()) {
//			//
//			// stimulate ACK retransmission
//			//
//			m_outdatedGensInform.remove(genId);
//		}
	} else {

		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "from " << id << ", number " << num);

		m_inLinDepMap[id].add(num, 1);

		m_inRcvNum[genId][id] += num;
		m_logItem.nr = num;
		m_logItem.rank = m_getRank(genId);
		m_logItem.gsn = genId;
		if (m_addLog) m_addLog(m_logItem, m_id);
		m_logItem.nr = 0;

		DoForgetGeneration();

		DoFilter();
	}

	m_logItem.aw.s_rx = GetRxWinStart();
	m_logItem.aw.s_tx = GetTxWinStart();
	m_logItem.aw.e_tx = GetTxWinEnd();
	m_logItem.aw.e_rx = GetRxWinEnd();

}
void NcRoutingRules::UpdateLoss(GenId genId, UanAddress id) {

	SIM_LOG_FUNC(BRR_LOG);
	uint32_t num = 1;
	m_inRcvMap[id].add(num, 1);
}
//////////////////////////////////////////////////////////////////////
TxPlan NcRoutingRules::GetTxPlan() {

	SIM_LOG_FUNC(BRR_LOG);

	auto txPlan = m_txPlan;

#ifdef HALVE_TX_PLAN

//	if (m_nodeType != SOURCE_NODE_TYPE) {
//		for (auto &item : txPlan) {
//			auto coef = eq(m_cr,0) ? 1 : 1-1/m_cr;
//			coef = coef < 0.5 ? 1 : coef;
//			if(item.second.num_all > (m_sp.genSize* 0.75))
//			item.second.num_all = ceil((double)item.second.num_all * 0.6);
//		}
//	}

#else

#endif

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "TX plan " << txPlan);
	return txPlan;
}

BrrHeader NcRoutingRules::GetHeader(TxPlan txPlan, FeedbackInfo f) {
	auto h = GetHeaderInfo(txPlan);
	f.addr = h.addr;
	f.p = h.p;
	return BrrHeader(h, f);
}

HeaderInfo NcRoutingRules::GetHeaderInfo() {

	SIM_LOG_FUNC(BRR_LOG);

	DoUpdateFilter();

	return m_h;
}
HeaderInfo NcRoutingRules::GetHeaderInfo(TxPlan txPlan) {

	SIM_LOG_FUNC(BRR_LOG);

	DoUpdateFilter();

	m_h.txPlan = txPlan;

	return m_h;
}

UanAddress NcRoutingRules::GetSinkVertex() {

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "My coalition: " << m_coalition);
	return (m_coalition.empty() ? -1 : m_coalition.begin()->first);

}
FeedbackInfo NcRoutingRules::GetServiceMessage() {
	SIM_LOG_FUNC(BRR_LOG);

	assert(m_service.ready_to_start());

	//
	// the most part of the feedback message is already prepared
	//
	m_f.rcvMap.clear();
	m_f.rcvMap = m_inRcvMap;
	m_f.rcvNum.clear();
	m_f.rcvNum = m_inRcvNum;
	m_fastFeedback = false;
	m_sent = 0;

	for (std::map<UanAddress, RcvMap>::iterator it = m_f.rcvMap.begin(); it != m_f.rcvMap.end(); it++) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Prepare feedback with receive map for " << it->first);
	}
	//
	SetAcks();
	m_outdatedGensInform.tic();
	//
	CheckReqEteAckII();
	//
	m_f.type.copy(m_service.get_type());
	//
	PlanExpectedReaction();
	//
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Service message type: " << m_f.type.GetAsInt());
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "" << m_service);
	return m_f;
}
bool NcRoutingRules::NeedGen() {
	SIM_LOG_FUNC(BRR_LOG);

	assert(GetActualRxWinSize() <= m_sp.numGen);

	if (m_nodeType != SOURCE_NODE_TYPE) return false;

	return (GetActualRxWinSize() < m_sp.numGen);

//	return (!m_congControl->BlockGen(m_txPlan.size(), m_sp.numGenBuffering));
}
uint32_t NcRoutingRules::GetFreeBufferSize() {
	SIM_LOG_FUNC(BRR_LOG);

	assert(GetActualRxWinSize() <= m_sp.numGen);

	return (m_sp.numGen - GetActualRxWinSize());
}
bool NcRoutingRules::MaySend(double dr) {
	SIM_LOG_FUNC(BRR_LOG);

	CheckServiceMessage();

	return (MaySendServiceMessage() || MaySendData(dr));
}
bool NcRoutingRules::MaySendData(double dr) {

	SIM_LOG_FUNC(BRR_LOG);

	if (m_coalition.empty()) return false;

#ifdef SECONDRY_CODE
	//
	// if retransmission was requested
	//
	if (!m_oldestRetransGenId.is_default()) {
		//
		// if I do not have anything requested for the retransmission then I should say NO
		//
		if (!HaveDataForRetransmissions()) return false;
	}
#endif

	return (GetAmountTxData() != 0);
}
bool NcRoutingRules::MaySendServiceMessage() {
	SIM_LOG_FUNC(BRR_LOG);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "" << m_service);

	return m_service.ready_to_start();
}

void NcRoutingRules::CheckGeneralFeedback() {
	SIM_LOG_FUNC(BRR_LOG);

	auto check = [this]()->bool
	{
		//
		// source never sends the feedback
		//
			if (m_nodeType == SOURCE_NODE_TYPE) return false;

			if (m_fastFeedback) {
				return true;
			}
			//
			// the coalition of the destination is always empty; the nodes without coalition cannot help the sender
			// So, they do not have to inform the sender about their presence sending the feedback
			//
			if (m_coalition.empty() && m_id != m_dst) return false;

			if (HaveAcksToSend()) return true;

			if (m_sent >= GetRegularFeedbackFreq()) {
				return true;
			}

//			return (m_dis(m_gen) < m_feedbackP);
			return false;
		};

	if (m_service.admit(ServiceMessType::REGULAR)) {
		m_service.set_want_start_service(check());
		if (m_service.init()) m_f.ttl = -1;
	}
}
void NcRoutingRules::CheckReqPtpAck() {

	SIM_LOG_FUNC(BRR_LOG);
	auto func = [this](GenId gid)->bool
	{
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "Checking ACK");
		if (!IsSoftAck(gid) && !IsHardAck(gid)) {
			m_service.set_want_start_service(true);
			return true;
		}
		return false;
	};

	if (m_service.admit(ServiceMessType::REQ_PTP_ACK)) {
		WorkInPtpAckRange(func);
		if (m_service.init()) m_f.ttl = -1;
	}
}

void NcRoutingRules::CheckReqEteAckI() {
	SIM_LOG_FUNC(BRR_LOG);
	//
	// source does EteAck request instead of sending the Retransmission Requests
	// other nodes do send these Requests
	//
	auto func = [this](GenId gid)->bool
	{
		if (!IsSoftAck(gid) && !IsHardAck(gid)) {
			m_service.set_want_start_service(true);
			return true;
		}
		return false;
	};

	if (m_service.admit(ServiceMessType::REQ_ETE_ACK)) {
		WorkInEteAckRange(func);
		if (m_service.init()) m_f.ttl = m_maxTtl;
	}
}

void NcRoutingRules::CheckReqEteAckII() {
	SIM_LOG_FUNC(BRR_LOG);

	auto dec_count = [this](GenId gid)->bool
	{
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "updating the ReqPtpAck count");
		assert(m_f.ackInfo.find(gid) != m_f.ackInfo.end());			//because of called after SetAcks
			if (!m_f.ackInfo[gid]) {
				if (m_ptpAckCount.find(gid) == m_ptpAckCount.end()) {
					m_ptpAckCount[gid] = m_sp.numPtpAckBeforeEteAck;
				} else {
					if (m_ptpAckCount[gid]-- == 0) {
						m_ptpAckCount[gid] = m_sp.numPtpAckBeforeEteAck;
						m_service.set_want_start_service(true);
					}
				}
			}
			return false;		// work for all generation in the PtpAck range
		};
	auto make_default = [this](GenId gid)->bool
	{
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "defaulting the ReqPtpAck count");
		m_ptpAckCount[gid] = m_sp.numPtpAckBeforeEteAck;
		return false;			// work for all generation in the PtpAck range
		};

	if (m_service.get_type() == ServiceMessType::REQ_PTP_ACK) {
		if (m_service.admit(ServiceMessType::REQ_ETE_ACK)) {
			WorkInPtpAckRange(dec_count);
			if (m_service.init()) m_f.ttl = m_maxTtl;
		}
	} else {
		WorkInPtpAckRange(make_default);
	}
}

void NcRoutingRules::CheckNetDisc() {
	SIM_LOG_FUNC(BRR_LOG);

	auto check = [this]()->bool
	{
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Node type: " << m_nodeType << ", coalition size: " << m_coalition.size());

//		//
//		// source never sends the network discovery messages
//		//
//			if (m_nodeType == SOURCE_NODE_TYPE) return false;

			//
			// if the current node has at least anybody in its coalition, it does not need explore the network
			// by sending the network discovery message; instead it has an opportunity to send the data packets
			// and can receive the feedback for them, which gives to the current node the same information as the
			// response on the network discovery message
			//
			if (!m_coalition.empty()) return false;
//			//
//			// the destination does not need to explore the network itself; it should only respond on the
//			// network discovery messages
//			//
//			if (m_id == m_dst) return false;

			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Is connected: " << IsConnected());

			if(!IsConnected())
			{
				SetConnected(true);			// assume to be connected after sending the network discovery message
				return true;
			}
			return false;
//			return (m_dis(m_gen) < m_netDiscP);
		};

	if (m_service.admit(ServiceMessType::NET_DISC)) {
		m_service.set_want_start_service(check());
		if (m_service.init()) m_f.ttl = 1;// allow the reply by the nearest neighbors only
	}
}
void NcRoutingRules::CheckServiceMessage() {
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "(1) " << m_service);

	CheckGeneralFeedback();
	CheckReqPtpAck();
	CheckReqEteAckI();
	CheckNetDisc();

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "(2) " << m_service);
}

void NcRoutingRules::WorkInPtpAckRange(std::function<bool(GenId)> func) {
	SIM_LOG_FUNC(BRR_LOG);
	//
	// refer to arq_buffer_description.txt
	//
	if (m_nodeType != SOURCE_NODE_TYPE) {
		//
		// avoid overflow
		//
		auto Wr = GetActualRxWinSize();
		auto Wrmax = m_sp.numGen;
		auto rGen = std::max(0, Wr - (Wrmax - m_sp.numGenRetrans));
		auto sGen = std::min((const int) m_sp.numGenPtpAck, std::max(0, Wr - (Wrmax - m_sp.numGenRetrans - m_sp.numGenPtpAck)));
		auto rs = rGen;
		auto re = rGen + sGen;
		auto gids = m_inRcvNum.get_key_range(rs, re);
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "key range [" << rs << "," << re << ")");
		for (auto gid : gids) {
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "work for generation " << gid);
			if (func(gid)) return;
		}
	} else {
		//
		// avoid throttling of the traffic generator
		//
		auto Wt = GetActualTxWinSize();
		auto Wr = GetActualRxWinSize();
		auto sGen = std::min((const int) m_sp.numGenPtpAck, std::max(0, m_sp.numGenRetrans + m_sp.numGenPtpAck - Wt));
		auto hGen = std::max(0, m_sp.numGenRetrans - Wt);
		auto rs = std::min((int) Wr, hGen);
		auto re = std::min((int) Wr, hGen + sGen);
		auto gids = m_inRcvNum.get_key_range(rs, re);
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "key range [" << rs << "," << re << ")");
		for (auto gid : gids) {
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "work for generation " << gid);
			if (func(gid)) return;
		}
	}
}
void NcRoutingRules::WorkInEteAckRange(std::function<bool(GenId)> func) {
	if (m_nodeType != SOURCE_NODE_TYPE) {

		// do nothing

	} else {

		auto Wt = GetActualTxWinSize();
		auto Wr = GetActualRxWinSize();
		auto hGen = std::max(0, m_sp.numGenRetrans - Wt);
		auto rs = 0;
		auto re = std::min((int) Wr, hGen);
		auto gids = m_inRcvNum.get_key_range(rs, re);
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "key range [" << rs << "," << re << ")");
		for (auto gid : gids) {
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "work for generation " << gid);
			if (func(gid)) return;
		}
	}
}
void NcRoutingRules::WorkInRetransRange(std::function<bool(GenId)> func) {
	if (m_nodeType != SOURCE_NODE_TYPE) {
		auto Wrmax = m_sp.numGen;
		auto Wr = GetActualRxWinSize();
		auto rGen = std::max(0, Wr - (Wrmax - m_sp.numGenRetrans));
		auto rs = 0;
		auto re = rGen;
		auto gids = m_inRcvNum.get_key_range(rs, re);
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "key range [" << rs << "," << re << ")");
		for (auto gid : gids) {
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "work for generation " << gid);
			if (func(gid)) return;
		}
	} else {

		// do nothing
	}
}
void NcRoutingRules::ProcessServiceMessage(FeedbackInfo f) {

	SIM_LOG_FUNC(BRR_LOG);

	SetConnected(true);
	ValidateReaction(f);
	PlanExpectedReaction(f);

	ProcessRegularFeedback(f);

	if (f.type == ServiceMessType::REGULAR) {
		return;
	}
	if (f.type == ServiceMessType::REQ_PTP_ACK) {
		ProcessReqPtpAck(f);
		return;
	}
	if (f.type == ServiceMessType::REQ_ETE_ACK) {
		ProcessReqEteAck(f);
		return;
	}
	if (f.type == ServiceMessType::RESP_PTP_ACK) {
		ProcessRespPtpAck(f);
		return;
	}
	if (f.type == ServiceMessType::RESP_ETE_ACK) {
		ProcessRespEteAck(f);
		return;
	}
	if (f.type == ServiceMessType::NET_DISC || f.type == ServiceMessType::REP_NET_DISC) {
		ProcessNetDisc(f);
		return;
	}
	if (f.type == ServiceMessType::REQ_RETRANS || f.type == ServiceMessType::REP_REQ_RETRANS) {
		ProcessReqRetrans(f);
		return;
	}

	assert(0);
}

void NcRoutingRules::ProcessRegularFeedback(FeedbackInfo l) {
	SIM_LOG_FUNC(BRR_LOG);

	m_outputs.add(l.addr, l.p);
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Feedback source " << l.addr << " with priority " << l.p << " outputs " << m_outputs);
	if (l.rcvMap.find(m_id) != l.rcvMap.end()) {
		m_outRcvMap[l.addr] = l.rcvMap.at(m_id);
		m_logItem.eps[l.addr] = m_outRcvMap[l.addr].val_unrel();
		if (m_addLog) m_addLog(m_logItem, m_id);
	} else {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "feedback has no receive map for me");
	}

	if (DoesItCooperate(l.addr)) ProcessAcks(l);

	for (auto r : l.rcvNum) {
		auto gid = r.first;
		auto it = r.second.find(m_id);
		if (it != r.second.end()) m_outRcvNum[gid][l.addr] = it->second;
	}
	EvaluateSoftAck();

	UpdateCoalition();
	DoCalcRedundancy();
}
void NcRoutingRules::ProcessReqPtpAck(FeedbackInfo f) {
	SIM_LOG_FUNC(BRR_LOG);

	if (m_service.admit(ServiceMessType::RESP_PTP_ACK)) {
		m_service.set_want_start_service(DoICooperate(f.addr));
		if (m_service.init()) m_f.ttl = -1;
	}
}
void NcRoutingRules::ProcessReqEteAck(FeedbackInfo f) {
	SIM_LOG_FUNC(BRR_LOG);

	if (!DoICooperate(f.addr)) return;

	for (auto r : f.rcvNum) {
		auto gid = r.first;
		assert(f.ackInfo.find(gid) != f.ackInfo.end());
		m_f.ackInfo[gid] = f.ackInfo[gid] ? f.ackInfo[gid] : m_f.ackInfo[gid];
	}

	if (m_nodeType == DESTINATION_NODE_TYPE) {

		if (m_service.admit(ServiceMessType::RESP_PTP_ACK)) {
			m_service.set_want_start_service(true);
			if (m_service.init()) m_f.ttl = m_maxTtl;
		}

	} else {

		//
		// we send the response only if all generations are positively ACKed;
		// until TTL expires we forward the request
		//
		for (auto r : f.rcvNum) {
			auto gid = r.first;
			if (!m_f.ackInfo[gid]) {
				if (m_service.admit(ServiceMessType::REQ_ETE_ACK)) {
					m_service.set_want_start_service(f.ttl > 0);
					if (m_service.init()) m_f.ttl = f.ttl - 1;
				}
				return;
			}
		}
	}
}
void NcRoutingRules::ProcessRespPtpAck(FeedbackInfo f) {
	SIM_LOG_FUNC(BRR_LOG);

	// do nothing; it is sufficient to process the regular feedback containing in this service message
}
void NcRoutingRules::ProcessRespEteAck(FeedbackInfo f) {
	SIM_LOG_FUNC(BRR_LOG);

	if (DoICooperate(f.addr)) return;

	for (auto r : f.rcvNum) {
		auto gid = r.first;
		m_f.ackInfo[gid] = f.ackInfo[gid] ? f.ackInfo[gid] : m_f.ackInfo[gid];
	}
	//
	// forward the response
	//
	if (m_service.admit(ServiceMessType::RESP_PTP_ACK)) {
		m_service.set_want_start_service(f.ttl > 0);
		if (m_service.init()) m_f.ttl = f.ttl - 1;
	}
}
void NcRoutingRules::ProcessNetDisc(FeedbackInfo f) {
	SIM_LOG_FUNC(BRR_LOG);

	//
	// source and destination does not forward the network discovery message
	//
	if (m_nodeType == SOURCE_NODE_TYPE || m_nodeType == DESTINATION_NODE_TYPE) {
		return;
	}

//	if (!m_coalition.empty()) {
//		if (m_service.prepare(ServiceMessType::NET_DISC, ServiceMessType::NONE)) m_f.ttl = -1;
//		return;
//	}
	if (m_service.admit(ServiceMessType::REP_NET_DISC)) {
		m_service.set_want_start_service(f.ttl > 0);
		if (m_service.init()) m_f.ttl = f.ttl - 1;
	}
}
void NcRoutingRules::ProcessReqRetrans(FeedbackInfo f) {
	SIM_LOG_FUNC(BRR_LOG);

	auto check = [this](FeedbackInfo f)->bool {

		if (f.ttl == 0)return false;

		if (m_id == m_dst) {
			//
			// check if the request is old
			//
			if (IsRetransRequestOld(f)) SetFastAck();
			return false;
		}

		if (DoICooperate(f.addr)) {
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Ignore RR from the node with priority " << f.p);
			//
			// check if the request is old
			//
			if (IsRetransRequestOld(f)) SetFastAck();
			return false;
		}

		assert(!f.rrInfo.empty());
		assert(f.ttl > 0);

		//
		// remove information already not present at this node
		//
		RefineFeedback(f);
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Remaining feedback is" << (f.rrInfo.empty() ? "" : " NOT") << " empty");
		if (f.rrInfo.empty()) {
			return false;
		}

		//
		// calculate the difference between the achievable rank
		// at the node requesting the retransmission if the current node retransmits available information
		// which is not present on the sender of the feedback yet
		//
		std::map<GenId, CoderHelpInfo> helpInfo;
		GetAchievableRank(f, helpInfo);

		//
		// remove already done RR
		//
		RefineCoderHelpInfo(helpInfo);
		//
		// update retransmission plan
		//
		DoUpdateRetransPlan(helpInfo);
		//
		// decide on RR replication
		//
		if (f.rrInfo.forwarder == m_id) {

			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "I am the specified forwarder");

			FormRrInfo(f, helpInfo);
			return (!m_f.rrInfo.empty());
		} else {

			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "I am not the specified forwarder " << m_f.rrInfo.forwarder);
			return false;
		}

		return false;
	};

	if (m_service.admit(ServiceMessType::REP_REQ_RETRANS)) {
		m_service.set_want_start_service(check(f) && m_nodeType != SOURCE_NODE_TYPE);
		if (m_service.init()) m_f.ttl = f.ttl - 1;
	}
}

bool NcRoutingRules::IsConnected() {
	return !(countTxopNetDisc++ >= m_sp.numTxopNetDisc);
}
void NcRoutingRules::SetConnected(bool v) {

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Set connected " << v);
	countTxopNetDisc = v ? 0 : m_sp.numTxopNetDisc;
	m_service.stop_if(ServiceMessType::NET_DISC);
	m_service.stop_if(ServiceMessType::REP_NET_DISC);
}
void NcRoutingRules::CheckReqRetrans(UanAddress id, GenId genId, bool all_prev_acked) {

	SIM_LOG_FUNC(BRR_LOG);

//	auto check = [this](UanAddress id, GenId genId, bool all_prev_acked)->bool {
////		//
////		// do not send RR if
////		// the sending vertex have all previous generations acked
////		//
////			if (all_prev_acked) {
////				if (GetActualRxWinSize() + m_sp.numGenRetrans >= m_sp.numGen) {
////					//
////					// send some data from the oldest generations
////					//
////					assert(!m_inRcvNum.empty());
////					auto gid = m_inRcvNum.begin_orig_order()->first;
////					SIM_LOG_NPG(1, m_id, m_p, genId, "Relay overshoots GID: " << gid);
////					Overshoot(gid);
////				}
////				SIM_LOG_NPD(1, m_id, m_p, m_dst, "Say NO to sending of RR. Previous are ACKed");
////				return false;
////			}
//
//			if (MayRepeatReqRetrans(id, genId)) {
//				return true;
//			} else {
//				return MayOrigReqRetrans(genId);
//			}
//
//			assert(0);
//			return false;
//		};

	if (m_service.admit(ServiceMessType::REQ_RETRANS)) {
		m_service.set_want_start_service(MayOrigReqRetrans(genId) && m_nodeType != SOURCE_NODE_TYPE);
		if (m_service.init()) m_f.ttl = m_maxTtl;
	}
}
bool NcRoutingRules::MayOrigReqRetrans(GenId genId) {

	SIM_LOG_FUNC(BRR_LOG);

	if (IsOverflowDanger()) {
		return DoCreateRetransRequest(genId);
	}

	return false;
}
//
//bool NcRoutingRules::MayRepeatReqRetrans(UanAddress id, GenId genId) {
//
//	SIM_LOG_FUNC(BRR_LOG);
//	//
//	// if there is no retransmission pending
//	//
//	if (m_oldestRetransGenId.is_default()) {
//		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Say NO to sending of RR. No pending request");
//		return false;
//	}
//
//	if (m_outputs.find(id) != m_outputs.end()) {
//		//
//		// if the sending node has higher priority
//		//
//		if (m_outputs.at(id) > m_p) {
//			SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Say NO to sending of RR. The sender with higher priority: " << m_outputs.at(id));
//			return false;
//		}
//	}
//	//
//	// if receiving request from not known vertex
//	//
//
//	else {
//		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Say NO to sending of RR. Not known sender");
//		return false;
//	}
//
//	//
//	// if receiving the symbols from older generation
//	// than the retransmission is (previously) requested
//	//
//
//	if (m_oldestRetransGenId.geq(genId)) {
//		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Say NO to sending of RR. Received correct symbol");
//		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Default the oldest generation ID " << m_oldestRetransGenId);
//		ResetRetransInfo();
//		return false;
//	}
//
//	SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Say YES to sending of RR " << m_oldestRetransGenId);
//	//	std::cout << "\t\t\t>>>> RR (replication) from " << m_id << ": ";
//	//	for(auto r : m_f.rrInfo)std::cout << r.first << ",";
//	//	std::cout << std::endl;
//	return true;
//}

void NcRoutingRules::ResetRetransInfo() {

	m_f.rrInfo.clear();
	m_oldestRetransGenId.set_default();
}

uint32_t NcRoutingRules::GetAmountTxData() {

	//
	// get the number of symbols that are already present in TX plan
	//
	uint32_t sum_tx = 0;
	for (auto item : m_txPlan) {
		sum_tx += item.second.num_all;
	}
	//
	// get maximum eligible to be sent from the view of BRR
	//
	uint32_t max_el_brr = floor(GetMaxAmountTxData() * m_cr);
	//
	// the source can generate additional data to reach the limit
	//
//	uint32_t act_tx = (m_nodeType == SOURCE_NODE_TYPE) ? max_el_brr : sum_tx;
	uint32_t act_tx = sum_tx;

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Rcv gen IDs: " << m_inRcvNum.print_ids());

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,
			"[sum_tx " << sum_tx << "],[max_el_brr " << max_el_brr << "],[act_tx " << act_tx << "],[res " << ((act_tx > max_el_brr) ? max_el_brr : act_tx) << "]");

	return (act_tx > max_el_brr) ? max_el_brr : act_tx;
}
uint32_t NcRoutingRules::GetGenBufSize(uint32_t maxPkts) {

	assert(m_nodeType == SOURCE_NODE_TYPE);

	//
	// obtain the last generation ID
	//
	GenId s_r = GetRxWinStart();

	//
	// obtain the earliest end of the ACK window between the vertices in the coalition and me
	//
	GenId e_ack = GetTxWinEnd();

	//
	// distance between s_f and e_ack
	//
	GenId max_el_brr = 0;
	if (s_r == MAX_GEN_SSN || e_ack == MAX_GEN_SSN) {
		max_el_brr = m_sp.numGen;
	} else {
		max_el_brr = (gen_ssn_t(s_r) < gen_ssn_t(e_ack)) ? gen_ssn_t::get_distance(s_r, e_ack) - 1 : 0;
	}
	max_el_brr *= m_sp.genSize;
	//
	// get maximum eligible to be sent from the view of MAC
	//
	uint32_t max_el_mac = maxPkts;
	//
	// get harder constaint
	//
	uint32_t max_el = (max_el_brr < max_el_mac) ? max_el_brr : max_el_mac;

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "[s_r " << s_r << "],[max_el_brr " << max_el_brr << "],[max_el_mac " << max_el_mac << "],[max_el " << max_el << "]");

	return max_el;
}

uint32_t NcRoutingRules::GetMaxAmountTxData() {

	//
	// obtain the earliest end of the ACK window between the vertices in the coalition and me
	//
	GenId e_ack = GetTxWinEnd();
	//
	// actual oldest generation in the forwarding plan
	//
	GenId s_f = GetTxWinStart();
	//
	// distance between s_f and e_ack
	//
	GenId dse = 0;
	if (e_ack != MAX_GEN_SSN && s_f != MAX_GEN_SSN) {
		dse = (gen_ssn_t(s_f) < gen_ssn_t(e_ack)) ? gen_ssn_t::get_distance(s_f, e_ack) : 0;
	}

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Before: [e_ack " << e_ack << "],[s_f " << s_f << "],[dse " << dse << "],[res " << dse * m_sp.genSize << "]");

	for (auto rcvNum : m_inRcvNum) {
		if (gen_ssn_t(rcvNum.first) >= gen_ssn_t(e_ack) || s_f == MAX_GEN_SSN) continue;
		if (gen_ssn_t(rcvNum.first) > gen_ssn_t(s_f + m_sp.numGenBuffering)) {
			if (m_forwardPlan.find(rcvNum.first) == m_forwardPlan.end()) {
				assert(dse > 1);
				dse--;
			}
		}
	};;

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "[e_ack " << e_ack << "],[s_f " << s_f << "],[dse " << dse << "],[res " << dse * m_sp.genSize << "]");

	return dse * m_sp.genSize;
}
priority_t NcRoutingRules::GetPriority() {
	return m_p;
}

double NcRoutingRules::GetInfoOnDsts() {

	FilterArithmetics arith;
	for (auto rm : m_outRcvMap) {
		if (m_outputs.at(rm.first) == DESTINATION_PRIORITY) arith.add(rm.second);
	}
	double a = 0;
	if (arith.check_sync()) a = 1 - arith.do_and().val_unrel();

	return a;
}
GenId NcRoutingRules::GetTxWinStart() {
	GenId s_f = (m_forwardPlan.empty()) ? MAX_GEN_SSN : m_forwardPlan.begin_orig_order()->first;
	if (s_f == MAX_GEN_SSN) s_f = m_forwardPlan.last_key_in_mem();
	if (s_f == MAX_GEN_SSN) {
		s_f = m_outdatedGens.empty() ? MAX_GEN_SSN : *(m_outdatedGens.last());
		s_f = (s_f == MAX_GEN_SSN) ? MAX_GEN_SSN : ((++gen_ssn_t(s_f)).val());
	}
	return s_f;
}
GenId NcRoutingRules::GetRxWinStart() {
	GenId s_r = (m_inRcvNum.empty()) ? MAX_GEN_SSN : m_inRcvNum.begin_orig_order()->first;

	if (s_r == MAX_GEN_SSN) {
		s_r = m_outdatedGens.empty() ? MAX_GEN_SSN : *(m_outdatedGens.last());
		s_r = (s_r == MAX_GEN_SSN) ? MAX_GEN_SSN : ((++gen_ssn_t(s_r)).val());
	}
	return s_r;
}
GenId NcRoutingRules::GetRxWinEnd() {

	GenId s_r = GetRxWinStart();
	GenId ack_win = m_sp.numGen;
	GenId oe_ack = (s_r == MAX_GEN_SSN) ? MAX_GEN_SSN : gen_ssn_t::rotate(s_r, ack_win);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "[s_r " << s_r << "],[ack_win " << ack_win << "],[oe_ack " << oe_ack << "]");

	return oe_ack;
}
GenId NcRoutingRules::GetTxWinEnd() {
	//
	// obtain the earliest end of the ACK window between the vertices in the coalition and me
	//
	GenId e_ack = MAX_GEN_SSN;
	for (auto i : m_coalition) {
		//
		// consider that the vertices in the coalition are sorted in the descending order of their priorities
		//
		auto addr = i.first;
		if (m_remoteRxWinEnd.find(addr) != m_remoteRxWinEnd.end()) {
			auto cg = m_remoteRxWinEnd.at(addr);
			assert(cg != MAX_GEN_SSN);
			if (gen_ssn_t(cg) > gen_ssn_t(e_ack) || e_ack == MAX_GEN_SSN) e_ack = cg;
		}
	}
	//
	// get own end of ACK window
	//
	GenId oe_ack = GetRxWinEnd();
	e_ack = (e_ack == MAX_GEN_SSN) ? oe_ack : e_ack;

	GenId s_ack = GetTxWinStart();
	if (gen_ssn_t(e_ack) < gen_ssn_t(s_ack)) e_ack = gen_ssn_t::rotate(s_ack, 1);
//	if (m_nodeType == SOURCE_NODE_TYPE) {
//		auto v = gen_ssn_t::rotate(s_ack, m_sp.numGen >> 2);
//		if (gen_ssn_t(e_ack) > gen_ssn_t(v)) e_ack = v;
//	}

	return e_ack;
}
GenId NcRoutingRules::GetActualRxWinSize() {

	return m_inRcvNum.size();
}
GenId NcRoutingRules::GetActualTxWinSize() {
	return m_txPlan.size();
}
bool NcRoutingRules::DoesItCooperate(UanAddress addr) {
	//return m_p < m_outputs.at(addr);
	return m_coalition.is_in(addr);
}
bool NcRoutingRules::DoICooperate(UanAddress addr) {
	// return m_p > m_outputs.at(addr);
	return (m_inF.find(addr) != m_inF.end());
}
void NcRoutingRules::SetSendingRate(Datarate d) {

	SIM_LOG_FUNC(BRR_LOG);
	//
	// check if the change in the sending data rate is significant
	//
	if (fabs(m_d - d) / d > 0.01) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Significant change in data rate: " << m_d << "<->" << d);
		m_d = d;
		m_logItem.d = m_d;
		if (m_addLog) m_addLog(m_logItem, m_id);
		Reset();
		UpdateCoalition();
	}

}
uint16_t NcRoutingRules::GetAckBacklogSize() {
	return m_outdatedGens.tiefe();
}
uint16_t NcRoutingRules::GetCoalitionSize() {
	return m_coalition.size();
}
double NcRoutingRules::GetCodingRate() {
	return m_cr;
}
void NcRoutingRules::AddToCoalition(UanAddress addr) {
	//
	// forcing to use this vertex in the coalition
	//
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "[FORCE] Adding to coalition: " << addr);
	m_inF[addr] = 0;
	m_outputs.add(addr, priority_t(m_p.val() * 2));
	m_outRcvMap[addr].fill(0);

	UpdateCoalition();
}
void NcRoutingRules::UpdateCoalition() {

	SIM_LOG_FUNC(BRR_LOG);

	DoUpdateCoalition();

	m_logItem.cs = m_coalition.size();
	if (m_addLog) m_addLog(m_logItem, m_id);
}
//////////////////////////////////////////////////////////////////////
void NcRoutingRules::DoFilter() {

	SIM_LOG_FUNC(BRR_LOG);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Node Type is " << m_nodeType);
	if (m_nodeType == DESTINATION_NODE_TYPE) return;

	m_filteredPlan.clear();
	//
	// for all generations
	//
	//	std::cout << "Generations in receiver buffer " << GetActualRxWinSize() << std::endl;
	for (RcvNum::iterator it = m_inRcvNum.begin(); it != m_inRcvNum.end(); it++) {

		auto genId = it->first;
		;
		if (it->second.empty()) {
			SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Nothing received yet");
			continue;
		}

		//
		// with a view to use the full power of recoding, each vertex should ensure before
		// starting recoding the symbols of the given generation that it will not receive any
		// more symbols of this generation;
		//
		// on source: start recoding when the generation has the full rank
		// on helpers: there must be n generations in the buffer before the recoding the earliest generation
		// of the earliest generation can start
		//
#ifdef FULL_VECTOR
		if (m_nodeType == SOURCE_NODE_TYPE) {
			if (m_getRank(genId) != m_sp.genSize) {
				SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Rank " << m_getRank(genId)
						<< ", size " << m_sp.genSize << " - not full. Skip..");
				continue;
			}
		}
		else {
			//			if (!m_inRcvNum.is_in_tail(m_sp.numGenBuffering, genId)) {
			//				SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,  "Generation " << genId << ", rank " << m_getRank(genId)
			//						<< ", size " << m_sp.genSize << ", number buffering "
			//						<< m_sp.numGenBuffering << " - not in tail. Skip..");
			//				continue;
			//			}
		}
#else
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Not using full vector encoding");
#endif

		//
		// for all nodes
		//
		double k_t = 0;
		for (NodeVarList::iterator itv = it->second.begin(); itv != it->second.end(); itv++) {

			//
			// if no filter exists then the received data is obtained from the node with greater or equal priority
			// and it does not have to be forwarded
			//
			if (m_inF.find(itv->first) == m_inF.end()) {
				SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "No filter exists for node with ID " << itv->first);
				continue;
			}
			if (m_id != itv->first) {

				assert(m_outputs.find(itv->first) != m_outputs.end());
				if (m_outputs.at(itv->first) >= m_p) {
					SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Ignore edge with high priority sink node. ID " << itv->first);
					continue;
				}
			}
			double k = itv->second * (1 - m_inF.at(itv->first));
			k_t += k;

			SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId,
					"sender " << itv->first << ", filtering probability " << m_inF.at(itv->first) << ", received " << itv->second << ", to forward " << k);
		}

		m_filteredPlan[genId] = k_t;
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Filtered to forward " << k_t);
	}

	DoUpdateForwardPlan();
}
void NcRoutingRules::DoUpdateFilter() {

	SIM_LOG_FUNC(BRR_LOG);

	FilterArithmetics arith;
	m_outF.clear();

	for (node_map_it it = m_outputs.begin(); it != m_outputs.end(); it++) {
		if (m_coalition.find(it->first) == m_coalition.end()) {
			m_outF[it->first] = 1;
		}
	}

	for (node_map_it it = m_coalition.begin(); it != m_coalition.end(); it++) {
		double e = 1;
		//
		// e is always zero for the sink node of the first edge in the coalition
		//
		if (arith.check_sync()) e = arith.do_and().val_unrel();
		m_outF[it->first] = 1 - e;
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Address " << it->first << ", filter probability " << m_outF[it->first]);
		//
		// if no receiving map is present consider the link without losses
		// which means that the remaining nodes have to drop all received data
		//
		if (m_outRcvMap.find(it->first) == m_outRcvMap.end()) break;
		arith.add(m_outRcvMap.at(it->first));
	}
	m_h.pf = m_outF;

}
void NcRoutingRules::DoCalcRedundancy() {

	SIM_LOG_FUNC(BRR_LOG);

	FilterArithmetics arith;
	for (node_map_it it = m_coalition.begin(); it != m_coalition.end(); it++) {
		arith.add(m_outRcvMap[it->first]);
	}
	if (arith.check_sync()) {

		double eps = arith.do_and().val_unrel();

		switch (m_sp.crCalcWay) {
		case EXACT_EXPECTATION_REDANDANCY: {
			m_cr = 1 / (1 - eps);
			break;
		}
		case PLUS_SIGMA_REDUNDANCY: {
			double numStd = m_sp.crNumSigma;
			double gamma = pow(numStd, 2) * eps / (double) m_sp.genSize / 4.0;
			gamma = (gamma > 1) ? 1 : gamma;
			double alpha = 1 + 2 * sqrt(gamma * (gamma + 1));

			m_cr = alpha / (1 - eps);
			break;
		}
		case MINUS_DELTA_REDUNDANCY: {
			m_cr = 1 + (1 / (1 - eps) - 1) * m_sp.crReducFactor;
			break;
		}
		default: {
			assert(0);
		}
		}

		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "" << 1/ (1 - eps) << "\t" << m_cr);
	} else {
		m_cr = 1;
	}

	assert(geq(m_cr, 1));

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "coding rate " << m_cr);

	m_logItem.cr = m_cr;
	if (m_addLog) m_addLog(m_logItem, m_id);

}
void NcRoutingRules::DoUpdateTxPlan() {

	SIM_LOG_FUNC(BRR_LOG);

	if (m_nodeType == DESTINATION_NODE_TYPE) return;

	m_txPlan.clear();

	auto it = m_forwardPlan.begin();
	for (; it != m_forwardPlan.end(); it++) {

		auto genId = it->first;
		auto toForward = it->second;

		if (eq(toForward, 0)) {
			SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Nothing to send");
			continue;
		}
		//
		// find number of symbols to send including redundancy
		//
		m_txPlan[genId].num_all = ceil(toForward * m_cr);
		//		double p = toForward * m_cr - m_txPlan[genId].num_all;
		//		if (m_dis(m_gen) < p) m_txPlan[genId].num_all++;

		//		//
		//		// define the position of each redundant packet
		//		//
		//		m_txPlan[genId].placement.resize(m_txPlan[genId].num_all - toForward, 0);
		//		std::uniform_int_distribution<> dis(0, m_txPlan[genId].placement.size() - 1);
		//		uint16_t i = 0;
		//		while (i++ != m_txPlan[genId].num_all) {
		//			auto index = dis(m_gen);;
		//			assert(index < m_txPlan[genId].placement.size());
		//			m_txPlan[genId].placement.at(index)++;
		//		}
		//		//
		//		// check check
		//		//
		//		auto v = m_txPlan[genId].placement;;
		//		assert(m_txPlan[genId].num_all == std::accumulate(v.begin(), v.end(), 0));
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "TX number " << m_txPlan[genId].num_all);
	}

	//
	// mark TX plan if all previous generations are acked
	//
	bool v = true;
	for (auto r : m_inRcvNum) {
		auto genId = r.first;

		if (m_txPlan.find(genId) == m_txPlan.end()) if (!m_outdatedGens.is_in(genId)) {
			v = false;
			break;
		}
	};;
	if (v) {
		for (auto &t : m_txPlan)
			t.second.all_prev_acked = true;

		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "All previous generations are acked");
	}
}
void NcRoutingRules::DoUpdatePriority(node_map_t outputs) {

	SIM_LOG_FUNC(BRR_LOG);
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Update priority with coalition " << outputs);

	if (m_dst == m_id) {
		m_p = DESTINATION_PRIORITY;
		return;
	}

	if (outputs.empty()) {
		m_p = m_thresholdP;
		return;
	}

	auto a = [&]()->double
	{
		FilterArithmetics arith;
		double prod_e = 1;
		for (node_map_it it = outputs.begin(); it != outputs.end(); it++)
		{
			if(m_outRcvMap[it->first].is_ready())
			{
				arith.add(m_outRcvMap.at(it->first));
				SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "it->first " << it->first << ", m_outRcvMap.at(it->first) " << m_outRcvMap.at(it->first).val_unrel());
			}
			else
			{
				prod_e *= m_outRcvMap.at(it->first).val_unrel();
				SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "it->first " << it->first << ", prod_e " << prod_e);
			}
		}
		double v = (arith.check_sync()) ? arith.do_and().val_unrel() : 1;

		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "a coefficient: prod_e = " << prod_e << ", v = "
				<< v << ", m_d = " << m_d);
		return m_d * (1 - prod_e * v);
	};
	;

	auto b = [&](UanAddress u)->double
	{
		if(u == m_dst)return 0;

		FilterArithmetics arith;
		double prod_e = 1;
		for (node_map_it it = outputs.begin(); it != outputs.end(); it++)
		{
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Try for node " << it->first << ", requested " << u);
			if(it->first == u)break;
			SIM_ASSERT_MSG(it->second >= outputs.at(u), "" << it->second << " < " << outputs.at(u));
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Use for node " << it->first << ", requested " << u);
			if(m_outRcvMap[it->first].is_ready())
			{
				arith.add(m_outRcvMap.at(it->first));
			}
			else
			{
				prod_e *= m_outRcvMap.at(it->first).val_unrel();
			}
		}
		double v = (arith.check_sync()) ? arith.do_and().val_unrel() : 1;

		double loss_on_e = (m_outRcvMap.find(u) == m_outRcvMap.end()) ? 0 : m_outRcvMap.at(u).val_unrel();
		double p = m_d * (1 - loss_on_e) * prod_e * v;
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "output node " << u << ", b coefficient: prod_e = " << prod_e << ", loss on e = "
				<< loss_on_e << ", v = " << v << ", m_d = " << m_d << ", p = " << p);
		//		assert(geq(p,0.0));
			return p;
		};
	;

	double denominator = 1;
	for (node_map_it it = outputs.begin(); it != outputs.end(); it++) {

//		assert(m_p < it->second);
		assert(it->second > m_thresholdP);

		assert(!eq(it->second.val(), 0));

		double bb = b(it->first);
		denominator += bb / it->second;
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "sink node " << it->first << ", priority " << it->second << ", b coefficient " << bb);
	}

	double aa = a();
	double p = m_p.val();
	assert(!eq(denominator, 0));
	m_p = aa / denominator;
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "a coefficient: " << aa << ", old priority: " << p);

	m_h.p = m_p;
	m_f.p = m_p;
	m_logItem.p = m_p;
	if (m_addLog) m_addLog(m_logItem, m_id);
}
void NcRoutingRules::DoUpdateCoalition() {

	SIM_LOG_FUNC(BRR_LOG);

	auto p_temp = m_p;
	auto coalition_temp = m_coalition;

	m_coalition.clear();
	if (m_dst == m_id) {
		return;
	}
	double old_p = 0;

	//
	// verify the order
	//
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "using output edges " << m_outputs);
	if (m_outputs.empty()) {
		//		m_p = m_d;
		return;
	}
	node_map_it it = m_outputs.begin();
	it++;
	for (node_map_it itt = m_outputs.begin(); it != m_outputs.end(); itt++) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "check " << itt->second << " => " << it->second);
		assert(itt->second >= it->second);
		it++;
	}

	////////////////////////////////////////
	m_p = m_thresholdP;
	if (m_p < m_outputs.begin()->second && m_outputs.begin()->second > m_thresholdP) {
		m_coalition.add(*m_outputs.begin());
		DoUpdatePriority(m_coalition);
	} else {
		return;
	}
	//
	// do the job
	//
	for (node_map_it it = m_outputs.begin() + 1; it != m_outputs.end(); it++) {

		//		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,  "(it->second > m_p) " << (it->second > m_p));
		//		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,  "(it->second > m_thresholdP) " << (it->second > m_thresholdP));
		if (m_p < it->second && it->second > m_thresholdP) {
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,
					"adding edge to coalition. Sink node ID " << it->first << ", priority " << it->second << ", threshold " << m_thresholdP);

			m_coalition.add(*it);

			DoUpdatePriority(m_coalition);
			if (m_p < priority_t(old_p) || m_p.val() - old_p < m_b * m_p.val()) {
				m_coalition.remove(*it);
				break;
			}
			old_p = m_p.val();
		} else {
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,
					"NOT adding edge to coalition. Sink node ID " << it->first << ", priority " << it->second << ", threshold " << m_thresholdP);
		}
	}
	////////////////////////////////////////
//
//	if(m_p.val() - p_temp.val() < m_b * m_p.val())
//	{
//		m_p = p_temp;
//		m_coalition = coalition_temp;
//	}

//	//
//	// do the job
//	//
//	for (node_map_it it = m_outputs.begin(); it != m_outputs.end(); it++) {
//
//		//		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,  "(it->second > m_p) " << (it->second > m_p));
//		//		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,  "(it->second > m_thresholdP) " << (it->second > m_thresholdP));
//		if ((it->second > m_p) && (it->second > m_thresholdP)) {
//			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,
//					"adding edge to coalition. Sink node ID " << it->first << ", priority " << it->second << ", threshold " << m_thresholdP);
//
//			m_coalition.add(*it);
//
//			DoUpdatePriority(m_coalition);
//			if (m_p.val() - old_p < m_b * m_p.val()) {
//				m_coalition.remove(*it);
//				break;
//			}
//			old_p = m_p.val();
//		}
//		else {
//			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,
//					"NOT adding edge to coalition. Sink node ID " << it->first << ", priority " << it->second << ", threshold " << m_thresholdP);
//		}
//	}

	if (m_coalition.size() > m_sp.maxCoalitionSize) m_coalition.resize(m_sp.maxCoalitionSize);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Coalition " << m_coalition);
}
void NcRoutingRules::DoUpdateForwardPlan() {

	m_forwardPlan.clear();

	for (auto p : m_filteredPlan) {
		m_forwardPlan[p.first] = p.second;
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, p.first, "Add filtered " << p.second);
	};;

	for (auto p : m_retransPlan) {
		m_forwardPlan[p.first] += p.second;
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, p.first, "Add requested to retransmit " << p.second);
	};;

	assert(m_getRank);
	for (auto &p : m_forwardPlan) {
		auto genId = p.first;
		uint32_t r = m_getRank(genId);
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Limit to rank " << r);
		p.second = (p.second > r) ? r : p.second;
	};;

	for (auto s : m_sentNum) {
		auto genId = s.first;
		auto s_t = (double) s.second / m_cr;
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, s.first, "Already sent " << s.second << " coding rate " << m_cr);
		m_forwardPlan[genId] = (s_t >= m_forwardPlan[genId]) ? 0 : (m_forwardPlan[genId] - s_t);
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "To forward " << m_forwardPlan[genId]);
		if (eq(m_forwardPlan[genId], 0)) m_forwardPlan.erase(genId);
	};;

#ifdef BUFFERING_AT_HELPERS

	CheckBuffering();

#endif

	DoUpdateTxPlan();
}
void NcRoutingRules::DoForgetGeneration() {

	SIM_LOG_FUNC(BRR_LOG);
	//
	// remove the ACKed generations in the front
	//
	std::vector<GenId> ids;
	auto it = m_inRcvNum.begin_orig_order();
	while (it != m_inRcvNum.end()) {
		if (m_outdatedGens.is_in(it->first)) {
			ids.push_back(it->first);
		} else {
			break;
		}
		it = m_inRcvNum.next_orig_order(it);
	}
	for (auto id : ids) {
		DoForgetGeneration(id);
	}

	//
	// remove the oldest generations if the number of generations in the receive buffer
	// exceeds the ACK window size
	//
	while (GetActualRxWinSize() > m_sp.numGen) {
		auto it = m_inRcvNum.begin_orig_order();
		assert(it != m_inRcvNum.end());

		DoForgetGeneration(it->first);
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Erasing generation " << it->first << " due to overflow");
	}
}
void NcRoutingRules::DoForgetGeneration(GenId genId) {

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Erasing generation " << genId << (m_outdatedGens.is_in(genId) ? " acked" : " not acked"));
	if (!m_outdatedGens.is_in(genId)) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Erasing generation " << genId << " not acked");
	}

//	if (m_id == 0) std::cout << "Node " << m_id << ". Forgetting generation " << genId << " after ACK " << std::endl;

//	if (m_id == m_dst && m_getRank(genId) != 0) {
//		SIM_ASSERT_MSG(m_getRank(genId) == m_sp.genSize, "Erasing generation " << genId << " with rank " << m_getRank(genId));
//	}

	m_forwardPlan.erase(genId);
	if (m_txPlan.find(genId) != m_txPlan.end()) {
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Dropping " << m_txPlan.at(genId).num_all);
		m_txPlan.erase(genId);
	}
	m_retransPlan.erase(genId);
	m_filteredPlan.erase(genId);
	m_inRcvNum.erase(genId);
	m_sentNum.erase(genId);
	m_ccack.erase(genId);
	m_numRr->forget(genId);
	softAckInfo.erase(genId);

	m_f.ackInfo.rxWinEnd = GetRxWinEnd();
}

void NcRoutingRules::PlanForgetGeneration(GenId gid) {
	if (!m_outdatedGens.is_in(gid)) {
		m_outdatedGensInform.add(gid);
		m_outdatedGens.add(gid);
	}
}

UanAddress NcRoutingRules::SelectRetransRequestForwarder() {

	SIM_LOG_FUNC(BRR_LOG);

	std::vector<UanAddress> candidates;
	std::vector<double> costs;

	for (auto v : m_inRcvMap) {
		auto id = v.first;
		if (m_id == id) continue;
		//		assert(m_outputs.find(id) != m_outputs.end());
		if (m_outputs.find(id) == m_outputs.end()) continue;
		if (m_outputs.at(id) > m_p) continue;

		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Adding forwarder candidate " << id);
		candidates.push_back(id);
		costs.push_back(v.second.val_unrel());
	};;

	if (candidates.empty()) return 0;

	if (m_sp.giveRrPriorToSrc) {
		//
		// select no forwarder if the source is in the list of candidates
		//
		if (std::find(candidates.begin(), candidates.end(), SRC_UANADDRESS) != candidates.end()) return SRC_UANADDRESS;
	}

	UanAddress id = *(candidates.begin());

	switch (m_sp.rrCanSel) {
	case RANDOM_RETRANSMITTER_RR_CANDIDATE_SELECTION: {
		std::uniform_int_distribution<> dis(0, candidates.size() - 1);
		auto c = dis(m_gen);
		;
		id = candidates.at(c);
		break;
	}
	case CONNECTION_QUALITY_RR_CANDIDATE_SELECTION: {
		double v = std::numeric_limits<double>::max();
		for (auto c : candidates) {
			auto vv = m_inRcvMap[c].val_unrel();
			if (vv < v) {
				v = vv;
				id = c;
			}
		};;
		break;
	}
	case LIN_DEP_FREQ_RR_CANDIDATE_SELECTION: {
		double v = 0;
		for (auto c : candidates) {
			auto vv = m_inLinDepMap[c].val_unrel();
			//		std::cout << c << " - lin dep - " << vv << std::endl;
			if (vv > v) {
				v = vv;
				id = c;
			}
		};;
		break;
	}
	case LIN_DEP_FREQ_RAND_RR_CANDIDATE_SELECTION: {
		double s = 0;
		for (auto c : candidates)
			s += m_inLinDepMap[c].val_unrel();

		std::uniform_real_distribution<> dis(0, s);
		auto v = dis(m_gen);
		;

		s = 0;
		for (auto c : candidates) {
			s += m_inLinDepMap[c].val_unrel();
			if (v < s) {
				id = c;
				break;
			}
		};;
		break;
	}
	case HIHGEST_PRIORITY_RR_CANDIDATE_SELECTION: {
		priority_t v;
		for (auto c : candidates) {
			auto vv = m_outputs.at(c);
			if (vv > v) {
				v = vv;
				id = c;
			}
		};;
		break;
	}
	default: {
		assert(0);
	}
	}

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Selected forwarder " << id);

	return id;
}

void NcRoutingRules::RefineFeedback(FeedbackInfo &fb) {

	SIM_LOG_FUNC(BRR_LOG);

	for (RetransRequestInfo::iterator it = fb.rrInfo.begin(); it != fb.rrInfo.end(); it++) {
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, it->first, "In feedback");
	}

	auto it = m_inRcvNum.begin();
	for (; it != m_inRcvNum.end(); it++) {
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, it->first, "I have locally");
	}

	std::vector<GenId> ids;
	for (auto d : fb.rrInfo) {

		auto genId = d.first;

		//
		// check if genId is old
		//
		if (!m_inRcvNum.empty()) {
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "CMP " << genId << " and " << m_inRcvNum.begin_orig_order()->first);
			if (gen_ssn_t(genId) < gen_ssn_t(m_inRcvNum.begin_orig_order()->first)) {
				assert(m_inRcvNum.find(genId) == m_inRcvNum.end());
				ids.push_back(genId);
				SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "OLD: Erase generation info");
			}
		}
		//
		// check if the generation was already ACKed
		//
		if (m_outdatedGens.is_in(genId)) {
			ids.push_back(genId);
			SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "ACKed: Erase generation info");
		}
	};;

	for (auto id : ids) {
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, id, "Actual erasing");
		fb.rrInfo.erase(id);
		;
	};;
}
void NcRoutingRules::GetAchievableRank(FeedbackInfo fb, std::map<GenId, CoderHelpInfo> &helpInfo) {

	SIM_LOG_FUNC(BRR_LOG);

	for (auto inf : fb.rrInfo) {

		auto genId = inf.first;
		helpInfo[genId] = m_getCoderHelpInfo(inf.second.codingCoefs, inf.second.coderInfo, genId);
		if (!fb.rrInfo[genId].hashVector.empty()) {
			assert(m_hashMatrixSet);
			if (m_ccack.find(genId) != m_ccack.end()) {
				//
				// here the origRank changes the meaning in comparison to the feedback estimator and
				// full coding matrix feedback; now it is not the original rank of the feedback sender
				// but the current rank of the current vertex;
				// nevertheless, the meaning of the difference between origRank and finRank is pertained
				//
				helpInfo[genId].origRank = m_ccack[genId]->GetHeardSymbNum();
				m_ccack[genId]->RcvHashVector(fb.rrInfo[genId].hashVector);
				helpInfo[genId].finRank = m_ccack[genId]->GetHeardSymbNum();
				if (m_nodeType == SOURCE_NODE_TYPE) helpInfo[genId].finRank = m_sp.genSize;
				m_ccack[genId]->Reset();
			}
		}
	}

	uint32_t checkGenNum = helpInfo.size();
	uint16_t finSumRank = 0, origSumRank = 0;

	for (auto inf : helpInfo) {
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, inf.first, "FIN rank: " << inf.second.finRank << ", ORIG rank: " << inf.second.origRank);
		origSumRank += inf.second.origRank;
		finSumRank += inf.second.finRank;
	};;

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,
			"Consider the first " << checkGenNum << " generations. FIN sum rank: " << finSumRank << ", ORIG sum rank: " << origSumRank << ", generation size: " << m_sp.genSize);

	assert(origSumRank != m_sp.genSize * checkGenNum);
}

void NcRoutingRules::DoUpdateRetransPlan(std::map<GenId, CoderHelpInfo> helpInfo) {

	SIM_LOG_FUNC(BRR_LOG);

	if (helpInfo.empty()) return;

	for (auto inf : helpInfo) {
		auto genId = inf.first;

		switch (m_sp.rrCanSend) {
		case ALL_WHO_HEAR_LEGAL: {
			m_retransPlan[genId] = (inf.second.finRank - inf.second.origRank);
			break;
		}
		case ONE_SELECTED_LEGAL: {
			m_retransPlan[genId] = (inf.second.finRank - inf.second.origRank);
			break;
		}
		default: {
			assert(0);
		}
		}
		if (m_inRcvNum.find(genId) != m_inRcvNum.end()) for (auto &r : m_inRcvNum[genId])
			r.second = 0;
		m_sentNum[genId] = 0;
		m_filteredPlan[genId] = 0;
		if (m_forwardPlan.find(genId) != m_forwardPlan.end()) m_forwardPlan.erase(genId);
		assert(!m_outdatedGens.is_in(genId));

		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId,
				"FIN rank " << inf.second.finRank << ", ORIG rank " << inf.second.origRank << ", remaining to retransmit " << m_retransPlan[genId]);
	};;

	DoUpdateForwardPlan();
}

void NcRoutingRules::ProcessAcks(FeedbackInfo l) {

	SIM_LOG_FUNC(BRR_LOG);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Rcvd ACKs 1 " << l.ackInfo);

	//
	// 1. FORGET GENERATIONS
	//

	GenId oldestGenId = gen_ssn_t::rotate_back(l.ackInfo.rxWinEnd, m_sp.numGen + 1);
	GenId oldestMentionedGenId = MAX_GEN_SSN;

	if (!l.ackInfo.empty()) {

		assert(l.ackInfo.begin() != l.ackInfo.end());

		bool continues_ack = true;
		for (auto a : l.ackInfo) {

			auto genId = a.first;
			SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, (a.second ? "Receive ACK" : "Received NACK") << " from " << l.addr);

			oldestMentionedGenId = (oldestMentionedGenId == MAX_GEN_SSN) ? genId : oldestMentionedGenId;
			oldestMentionedGenId = (gen_ssn_t(genId) > gen_ssn_t(oldestMentionedGenId)) ? genId : oldestMentionedGenId;

			if (a.second) {
				PlanForgetGeneration(genId);
				DoForgetGeneration(genId);
				oldestGenId = (gen_ssn_t(genId) > gen_ssn_t(oldestGenId) && continues_ack) ? genId : oldestGenId;
			} else {
				continues_ack = false;
			}
		};;
	}

	//
	// forget all generations before the oldest one
	//
	std::vector<GenId> ids;
	for (auto r : m_inRcvNum) {
		auto genId = r.first;
		if (gen_ssn_t(oldestGenId) > gen_ssn_t(genId)) {
			ids.push_back(genId);
			PlanForgetGeneration(genId);
			SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Oldest generation " << oldestGenId << ", forget " << genId);
		}
	};;

	for (auto id : ids) {
		DoForgetGeneration(id);
	}

	if (l.ackInfo.rxWinEnd != MAX_GEN_SSN) m_remoteRxWinEnd[l.addr] = l.ackInfo.rxWinEnd;

	//
	// 2. RECOVER BIG GROUP LOSSES
	//

	for (auto r : m_inRcvNum) {
		auto genId = r.first;
		if (gen_ssn_t(genId) > gen_ssn_t(oldestMentionedGenId)) {
			m_sentNum[genId] = 0;
		}
	}

//	//
//	// 3. OVERSHOOT
//	//
//
//	//
//	// get the oldest NACKed generation (not counting the last m_sp.numGenBuffering)
//	//
//	uint16_t s = (l.ackInfo.size() > m_sp.numGenRetrans) ? l.ackInfo.size() - m_sp.numGenRetrans : 0;
//	GenId oldest_nacked = MAX_GEN_SSN;
//	for (auto a : l.ackInfo) {
//		if (!a.second) {
//			oldest_nacked = a.first;
//			break;
//		}
//		if (--s == 0) break;
//	}
//
//	//
//	// check if it is on the begin of the ACK window of the ACK sender
//	//
//	auto ack_win_b = gen_ssn_t::rotate_back(l.ackInfo.rxWinEnd, m_sp.numGen >> 1);
//	bool in_rr_win = (gen_ssn_t(oldest_nacked) <= gen_ssn_t(ack_win_b));
//
//	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst,
//			"[oldest_nacked " << oldest_nacked << "],[m_getRank(oldest_nacked) " << m_getRank(oldest_nacked) << "]" ",[ack_win_b " << ack_win_b << "]");
//	if (m_getRank(oldest_nacked) == m_sp.genSize && in_rr_win && m_inRcvNum.find(oldest_nacked) != m_inRcvNum.end()) {
//
//		Overshoot(oldest_nacked);
//	}

	if (m_addLog) m_addLog(m_logItem, m_id);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Receive ACK " << l.ackInfo);
}

void NcRoutingRules::SetAcks() {

	SIM_LOG_FUNC(BRR_LOG);

	ClearAcks();

	auto ack = [this](GenId genId)
	{
		//
		// source does not acknowledge itself
		//
			m_f.ackInfo[genId] = (m_nodeType == SOURCE_NODE_TYPE) ? false : (m_getRank(genId) == m_sp.genSize);
			if(m_nodeType == DESTINATION_NODE_TYPE && m_f.ackInfo[genId])PlanForgetGeneration(genId);
			SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, (m_f.ackInfo[genId] ? "Set ACK" : "Set NACK"));
		};

	for (auto r : m_inRcvNum)
		ack(r.first);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Set ACKs of rcvd " << m_f.ackInfo);

	for (auto r : m_forwardPlan)
		ack(r.first);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Set ACKs of rcvd+fwrd " << m_f.ackInfo);

	auto acked = m_outdatedGens.get_last(m_sp.numGen);
	for (auto genId : acked) {
		m_f.ackInfo[genId] = true;
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Set ACK");
	}

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Set ACKs of rcvd+fwrd+bckl " << m_f.ackInfo);

	if (m_nodeType == DESTINATION_NODE_TYPE) {

		DoForgetGeneration();
	}
	m_f.ackInfo.rxWinEnd = GetRxWinEnd();
}

void NcRoutingRules::ClearAcks() {

	SIM_LOG_FUNC(BRR_LOG);
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Clear ACKs");
	m_f.ackInfo.clear();
}

void NcRoutingRules::EvaluateSoftAck() {

	SIM_LOG_FUNC(BRR_LOG);

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "IN: " << m_inRcvNum);
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "OUT: " << m_outRcvNum);

	//
	// 1. Estimate the number of coded packets that will be forwarded by the cooperating nodes
	//
	std::map<GenId, double> r;
	//
	// for each generation in the receive buffer
	//
	for (auto rcv_buffer : m_inRcvNum) {

		auto gid = rcv_buffer.first;
		//
		// find cooperating nodes that sent the feedback
		//
		std::vector<UanAddress> neighbors;
		for (auto i : m_coalition) {
			UanAddress id = i.first;
			auto it = m_outRcvNum.find(gid);
			if (it != m_outRcvNum.end()) {
				auto itt = it->second.find(id);
				if (itt != it->second.end()) {
					neighbors.push_back(id);
					r[gid] += itt->second * (1 - m_outF[id]);
					SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "Neighbor " << id << " RCVD " << itt->second << ", pf " << m_outF[id]);
				}
			}
		}
	}

	//
	// 2. Find the number of coded packets that the cooperating nodes should forward on average
	//    with a view to transfer the complete message sent by the current node
	//
	std::map<GenId, double> s;
	//
	// for each generation in the receive buffer, for which at least one feedback from the
	// cooperating nodes was received
	//
	for (auto v : r) {
		auto gid = v.first;
		auto it = m_sentNum.find(gid);
		// this gid MUST be in the sender buffer since it is present in the receiver buffer
		assert(it != m_sentNum.end());
		s[gid] = it->second / m_cr;
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "SENT " << it->second << ", cr " << m_cr);
	}

	for (auto v : r) {
		auto gid = v.first;
		softAckInfo[gid] = (r[gid] > s[gid] * m_sp.softAckDecision);
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid,
				"RCVD " << r[gid] << ", SENT " << s[gid] << ", SENT* " << s[gid] * m_sp.softAckDecision << ", ACK " << softAckInfo[gid]);
	}
}
uint32_t NcRoutingRules::GetRegularFeedbackFreq() {
	return (m_sp.genSize - 1); // [m_cr * (m_sp.genSize - 1)] / m_cr;
}
bool NcRoutingRules::IsSoftAck(GenId gid) {

	return softAckInfo[gid];
}
bool NcRoutingRules::IsHardAck(GenId gid) {
	return (m_f.ackInfo[gid] || m_outdatedGens.is_in(gid));
}
bool NcRoutingRules::HaveAcksToSend() {

	SIM_LOG_FUNC(BRR_LOG);

	SetAcks();

	return !m_outdatedGensInform.empty();
}

bool NcRoutingRules::HaveDataForRetransmissions() {

	SIM_LOG_FUNC(BRR_LOG);

	bool v = false;
	for (auto t : m_txPlan) {
		auto genId = t.first;
		if (m_oldestRetransGenId.geq(genId)) {
			v = true;
		}
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "RR oldest generation: " << m_oldestRetransGenId << " older ? " << m_oldestRetransGenId.geq(genId));
	}
	if (m_txPlan.empty()) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "TX plan is empty");
	}
	if (!v) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Have no actual data to transmit (in accordance with RR)");
		return false;
	}
	return true;
}

bool NcRoutingRules::IsRetransRequestOld(FeedbackInfo fb) {

	SIM_LOG_FUNC(BRR_LOG);
	//
	// find newest generation in RR
	//
	GenId ogid = fb.rrInfo.begin()->first;
	for (auto r : fb.rrInfo) {
		if (gen_ssn_t(ogid) < gen_ssn_t(r.first)) ogid = r.first;
	};;
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Newest generation in RR: " << ogid);
	//
	// check if it is older than any present local
	//
	bool older = true;
	for (auto r : m_inRcvNum) {
		if (gen_ssn_t(ogid) > gen_ssn_t(r.first)) older = false;
	};;
	if (!older) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "It is NOT older than generation locally");
		//
		// check if any older local generations are not acknowledged yet
		//
		bool acked = true;
		for (auto r : m_inRcvNum) {
			if (gen_ssn_t(ogid) > gen_ssn_t(r.first)) if (!m_outdatedGens.is_in(r.first)) acked = false;
		};;
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, (acked ? "All" : "Not all")<< " older local generations are ACKed");
		return acked;
	} else {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "It is older than any local generation");
		return false;
	}

	assert(0);
}

void NcRoutingRules::SetFastAck() {

	SIM_LOG_FUNC(BRR_LOG);

	SetAcks();
	m_fastFeedback = true;
}

void NcRoutingRules::FormRrInfo(FeedbackInfo fb, std::map<GenId, CoderHelpInfo> helpInfo) {

	SIM_LOG_FUNC(BRR_LOG);

	assert(!fb.rrInfo.empty());

	ResetRetransInfo();

	std::vector<GenId> ids;
	for (auto inf : fb.rrInfo) {
		auto genId = inf.first;

		if (helpInfo.find(genId) != helpInfo.end()) {
			if (helpInfo.at(genId).finRank != m_sp.genSize) {
				m_f.rrInfo[genId] = helpInfo.at(genId);
				m_oldestRetransGenId.set_if_older(genId);
				SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Adding modified info");
			} else {
				//
				// in this case RR is not forwarded; thus, no RR info is needed
				//
				SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Adding no info");
			}
		} else {
			m_f.rrInfo[genId] = fb.rrInfo[genId];
			m_oldestRetransGenId.set_if_older(genId);
			SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Adding original info");
		}
	};;

	if (m_f.rrInfo.empty()) {
		m_oldestRetransGenId.set_default();
	} else {
		m_f.rrInfo.forwarder = SelectRetransRequestForwarder();
	}

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Oldest generation ID to retransmit " << m_oldestRetransGenId);
}
void NcRoutingRules::RefineCoderHelpInfo(std::map<GenId, CoderHelpInfo> &helpInfo) {

	SIM_LOG_FUNC(BRR_LOG);

	if (helpInfo.empty()) return;

	std::vector<GenId> ids;
	for (auto inf : helpInfo) {
		auto genId = inf.first;

		//
		// surely send RR if I have the full rank
		//
		if (m_getRank(genId) == m_sp.genSize) {
			continue;
		}

		//
		// the forwarding plan may not have the entries corresponding to the retransmission plan
		// if the buffering is activated
		//
		if (m_forwardPlan.find(genId) == m_forwardPlan.end()) {
			//
			// no retransmissions, no RR replication
			//
			ids.push_back(genId);
			continue;
		}

		auto rr = (m_retransPlan.find(genId) != m_retransPlan.end());
		auto tx = rr ? (!eq(m_forwardPlan.at(genId), 0)) : false;
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "RR: " << rr << " Transmitting: " << tx);
		//
		// if RR was already requested but not all data was retransmitted
		//
		if (rr && tx) {
			//
			// no additional retransmissions, no RR replication
			//
			ids.push_back(genId);
			continue;
		}
		//
		// if RR was already requested and all data was retransmitted
		//
		if (rr && !tx) {
			//
			// no additional retransmissions, replicate RR
			//
			ids.push_back(genId);
		}
	};;

	for (auto id : ids)
		helpInfo.erase(id);
	;
}
void NcRoutingRules::CreateRetransRequest() {
	//
	// here we force attaching the RR info
	//
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Rcv buffer: " << m_inRcvNum.print_ids());

	if (m_inRcvNum.empty()) return;

	GenId genId = m_inRcvNum.last_orig_order()->first;

	DoCreateRetransRequest(genId);
}

bool NcRoutingRules::DoCreateRetransRequest(GenId newGid) {
	SIM_LOG_FUNC(BRR_LOG);

	auto func = [this](GenId gid)->bool
	{
		if (m_numRr->is_expired(gid)) {
			SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "RR counter is expired");
			return false;
		} else {
			SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "RR counter is NOT expired yet");
			m_numRr->increment(gid);
		}

		assert(m_getRank);
		auto r = m_getRank(gid);
		if(r == 0)return false;

		if (r < m_sp.genSize) {

			m_f.rrInfo[gid].codingCoefs.clear();
			m_f.rrInfo[gid].hashVector.clear();
			switch (m_sp.fbCont) {
				case ALL_VECTORS_FEEDBACK_ART: {
					assert(m_getCodingMatrix);
					SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "Adding coding matrix with rank: " << r);
					m_f.rrInfo[gid].codingCoefs = m_getCodingMatrix(gid);
					break;
				}
				case HASH_VECTOR_FEEDBACK_ART: {
					assert(m_ccack.find(gid) != m_ccack.end());
					m_f.rrInfo[gid].hashVector = m_ccack[gid]->GetHashVector();
					SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "Hash vector " << m_f.rrInfo[gid].hashVector);
					break;
				}
				case SEEN_DEC_RANK_FEEDBACK_ART: {
					assert(m_getCoderInfo);
					m_f.rrInfo[gid].coderInfo = m_getCoderInfo(gid);
					break;
				}
				default: {
					assert(0);
				}
			}
		}
		return false;	// apply for generations in the retransmission range
		};

	if (m_nodeType == SOURCE_NODE_TYPE) return false;

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Creating RR");

	ResetRetransInfo();

	WorkInRetransRange(func);

	//
	// do not send RR for the generation if just received the symbol from the actually requesting
	// or old generation
	//
	if (IsRequestedForRetrans(newGid)) return false;

	if (!m_f.rrInfo.empty()) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Say YES to sending of RR");
		m_f.rrInfo.forwarder = SelectRetransRequestForwarder();
		return true;
	} else {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Say NO to sending of RR. I have not sufficient generations in memory");
		return false;
	}
}
bool NcRoutingRules::IsRequestedForRetrans(GenId newGid) {

	SIM_LOG_FUNC(BRR_LOG);

	bool b = false;
	for (auto r : m_f.rrInfo) {
		auto gid = r.first;
		if (gen_ssn_t(gid) >= gen_ssn_t(newGid)) b = true;
	}
	return b;
}
void NcRoutingRules::Reset() {
	//
	// TODO
	//
}

void NcRoutingRules::CheckBuffering() {

	SIM_LOG_FUNC(BRR_LOG);

	if (m_inRcvNum.empty()) return;
	//
	// get the ID of the newest generation currently in buffer
	//
	auto last_act = gen_ssn_t(m_inRcvNum.last_orig_order()->first);
	//
	// get the ID of the newest generation that is already ACKed
	//
	auto last_acked = gen_ssn_t(*(m_outdatedGens.last()));
	//
	// take the newest one
	//
	auto last = (last_act > last_acked) ? last_act : last_acked;
	//
	// form set of generations that should buffer
	//
	std::vector<gen_ssn_t> buf_gens;
	for (uint16_t i = 0; i < m_sp.numGenBuffering; i++) {
		buf_gens.push_back(last);
		last--;
	}

	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "[last_act " << last_act << "],[last_acked " << last_acked << "]");

	std::vector<gen_ssn_t> gens;
	for (auto f : m_forwardPlan) {
		auto gid = f.first;
		if (std::find(buf_gens.begin(), buf_gens.end(), gid) != buf_gens.end()) {
			gens.push_back(gid);
		}
	}
	for (auto gen : gens) {
		auto gid = gen.val();
		SIM_LOG_NPG(BRR_LOG, m_id, m_p, gid, "Erase from forwarding due to buffering");
		m_forwardPlan.erase(gid);
	}

	//
	//
	//	std::vector<gen_ssn_t> s;
	//
	//	//
	//	// sort the generations: the newest to the beginning
	//	//
	//	for (auto f : m_forwardPlan) {
	//		gen_ssn_t i(f.first);
	//
	//		auto it = s.begin();
	//		for (; it != s.end(); it++) {
	//			if (i > *it) {
	//				s.insert(it, i);
	//				break;
	//			}
	//		}
	//		if (it == s.end()) s.push_back(i);
	//	};;
	//
	//	uint16_t m = (s.size() > m_sp.numGenBuffering) ? m_sp.numGenBuffering : s.size();
	//
	//	for (uint16_t i = 0; i < m; i++) {
	//		auto genId = s.at(i).val();
	//		SIM_LOG_NPG(BRR_LOG, m_id, m_p, genId, "Erase from forwarding due to buffering");
	//		m_forwardPlan.erase(genId);
	//	}

}

bool NcRoutingRules::IsOverflowDanger() {
	//
	// source avoids the overflow by throtling the traffic generator
	//
	if (m_nodeType == SOURCE_NODE_TYPE) return false;

	return (GetActualRxWinSize() + m_sp.numGenRetrans > m_sp.numGen);
}

void NcRoutingRules::Overshoot(GenId gid) {
	SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Overshooting the generation " << gid);

	if (m_inRcvNum.find(gid) == m_inRcvNum.end()) {
		SIM_LOG_NPD(BRR_LOG, m_id, m_p, m_dst, "Cannot overshoot. The generation is already away: " << gid);
		assert(0);
	} else {
		m_retransPlan[gid] = 0;
		m_sentNum[gid] = 0;
		DoFilter();
		if (m_filteredPlan.find(gid) == m_filteredPlan.end()) {
			m_filteredPlan[gid] = m_sp.genSize;
			DoUpdateForwardPlan();
		} else {
			if (eq(m_filteredPlan[gid], 0)) {
				m_filteredPlan[gid] = m_sp.genSize;
				DoUpdateForwardPlan();
			}
		}
	}
}
void NcRoutingRules::ValidateReaction(GenId genId, UanAddress id) {

	m_service.set_repeat(false);	// be optimistic by default

	if (m_service.get_type() == ServiceMessType::REQ_PTP_ACK) {
		if (DoesItCooperate(id)) m_service.set_repeat(true);
		return;
	}
	if (m_service.get_type() == ServiceMessType::REQ_ETE_ACK) {
		if (DoesItCooperate(id)) m_service.set_repeat(true);
		return;
	}
	if (m_service.get_type() == ServiceMessType::REQ_RETRANS || m_service.get_type() == ServiceMessType::REP_REQ_RETRANS) {
		if (DoICooperate(id)) if (!IsRequestedForRetrans(genId)) m_service.set_repeat(true);
		return;
	}

}
void NcRoutingRules::ValidateReaction(FeedbackInfo l) {

	auto react_bad_if_higher_prior = [this](FeedbackInfo l)
	{
		if (!l.type.is_higher_prior(m_service.get_type())) {
			m_service.set_repeat(true);
			return;
		}
	};

	m_service.set_repeat(false);	// be optimistic by default

	if (m_service.get_type() == ServiceMessType::REQ_PTP_ACK) {
		if (DoesItCooperate(l.addr)) react_bad_if_higher_prior(l);
		return;
	}
	if (m_service.get_type() == ServiceMessType::REQ_ETE_ACK) {
		if (DoesItCooperate(l.addr)) react_bad_if_higher_prior(l);
		return;
	}
}
void NcRoutingRules::PlanExpectedReaction(GenId genId, UanAddress id) {

	if (m_service.get_type() == ServiceMessType::REQ_RETRANS || m_service.get_type() == ServiceMessType::REP_REQ_RETRANS) {
		if (DoICooperate(id)) if (IsRequestedForRetrans(genId)) m_service.stop();
		return;
	}
}
void NcRoutingRules::PlanExpectedReaction(FeedbackInfo l) {

	if (m_service.get_type() == ServiceMessType::REQ_PTP_ACK) {
		if (DoesItCooperate(l.addr)) {
			if (l.type == ServiceMessType::RESP_PTP_ACK || l.type == ServiceMessType::REGULAR) {
				m_service.stop();
			}
		}
		return;
	}
	if (m_service.get_type() == ServiceMessType::REQ_ETE_ACK) {
		if (DoesItCooperate(l.addr)) {
			if (l.type == ServiceMessType::RESP_ETE_ACK) {
				m_service.stop();
			}
		}
		return;
	}
}
void NcRoutingRules::PlanExpectedReaction() {

	m_service.start();

	if (m_service.get_type() == ServiceMessType::REGULAR || m_service.get_type() == ServiceMessType::RESP_PTP_ACK
			|| m_service.get_type() == ServiceMessType::RESP_ETE_ACK || m_service.get_type() == ServiceMessType::NET_DISC
			|| m_service.get_type() == ServiceMessType::REP_NET_DISC) {
		//
		// expect no reaction
		//
		m_service.stop();
		return;
	}
}
} //ncr
