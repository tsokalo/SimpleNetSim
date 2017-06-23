/*
 * utils.h
 *
 *  Created on: 27.10.2016
 *      Author: tsokalo
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <limits>
#include <vector>
#include <stdexcept>
#include <map>
#include <utility>
#include <algorithm>
#include <functional>
#include <cmath>

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "header.h"
#include "utils/coding-vector.h"

namespace ncr {
void
PrintSymbol(NcSymbol sym);

typedef std::vector<std::string> FileList;
typedef struct stat Stat;

void GetDirListing(FileList& result, const std::string& dirpath);
int16_t CreateDirectory(std::string path);
bool RemoveDirectory(std::string folderPath);
FileList FindFiles(std::string searchPath, std::string filePartName);

std::string GetLogFileName();
std::string GetSimParamFileName();
LogBank ReadLogBank(std::string path);

void PlotPriorities(uint32_t numNodes, std::vector<UanAddress> dstIds, LogBank lb, std::string path, bool useSns);
void PlotInputFilters(uint32_t numNodes, std::vector<UanAddress> dstIds, LogBank lb, std::string path);
void PlotLossRatios(std::vector<UanAddress> nids, LogBank lb, std::string path);
void PlotCoalitions(uint32_t numNodes, std::vector<UanAddress> dstIds, LogBank lb, std::string path, std::string logfile, bool useSns);
void PlotCodingRates(uint32_t numNodes, std::vector<UanAddress> dstIds, LogBank lb, std::string path, std::string logfile);
void PlotSendingStatistics(LogBank lb, std::string path, TdmAccessPlan optPlan, uint32_t warmup);
void PlotResourceWaste(LogBank lb, std::string path, double sigma, uint32_t warmup_period);
void PlotRatesPerDst(LogBank lb, std::string path, std::vector<UanAddress> dstIds, std::map<UanAddress, Datarate> d, uint32_t warmup_period);
void PlotRates(LogBank lb, std::string path, double opt, double single_opt, std::map<UanAddress, Datarate> d, uint32_t warmup_period, std::string sim_par);
void PlotRetransmissionRequests(LogBank lb, std::string path, uint32_t warmup_period);
void PlotOutputStability(LogBank lb, std::string path, double opt, UanAddress dst);
void PlotSrcPriorStability(LogBank lb, std::string path, double opt, UanAddress src);

void ExecuteCommand(const char * cmd);
CodingVector ExtractCodingVector(std::vector<uint8_t> payload, uint16_t genSize);
void PrintProgress(uint32_t m, uint32_t c);

//std::ostream& operator <<(std::ostream& os, MessType& m);
}

#endif /* UTILS_H_ */
