/*
 * log.h
 *
 *  Created on: 02.10.2016
 *      Author: tsokalo
 */

#ifndef LOG_H_
#define LOG_H_

#include <iostream>

#define COMM_NET_LOG            0
#define COMM_NODE_LOG           0
#define EDGE_LOG                0
#define LOSS_PROCESS_LOG        0
#define NC_SYMBOL_LOG           0
#define NC_POLICY_LOG           0
#define GOD_VIEW                0
#define EXOR_SOLVER_LOG         0
#define BRR_LOG                 0
#define ARQ_LOG                 0
#define FILTER_LOG              0
#define SC_POL_LOG              0
#define TRAF_GEN_LOG            0
#define CODER_LOG				0
#define SIMULATOR_LOG			0
#define GRAPH_LOG     			1
#define LPSOLVER_LOG    		0
#define CCACK_LOG	    		0
#define TEMP_LOG				0

//#define COMM_NET_LOG            1
//#define COMM_NODE_LOG           1
//#define EDGE_LOG                1
//#define LOSS_PROCESS_LOG        1
//#define NC_SYMBOL_LOG           1
//#define NC_POLICY_LOG           1
//#define FILTER_LOG              0
//#define SC_POL_LOG              1
//#define GOD_VIEW                1
//#define EXOR_SOLVER_LOG         1
//#define BRR_LOG                 1
//#define ARQ_LOG                 1
//#define TRAF_GEN_LOG            1
//#define CODER_LOG				1
//#define SIMULATOR_LOG			1
//#define GRAPH_LOG     			0
//#define LPSOLVER_LOG    		0
//#define CCACK_LOG	    		1
//#define TEMP_LOG				0

#define SIM_LOG(condition, message) \
    do { \
    if (condition) { \
    std::cout << "`" #condition "`:\t" << \
    "FUNC<" << __func__ << ">:\t" << \
    "`" << message << "`" << std::endl; \
    } \
    } while (false)

#define SIM_LOG_N(condition, node, message) \
    do { \
    if (condition) { \
    std::cout << "`" #condition "`:\t" << \
    "FUNC<" << __func__ << ">:\tNode " << node << " -> "  << \
    "`" << message << "`" << std::endl; \
    } \
    } while (false)

#define SIM_LOG_NP(condition, node, priority, message) \
    do { \
    if (condition) { \
    std::cout << "`" #condition "`:\t" << \
    "FUNC<" << __func__ << ">:\tNode " << node << " -> p " << priority << " -> "  << \
    "`" << message << "`" << std::endl; \
    } \
    } while (false)

#define SIM_LOG_NPG(condition, node, priority, generation, message) \
    do { \
    if (condition) { \
    std::cout << "`" #condition "`:\t" << \
    "FUNC<" << __func__ << ">:\tNode " << node << " -> p " << priority << " -> GID " << generation << " -> "  << \
    "`" << message << "`" << std::endl; \
    } \
    } while (false)


#define SIM_LOG_FUNC(condition) \
    do { \
    if (condition) { \
    std::cout << "`" #condition "`:\t" << \
    "FUNC<" << __func__ << ">" << std::endl; \
    } \
    } while (false)

#define SIM_LOG_FUNC_N(condition, node) \
    do { \
    if (condition) { \
    std::cout << "`" #condition "`:\t" << \
    "FUNC<" << __func__ << ">:\tNode " << node << std::endl; \
    } \
    } while (false)

#define SIM_ASSERT_MSG(condition, message) \
    do { \
    if (!(condition)) { \
    std::cout << "`" #condition "`:\t" << \
    "FUNC<" << __func__ << ">:\t" << \
    "`" << message << "`" << std::endl; assert(0);\
    } \
    } while (false)

#endif /* LOG_H_ */
