/*
 * MulticastBrr.h
 *
 *  Created on: 11.05.2017
 *      Author: tsokalo
 */

#ifndef ROUTING_RULES_MULTICAST_BRR_H_
#define ROUTING_RULES_MULTICAST_BRR_H_

#include "routing-rules/nc-routing-rules.h"

namespace ncr {

class MulticastBrr {

	typedef std::shared_ptr<NcRoutingRules> routing_rules_ptr;
	typedef std::function<CodingMatrix(GenId)> get_coding_matrix_func;
	typedef std::function<CoderInfo(GenId)> get_coder_info_func;
	typedef std::function<CoderHelpInfo(CodingMatrix, CoderInfo, GenId)> get_help_info_func;
	typedef node_map_base_t::iterator node_map_it;
	typedef std::function<symb_ssn_t(OrigSymbol)> receive_app_func;
	typedef std::shared_ptr<CongestionControl> congestion_control_ptr;
	typedef std::shared_ptr<Ccack> ccack_ptr;
	typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;

public:
	MulticastBrr(UanAddress ownAddress, NodeType type, std::vector<UanAddress> destAddresses, SimParameters sp);
	virtual ~MulticastBrr();

	void SetLogCallback(add_log_func addLog);
	void SetGetRankCallback(get_rank_func c);
	void SetGetCodingMatrixCallback(get_coding_matrix_func c);
	void SetGetCoderInfoCallback(get_coder_info_func c);
	void SetCoderHelpInfoCallback(get_help_info_func c);
	void SetReceiveAppCallback(receive_app_func c);

	void EnableCcack(hash_matrix_set_ptr hashMatrixSet);

	/*
	 * INPUTS
	 */
	void RcvHeaderInfo(HeaderInfo l);
	void RcvFeedbackInfo(FeedbackInfo l);
	void UpdateSent(GenId genId, uint32_t num, bool notify_sending = false);
	void UpdateRcvd(GenId genId, UanAddress id, bool linDep = false);
	void UpdateRcvd(GenId genId, UanAddress id, std::vector<OrigSymbol> v);
	void UpdateLoss(GenId genId, UanAddress id);
	void NotifySending();
	void AddSentCcack(GenId genId, CodingVector cv);
	void AddRcvdCcack(GenId genId, CodingVector cv);
	//
	void AddToCoalition(UanAddress addr);
	void SetSendingRate(Datarate d);

	/*
	 * OUTPUTS
	 */
	TxPlan GetTxPlan();
	BrrMHeader GetHeader(TxPlan txPlan, FeedbackInfo f);
	FeedbackInfo GetFeedbackInfo();
	FeedbackInfo GetRetransRequestInfo(ttl_t ttl = -2);
	HeaderInfo GetHeaderInfo();
	HeaderInfo GetHeaderInfo(TxPlan txPlan);
	NetDiscoveryInfo GetNetDiscoveryInfo(ttl_t ttl = -2);
	UanAddress GetSinkVertex();
	//
	bool NeedGen();
	uint32_t GetNumGreedyGen();
	bool MaySendData(double dr = 0);
	bool MaySendFeedback();
	bool MaySendNetDiscovery(ttl_t ttl = -1);
	// retransmission requests
	bool MaySendRetransRequest(std::map<GenId, uint32_t> ranks, UanAddress id, GenId genId, bool all_prev_acked);
	bool ProcessRetransRequest(FeedbackInfo fb);
	bool HasRetransRequest(FeedbackInfo fb);
	void ResetRetransInfo();
	void UpdateRetransRequest();
	//
	uint32_t GetGenBufSize(uint32_t maxPkts);
	uint32_t GetAmountTxData();
	uint16_t GetAckBacklogSize();
	uint16_t GetCoalitionSize();


private:

	void FindLeadingDst();

	UanAddress m_lead;
	std::map<UanAddress, routing_rules_ptr> m_brr;
};
}

#endif /* ROUTING_RULES_MULTICAST_BRR_H_ */
