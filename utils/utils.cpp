/*
 * utils.cpp
 *
 *  Created on: 27.10.2016
 *      Author: tsokalo
 */

#include <string>
#include <bitset>

#include "utils.h"

namespace ncr {

void PrintSymbol(NcSymbol sym) {
	for (auto i : sym)
		std::cout << i << " ";
	std::cout << std::endl;
}

void GetDirListing(FileList& result, const std::string& dirpath) {
	DIR* dir = opendir(dirpath.c_str());
	if (dir) {
		struct dirent* entry;
		while ((entry = readdir(dir))) {
			struct stat entryinfo;
			std::string entryname = entry->d_name;
			std::string entrypath = dirpath + "/" + entryname;
			if (!stat(entrypath.c_str(), &entryinfo)) result.push_back(entrypath);
		}
		closedir(dir);
	}
}

int16_t CreateDirectory(std::string path) {
	//mode_t mode = 0x0666;
	Stat st;
	int32_t status = 0;

	if (stat(path.c_str(), &st) != 0) {
		/* Directory does not exist. EEXIST for race condition */
		if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) status = -1; //, mode
	} else if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		status = -1;
	}

	return status;
}

bool RemoveDirectory(std::string folderPath) {
	std::cout << "Deleting directory: " << folderPath << std::endl;
	FileList dirtree;
	GetDirListing(dirtree, folderPath);
	int32_t numofpaths = dirtree.size();

	for (int32_t i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int32_t pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		struct stat st_buf;
		stat(fullPath.c_str(), &st_buf);
		if (S_ISDIR(st_buf.st_mode)) {
			RemoveDirectory(fullPath);
		} else {
			std::remove(fullPath.c_str());
		}
		rmdir(fullPath.c_str());
	}
	return true;
}
FileList FindFiles(std::string searchPath, std::string filePartName) {
	//  std::cout << "Searching in directory: " << searchPath << " for file with " << filePartName << " in its name" << std::endl;
	FileList matchFullPath;
	FileList dirtree;
	GetDirListing(dirtree, searchPath);
	int32_t numofpaths = dirtree.size();

	for (int32_t i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int32_t pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		struct stat st_buf;
		stat(fullPath.c_str(), &st_buf);
		if (S_ISDIR(st_buf.st_mode)) {
			continue;
		} else {
			std::size_t found = str.find(filePartName);
			if (found != std::string::npos) {
				if (str.find('~') == std::string::npos) {
					matchFullPath.push_back(fullPath);
					//                  std::cout << "Found file: " << fullPath << std::endl;
				}
			}
		}
	}
	return matchFullPath;
}
std::string GetLogFileName() {
	return "log.txt";
}
std::string GetSimParamFileName() {
	return "sim_param.txt";
}
LogBank ReadLogBank(std::string path) {

	LogBank lb;

	std::ifstream f(path, std::ios_base::in);
	std::string line;

	//
	// ignore the first line - with header
	//
	getline(f, line, '\n');

	while (getline(f, line, '\n')) {
		std::stringstream ss(line);
		UanAddress addr;
		uint64_t t;
		uint16_t m;
		ss >> t;
		ss >> m;
		ss >> addr;
		lb[addr].push_back(LogPair(t, MessType(m), LogItem(line)));
		LogItem item(line);
	}
	f.close();
	return lb;
}

void PlotPriorities(uint32_t numNodes, std::vector<UanAddress> dstIds, LogBank lb, std::string path, bool useSns) {

	std::cout << "Num nodes " << numNodes;
	for (auto dst : dstIds)
		std::cout << ", dst: " << dst;
	std::cout << std::endl;

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";

	for (auto dst : dstIds) {
		std::string figure_file = res_dir + "priorities_" + std::to_string(dst) + ".svg";

		//
		// make data file
		//
		typedef std::map<UanAddress, std::map<uint64_t, priority_t> > priorities_t;
		priorities_t priorities;
		for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
			for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
				if (tt->log.dst == dst) {
					if (dst == t->first) continue;
					priorities[t->first][tt->t] = tt->log.p;
				}
			}
		}
		std::ofstream fd(data_file, std::ios_base::out);
		for (priorities_t::iterator t = priorities.begin(); t != priorities.end(); t++) {
			for (std::map<uint64_t, priority_t>::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
				if (tt->second == DESTINATION_PRIORITY) continue;
				fd << tt->first << "\t" << t->first << "\t" << tt->second << std::endl;
			}
		}
		fd.close();

		//
		// make plot command
		//
		std::stringstream plot_command;

		plot_command << "set output '" << figure_file << "'" << std::endl;
		plot_command << "plot ";
		for (uint16_t i = 0; i < numNodes; i++) {
			if (i == dst) continue;
			if (useSns) {
				plot_command << "\"" << data_file << "\" every 10" << " using ($1):(($2==" << i << ") ? $3/1000000 : 1/0)";
			} else {
				plot_command << "\"" << data_file << "\" every 10" << " using ($1/1000):(($2==" << i << ") ? $3 / 1024 : 1/0)";
			}
			plot_command << " with linespoints ls 1 lw 1 linecolor " << i + 1 << " pt 7 ps 0.3 title \"vertex=" << i << "\"";
			if (i != numNodes - 1) plot_command << ",\\" << std::endl;
		};;

		auto str_to_const_char = [](std::string str)
		{
			return str.c_str();
		};
		;
		//
		// make plot
		//
		std::string gnuplot_filename = gnuplot_dir + (useSns ? "plot_priorities_sns.p" : "plot_priorities.p");
		std::string gnuplot_filename_temp = gnuplot_dir + "plot_priorities_temp.p";
		ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
		std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
		f << plot_command.str();
		f.close();
		ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
	}
}
void PlotInputFilters(uint32_t numNodes, std::vector<UanAddress> dstIds, LogBank lb, std::string path) {

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";

	for (auto dst : dstIds) {

		//
		// make data file
		//
		double prev_value = 0;
		typedef std::map<UanAddress, std::map<UanAddress, bool> > node_pairs_t;
		node_pairs_t node_pairs;
		std::ofstream fd(data_file, std::ios_base::out);
		for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
			for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
				for (std::map<int16_t, double>::iterator ttt = tt->log.fp.begin(); ttt != tt->log.fp.end(); ttt++) {
					node_pairs[t->first][ttt->first] = true;
					if (tt->log.dst == dst) {
						if (ttt == tt->log.fp.begin() || !eq(prev_value, ttt->second)) {
							prev_value = ttt->second;
							fd << tt->t << "\t" << t->first << "\t" << ttt->first << "\t" << 1 - prev_value << std::endl;
						}
					}
				}
			}
		}
		fd.close();

		for (node_pairs_t::iterator it = node_pairs.begin(); it != node_pairs.end(); it++) {
			//
			// make plot command
			//
			std::stringstream plot_command;
			std::stringstream figure_file;
			figure_file << res_dir << "input_filters_" << it->first << "_" << dst << ".svg";
			plot_command << "set output '" << figure_file.str() << "'" << std::endl;
			plot_command << "plot ";
			for (std::map<UanAddress, bool>::iterator itt = it->second.begin(); itt != it->second.end();) {

				plot_command << "\"" << data_file << "\" every 10" << " using 1:(($2==" << it->first << "&&$3==" << itt->first << ") ? $4 : 1/0)";
				plot_command << " with linespoints ls 1 lw 1 linecolor " << itt->first + 1 << " pt 7 ps 0.3 title \"edge=<" << itt->first << "," << it->first
						<< ">\"";
				itt++;
				if (itt != it->second.end()) plot_command << ",\\" << std::endl;
			};;

			auto str_to_const_char = [](std::string str)
			{
				return str.c_str();
			};
			;
			//
			// make plot
			//
			std::string gnuplot_filename = gnuplot_dir + "plot_input_filters.p";
			std::string gnuplot_filename_temp = gnuplot_dir + "plot_input_filters_temp.p";
			ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
			std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
			f << plot_command.str();
			f.close();
			ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
		}
	}
}
void PlotLossRatios(std::vector<UanAddress> nids, LogBank lb, std::string path) {

	std::cout << "Plot loss ratios" << std::endl;
	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";

	//
	// make data file
	//
	typedef std::map<UanAddress, std::map<UanAddress, bool> > node_pairs_t;
	node_pairs_t node_pairs;
	std::map<UanAddress, std::map<UanAddress, double> > v;
	std::ofstream fd(data_file, std::ios_base::out);
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
			for (std::map<int16_t, double>::iterator ttt = tt->log.eps.begin(); ttt != tt->log.eps.end(); ttt++) {
				if (v[t->first][ttt->first] == ttt->second) continue;
				v[t->first][ttt->first] = ttt->second;
				if (ttt->second != 0) node_pairs[t->first][ttt->first] = true;
				fd << tt->t << "\t" << t->first << "\t" << ttt->first << "\t" << ttt->second << std::endl;
			}
		}
	}
	fd.close();

	for (node_pairs_t::iterator it = node_pairs.begin(); it != node_pairs.end(); it++) {
		//
		// make plot command
		//
		std::stringstream plot_command;
		std::stringstream figure_file;
		figure_file << res_dir << "loss_ratios_" << it->first << ".svg";
		plot_command << "set output '" << figure_file.str() << "'" << std::endl;
		plot_command << "plot ";
		for (std::map<UanAddress, bool>::iterator itt = it->second.begin(); itt != it->second.end();) {

			plot_command << "\"" << data_file << "\"" << " using 1:(($2==" << it->first << "&&$3==" << itt->first << ") ? $4 : 1/0)";
			plot_command << " with linespoints ls 1 lw 1 linecolor " << itt->first + 1 << " pt 7 ps 0.3 title \"edge=<" << it->first << "," << itt->first
					<< ">\"";
			itt++;
			if (itt != it->second.end()) plot_command << ",\\" << std::endl;
		};;

		auto str_to_const_char = [](std::string str)
		{
			return str.c_str();
		};
		;
		//
		// make plot
		//
		std::string gnuplot_filename = gnuplot_dir + "plot_loss_ratios.p";
		std::string gnuplot_filename_temp = gnuplot_dir + "plot_loss_ratios_temp.p";
		ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
		std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
		f << plot_command.str();
		f.close();
		ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
	}
}
void PlotCoalitions(uint32_t numNodes, std::vector<UanAddress> dstIds, LogBank lb, std::string path, std::string logfile, bool useSns) {

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";

	for (auto dst : dstIds) {
		std::string figure_file = res_dir + "coalitions_" + std::to_string(dst) + ".svg";

		//
		// make data file
		//
		typedef std::map<UanAddress, std::map<uint64_t, uint16_t> > csizes_t;
		csizes_t csizes;
		for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
			for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
				if (tt->log.dst == dst) {
					if (dst == t->first) continue;
					csizes[t->first][tt->t] = tt->log.cs;
				}
			}
		}
		std::ofstream fd(data_file, std::ios_base::out);
		for (csizes_t::iterator t = csizes.begin(); t != csizes.end(); t++) {
			for (std::map<uint64_t, uint16_t>::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
				fd << tt->first << "\t" << t->first << "\t" << tt->second << std::endl;
			}
		}
		fd.close();

		//
		// make plot command
		//
		std::stringstream plot_command;

		plot_command << "set output '" << figure_file << "'" << std::endl;
		plot_command << "plot ";
		for (uint16_t i = 0; i < numNodes; i++) {

			if (i == dst) continue;
			if (useSns) {
				plot_command << "\"" << data_file << "\" every 10" << " using ($1):(($2==" << i << ") ? $3 : 1/0)";
			} else {
				plot_command << "\"" << data_file << "\" every 10" << " using ($1/1000):(($2==" << i << ") ? $3 : 1/0)";
			}

			plot_command << " with linespoints ls 1 lw 1 linecolor " << i + 1 << " pt 7 ps 0.3 title \"vertex=" << i << "\"";
			if (i != numNodes - 1) plot_command << ",\\" << std::endl;
		};;

		auto str_to_const_char = [](std::string str)
		{
			return str.c_str();
		};
		;
		//
		// make plot
		//
		std::string gnuplot_filename = gnuplot_dir + (useSns ? "plot_coalitions_sns.p" : "plot_coalitions.p");
		std::string gnuplot_filename_temp = gnuplot_dir + "plot_coalitions_temp.p";
		ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
		std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
		f << plot_command.str();
		f.close();
		ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
	}
}
void PlotCodingRates(uint32_t numNodes, std::vector<UanAddress> dstIds, LogBank lb, std::string path, std::string logfile) {

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";

	for (auto dst : dstIds) {
		std::string figure_file = res_dir + "coding_rate_" + std::to_string(dst) + ".svg";

		//
		// make data file
		//
		typedef std::map<UanAddress, std::map<uint64_t, double> > crs_t;
		crs_t crs;
		for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
			for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
				if (tt->log.dst == dst) {
					if (dst == t->first) continue;
					crs[t->first][tt->t] = 1 / tt->log.cr;
				}
			}
		}
		std::ofstream fd(data_file, std::ios_base::out);
		for (crs_t::iterator t = crs.begin(); t != crs.end(); t++) {
			for (std::map<uint64_t, double>::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
				fd << tt->first << "\t" << t->first << "\t" << tt->second << std::endl;
			}
		}
		fd.close();

		//
		// make plot command
		//
		std::stringstream plot_command;

		plot_command << "set output '" << figure_file << "'" << std::endl;
		plot_command << "plot ";
		for (uint16_t i = 0; i < numNodes; i++) {

			if (i == dst) continue;
			plot_command << "\"" << data_file << "\" every 10" << " using ($1):(($2==" << i << ") ? $3 : 1/0)";
			plot_command << " with linespoints ls 1 lw 1 linecolor " << i + 1 << " pt 7 ps 0.3 title \"vertex=" << i << "\"";
			if (i != numNodes - 1) plot_command << ",\\" << std::endl;
		};;

		auto str_to_const_char = [](std::string str)
		{
			return str.c_str();
		};
		;
		//
		// make plot
		//
		std::string gnuplot_filename = gnuplot_dir + "plot_coding_rate.p";
		std::string gnuplot_filename_temp = gnuplot_dir + "plot_coding_rate_temp.p";
		ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
		std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
		f << plot_command.str();
		f.close();
		ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
	}
}

void PlotSendingStatistics(LogBank lb, std::string path, TdmAccessPlan optPlan, uint32_t warmup, uint32_t warmdown) {

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";
	std::string figure_file = res_dir + "send_statistics.svg";

	//
	// make data file
	//
	gen_ssn_t gsn = 0;
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::reverse_iterator tt = t->second.rbegin(); tt != t->second.rend(); tt++) {
			if (tt->t <= warmdown) {
				gsn = tt->log.gsn;
				break;
			}
		}
	}
	std::map<UanAddress, uint64_t> sum_send;
	uint64_t totalSum = 0;
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
			if (tt->t < warmup) continue;
			if (tt->log.gsn > gsn) continue;
			if (tt->m == DATA_MSG_TYPE) sum_send[t->first] += tt->log.ns;
			if (tt->m == DATA_MSG_TYPE) totalSum += tt->log.ns;
		}
	}

	TdmAccessPlan actualPlan;
	for (std::map<UanAddress, uint64_t>::iterator t = sum_send.begin(); t != sum_send.end(); t++) {
		actualPlan[t->first] = (long double) t->second / (long double) totalSum;
	}

	auto add_zeros = [](TdmAccessPlan &p1, TdmAccessPlan &p2)
	{
		for (TdmAccessPlan::iterator t = p1.begin(); t != p1.end(); t++) {
			p2[t->first];
		}
		for (TdmAccessPlan::iterator t = p2.begin(); t != p2.end(); t++) {
			p1[t->first];
		}
	};
	;

	add_zeros(actualPlan, optPlan);

	assert(actualPlan.size() == optPlan.size());
	std::ofstream fd(data_file, std::ios_base::out);
	auto opt_it = optPlan.begin();

	fd << "Node" << "\t" << "Simulation" << "\t" << "Optimal" << std::endl;
	for (TdmAccessPlan::iterator t = actualPlan.begin(); t != actualPlan.end(); t++) {
		assert(opt_it->first == t->first);
		fd << t->first << "\t" << t->second << "\t" << opt_it->second << std::endl;
		opt_it++;
	}
	//	fd << "Node" << "\t" << "Simulation" << "\t" << "Calculation" << "\t" << "Optimal" << std::endl;
	//	for (TdmAccessPlan::iterator t = actualPlan.begin(); t != actualPlan.end(); t++) {
	//		assert(god_it->first == t->first && opt_it->first == t->first);
	//		fd << t->first << "\t" << t->second << "\t" << god_it->second << "\t" << opt_it->second << std::endl;
	//		god_it++;
	//		opt_it++;
	//	}
	fd.close();

	//
	// make plot command
	//
	std::stringstream plot_command;

	plot_command << "set output '" << figure_file << "'" << std::endl;

	plot_command << "plot ";
	plot_command << "\"" << data_file << "\"";
	plot_command << " using 2 ti col, '' using 3 ti col, '' using 4:xticlabels(1) ti col";

	auto str_to_const_char = [](std::string str)
	{
		return str.c_str();
	};
	;
	//
	// make plot
	//
	std::string gnuplot_filename = gnuplot_dir + "plot_send_statistics.p";
	std::string gnuplot_filename_temp = gnuplot_dir + "plot_send_statistics_temp.p";
	ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
	std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
	f << plot_command.str();
	f.close();
	ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
}

void PlotResourceWaste(LogBank lb, std::string path, double sigma, uint32_t warmup, uint32_t warmdown) {
	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";
	std::string figure_file = res_dir + "resource_waste.svg";

	//
	// calculate total amount of linear independent packets received by the destination
	// calculate total amount of feedback messages sent by all nodes
	// calculate total amount of network discovery messages sent by all nodes
	// calculate total amount of sent messages by all nodes
	//
	uint32_t nr = 0, fb = 0, nd = 0, ns = 0, nrr = 0;

	gen_ssn_t gsn = 0;
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::reverse_iterator tt = t->second.rbegin(); tt != t->second.rend(); tt++) {
			if (tt->t <= warmdown) {
				gsn = tt->log.gsn;
				break;
			}
		}
	}
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
			if (tt->t < warmup) continue;
			if (tt->log.gsn > gsn) continue;
			if (tt->m == FEEDBACK_MSG_TYPE) fb += tt->log.ns;
			if (tt->m == NETDISC_MSG_TYPE) nd += tt->log.ns;
			if (tt->m == RETRANS_REQUEST_MSG_TYPE) nrr += tt->log.ns;
			if (tt->m == DATA_MSG_TYPE) ns += tt->log.ns;
			if (tt->log.p == DESTINATION_PRIORITY) nr += tt->log.nr;
		}
	}

	ns = ns + fb + nd + nrr;

	std::cout << "ns " << ns << " fb " << fb << " nd " << nd << " nr " << nr << " nrr " << nrr << " sigma " << sigma << std::endl;
	std::ofstream fd(data_file, std::ios_base::out);
	fd << "\"NDM\"\t" << (double) nd / (double) ns * 100 << std::endl;
	fd << "\"\\RPIM\"\t" << (double) fb / (double) ns * 100 << std::endl;
	fd << "\"RRM\"\t" << (double) nrr / (double) ns * 100 << std::endl;
	fd << "\"\\nExcessive redundancy\"\t" << (double) (ns - fb - nd - nrr - (double) nr / sigma) / (double) ns * 100 << std::endl;
	fd << "\"Main data\"\t" << (double) nr / sigma / (double) ns * 100 << std::endl;
	fd.close();

	//
	// make plot command
	//
	std::stringstream plot_command;

	plot_command << "set output '" << figure_file << "'" << std::endl;
	plot_command << "plot ";
	plot_command << "\"" << data_file << "\"" << " using 2:xticlabels(1)";
	plot_command << " with boxes ls 1 lw 1 linecolor 3 notitle";

	auto str_to_const_char = [](std::string str)
	{
		return str.c_str();
	};
	;
	//
	// make plot
	//
	std::string gnuplot_filename = gnuplot_dir + "plot_resource_waste.p";
	std::string gnuplot_filename_temp = gnuplot_dir + "plot_resource_waste_temp.p";
	ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
	std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
	f << plot_command.str();
	f.close();
	ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
}
void PlotRatesPerDst(LogBank lb, std::string path, std::vector<UanAddress> dstIds, std::map<UanAddress, Datarate> d, uint32_t warmup, uint32_t warmdown) {
	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";
	std::string figure_file = res_dir + "rates_dsts.svg";

	std::map<UanAddress, uint32_t> nr;
	std::map<UanAddress, uint32_t> ns;

	gen_ssn_t gsn = 0;
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::reverse_iterator tt = t->second.rbegin(); tt != t->second.rend(); tt++) {
			if (tt->t <= warmdown) {
				gsn = tt->log.gsn;
				break;
			}
		}
	}
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
			if (tt->t < warmup) continue;
			if (tt->log.gsn > gsn) continue;
			if (tt->log.p == DESTINATION_PRIORITY) {
				nr[tt->log.dst] += tt->log.nr;
			}
			ns[t->first] += tt->log.ns;
		}
	}

	//
	// unit [packet per bit per second]
	//
	double dur = 0;
	for (auto n : ns) {
		assert(d.find(n.first) != d.end());
		dur += n.second / d.at(n.first);
	}

	//
	// make data file
	//
	std::ofstream fd(data_file, std::ios_base::out);
	for (auto dst : dstIds) {
		fd << dst << "\t" << (double) nr[dst] / dur / 1000000 << std::endl;
	}
	fd.close();

	//
	// make plot command
	//
	std::stringstream plot_command;

	plot_command << "set output '" << figure_file << "'" << std::endl;
	plot_command << "plot ";
	plot_command << "\"" << data_file << "\"" << " using 2:xticlabels(1)";
	plot_command << " with boxes ls 1 lw 1 linecolor 3 notitle";

	auto str_to_const_char = [](std::string str)
	{
		return str.c_str();
	};
	;
	//
	// make plot
	//
	std::string gnuplot_filename = gnuplot_dir + "plot_rates_dsts.p";
	std::string gnuplot_filename_temp = gnuplot_dir + "plot_rates_dsts_temp.p";
	ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
	std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
	f << plot_command.str();
	f.close();
	ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));

}

void PlotRates(LogBank lb, std::string path, double opt, double single_opt, std::map<UanAddress, Datarate> d, uint32_t warmup, uint32_t warmdown,
		uint64_t simdur, std::string sim_par) {

	gen_ssn_t gsn = 0;
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::reverse_iterator tt = t->second.rbegin(); tt != t->second.rend(); tt++) {
			if (tt->t <= warmdown) {
				gsn = tt->log.gsn;
				break;
			}
		}
	}

	{
		//
		// average values
		//

		std::string gnuplot_dir = path + "gnuplot/";
		std::string res_dir = path + "Results/";
		std::string data_file = gnuplot_dir + "data.txt";
		std::string figure_file = res_dir + "rates.svg";

		//
		// calculate total amount of linear independent packets received by the destination
		// calculate total amount of feedback messages sent by all nodes
		// calculate total amount of network discovery messages sent by all nodes
		// calculate total amount of sent messages by all nodes
		//
		uint32_t nr = 0, nru = 0;
		std::map<UanAddress, uint32_t> ns;

		for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
			for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
				if (tt->t < warmup) continue;
				if (tt->log.gsn > gsn) continue;
				if (tt->log.p == DESTINATION_PRIORITY && tt->m != ORIG_MSG_TYPE) nr += tt->log.nr;
				if (tt->log.p == DESTINATION_PRIORITY && tt->m == ORIG_MSG_TYPE && tt->log.ssn != 0) nru++;
				ns[t->first] += tt->log.ns;
			}
		}

		//
		// unit [packet per bit per second]
		//
		double dur = 0;
		for (auto n : ns) {
			assert(d.find(n.first) != d.end());
			dur += n.second / d.at(n.first);
		}

		assert(!eq(dur, 0));

		//
		// make data file
		//
		{
			std::ofstream fd(data_file, std::ios_base::out);
			fd << "\"Simulation (decoded)\"\t" << (double) nru / dur / 1000000 << std::endl;
			fd << "\"\\nSimulation (coded)\"\t" << (double) nr / dur / 1000000 << std::endl;
			fd << "\"Maximum with ORP\"\t" << opt / 1000000 << std::endl;
			fd << "\"\\nMaximum with SRP\"\t" << single_opt / 1000000 << std::endl;
			fd.close();
		}
		{
			std::ofstream fd(path + "noc_sim_res.txt", std::ios_base::out | std::ios_base::app);
			fd << sim_par << "\t" << (double) nru / dur / 1000000 << "\t" << (double) nr / dur / 1000000 << "\t" << opt / 1000000 << "\t"
					<< single_opt / 1000000 << std::endl;
			fd.close();

		}

		//
		// make plot command
		//
		std::stringstream plot_command;

		plot_command << "set output '" << figure_file << "'" << std::endl;
		plot_command << "plot ";
		plot_command << "\"" << data_file << "\"" << " using 2:xticlabels(1)";
		plot_command << " with boxes ls 1 lw 1 linecolor 3 notitle";

		auto str_to_const_char = [](std::string str)
		{
			return str.c_str();
		};
		;
		//
		// make plot
		//
		std::string gnuplot_filename = gnuplot_dir + "plot_rates.p";
		std::string gnuplot_filename_temp = gnuplot_dir + "plot_rates_temp.p";
		ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
		std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
		f << plot_command.str();
		f.close();
		ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
	}
	//////////////////////////////////////////////////////////////////////////////////

	{
		//
		// time series
		//

		std::string gnuplot_dir = path + "gnuplot/";
		std::string res_dir = path + "Results/";
		std::string data_file = gnuplot_dir + "data.txt";
		std::string figure_file = res_dir + "rates_time_series.svg";

		//
		// calculate total amount of linear independent packets received by the destination
		// calculate total amount of feedback messages sent by all nodes
		// calculate total amount of network discovery messages sent by all nodes
		// calculate total amount of sent messages by all nodes
		//
		struct slot {

			//
			// unit [packet per bit per second]
			//
			double dur;
			double nr;
			double nru;
		};

		uint32_t ave_win = 100;
		auto get_win_n = [&](uint64_t t)
		{
			return floor((long double)t / (double)ave_win);
		};

		std::map<uint32_t, slot> n;

		for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {

			for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {

				auto ti = get_win_n(tt->t);

				if (tt->t > warmup && tt->log.gsn < gsn) {
					if (tt->log.p == DESTINATION_PRIORITY && tt->m != ORIG_MSG_TYPE) n[ti].nr += tt->log.nr;
					if (tt->log.p == DESTINATION_PRIORITY && tt->m == ORIG_MSG_TYPE && tt->log.ssn != 0) n[ti].nru++;
				}

				if (tt->log.ns != 0) n[ti].dur += 1 / d.at(t->first);
			}
		}

		//
		// make data file
		//
		{
			std::ofstream fd(data_file, std::ios_base::out);

			double t = 0;
			for (auto i : n) {
				t += i.second.dur;
				fd << t * 1000 << "\t" << i.second.nr / i.second.dur / 1000000 << "\t" << i.second.nru / i.second.dur / 1000000 << std::endl;
			}
			fd.close();
		}

		//
		// make plot command
		//
		std::stringstream plot_command;

		plot_command << "set output '" << figure_file << "'" << std::endl;
		plot_command << "plot ";
		plot_command << "\"" << data_file << "\"" << " using 1:2";
		plot_command << " with linespoints ls 1 lw 1 pt 7 ps 0.3 linecolor 1 title \"Simulation (coded)\",\\" << std::endl;
		plot_command << "\"" << data_file << "\"" << " using 1:3";
		plot_command << " with linespoints ls 1 lw 1 pt 5 ps 0.3 linecolor 2 title \"Simulation (decoded)\"";

		auto str_to_const_char = [](std::string str)
		{
			return str.c_str();
		};
		;
		//
		// make plot
		//
		std::string gnuplot_filename = gnuplot_dir + "plot_rates_time_series.p";
		std::string gnuplot_filename_temp = gnuplot_dir + "plot_rates_time_series_temp.p";
		ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
		std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
		f << plot_command.str();
		f.close();
		ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));

	}
}

void PlotRanks(LogBank lb, std::string path, uint32_t warmup, uint32_t warmdown) {
	//
	// time series
	//

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";
	std::string figure_file = res_dir + "ranks.svg";

	gen_ssn_t gsn = 0;
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::reverse_iterator tt = t->second.rbegin(); tt != t->second.rend(); tt++) {
			if (tt->t <= warmdown) {
				gsn = tt->log.gsn;
				break;
			}
		}
	}
	std::map<uint32_t, uint16_t> ranks;
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {

		for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {

			if (tt->t > warmup && tt->log.gsn < gsn) {
				if (tt->log.p == DESTINATION_PRIORITY && ranks[tt->log.gsn.val()] < tt->log.rank) ranks[tt->log.gsn.val()] = tt->log.rank;
			}
		}
	}

	//
	// make data file
	//
	{
		std::ofstream fd(data_file, std::ios_base::out);

		for (auto i : ranks) {
			fd << i.first << "\t" << i.second << std::endl;
		}
		fd.close();
	}

	//
	// make plot command
	//
	std::stringstream plot_command;

	plot_command << "set output '" << figure_file << "'" << std::endl;
	plot_command << "plot ";
	plot_command << "\"" << data_file << "\"" << " using 1:2";
	plot_command << " with linespoints ls 1 lw 1 pt 7 ps 0.3 linecolor 1 notitle" << std::endl;

	auto str_to_const_char = [](std::string str)
	{
		return str.c_str();
	};
	;
	//
	// make plot
	//
	std::string gnuplot_filename = gnuplot_dir + "plot_ranks.p";
	std::string gnuplot_filename_temp = gnuplot_dir + "plot_ranks_temp.p";
	ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
	std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
	f << plot_command.str();
	f.close();
	ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));

}

void PlotRetransmissionRequests(LogBank lb, std::string path, uint32_t warmup, uint32_t warmdown) {

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";
	std::string figure_file = res_dir + "retrans_requests.svg";

	//
	// calculate the number of retransmission requests per node
	//
	uint32_t nrr_total = 0;
	std::map<UanAddress, uint32_t> nrr;

	gen_ssn_t gsn = 0;
	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::reverse_iterator tt = t->second.rbegin(); tt != t->second.rend(); tt++) {
			if (tt->t <= warmdown) {
				gsn = tt->log.gsn;
				break;
			}
		}
	}

	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {
		for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
			if (tt->t < warmup) continue;
			if (tt->log.gsn > gsn) continue;
			if (tt->m == RETRANS_REQUEST_MSG_TYPE) {
				nrr[t->first] += tt->log.ns;
				nrr_total += tt->log.ns;
			}
		}
	}

	std::ofstream fd(data_file, std::ios_base::out);
	for (auto n : nrr) {

		auto v = (nrr_total == 0) ? 0.0 : (double) n.second / (double) nrr_total * 100;
		fd << n.first << "\t" << v << std::endl;
	}
	fd.close();

	//
	// make plot command
	//
	std::stringstream plot_command;

	plot_command << "set output '" << figure_file << "'" << std::endl;
	plot_command << "plot ";
	plot_command << "\"" << data_file << "\"" << " using 2:xticlabels(1)";
	plot_command << " with boxes ls 1 lw 1 linecolor 3 notitle";

	auto str_to_const_char = [](std::string str)
	{
		return str.c_str();
	};
	;
	//
	// make plot
	//
	std::string gnuplot_filename = gnuplot_dir + "plot_retrans_requests.p";
	std::string gnuplot_filename_temp = gnuplot_dir + "plot_retrans_requests_temp.p";
	ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
	std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
	f << plot_command.str();
	f.close();
	ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));

}

void PlotOutputStability(LogBank lb, std::string path, double opt, UanAddress dst) {

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";
	std::string figure_file = res_dir + "stability.svg";

	//
	// calculate the number of received symbols by the destination (for each batch)
	// calculate the total number of sent symbols (for each batch)
	// calculate the reception data rate by the destination (for each batch)
	//
	uint16_t num_batches = 100;
	uint32_t batch_size = (long double) (lb.begin()->second.end() - 1)->t / (double) (num_batches);
	std::vector<double> dr(num_batches, 0), sent(num_batches, 0);
	uint32_t c_in_b, c_out_b = 0;
	uint32_t sum_send = 0, sum_rcvd = 0;

	for (LogBank::iterator t = lb.begin(); t != lb.end(); t++) {

		c_out_b = 0;
		for (LogHistory::iterator tt = t->second.begin(); tt != t->second.end(); tt++) {
			if (tt->m == DATA_MSG_TYPE) {

				if (t->first == dst) {
					dr.at(c_out_b) += tt->log.nr;
				} else {
					sent.at(c_out_b) += tt->log.ns;
				}

				c_in_b = tt->t - batch_size * c_out_b;
				if (c_in_b >= batch_size) {
					c_out_b++;
					if (c_out_b >= num_batches) break;
				}

			}
		}
	}

	for (uint16_t i = 0; i < num_batches; i++) {
//		assert(dr.at(i) != 0);
		if (sent.at(i) == 0) return;
		dr.at(i) /= sent.at(i);
	}

	//
	// calculate error in batches
	//
	for (auto &d : dr) {
		assert(opt != 0);
		d -= opt;
		d /= opt;
	}

	//
	// make data file
	//
	std::ofstream fd(data_file, std::ios_base::out);
	for (auto i = 0; i < dr.size(); i++)
		fd << i << "\t" << dr.at(i) << std::endl;
	fd.close();

	//
	// make plot command
	//
	std::stringstream plot_command;

	plot_command << "set output '" << figure_file << "'" << std::endl;
	plot_command << "plot ";
	plot_command << "\"" << data_file << "\"" << " using 1:2";
	plot_command << " with linespoints ls 1 lw 1 linecolor 1 pt 7 ps 0.3 notitle" << std::endl;

	auto str_to_const_char = [](std::string str)
	{
		return str.c_str();
	};
	;
	//
	// make plot
	//
	std::string gnuplot_filename = gnuplot_dir + "plot_stability.p";
	std::string gnuplot_filename_temp = gnuplot_dir + "plot_stability_temp.p";
	ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
	std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
	f << plot_command.str();
	f.close();
	ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
}

void PlotSrcPriorStability(LogBank lb, std::string path, double opt, UanAddress src) {

	std::string gnuplot_dir = path + "gnuplot/";
	std::string res_dir = path + "Results/";
	std::string data_file = gnuplot_dir + "data.txt";
	std::string figure_file = res_dir + "stability_priority.svg";

	//
	// calculate the number of received symbols by the destination (for each batch)
	// calculate the total number of sent symbols (for each batch)
	// calculate the reception data rate by the destination (for each batch)
	//
	uint16_t num_batches = 100;
	uint32_t batch_size = (long double) (lb.begin()->second.end() - 1)->t / (double) (num_batches);
	std::vector<double> dr(num_batches, 0);
	std::vector<double> dt(num_batches, 0);
	uint32_t c_in_b, c_out_b = 0;
	uint32_t sum_send = 0, sum_rcvd = 0;

	auto lh = lb[src];

	for (LogHistory::iterator tt = lh.begin(); tt != lh.end(); tt++) {
		if (tt->m == DATA_MSG_TYPE) {

			dr.at(c_out_b) = tt->log.p + dr.at(c_out_b);
			dt.at(c_out_b)++;

			c_in_b
			= tt->t - batch_size * c_out_b;
			if (c_in_b >= batch_size) {
				c_out_b++;
				if (c_out_b >= num_batches) break;
			}
		}
	}

	//
	// calculate error in batches
	//
	for (uint16_t i = 0; i < dr.size(); i++) {
		assert(dr.at(i) != 0);
		assert(dt.at(i) != 0);
		dr.at(i) /= dt.at(i);
		dr.at(i) -= opt;
		dr.at(i) /= opt;
	}

	//
	// make data file
	//
	std::ofstream fd(data_file, std::ios_base::out);
	for (auto i = 0; i < dr.size(); i++)
		fd << i << "\t" << dr.at(i) << std::endl;
	fd.close();

	//
	// make plot command
	//
	std::stringstream plot_command;

	plot_command << "set output '" << figure_file << "'" << std::endl;
	plot_command << "plot ";
	plot_command << "\"" << data_file << "\"" << " using 1:2";
	plot_command << " with linespoints ls 1 lw 1 linecolor 1 pt 7 ps 0.3 title \"Simulation duration 40000\"" << std::endl;

	auto str_to_const_char = [](std::string str)
	{
		return str.c_str();
	};
	;
	//
	// make plot
	//
	std::string gnuplot_filename = gnuplot_dir + "plot_stability_priority.p";
	std::string gnuplot_filename_temp = gnuplot_dir + "plot_stability_priority_temp.p";
	ExecuteCommand(str_to_const_char("cp " + gnuplot_filename + " " + gnuplot_filename_temp));
	std::ofstream f(gnuplot_filename_temp, std::ios_base::out | std::ios_base::app);
	f << plot_command.str();
	f.close();
	ExecuteCommand(str_to_const_char("gnuplot " + gnuplot_filename_temp));
}
void ExecuteCommand(const char * cmd) {
	auto r = system(cmd);
	std::cout << "Execution <" << r << ">" << cmd << std::endl;
}

CodingVector ExtractCodingVector(std::vector<uint8_t> payload, uint16_t genSize) {
	CodingVector cv;
	cv.insert(cv.begin(), payload.begin() + 1, payload.begin() + 1 + genSize);
	return cv;
}
void PrintProgress(uint32_t m, uint32_t c) {
	float progress = (double) c / (double) m;

	int barWidth = 100;

	std::cout << "[";
	int pos = barWidth * progress;
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	std::cout << "] " << ceil(progress * 100.0) << " %\r";
	std::cout.flush();
}
//std::ostream& operator <<(std::ostream& os, MessType& m) {
//	if (m == DATA_MSG_TYPE) os << "DATA";
//	if (m == FEEDBACK_MSG_TYPE) os << "FEEDBACK";
//	if (m == NETDISC_MSG_TYPE) os << "DISCOVERY";
//	if (m == RETRANS_REQUEST_MSG_TYPE) os << "RR";
//	if (m == ORIG_MSG_TYPE) os << "ORIGINAL";
//	return os;
//}
}
