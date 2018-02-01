/*
 * MulticastBrr.h
 *
 *  Created on: 11.05.2017
 *      Author: tsokalo
 */

#ifndef ROUTING_RULES_MULTICAST_BRR_H_
#define ROUTING_RULES_MULTICAST_BRR_H_

#include "utils/brrm-header.h"
#include "utils/brrm-pkt-header.h"
#include "utils/brrm-feedback.h"
#include "utils/brrm-netdiscovery.h"
#include "utils/filter-arithmetics.h"
#include "routing-rules/nc-routing-rules.h"

#define USE_MAX_FILTERING_COEFS
//#define USE_MIN_FILTERING_COEFS
//#define NORM_REST_FILTERING_COEFS
//#define DEST_AWARE_FILTERING_COEFS

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
	void ProcessHeaderInfo(HeaderMInfo l);
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
	//
	void ProcessServiceMessage(FeedbackMInfo f);
	void CheckReqRetrans(UanAddress id, GenId genId, bool all_prev_acked);
	//
	void ResetRetransInfo();

	/*
	 * OUTPUTS
	 */
	TxPlan GetTxPlan();
	BrrMHeader GetHeader(TxPlan txPlan, FeedbackMInfo f);
	HeaderMInfo GetHeaderInfo();
	HeaderMInfo GetHeaderInfo(TxPlan txPlan);
	FeedbackMInfo GetServiceMessage();
	//
	bool NeedGen();
	uint32_t GetFreeBufferSize();
	bool MaySend(double dr = 0);
	bool MaySendData(double dr = 0);
	bool MaySendServiceMessage();
	//
	void CreateRetransRequest();
	//
	uint32_t GetGenBufSize(uint32_t maxPkts);
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
