/*
 * test-ccack-simple.h
 *
 *  Created on: 16.01.2017
 *      Author: tsokalo
 */

#ifndef CCACK_TEST_CCACK_SIMPLE_H_
#define CCACK_TEST_CCACK_SIMPLE_H_

#include <vector>
#include <cassert>
#include "ccack/ccack.h"

namespace ncr {

void TestCcackSimple() {

	auto print_vec = [](std::string str, CodingVector vec)
	{
		std::cout << str << ": " << vec << std::endl;
	};

	srand(static_cast<uint32_t>(time(0)));

	// Set the number of symbols (i.e. the generation size in RLNC
	// terminology) and the size of a symbol in bytes
	uint32_t symbols = 50;
	uint32_t symbol_size = 5;
	uint16_t levels = 2;
	uint16_t fs = 8;
	//
	// generate the hash matrices once for all nodes
	//
	typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;
	hash_matrix_set_ptr hashMatrixSet = hash_matrix_set_ptr(new HashMatrixSet(levels, symbols, fs));
	Ccack ccackS(symbols, hashMatrixSet), ccackD(symbols, hashMatrixSet);

	auto extract_coding_vector = [&](std::vector<uint8_t> payload)->CodingVector
	{
		CodingVector cv;
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
	auto decoder = decoder_factory.build();

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

		std::cout << std::endl << ">>>>>>>>>>>>>>> BROADCAST <<<<<<<<<<<<<<<<" << std::endl << std::endl;

		std::vector<uint8_t> cv;
		//
		// send the symbol
		//
		encoder->write_payload(payload.data());
		cv = extract_coding_vector(payload);
		print_vec("Encoding CV", cv);
		ccackS.SaveSnt(cv);

		//
		// Pass that packet to decoder; emulate losses
		//
		if ((rand() % 2)) {
			decoder->read_payload(payload.data());
			print_vec("Decoding at D", cv);
			ccackD.SaveRcv(cv);
			//
			// generate feedback
			//
			auto hashD = ccackD.GetHashVector();
			//
			// deliver the feedback to the encoder without losses
			//
			ccackS.RcvHashVector(hashD);
		}
	}

	std::vector<uint8_t> data_out(decoder->block_size());
	decoder->copy_from_symbols(storage::storage(data_out));

// Check if we properly decoded the data
	if (std::equal(data_out.begin(), data_out.end(), data_in.begin())) {
		std::cout << "Data decoded correctly" << std::endl;
	}
	else {
		std::cout << "Unexpected failure to decode " << "please file a bug report :)" << std::endl;
	}
}

}

#endif /* CCACK_TEST_CCACK_SIMPLE_H_ */
