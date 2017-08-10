#!/bin/bash

./waf build

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/tsokalo/workspace/coin-Clp/lib/:/home/tsokalo/workspace/SnsMulticast/build/utils/:/home/tsokalo/workspace/SnsMulticast/build/galois-field/:/home/tsokalo/workspace/SnsMulticast/build/ccack/:/home/tsokalo/workspace/SnsMulticast/build/lp-solver/:/home/tsokalo/workspace/SnsMulticast/build/network/:/home/tsokalo/workspace/SnsMulticast/build/traffic/:/home/tsokalo/workspace/SnsMulticast/build/routing-rules/:/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/kodo_build ./build/simple_simulator "$1"
