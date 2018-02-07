#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == linux ]]; then
  sudo apt-get update
  sudo apt-get install doxygen libboost-filesystem-dev libboost-locale-dev libboost-regex-dev libboost-signals-dev libboost-system-dev libgoogle-glog-dev libleveldb-dev libmarisa-dev libyaml-cpp-dev -y
  make thirdparty/gtest
  make -C thirdparty/src/opencc build
  sudo env "PATH=$PATH" make -C thirdparty/src/opencc install
elif [[ "$TRAVIS_OS_NAME" == osx ]]; then
  make -f Makefile.xcode thirdparty
fi
