#!/bin/bash

./waf build
./build/simple_simulator "$1"
