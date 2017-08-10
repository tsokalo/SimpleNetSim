#!/bin/bash

my_dir="$(dirname "$0")"
source "$my_dir/build.conf"

echo $KODO_RLNC_LIB


./waf configure --kodo_rlnc_lib=$KODO_RLNC_LIB --coin_clp_lib=$COIN_CLP_LIB

