/*
 * sim-parameters.h
 *
 *  Created on: 19.01.2017
 *      Author: tsokalo
 */

#ifndef UTILS_SIM_PARAMETERS_H_
#define UTILS_SIM_PARAMETERS_H_

#include <fstream>
#include <iostream>

#include "header.h"

namespace ncr {
struct SimParameters {

	SimParameters() {
		numGen = 40;
		genSize = 100;
		symbolSize = 10;
		apiRate = 1000000;
		sendRate = 1000000;
		numGenBuffering = 2;
		numGenPtpAck = (numGen > numGenBuffering + 2) ? numGenBuffering + 2: numGen;
		numGenRetrans = (numGen > 2) ? numGen - 2 : 0;
		numGenSingleTx = 20;
		fieldSize = 8;
		ccackLevels = 2;
		maxCoalitionSize = 4;
		mutualPhyLlcCoding = false;
		per = 0.2;
		numRr = 3;
		crCalcWay = MINUS_DELTA_REDUNDANCY;
		crNumSigma = 3;
		crReducFactor = 0.0;
		rrCanSel = HIHGEST_PRIORITY_RR_CANDIDATE_SELECTION;
		fbCont = SEEN_DEC_RANK_FEEDBACK_ART;
		giveRrPriorToSrc = true;
		rrCanSend = ONE_SELECTED_LEGAL;
		warmup = 1000;
		warmdown= 1000;
		simDuration = 10000;
	}

	SimParameters(std::string path) {

		ReadFromFile(path);
		Print();
	}

	SimParameters&
	operator=(const SimParameters& other) // copy assignment
			{
		if (this != &other) { // self-assignment check expected

			this->numGen = other.numGen;
			this->genSize = other.genSize;
			this->symbolSize = other.symbolSize;
			this->apiRate = other.apiRate;
			this->sendRate = other.sendRate;
			this->numGenBuffering = other.numGenBuffering;
			this->numGenPtpAck = other.numGenPtpAck;
			this->numGenRetrans = other.numGenRetrans;
			this->numGenSingleTx = other.numGenSingleTx;
			this->fieldSize = other.fieldSize;
			this->ccackLevels = other.ccackLevels;
			this->maxCoalitionSize = other.maxCoalitionSize;
			this->mutualPhyLlcCoding = other.mutualPhyLlcCoding;
			this->per = other.per;
			this->numRr = other.numRr;
			this->crCalcWay = other.crCalcWay;
			this->crNumSigma = other.crNumSigma;
			this->crReducFactor = other.crReducFactor;
			this->rrCanSel = other.rrCanSel;
			this->fbCont = other.fbCont;
			this->giveRrPriorToSrc = other.giveRrPriorToSrc;
			this->rrCanSend = other.rrCanSend;
			this->warmup = other.warmup;
			this->warmdown = other.warmdown;
			this->simDuration = other.simDuration;
		}
		return *this;
	}

	void WriteToFile(std::string path) {
		std::ofstream of(path, std::ios_base::out);
		ParamToStream(of);
		of.close();
	}

	void ReadFromFile(std::string path) {
		std::ifstream in_f(path, std::ios_base::in);

		numGen = ReadVal<uint16_t>(in_f);
		genSize = ReadVal<uint32_t>(in_f);
		symbolSize = ReadVal<uint32_t>(in_f);
		apiRate = ReadVal<Datarate>(in_f);
		sendRate = ReadVal<Datarate>(in_f);
		numGenBuffering = ReadVal<uint16_t>(in_f);
		numGenPtpAck = ReadVal<uint16_t>(in_f);
		numGenRetrans = ReadVal<uint16_t>(in_f);
		numGenSingleTx = ReadVal<uint16_t>(in_f);
		fieldSize = ReadVal<uint16_t>(in_f);
		ccackLevels = ReadVal<uint16_t>(in_f);
		maxCoalitionSize = ReadVal<uint16_t>(in_f);
		mutualPhyLlcCoding = ReadVal<bool>(in_f);
		per = ReadVal<double>(in_f);
		numRr = ReadVal<uint16_t>(in_f);
		crCalcWay = CodeRedundancyCalcWay(ReadVal<uint16_t>(in_f));
		crNumSigma = ReadVal<uint16_t>(in_f);
		crReducFactor = ReadVal<double>(in_f);
		rrCanSel = RrCandidateSelection(ReadVal<uint16_t>(in_f));
		fbCont = TypeOfFeedback(ReadVal<uint16_t>(in_f));
		giveRrPriorToSrc = ReadVal<bool>(in_f);
		rrCanSend = WhoCanSendRr(ReadVal<uint16_t>(in_f));
		warmup = ReadVal<uint32_t>(in_f);
		warmdown = ReadVal<uint32_t>(in_f);
		simDuration = ReadVal<uint64_t>(in_f);
		in_f.close();
	}

	void Print() {
		std::cout << "Simulation parameters: [" << std::endl;
		ParamToStream(std::cout);
		std::cout << "]" << std::endl;
	}

	void ParamToStream(std::ostream &os) {
		os << "Number of generations\t\t\t" << numGen << std::endl;
		os << "Generation size\t\t\t\t" << genSize << std::endl;
		os << "Symbol size / bytes\t\t\t" << symbolSize << std::endl;
		os << "Application data rate / bps\t\t" << apiRate << std::endl;
		os << "Sending data rate / bps\t\t\t" << sendRate << std::endl;
		os << "Number of buffering generations (Tx)\t" << numGenBuffering << std::endl;
		os << "Number of generations before soft ACK\t" << numGenPtpAck << std::endl;
		os << "Number of generations before RR\t\t\t" << numGenRetrans << std::endl;
		os << "Number of generations in single MPDU\t" << numGenSingleTx << std::endl;
		os << "Field size / power of 2\t\t\t" << fieldSize << std::endl;
		os << "CCACK levels\t\t\t\t" << ccackLevels << std::endl;
		os << "Maximum coalition size\t\t\t" << maxCoalitionSize << std::endl;
		os << "Mutual PHY and LLC coding / flag\t" << mutualPhyLlcCoding << std::endl;
		os << "Target packet loss ratio\t\t" << per << std::endl;
		os << "Maximum number of RRs\t\t\t" << numRr << std::endl;
		os << "CR calculation way\t\t\t" << (uint16_t) crCalcWay << std::endl;
		os << "Number of sigmas\t\t\t" << crNumSigma << std::endl;
		os << "CR reduction factor\t\t\t" << crReducFactor << std::endl;
		os << "RR candidate selection way\t\t" << (uint16_t) rrCanSel << std::endl;
		os << "Type of the feedback contents\t\t" << (uint16_t) fbCont << std::endl;
		os << "Give RR priority to SRC\t\t\t" << (uint16_t) giveRrPriorToSrc << std::endl;
		os << "Who can forward RR\t\t\t" << rrCanSend << std::endl;
		os << "Warm-up period\t\t\t\t" << warmup << std::endl;
		os << "Warm-down period\t\t\t" << warmdown << std::endl;
		os << "Simulation duration\t\t\t" << simDuration << std::endl;
	}

	std::string GetInLine() {
		std::stringstream os;
		os << numGen << '\t';
		os << genSize << '\t';
		os << symbolSize << '\t';
		os << apiRate << '\t';
		os << sendRate << '\t';
		os << numGenBuffering << '\t';
		os << numGenPtpAck << '\t';
		os << numGenRetrans << '\t';
		os << numGenSingleTx << '\t';
		os << fieldSize << '\t';
		os << ccackLevels << '\t';
		os << maxCoalitionSize << '\t';
		os << mutualPhyLlcCoding << '\t';
		os << per << '\t';
		os << numRr << '\t';
		os << (uint16_t) crCalcWay << '\t';
		os << crNumSigma << '\t';
		os << crReducFactor << '\t';
		os << (uint16_t) rrCanSel << '\t';
		os << (uint16_t) fbCont << '\t';
		os << (uint16_t) giveRrPriorToSrc << '\t';
		os << rrCanSend << '\t';
		os << warmup << '\t';
		os << warmdown << '\t';
		os << simDuration;
		return os.str();
	}
	template<typename T>
	T ReadVal(std::ifstream &fs) {
		std::string line;
		std::getline(fs, line);
		auto s = line.rfind('\t');
		std::stringstream ss(line.substr(s, line.size()));
		T v;
		ss >> v;
		return v;
	}

	/*
	 * maximum number of generations in the buffer
	 */
	uint16_t numGen;
	/*
	 * generation size
	 */
	uint32_t genSize;
	/*
	 * coded symbol size
	 */
	uint32_t symbolSize;
	/*
	 * data rate of the application layer
	 */
	Datarate apiRate;
	/*
	 * sending data rate of the layer with the coder
	 */
	Datarate sendRate;
	/*
	 * minimum number of generation in the buffer before TX can start
	 */
	uint16_t numGenBuffering;
	/*
	 * maximum number of generation that are allowed to be transferred in a single PDU of the underlying layer
	 */
	uint16_t numGenSingleTx;
	/*
	 * field size
	 */
	uint16_t fieldSize;
	/*
	 * number of rotation matrices in CCACK method
	 */
	uint16_t ccackLevels;
	/*
	 * maximum size of the coalition of any vertex
	 */
	uint16_t maxCoalitionSize;
	/*
	 * set true if PHY layer can allow PER > 0.00001
	 */
	bool mutualPhyLlcCoding;
	/*
	 * target PER after PHY decoding
	 */
	double per;
	/*
	 * number of generations that should be buffered before the retransmissions can be requested
	 */
	uint16_t numGenRetrans;
	/*
	 * number of generations in the reception buffer over which the Ptp ACKs will be requested
	 */
	uint16_t numGenPtpAck;
	/*
	 * maximum number of retransmission requests
	 */
	uint16_t numRr;
	/*
	 * selecting the way of coding redundancy calculation
	 */
	CodeRedundancyCalcWay crCalcWay;
	/*
	 * number of sigma for the coding redundancy calculation (only if selecting crCalcWay = 1)
	 */
	uint16_t crNumSigma;
	/*
	 * reduction factor for the coding redundancy calculation (only if selecting crCalcWay = 2)
	 */
	double crReducFactor;
	/*
	 * selection type of the retransmission request forwarding candidate
	 */
	RrCandidateSelection rrCanSel;
	/*
	 * contents of the feedback
	 */
	TypeOfFeedback fbCont;
	/*
	 * selecting the source for answering the retransmission request
	 */
	bool giveRrPriorToSrc;
	/*
	 * legimitation of forwarding the retransmission request
	 */
	WhoCanSendRr rrCanSend;
	/*
	 * warm-up period; considered in the evaluation mode
	 */
	uint32_t warmup;
	/*
	 * warm-down period; considered in the evaluation mode
	 */
	uint32_t warmdown;
	/*
	 * simulation duration
	 */
	uint64_t simDuration;
};
}

#endif /* UTILS_SIM_PARAMETERS_H_ */
