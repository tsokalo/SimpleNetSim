/*
 * logger.h
 *
 *  Created on: 22.11.2016
 *      Author: tsokalo
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "header.h"
#include "utils.h"

#include <stdint.h>
#include <functional>
#include <memory>
#include <deque>
#include <vector>
#include <fstream>

namespace ncr {

class Logger {

public:

	Logger(std::string path) :
			m_outFile(path + GetLogFileName(), std::ios_base::out) {

		m_t = 0;
		m_msgType = DATA_MSG_TYPE;
		m_outFile << "T" << "\t" << "M" << "\t" << "N" << "\t" << "D" << "\t" << "P" << "\t" << "CR" << "\t" << "CS" << std::endl;
	}

	~Logger() {
		m_outFile.close();
	}

	void IncTime(uint64_t t = 0) {
		if (t != 0) {
			m_t = t;
		}
		else {
			m_t++;
		}
	}
	void AddLog(LogItem item, int16_t node_id) {
		m_outFile << m_t << "\t" << m_msgType << "\t" << node_id << "\t" << item << std::endl;
	}
	void SetMessType(MessType t) {
		m_msgType = t;
	}

private:

	uint64_t m_t;
	MessType m_msgType;

	std::ofstream m_outFile;
};
}

#endif /* LOGGER_H_ */
