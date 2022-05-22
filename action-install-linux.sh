#!/bin/bash

dep_packages=(
    doxygen
    libboost-filesystem-dev
    libboost-locale-dev
    libboost-regex-dev
    libboost-system-dev
    libgoogle-glog-dev
    libleveldb-dev
    libmarisa-dev
    libyaml-cpp-dev
)

sudo apt-get update
sudo apt-get install ${dep_packages[@]} -y
make deps/gtest
make -C deps/opencc build
sudo env "PATH=$PATH" make -C deps/opencc install
