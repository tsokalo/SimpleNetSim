#!/bin/bash

KODO_RLNC_LIB="/home/tsokalo/workspace/new-kodo-rlnc/"
COIN_CLP_LIB="/home/tsokalo/workspace/coin-Clp/"
./waf configure --kodo_rlnc_lib=$KODO_RLNC_LIB --coin_clp_lib=$COIN_CLP_LIB
