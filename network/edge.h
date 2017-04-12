/*
 * Edge.h
 *
 *  Created on: Dec 15, 2015
 *      Author: tsokalo
 */

#ifndef EDGE_H_
#define EDGE_H_

#include <deque>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <memory>
#include <map>
#include <functional>
#include <assert.h>
#include <string.h>

#include "header.h"
#include "utils/loss-process.h"
#include "utils/nc-packet.h"

namespace ncr
{

class Edge {
public:

	//
	// constructor for output edge
	//
	Edge(std::function<void(NcPacket)> rcv_edge, uint16_t v) :
		v_(v), rcv_edge_(rcv_edge), marked_(false) {
		edge_type_ = "Output edge";
	}

	//
	// constructor for input edge
	//
	Edge(uint16_t v, std::function<void(Edge *, NcPacket)> rcv_node) :
		v_(v), rcv_node_(rcv_node), marked_(false) {
		edge_type_ = "Input edge";
	}
	~Edge() {
	}

	void SetNotifyLoss(std::function<void(Edge *, NcPacket)> func) {
		notify_loss_ = func;
	}
	//
	// can be called only by output edge
	//
	void Transmit(NcPacket symb) {
		assert(rcv_edge_);
		rcv_edge_(symb);
	}
	//
	// can be called only by input edge
	//
	inline void Receive(NcPacket symb) {
		assert(l_ && rcv_node_);
		l_->Toss();
		if (!l_->IsLost()) {
			rcv_node_(this, symb);
		}
		else {
			if (notify_loss_) notify_loss_(this, symb);
		}
//		if (m_) m_->add_val(l_->IsLost());
	}
//	inline void UpdateStats() {
//		m_->update_stats();
//	}
//	inline void SetLossCalculator(std::shared_ptr<LossCalculator> m) {
//		m_ = m;
//	}
	inline void SetLossProcess(std::shared_ptr<LossProcess> l) {
		l_ = l;
	}
//	inline void SetLinDepCalculator(std::shared_ptr<LinDepCalculator> d) {
//		d_ = d;
//	}
	inline std::shared_ptr<LossProcess> GetLossProcess() {
		return l_;
	}
//	inline std::shared_ptr<LossCalculator> GetLossCalculator() {
//		return m_;
//	}
//	inline std::shared_ptr<LinDepCalculator> GetLinDepCalculator() {
//		return d_;
//	}
	inline void SetMarked(bool m) {
		marked_ = m;
	}
	inline bool IsMarked() {
		return marked_;
	}
	//
	// v_ - end of edge
	// owner of the edge is the start of the edge
	//
	uint16_t v_;
	/*
	 * edge between the same pair of nodes but in the opposite direction
	 */
	std::shared_ptr<Edge> reverse_edge_;

private:

	std::function<void(NcPacket)> rcv_edge_;
	std::function<void(Edge *, NcPacket)> rcv_node_;
	std::function<void(Edge *, NcPacket)> notify_loss_;

//	std::shared_ptr<LossCalculator> m_;
	std::shared_ptr<LossProcess> l_;
//	std::shared_ptr<LinDepCalculator> d_;
	std::string edge_type_;

	bool marked_;

};

typedef std::shared_ptr<Edge> Edge_ptr;
typedef std::vector<Edge_ptr> Edges;
typedef Edges EdgesIn;
typedef Edges EdgesOut;

}//ncr
#endif /* EDGE_H_ */
