#!/bin/bash

# install and build boost
make xcode/thirdparty/boost

export BOOST_ROOT="$(pwd)/thirdparty/src/boost_1_75_0"

make xcode/thirdparty

make xcode
