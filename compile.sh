#!/bin/bash

rm -f simple_simulator

g++ \
-O2 \
-ftree-vectorize \
-std=c++0x \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/allocate-7c9f51/1.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/boost-abe3de/2.1.0 \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/cpuid-4d8071/4.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/endian-30a816/3.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/fifi-8960fd/24.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/gauge-f88f90/10.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/gtest-5c3bfe/3.0.0 \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/hex-4d9037/3.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/kodo-core-de4387/6.1.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/meta-768c5a/2.1.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/platform-bccd32/2.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/recycle-b2469b/2.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/sak-1bdcea/15.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/storage-20a331/2.1.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/stub-90d487/5.0.1/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/tables-c83c83/6.0.0/src \
-I/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/bundle_dependencies/waf-tools-51dff5/3.14.1 \
-I./network/ \
-I./routing-rules/ \
-I./utils/ \
-I./traffic/ \
-I./ \
main.cpp \
network/comm-net.cpp \
network/comm-node.cpp \
routing-rules/god-view-routing-rules.cpp \
routing-rules/nc-routing-rules.cc \
utils/utils.cpp \
traffic/traffic-generator.cpp \
-o simple_simulator \
-Wl,-Bstatic \
-L/home/tsokalo/workspace/new-kodo-rlnc/kodo-rlnc/kodo_build \
-lboost_chrono \
-lboost_filesystem \
-lboost_iostreams \
-lboost_program_options \
-lboost_system \
-lboost_thread \
-lboost_timer \
-lcpuid \
-lfifi \
-lfifi_no_dispatch \
-lgauge \
-lgtest \
-lsak \
-ltables \
-Wl,-Bdynamic
