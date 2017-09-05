/*
 * nc-routing-rules.h
 *
 *  Created on: 02.10.2016
 *      Author: tsokalo
 */

#ifndef NCROUTINGRULES_H_
#define NCROUTINGRULES_H_

#include <map>
#include <deque>
#include <unordered_map>
#include <random>
#include <memory>

#include "utils/conn-id.h"
#include "utils/filter-arithmetics.h"
#include "header.h"
#include "utils/node-map.h"
#include "utils/special-map.h"
#include "utils/congestion-control.h"
#include "utils/brr-pkt-header.h"
#include "utils/brr-feedback.h"
#include "utils/brr-netdiscovery.h"
#include "utils/brr-tx-plan.h"
#include "utils/coder-info.h"
#include "utils/coder-help-info.h"
#include "utils/feedback-estimator.h"
#include "utils/sim-parameters.h"
#include "utils/brr-header.h"
#include "utils/ack-backlog.h"
#include "utils/ack-countdown.h"
#include "utils/retrans-gen-id.h"
#include "utils/coding-vector.h"
#include "utils/retrans-request-counter.h"
#include "ccack/ccack.h"

namespace ncr {

class NcRoutingRules {

	typedef std::function<CodingMatrix(GenId)> get_coding_matrix_func;
	typedef std::function<CoderInfo(GenId)> get_coder_info_func;
	typedef std::function<CoderHelpInfo(CodingMatrix, CoderInfo, GenId)> get_help_info_func;
	typedef node_map_base_t::iterator node_map_it;
	typedef std::function<symb_ssn_t(OrigSymbol)> receive_app_func;
	typedef std::shared_ptr<CongestionControl> congestion_control_ptr;
	typedef std::shared_ptr<Ccack> ccack_ptr;
	typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;
	typedef std::shared_ptr<RetransRequestCounter> rr_counter_ptr;

public:
	NcRoutingRules(UanAddress ownAddress, NodeType type, UanAddress destAddress, SimParameters sp);
	virtual ~NcRoutingRules();

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
	void ProcessHeaderInfo(HeaderInfo l);
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
	void ProcessServiceMessage(FeedbackInfo f);
	void CheckReqRetrans(UanAddress id, GenId genId, bool all_prev_acked);
	//
	void ResetRetransInfo();

	/*
	 * OUTPUTS
	 */
	TxPlan GetTxPlan();
	BrrHeader GetHeader(TxPlan txPlan, FeedbackInfo f);
	HeaderInfo GetHeaderInfo();
	HeaderInfo GetHeaderInfo(TxPlan txPlan);
	UanAddress GetSinkVertex();
	FeedbackInfo GetServiceMessage();
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
	uint32_t GetAmountTxData();
	uint16_t GetAckBacklogSize();
	uint16_t GetCoalitionSize();
	double GetCodingRate();
	uint32_t GetMaxAmountTxData();
	priority_t GetPriority();
	//
	double GetInfoOnDsts();

private:

	/*
	 * BRR functions
	 */
	void UpdateCoalition();
	void DoFilter();
	void DoUpdateFilter();
	void DoCalcRedundancy();
	void DoUpdateTxPlan();
	void DoUpdatePriority(node_map_t outputs);
	void DoUpdateCoalition();
	void DoUpdateForwardPlan();
	void DoForgetGeneration();
	void DoForgetGeneration(GenId id);
	void PlanForgetGeneration(GenId gid);
	/*
	 * ARQ and congestion control
	 */
	void CheckGeneralFeedback();
	void CheckReqPtpAck();
	void CheckReqEteAckI();
	void CheckReqEteAckII();
	void CheckNetDisc();
	//
	void ProcessRegularFeedback(FeedbackInfo f);
	void ProcessReqPtpAck(FeedbackInfo f);
	void ProcessReqEteAck(FeedbackInfo f);
	void ProcessRespPtpAck(FeedbackInfo f);
	void ProcessRespEteAck(FeedbackInfo f);
	void ProcessNetDisc(FeedbackInfo f);
	void ProcessReqRetrans(FeedbackInfo f);
	//
	void WorkInPtpAckRange(std::function<bool(GenId)> func);
	void WorkInEteAckRange(std::function<bool(GenId)> func);
	void WorkInRetransRange(std::function<bool(GenId)> func);
	//
	void OriginateReqPtpAck();
	void OriginateReqEteAck();
	//
	bool IsConnected();
	void SetConnected(bool v);

	/*
	 * Retransmission requests
	 */
	UanAddress SelectRetransRequestForwarder();
	bool MayOrigReqRetrans(GenId genId);
//	bool MayRepeatReqRetrans(UanAddress id, GenId genId);
	void GetAchievableRank(FeedbackInfo fb, std::map<GenId, CoderHelpInfo> &helpInfo);
	void DoUpdateRetransPlan(std::map<GenId, CoderHelpInfo> helpInfo);
	bool HaveDataForRetransmissions();
	bool IsRetransRequestOld(FeedbackInfo fb);
	void FormRrInfo(FeedbackInfo fb, std::map<GenId, CoderHelpInfo> helpInfo);
	void RefineCoderHelpInfo(std::map<GenId, CoderHelpInfo> &helpInfo);
	bool DoCreateRetransRequest(GenId genId);
	/*
	 * Acknowledgements
	 */
	void RefineFeedback(FeedbackInfo &fb);
	void ProcessAcks(FeedbackInfo l);
	void SetAcks();
	void ClearAcks();
	bool HaveAcksToSend();
	void SetFastAck();
	void EvaluateSoftAck();
	uint32_t GetRegularFeedbackFreq();
	bool IsSoftAck(GenId gid);
	bool IsHardAck(GenId gid);

	void Reset();

	void CheckBuffering();
	/*
	 * the buffer overflow causes data drops; on relays and destination it happens due to reception of too much data
	 * without ACKs; on source the overflow does not happen, instead the protocol does not accept any new generated packets,
	 * thus on source the overflow problem is similar to the problem of blocking the protocol layer above
	 */
	bool IsOverflowDanger();

	node_map_it LookUpInputs(UanAddress id);
	node_map_it LookUpOutputs(UanAddress id);

	void UpdateLogItem();
	GenId GetTxWinStart();
	GenId GetRxWinStart();
	GenId GetRxWinEnd();
	GenId GetTxWinEnd();
	GenId GetActualRxWinSize();
	GenId GetActualTxWinSize();

	bool DoesItCooperate(UanAddress addr);
	bool DoICooperate(UanAddress addr);

	void Overshoot(GenId gid);

	HeaderInfo m_h;
	FeedbackInfo m_f;

	/********************************************************************************************************
	 * all received packets from the nodes with lower priority and increasing the coding matrix rank;
	 * they are the subject to be passed through the filter
	 */
	RcvNum m_inRcvNum;
	/*
	 * received packet by my neighbors from me; obtained from the feedback
	 */
	RcvNum m_outRcvNum;
	/*
	 * forwarding plan specifies the number of degrees of freedom to be forwarded to the sinks of output edges;
	 * it does not include redundancy; includes both retransmission and filtered symbols
	 */
	ForwardPlan m_forwardPlan;
	/*
	 * this plan specifies the last number of degrees of freedom to be forwarded. This value is obtained
	 * from the retransmission request
	 */
	RetransmissionPlan m_retransPlan;
	/*
	 * contains number of symbols to be forwarded from the received symbols; does not include the retransmission symbols
	 */
	FilteredPlan m_filteredPlan;
	/*
	 * transmission plan specifies the number of recoded packets to be sent to the sinks of output edges;
	 * it does include redundancy; roughly speaking the transmission plan is the forwarding plan plus redundancy
	 */
	TxPlan m_txPlan;
	/*
	 * number of actually sent recoded packets
	 */
	SentNum m_sentNum;
	/*
	 * the indeces of the generations that are already removed from the buffer
	 */
	AckBacklog m_outdatedGens;
	/*
	 * the indeces of the generations that are already removed from the buffer; and still no feedback message was sent with this info
	 */
	AckCountDown m_outdatedGensInform;
	/*
	 * counter of PtpAck
	 */
	std::map<GenId, uint16_t> m_ptpAckCount;
	/*
	 * the end of the RX window of the corresponding vertices
	 */
	std::map<UanAddress, GenId> m_remoteRxWinEnd;
	/********************************************************************************************************/

	/*
	 * filtering probabilities for the current node and data coming from source of input edges;
	 * to be used for filtering
	 */
	pf_t m_inF;
	/*
	 * filtering probabilities for sinks of the out-coming edges; to be sent with feedback
	 */
	pf_t m_outF;

	/*
	 * priorities of output nodes
	 */
	node_map_t m_outputs;
	/*
	 * coalition is the subset of outputs
	 */
	node_map_t m_coalition;
	/*
	 * own priority
	 */
	priority_t m_p;
	/*
	 * the node with the priority less or equal the threshold priority will not be added to a coalition
	 */
	const priority_t m_thresholdP;
	/*
	 * sending data rate
	 */
	Datarate m_d;
	/*
	 * coding rate
	 */
	double m_cr;

	/*
	 * receiving maps for in-coming edges are updated by the node itself
	 */
	std::map<UanAddress, RcvMap> m_inRcvMap;
	std::map<UanAddress, LinDepMap> m_inLinDepMap;

	/*
	 * receiving maps for out-coming edges are updated with the feedback information
	 */
	std::map<UanAddress, RcvMap> m_outRcvMap;

	/*
	 * destination address
	 */
	UanAddress m_dst;

	/*
	 * own address
	 */
	UanAddress m_id;

	/*
	 * different RVs
	 */
	std::random_device m_rd;
	std::mt19937 m_gen;
	std::uniform_real_distribution<> m_dis;

	/*
	 * threshold difference of priority for staying in a coalition
	 */
	double m_b;
	/*
	 * probability to send the feedback after reception
	 */
	double m_feedbackP;
	/*
	 * probability to send the network discovery message after the reception
	 */
	double m_netDiscP;
	/*
	 * time to live for network discovery messages
	 */
	ttl_t m_maxTtl;
	/*
	 * oldest generation ID to retransmit
	 */
	RetransGenId m_oldestRetransGenId;
	/*
	 * send feedback at next possible opportunity; fast feedback is targeted to ACK the generations,
	 * for which the vertices with lower priorities replicate RRs
	 */
	bool m_fastFeedback;
	/*
	 * counter of retransmission requests per generation
	 */
	rr_counter_ptr m_numRr;
	/*
	 * counter of broadcasted packets
	 */
	uint32_t m_sent;
	/*
	 * soft ACK information
	 */
	AckInfo softAckInfo;
	/*
	 * counter of Transmission opportunities (TXOPs) for NetDisc
	 */
	uint16_t countTxopNetDisc;

	NodeType m_nodeType;

	SimParameters m_sp;

	congestion_control_ptr m_congControl;

	LogItem m_logItem;
	add_log_func m_addLog;
	get_rank_func m_getRank;
	get_coding_matrix_func m_getCodingMatrix;
	get_coder_info_func m_getCoderInfo;
	get_help_info_func m_getCoderHelpInfo;
	receive_app_func m_rcvApp;
	hash_matrix_set_ptr m_hashMatrixSet;
	CcackMap m_ccack;

};

} //ncr
#endif /* NCROUTINGRULES_H_ */
