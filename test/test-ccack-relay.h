/*
 * test-ccack.h
 *
 *  Created on: 12.01.2017
 *      Author: tsokalo
 */

/// using @example encode_recode_decode_simple.cpp
///
///         +-----------+      +-----------+      +------------+
///         |  encoder  |+---->| decoder_1 |+---->|  decoder_2 |
///         +-----------+      | (recoder) |      +------------+
///                |           +-----------+ 			^
///					------------------------------------
#ifndef CCACK_TEST_CCACK_RELAY_H_
#define CCACK_TEST_CCACK_RELAY_H_

#include <vector>
#include <cassert>
#include "ccack/ccack.h"

namespace ncr {

void TestCcackRelay() {

	auto print_vec = [](std::string str, CodingVector vec)
	{
		std::cout << str << ":\t" << vec << std::endl;
	};

	srand(static_cast<uint32_t>(time(0)));

	// Set the number of symbols (i.e. the generation size in RLNC
	// terminology) and the size of a symbol in bytes
	uint32_t symbols = 4;
	uint32_t symbol_size = 5;
	uint16_t levels = 2;
	uint16_t fs = 8;
	//
	// generate the hash matrices once for all nodes
	//
	typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;
	hash_matrix_set_ptr hashMatrixSet = hash_matrix_set_ptr(new HashMatrixSet(levels, symbols, fs));
	Ccack ccackS(symbols, hashMatrixSet), ccackR(symbols, hashMatrixSet), ccackD(symbols, hashMatrixSet);

	auto extract_coding_vector = [&](std::vector<uint8_t> payload)->std::vector<uint8_t>
	{
		std::vector<uint8_t> cv;
		cv.insert(cv.begin(), payload.begin() + 1, payload.begin() + 1 + symbols);
		return cv;
	};

	// Typdefs for the encoder/decoder type we wish to use
	using rlnc_encoder = kodo_rlnc::full_vector_encoder<fifi_field>;
	using rlnc_decoder = kodo_rlnc::full_vector_decoder<fifi_field>;

	// In the following we will make an encoder/decoder factory.
	// The factories are used to build actual encoders/decoders
	rlnc_encoder::factory encoder_factory(symbols, symbol_size);
	auto encoder = encoder_factory.build();
	kodo_core::set_systematic_off(*encoder);

	rlnc_decoder::factory decoder_factory(symbols, symbol_size);
	auto decoder_1 = decoder_factory.build();
	auto decoder_2 = decoder_factory.build();

	// Allocate some storage for a "payload" the payload is what we would
	// eventually send over a network
	std::vector<uint8_t> payload(encoder->payload_size());

	// Allocate some data to encode. In this case we make a buffer
	// with the same size as the encoder's block size (the max.
	// amount a single encoder can encode)
	std::vector<uint8_t> data_in(encoder->block_size());

	// Just for fun - fill the data with random data
	std::generate(data_in.begin(), data_in.end(), rand);

	// Assign the data buffer to the encoder so that we may start
	// to produce encoded symbols from it
	encoder->set_const_symbols(storage::storage(data_in));

	while (encoder->rank() != ccackS.GetHeardSymbNum()) {

		std::vector<uint8_t> cv;
		//
		// send the symbol
		//
		auto recoder_send = ((rand() % 2) && decoder_1->rank() != 0);
		if (recoder_send) {
			decoder_1->write_payload(payload.data());
			cv = extract_coding_vector(payload);
			print_vec("\t\tR: send", cv);
			ccackR.SaveSnt(CodingVector(cv));
		}
		else {
			encoder->write_payload(payload.data());
			cv = extract_coding_vector(payload);
			print_vec("\t\tS: send", cv);
			ccackS.SaveSnt(CodingVector(cv));
		}
		//
		// Pass that packet to decoder_1; emulate losses
		//
		if (!recoder_send) {
			if ((rand() % 2)) {
				decoder_1->read_payload(payload.data());
				print_vec("\tR: receive", cv);
				ccackR.SaveRcv(CodingVector(cv));
			}
			else {
				std::cout << "\tR: loss" << std::endl;
			}
		}
		//
		// Pass that packet to decoder_2; emulate losses
		//
		if ((rand() % 2)) {
			decoder_2->read_payload(payload.data());
			print_vec("\tD: receive", cv);
			ccackD.SaveRcv(CodingVector(cv));
			//
			// generate feedback
			//
			auto hashD = ccackD.GetHashVector();
			//
			// deliver the feedback to the encoder without losses
			//
			ccackS.RcvHashVector(hashD);
			ccackR.RcvHashVector(hashD);

			auto heardS = ccackS.GetHeardSymb();
			for (auto h : heardS)
				print_vec("S: heard", h);
			auto heardR = ccackR.GetHeardSymb();
			for (auto h : heardR)
				print_vec("R: heard", h);
		}
		else {
			std::cout << "\tD: loss" << std::endl;
		}
		std::cout << ">>>>>>>>>>>>>> D: rank: " << decoder_2->rank() << std::endl;
	}

	// decoder_2 should now be complete
	std::vector<uint8_t> data_out_2(decoder_2->block_size());
	decoder_2->copy_from_symbols(storage::storage(data_out_2));

	// Check if we properly decoded the data
	if (std::equal(data_out_2.begin(), data_out_2.end(), data_in.begin())) {
		std::cout << "Data decoded correctly" << std::endl;
	}
	else {
		std::cout << "Unexpected failure to decode " << "please file a bug report :)" << std::endl;
	}
}

}

#endif /* CCACK_TEST_CCACK_RELAY_H_ */
