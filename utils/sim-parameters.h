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
		numGenRetrans = numGenBuffering + 1;
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
		}
		return *this;
	}

	void WriteToFile(std::string path) {
		std::ofstream of(path, std::ios_base::out);
		of << numGen << DELIMITER;
		of << genSize << DELIMITER;
		of << symbolSize << DELIMITER;
		of << apiRate << DELIMITER;
		of << sendRate << DELIMITER;
		of << numGenBuffering << DELIMITER;
		of << numGenRetrans << DELIMITER;
		of << numGenSingleTx << DELIMITER;
		of << fieldSize << DELIMITER;
		of << ccackLevels << DELIMITER;
		of << maxCoalitionSize << DELIMITER;
		of << mutualPhyLlcCoding << DELIMITER;
		of << per << DELIMITER;
		of << numRr << DELIMITER;
		of << (uint16_t)crCalcWay << DELIMITER;
		of << crNumSigma << DELIMITER;
		of << crReducFactor << DELIMITER;
		of << (uint16_t)rrCanSel << DELIMITER;
		of << (uint16_t)fbCont << DELIMITER;
		of << (uint16_t)giveRrPriorToSrc << DELIMITER;
		of << rrCanSend << DELIMITER;
		of.close();
	}

	void ReadFromFile(std::string path) {
		std::ifstream in_f(path, std::ios_base::in);
		in_f >> numGen;
		in_f >> genSize;
		in_f >> symbolSize;
		in_f >> apiRate;
		in_f >> sendRate;
		in_f >> numGenBuffering;
		in_f >> numGenRetrans;
		in_f >> numGenSingleTx;
		in_f >> fieldSize;
		in_f >> ccackLevels;
		in_f >> maxCoalitionSize;
		in_f >> mutualPhyLlcCoding;
		in_f >> per;
		in_f >> numRr;
		uint16_t v;
		in_f >> v;
		crCalcWay = CodeRedundancyCalcWay(v);
		in_f >> crNumSigma;
		in_f >> crReducFactor;
		in_f >> v;
		rrCanSel = RrCandidateSelection(v);
		in_f >> v;
		fbCont = TypeOfFeedback(v);
		in_f >> v;
		giveRrPriorToSrc = v;
		in_f >> v;
		rrCanSend = WhoCanSendRr(v);
		in_f.close();
	}

	void Print() {
		std::cout << "Simulation parameters: [" << std::endl;
		std::cout << "Number of generations\t\t\t" << numGen << std::endl;
		std::cout << "Generation size\t\t\t\t" << genSize << std::endl;
		std::cout << "Symbol size / bytes\t\t\t" << symbolSize << std::endl;
		std::cout << "Application data rate / bps\t\t" << apiRate << std::endl;
		std::cout << "Sending data rate / bps\t\t\t" << sendRate << std::endl;
		std::cout << "Number of buffering generations (Tx)\t" << numGenBuffering << std::endl;
		std::cout << "Number of buffering generations (RR)\t" << numGenRetrans << std::endl;
		std::cout << "Number of generataions in single MPDU\t" << numGenSingleTx << std::endl;
		std::cout << "Field size / power of 2\t\t\t" << fieldSize << std::endl;
		std::cout << "CCACK levels\t\t\t\t" << ccackLevels << std::endl;
		std::cout << "Maximum coalition size\t\t\t" << maxCoalitionSize << std::endl;
		std::cout << "Mutual PHY and LLC coding / flag\t" << mutualPhyLlcCoding << std::endl;
		std::cout << "Target packet loss ratio\t\t" << per << std::endl;
		std::cout << "Maximum number of RRs\t\t\t" << numRr << std::endl;
		std::cout << "CR calculation way\t\t\t" << (uint16_t)crCalcWay << std::endl;
		std::cout << "Number of sigmas\t\t\t" << crNumSigma << std::endl;
		std::cout << "CR reduction factor\t\t\t" << crReducFactor << std::endl;
		std::cout << "RR candidate selection way\t\t" << (uint16_t)rrCanSel << std::endl;
		std::cout << "Type of the feedback contents\t\t" << (uint16_t)fbCont << std::endl;
		std::cout << "Give RR priority to SRC\t\t\t" << (uint16_t)giveRrPriorToSrc << std::endl;
		std::cout << "Who can forward RR\t\t\t" << rrCanSend << std::endl;

		std::cout << "]" << std::endl;
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
};
}

#endif /* UTILS_SIM_PARAMETERS_H_ */
