/*
 * test-ccack-simple.h
 *
 *  Created on: 16.01.2017
 *      Author: tsokalo
 */

#ifndef CCACK_TEST_CCACK_SIMPLE_MONTE_CARLO_H_
#define CCACK_TEST_CCACK_SIMPLE_MONTE_CARLO_H_

#include <vector>
#include <cassert>
#include "ccack/ccack.h"

namespace ncr {

void TestCcackSimpleMonteCarlo() {

	srand(static_cast<uint32_t> (time(0)));

	// Set the number of symbols (i.e. the generation size in RLNC
	// terminology) and the size of a symbol in bytes
	uint32_t symbols = 50;
	uint32_t symbol_size = 5;
	uint16_t levels = 2;
	uint16_t fs = 8;
	uint32_t eplus = 0, eminus = 0;

	auto extract_coding_vector = [&](std::vector<uint8_t> payload)->CodingVector
	{
		CodingVector cv;
		cv.insert(cv.begin(), payload.begin() + 1, payload.begin() + 1 + symbols);
		return cv;
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

	// Typdefs for the encoder/decoder type we wish to use
	using rlnc_encoder = kodo_rlnc::full_vector_encoder<fifi_field>;
	using rlnc_decoder = kodo_rlnc::full_vector_decoder<fifi_field>;
	rlnc_encoder::factory encoder_factory(symbols, symbol_size);
	rlnc_decoder::factory decoder_factory(symbols, symbol_size);
	// Allocate some data to encode. In this case we make a buffer
	// with the same size as the encoder's block size (the max.
	// amount a single encoder can encode)
	std::vector<uint8_t> data_in(symbols * symbol_size);

	// Just for fun - fill the data with random data
	std::generate(data_in.begin(), data_in.end(), rand);

	uint32_t numit = 100, counter = 0;
	while (numit != ++counter) {
		print_progress(numit, counter);
		//
		// generate the hash matrices once for all nodes
		//
		typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;
		hash_matrix_set_ptr hashMatrixSet = hash_matrix_set_ptr(new HashMatrixSet(levels, symbols, fs));
		Ccack ccackS(symbols, hashMatrixSet), ccackD(symbols, hashMatrixSet);

		// In the following we will make an encoder/decoder factory.
		// The factories are used to build actual encoders/decoders

		auto encoder = encoder_factory.build();
		kodo_core::set_systematic_off(*encoder);
		auto decoder = decoder_factory.build();
		// Allocate some storage for a "payload" the payload is what we would
		// eventually send over a network
		std::vector<uint8_t> payload(encoder->payload_size());

		// Assign the data buffer to the encoder so that we may start
		// to produce encoded symbols from it
		encoder->set_const_symbols(storage::storage(data_in));

		uint32_t heardS = 0;

		do {

			std::vector<uint8_t> cv;
			//
			// send the symbol
			//
			encoder->write_payload(payload.data());
			cv = extract_coding_vector(payload);
			ccackS.SaveSnt(cv);

			//
			// Pass that packet to decoder; emulate losses
			//
			if ((rand() % 2)) {
				decoder->read_payload(payload.data());
				ccackD.SaveRcv(cv);
				//
				// generate feedback
				//
				auto hashD = ccackD.GetHashVector();
				//
				// deliver the feedback to the encoder without losses
				//
				ccackS.RcvHashVector(hashD);
				heardS = ccackS.GetHeardSymbNum();
			}

			auto r = decoder->rank();
			if (heardS > r) eplus += heardS - r;
			if (heardS < r) eminus += r - heardS;

		} while (encoder->rank() != heardS);
	}

	std::cout << std::endl << std::endl << "+" << eplus / (double)(symbols * numit) << " / " << "-" << eminus / (double)(symbols * numit) << std::endl;
}

}

#endif /* CCACK_TEST_CCACK_SIMPLE_MONTE_CARLO_H_ */
