/*
 * MulticastBrr.h
 *
 *  Created on: 11.05.2017
 *      Author: tsokalo
 */

#ifndef ROUTING_RULES_MULTICAST_BRR_H_
#define ROUTING_RULES_MULTICAST_BRR_H_

#include "routing-rules/nc-routing-rules.h"
#include "utils/brrm-header.h"
#include "utils/brrm-pkt-header.h"
#include "utils/brrm-feedback.h"
#include "utils/brrm-netdiscovery.h"
#include "utils/filter-arithmetics.h"

//#define USE_MAX_FILTERING_COEFS
//#define USE_MIN_FILTERING_COEFS
#define NORM_REST_FILTERING_COEFS

#define MAX_DATA_TXPLAN
//#define MAX_TRAFFICLOAD_TXPLAN

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
	void RcvHeaderInfo(HeaderMInfo l);
	void RcvFeedbackInfo(FeedbackMInfo l);
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
	BrrMHeader GetHeader(TxPlan txPlan, FeedbackMInfo f);
	FeedbackMInfo GetFeedbackInfo();
	FeedbackMInfo GetRetransRequestInfo(ttl_t ttl = -2);
	HeaderMInfo GetHeaderInfo();
	HeaderMInfo GetHeaderInfo(TxPlan txPlan);
	NetDiscoveryMInfo GetNetDiscoveryInfo(ttl_t ttl = -2);
	//
	bool NeedGen();
	uint32_t GetNumGreedyGen();
	bool MaySendData(double dr = 0);
	bool MaySendFeedback();
	bool MaySendNetDiscovery(ttl_t ttl = -1);
	// retransmission requests
	bool MaySendRetransRequest(std::map<GenId, uint32_t> ranks, UanAddress id, GenId genId, bool all_prev_acked);
	bool ProcessRetransRequest(FeedbackMInfo fb);
	bool HasRetransRequest(FeedbackMInfo fb);
	void ResetRetransInfo();
	void UpdateRetransRequest();
	//
	uint32_t GetGenBufSize(uint32_t maxPkts);
	uint32_t GetAmountTxData();
	uint16_t GetAckBacklogSize();
	std::map<UanAddress, uint16_t> GetCoalitionSize();


private:

	std::map<UanAddress, routing_rules_ptr> m_brr;

	/*
	 * own address
	 */
	UanAddress m_id;
	/*
	 * amount of data in TX plan per generation per destination
	 */
	std::map<UanAddress, TrafficLoadFilter> m_trafficLoad;
};
}

#endif /* ROUTING_RULES_MULTICAST_BRR_H_ */
