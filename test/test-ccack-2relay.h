/*
 * test-ccack.h
 *
 *  Created on: 12.01.2017
 *      Author: tsokalo
 */

/// using @example encode_recode_decode_simple.cpp
///
///         +-----------+      +-----------+      +------------+
///         |  encoder  |+---->| decoder_1 |+---->|  decoder_3 |
///         +-----------+      | (recoder) |      +------------+
///                |           +-----------+ 			^
///				   |									|
///        		   |		   +-----------+      		|
///         	   +---------->| decoder_2 |+-----------+
///         			       | (recoder) |
///                            +-----------+
#ifndef CCACK_TEST_CCACK_TWORELAY_H_
#define CCACK_TEST_CCACK_TWORELAY_H_

#include <vector>
#include <cassert>
#include "ccack/ccack.h"

namespace ncr {

void TestCcack2Relay() {

	auto print_vec = [](std::string str, CodingVector vec)
	{
		std::cout << str << ":\t" << vec << std::endl;
	};

	srand(static_cast<uint32_t>(time(0)));

	// Set the number of symbols (i.e. the generation size in RLNC
	// terminology) and the size of a symbol in bytes
	uint32_t symbols = 16;
	uint32_t symbol_size = 5;
	uint16_t levels = 2;
	uint16_t fs = 8;
	//
	// generate the hash matrices once for all nodes
	//
	typedef std::shared_ptr<HashMatrixSet> hash_matrix_set_ptr;
	hash_matrix_set_ptr hashMatrixSet = hash_matrix_set_ptr(new HashMatrixSet(levels, symbols, fs));
	Ccack ccackR1(symbols, hashMatrixSet), ccackR2(symbols, hashMatrixSet), ccackD(symbols, hashMatrixSet);

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
	auto decoder_v = decoder_factory.build();
	auto decoder_3 = decoder_factory.build();

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

	// Ensure that relays can decode the original message together
	// i.e. the source sends exactly sufficient amount of redundancy
	while (decoder_v->rank() != symbols) {
		encoder->write_payload(payload.data());
		if ((rand() % 2)) {
			decoder_1->read_payload(payload.data());
			decoder_v->read_payload(payload.data());
		}
		if ((rand() % 2)) {
			decoder_2->read_payload(payload.data());
			decoder_v->read_payload(payload.data());
		}
	}

	std::cout << "SENT\tR1r\tR2r\tDr\tR1h\tR2h" << std::endl;
	uint16_t sent = 0;
	while (decoder_1->rank() != ccackR1.GetHeardSymbNum()) {

		//
		// send one coded packet
		//
		std::vector<uint8_t> cv;
		decoder_1->write_payload(payload.data());
		cv = extract_coding_vector(payload);
		ccackR1.SaveSnt(CodingVector(cv));
		//
		// receive the packet at destination
		//
		decoder_3->read_payload(payload.data());
		ccackD.SaveRcv(CodingVector(cv));
		//
		// destination generates the feedback
		//
		auto hashD = ccackD.GetHashVector();
		//
		// deliver the feedback to the relays without losses
		//
		ccackR1.RcvHashVector(hashD);
		ccackR2.RcvHashVector(hashD);

		std::cout << ++sent << "\t" << decoder_1->rank() << "\t" << decoder_2->rank() << "\t" << decoder_3->rank() << "\t" << ccackR1.GetHeardSymbNum() << "\t"
				<< ccackR2.GetHeardSymbNum() << std::endl;

	}
}

}

#endif /* CCACK_TEST_CCACK_TWORELAY_H_ */
