/*
 * header.h
 *
 *  Created on: Dec 11, 2015
 *      Author: tsokalo
 */

#ifndef HEADER_H_
#define HEADER_H_

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <map>
#include <unordered_map>

#include "utils/filter-arithmetics.h"
#include "utils/priority.h"
#include "utils/ssn.h"

namespace ncr {

#define DELIMITER	"\t"

#define SRC_UANADDRESS	0

#define HALVE_TX_PLAN

#define BUFFERING_AT_HELPERS
#ifdef BUFFERING_AT_HELPERS
#define NUM_GENS_BUFF	1
#endif

/*
 * Redundancy amount
 */
enum CodeRedundancyCalcWay {
	EXACT_EXPECTATION_REDANDANCY = 0, PLUS_SIGMA_REDUNDANCY = 1, MINUS_DELTA_REDUNDANCY = 2
};

/*
 * Selection of RR forwarders
 */
enum RrCandidateSelection {
	RANDOM_RETRANSMITTER_RR_CANDIDATE_SELECTION,
	CONNECTION_QUALITY_RR_CANDIDATE_SELECTION,
	LIN_DEP_FREQ_RR_CANDIDATE_SELECTION,
	LIN_DEP_FREQ_RAND_RR_CANDIDATE_SELECTION,
	HIHGEST_PRIORITY_RR_CANDIDATE_SELECTION
};

/*
 * Art of feedback
 */
enum TypeOfFeedback {
	ALL_VECTORS_FEEDBACK_ART, HASH_VECTOR_FEEDBACK_ART, SEEN_DEC_RANK_FEEDBACK_ART
};

/*
 * RR legimitation
 */
enum WhoCanSendRr {
	ALL_WHO_HEAR_LEGAL, ONE_SELECTED_LEGAL
};

//#define FULL_VECTOR

/*
 * unit [bps]
 */
#define SMALLEST_SENDER_PHY_DATA_RATE		500
#define PRIORITY_HYSTERESIS_WIDTH 	(15000)
#define PHW							PRIORITY_HYSTERESIS_WIDTH

typedef Priority<PHW> priority_t;

extern uint16_t seed_corrector;

/*************************************************************************************************/
/********************************************** ENUMS ********************************************/
/*************************************************************************************************/
enum CurrentSender {
	RELAY_TRANS, SOURCE_TRANS
};

enum Policy {
	WFF_POLICY, WFH_POLICY, ZH_POLICY, PLNCOOL_POLICY, TRY_K_POLICY, TRY_S_POLICY, ZH_REAL_POLICY, TRY_S_RNO_POLICY
};

enum WeightCoefs {
	NO_WEIGHT_COEFS, LIN_LAST_MOST_WEIGHT_COEFS, LIN_FIRST_MOST_WEIGHT_COEFS
};
enum NodeType {
	SOURCE_NODE_TYPE, RELAY_NODE_TYPE, DESTINATION_NODE_TYPE, NO_NODE_TYPE
};

enum NcPolicyType {
	ANTiCS_NC_POLICY_TYPE, SENDALWAYS_NC_POLICY_TYPE, PLAYNCOOL_NC_POLICY_TYPE, ANTiCS_E_NC_POLICY_TYPE
};

enum UniqueDofEstimation {
	NODE_ID_INFORMATION, SENDER_UNIQUE_DOF_ESTIMATION, RECEIVER_SUM_UNIQUE_DOF_ESTIMATION, RECEIVER_VEC_UNIQUE_DOF_ESTIMATION
};
enum GenerationState {
	FORWARDING_GENERATION_STATE, RETRANSMITTING_GENERATION_STATE
};
enum ProgMode {
	RUN_MODE, EVAL_MODE, TEST_MODE
};
enum MessType {
	DATA_MSG_TYPE, FEEDBACK_MSG_TYPE, NETDISC_MSG_TYPE, RETRANS_REQUEST_MSG_TYPE, ORIG_MSG_TYPE
};

/*************************************************************************************************/
/******************************************** STRUCTURES *****************************************/
/*************************************************************************************************/

struct MarkovState {
	uint16_t relayDofRel;
	uint16_t relayDof;
	uint16_t destDof;
	CurrentSender trans;
};
struct SentPkts {
	uint32_t relay;
	uint32_t source;
	uint32_t source_before_relay;
};

struct Losses {
	double e1;
	double e2;
	double e3;
};
struct TransProb {
	double Prdr;
	double Prds;
	double Psrr;
	double Psrs;
	double Psds;
	double Psdr;
	double Psnr;
	double Psns;
	double Prns;
	double Prnr;
};

struct NcStat {
	uint32_t lost;
	uint32_t l_rlnc;
	uint32_t l_copy;
};

struct Statistic {
	// <iteration>
	std::vector<uint32_t> uninterrupted;
	std::vector<uint32_t> copies;
	double aveUninterrupted;
	double destDofPerTrans;
	double copiesPerTrans;
	// <node id> <iteration>
	std::vector<std::vector<uint32_t> > uniqueRcvd;
	std::vector<std::vector<uint32_t> > totalSent;
	// <node id>
	std::vector<double> uniqueRcvdAve;
	std::vector<double> totalSentAve;
	std::vector<double> uniqueRcvdCalc;
	std::vector<double> totalSentCalc;
};
/*************************************************************************************************/
/********************************************* NEW STUFF *****************************************/
/*************************************************************************************************/
typedef std::vector<uint8_t> NcSymbol;
typedef NcSymbol OrigSymbol;
typedef int16_t UanAddress;
typedef int16_t EdgeId;

/*
 * unit [bits per second]
 */
typedef double Datarate;

/*
 * The logging item for node v and time point t
 * All variables below are the estimation of the current node
 */

struct ArqWin {
	ArqWin() {
		s_rx = 0;
		s_tx = 0;
		e_tx = 0;
		e_rx = 0;
	}
	ArqWin&
	operator=(const ArqWin& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected

			this->s_rx = other.s_rx;
			this->s_tx = other.s_tx;
			this->e_tx = other.e_tx;
			this->e_rx = other.e_rx;
		}
		return *this;
	}
	friend bool operator==(ArqWin l, ArqWin r) // copy assignment
			{
		return (l.s_rx == r.s_rx && l.s_tx == r.s_tx && l.e_tx == r.e_tx && l.e_rx == r.e_rx);
	}
	bool is_def()
	{
		return (s_rx != MAX_GEN_SSN && s_tx != MAX_GEN_SSN && e_tx != MAX_GEN_SSN && e_rx != MAX_GEN_SSN);
	}
	GenId s_rx; //start of RX window
	GenId s_tx; //start of TX window
	GenId e_tx; //end of TX window
	GenId e_rx; //end of RX window
};

struct LogItem {

	LogItem() {
		cr = 1;
		d = 0;
		cs = 0;
		ns = 0;
		nr = 0;
		ssn = 0;
		gsn = 0;
		dst = 0;
		rank = 0;
	}
	LogItem(std::string init) {
		std::stringstream ss(init);

		int32_t v;
		ss >> v;
		ss >> v;
		ss >> v;
		ss >> dst;
		std::string str;
		ss >> str;
		d = std::stod(str);
		str.clear();
		ss >> str;
		if (str == "DESTINATION_PRIORITY") p = DESTINATION_PRIORITY;
		else p = std::stod(str);
		ss >> cr;
		ss >> cs;
		ss >> ns;
		ss >> nr;
		ss >> ssn;
		ss >> gsn;
		ss >> rank;
		ss >> aw.s_rx;
		ss >> aw.s_tx;
		ss >> aw.e_tx;
		ss >> aw.e_rx;
		uint16_t s_fp = 0, s_eps = 0;
		ss >> s_fp;
		ss >> s_eps;

		UanAddress iv = 0;
		double dv = 0;
		for (uint16_t i = 0; i < s_fp; i++) {
			ss >> iv;
			ss >> dv;
			fp[iv] = dv;
		}
		for (uint16_t i = 0; i < s_eps; i++) {
			ss >> iv;
			ss >> dv;
			eps[iv] = dv;
		}
	}

	LogItem&
	operator=(const LogItem& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected
			this->eps.clear();
			this->eps = other.eps;
			this->cr = other.cr;
			this->fp.clear();
			this->fp = other.fp;
			this->p = other.p;
			this->cs = other.cs;
			this->ns = other.ns;
			this->nr = other.nr;
			this->ssn = other.ssn;
			this->gsn = other.gsn;
			this->rank = other.rank;
			this->aw = other.aw;
			this->dst = other.dst;
		}
		return *this;
	}

	friend std::ostream&
	operator<<(std::ostream& os, const LogItem& l) {
		os << l.dst << "\t" << l.d << "\t" << l.p << "\t" << l.cr << "\t" << l.cs << "\t" << l.ns << "\t" << l.nr << "\t" << l.ssn << "\t" << l.gsn << "\t"
				<< l.rank << "\t" << l.aw.s_rx << "\t" << l.aw.s_tx << "\t" << l.aw.e_tx << "\t" << l.aw.e_rx << "\t" << l.fp.size() << "\t" << l.eps.size();

		for (std::map<int16_t, double>::const_iterator it = l.fp.begin(); it != l.fp.end(); it++)
			os << "\t" << it->first << "\t" << it->second;
		for (std::map<int16_t, double>::const_iterator it = l.eps.begin(); it != l.eps.end(); it++)
			os << "\t" << it->first << "\t" << it->second;
		return os;
	}

	/*
	 * estimation of the expectation value of packet loss ratio for each out-coming edge e in O(v)
	 * int16_t - id of the v^+(e)
	 */
	std::map<int16_t, double> eps;
	/*
	 * coding rate = 1 / (1 + redundancy)
	 */
	double cr;
	/*
	 * filtering probability for each in-coming edge e in I(v)
	 * int16_t - id of the v^-(e)
	 */
	std::map<int16_t, double> fp;
	/*
	 * priority of v
	 */
	priority_t p;
	/*
	 * sending data rate
	 */
	Datarate d;

	/*
	 * size of the coalition
	 */
	uint16_t cs;
	/*
	 * number of sent packets per the transmission opportunity (!not the total amount)
	 */
	uint16_t ns;
	/*
	 * number linear independent packets received by the destination
	 * values: 0 or 1
	 */
	uint16_t nr;
	/*
	 * sequence number
	 */
	symb_ssn_t ssn;
	/*
	 * generation sequence number
	 */
	gen_ssn_t gsn;
	/*
	 * generation rank
	 */
	uint32_t rank;
	/*
	 * destination address
	 */
	UanAddress dst;
	/*
	 * ARQ window
	 */
	ArqWin aw;
};
/*
 * The history of logging items for node v
 * int64_t - time point t
 */
struct LogPair {
	LogPair() {
		t = 0;
	}
	LogPair(int64_t t, MessType m, LogItem log) {
		this->t = t;
		this->log = log;
		this->m = m;
	}
	int64_t t;
	MessType m;
	LogItem log;
};
typedef std::vector<LogPair> LogHistory;
/*
 * The history of logging items for all nodes v in V
 * int16_t - id of v
 */
typedef std::map<UanAddress, LogHistory> LogBank;

typedef double Dof;

typedef std::unordered_map<UanAddress, double> pf_t;

//struct CodingMatrix: public std::vector<CodingVector> {
//
//	typedef std::size_t index_type;
//	typedef uint8_t value_type;
//	static index_type min_row(T const (&)[rows][columns]) {
//		return 0;
//	}
//	static index_type max_row(T const (&)[rows][columns]) {
//		return this->size() - 1;
//	}
//	static index_type min_column(T const (&)[rows][columns]) {
//		return 0;
//	}
//	static index_type max_column(T const (&)[rows][columns]) {
//		return this->begin()->size() - 1;
//	}
//	static value_type& element(T (&A)[rows][columns], index_type i, index_type k) {
//		return A[i][k];
//	}
//	static value_type element(T const (&A)[rows][columns], index_type i, index_type k) {
//		return A[i][k];
//	}
//};
typedef int16_t ttl_t;
/*
 * rank of the coding matrix containing all the received symbols
 */
typedef std::function<uint32_t(GenId)> get_rank_func;
/*
 * rank of the coding matrix containing the received symbols only from the nodes with higher priority
 */
typedef std::function<uint32_t(GenId)> get_rank_high_func;
typedef std::function<void(LogItem item, UanAddress node_id)> add_log_func;

struct TdmAccessPlan: std::map<uint16_t, double> {
	friend std::ostream&
	operator<<(std::ostream& os, const TdmAccessPlan& l) {
		os << "[";
		for (auto i : l) {
			os << i.first << ":" << i.second << "; ";
		}
		os << "]";
		return os;
	}
};

typedef std::vector<bool> BooleanVector;
typedef BooleanVector SeenMap;
typedef BooleanVector DecodedMap;

} //ncr
#endif /* HEADER_H_ */
