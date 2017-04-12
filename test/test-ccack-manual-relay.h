/*
 * test-ccack.h
 *
 *  Created on: 12.01.2017
 *      Author: tsokalo
 */

///
///         +-----------+      +-----------+      +------------+
///         |  source  |+---->| relay |+---->|  destination |
///         +-----------+      | (recoder) |      +------------+
///                |           +-----------+ 			^
///					------------------------------------
#ifndef CCACK_TEST_CCACK_MANUAL_RELAY_H_
#define CCACK_TEST_CCACK_MANUAL_RELAY_H_

#include <vector>
#include <cassert>
#include "ccack/ccack.h"
#include "ccack/matrix-echelon.h"

namespace ncr {

void TestCcackManualRelay() {

	auto print_vec = [](std::string str, CodingVector vec)
	{
		if(CCACK_LOG)std::cout << str << ":\t" << vec << std::endl;
	};
	auto print_progress = [](uint32_t m, uint32_t c)
	{
		float progress = (double) c / (double) m;

		int barWidth =100;

		std::cout << "[";
		int pos = barWidth * progress;
		for (int i = 0; i < barWidth; ++i) {
			if (i < pos) std::cout << "=";
			else if (i == pos) std::cout << ">";
			else std::cout << " ";
		}
		std::cout << "] " << ceil(progress * 100.0) << " %\r";
		std::cout.flush();
	};

	srand(static_cast<uint32_t>(time(0)));

	std::vector<double> srcSends = { 1, 0.75, 0.5, 0.25 };
	std::vector<double> dstRcvs = { 1, 0.5 };

	// Set the number of symbols (i.e. the generation size in RLNC
	// terminology) and the size of a symbol in bytes
	uint32_t symbols = 32;
	uint32_t symbol_size = 5;
	uint16_t fs = 8;

	auto extract_coding_vector = [&](std::vector<uint8_t> payload)->std::vector<uint8_t>
	{
		std::vector<uint8_t> cv;
		cv.insert(cv.begin(), payload.begin() + 1, payload.begin() + 1 + symbols);
		return cv;
	};

	// Typdefs for the source/decoder type we wish to use
	using rlnc_encoder = kodo_rlnc::full_vector_encoder<fifi_field>;
	using rlnc_decoder = kodo_rlnc::full_vector_decoder<fifi_field>;

	// In the following we will make an source/decoder factory.
	// The factories are used to build actual sources/decoders
	rlnc_encoder::factory source_factory(symbols, symbol_size);
	rlnc_decoder::factory decoder_factory(symbols, symbol_size);

	// Allocate some data to encode. In this case we make a buffer
	// with the same size as the source's block size (the max.
	// amount a single source can encode)
	std::vector<uint8_t> data_in(symbols * symbol_size);

	// Just for fun - fill the data with random data
	std::generate(data_in.begin(), data_in.end(), rand);

	for (auto srcSend : srcSends) {
		for (auto dstRcv : dstRcvs) {

			uint32_t num_it = 10000, counter = 0;
			uint64_t sum_n_vs = 0;

			while (counter++ != num_it) {
				auto source = source_factory.build();
				kodo_core::set_systematic_off(*source);
				auto relay = decoder_factory.build();
				auto destination = decoder_factory.build();

				// Allocate some storage for a "payload" the payload is what we would
				// eventually send over a network
				std::vector<uint8_t> payload(source->payload_size());

				// Assign the data buffer to the source so that we may start
				// to produce encoded symbols from it
				source->set_const_symbols(storage::storage(data_in));

				CodingMatrix b_tx_vs, b_rx_vr, b_tx_vr, b_rx_vd;
				//
				// the source sends all information it has and the relay receives all of it
				// the destination does not get anything
				//
				while (relay->rank() != symbols * srcSend) {
					source->write_payload(payload.data());
					auto rank = relay->rank();
					relay->read_payload(payload.data());
					// neglect the limits of the finite field
					if (rank < relay->rank()) {
						assert(rank + 1 == relay->rank());
						b_tx_vs.push_back(extract_coding_vector(payload));
						b_rx_vr.push_back(extract_coding_vector(payload));
					}
				}
				SIM_LOG(CCACK_LOG, "Relay rank: " << relay->rank());
				//
				// the relay sends as many symbols as needed to reach the [symbols - 1] rank at the destination
				// the source does not overhear this transmission
				//
				while (destination->rank() != symbols * srcSend * dstRcv - 1) {
					relay->write_payload(payload.data());
					auto rank = destination->rank();
					destination->read_payload(payload.data());
					// neglect the limits of the finite field
					if (rank < destination->rank()) {
						assert(rank + 1 == destination->rank());
						b_tx_vr.push_back(extract_coding_vector(payload));
						b_rx_vd.push_back(extract_coding_vector(payload));
					}
				}
				SIM_LOG(CCACK_LOG, "Destination rank: " << destination->rank());

				gf_actions_ptr gf;
				gf = gf_actions_ptr(new GfActions());
				to_reduced_row_echelon_form(b_rx_vd, gf);
				CodingMatrix nullSpaceBasis = FormNullSpace(b_rx_vd, gf);
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(1, (1 << fs) - 1);
				uint16_t dof = symbols - b_rx_vd.size();
				CodingVector hashD(dof);
				for (auto &v : hashD)
					v = dis(gen);
				hashD = MultiplyMatrixOnVector(nullSpaceBasis, hashD, gf, true);

				print_vec("Hash vector", hashD);

				TestHashVector(hashD, b_rx_vd, gf);

				uint32_t n_vs = 0;
				for (auto h : b_tx_vs) {
					auto prod = InnerProduct(h, hashD, gf);
					if (prod != 0) n_vs++;
					SIM_LOG(CCACK_LOG, "" << h << "x" << hashD << "=" << (int16_t) prod);
				}
				SIM_LOG(CCACK_LOG, "Number of innovative symbols: " << n_vs);
				sum_n_vs += n_vs;
//				print_progress(num_it, counter);
			}
			auto n_vs_r = symbols * srcSend * (1 - pow(1 / (double) (1 << fs), (double) symbols * srcSend * (1 - dstRcv) + 1));
			std::cout << symbols * srcSend << "\t" << symbols * srcSend * dstRcv - 1 << "\t" << (long double) sum_n_vs / (double) num_it << "\t" << n_vs_r
					<< std::endl;
//			std::cout << std::endl << "Average n_{v_s}: " << (long double) sum_n_vs / (double) num_it << std::endl;

		}
	}
}

}

#endif /* CCACK_TEST_CCACK_MANUAL_RELAY_H_ */
