/*
 * down-group-progress.h
 *
 *  Created on: 07.02.2018
 *      Author: tsokalo
 */

#ifndef UTILS_DOWN_GROUP_PROGRESS_H_
#define UTILS_DOWN_GROUP_PROGRESS_H_

#include <map>

#include "header.h"
#include "utils/ssn.h"

namespace ncr {

struct DownstreamGroupProgress {

	typedef std::map<UanAddress, GenId> np_t;

	void update(UanAddress addr, GenId gid) {
		pr[addr] = gid;
	}
	void remove(UanAddress addr) {
		pr.erase(addr);
	}
	/*
	 * with update(...) we always overwrite the previous values
	 * hence, the minimum latest gid on all nodes is just the minimum value over the saved gids for
	 * different nodes
	 */
	GenId get_latest() {
		auto it = std::min_element(pr.begin(), pr.end(),
				[](np_t::value_type& l, np_t::value_type& r) -> bool {return gen_ssn_t(l.second) < gen_ssn_t(r.second);});
		return it->second;
	}
private:
	np_t pr;
};

}

#endif /* UTILS_DOWN_GROUP_PROGRESS_H_ */
