#!/bin/bash

echo "Building..."
./waf build

my_dir="$(dirname "$0")"
source "$my_dir/build.conf"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LIB=$DIR"/build/"
CONF_FILE="/etc/ld.so.conf.d/sns-multicast.conf"

echo "Linking libraries..."
echo "# default SnsMulticast configuration" | sudo tee $CONF_FILE
echo $LIB"ccack" | sudo tee --append $CONF_FILE
echo $LIB"galois-field" | sudo tee --append $CONF_FILE
echo $LIB"lp-solver" | sudo tee --append $CONF_FILE
echo $LIB"network" | sudo tee --append $CONF_FILE
echo $LIB"routing-rules" | sudo tee --append $CONF_FILE
echo $LIB"traffic" | sudo tee --append $CONF_FILE
echo $LIB"utils" | sudo tee --append $CONF_FILE
echo $LIB"test" | sudo tee --append $CONF_FILE
echo $COIN_CLP_LIB"lib" | sudo tee --append $CONF_FILE
echo $KODO_RLNC_LIB"kodo-rlnc/kodo_build" | sudo tee --append $CONF_FILE

sudo ldconfig

echo "Now exectue run.sh <param>"
