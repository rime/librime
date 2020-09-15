#!/bin/bash

dep_packages=(
    capnproto
    doxygen
    libboost-filesystem-dev
    libboost-locale-dev
    libboost-regex-dev
    libboost-system-dev
    libcapnp-dev
    libgoogle-glog-dev
    libleveldb-dev
    libmarisa-dev
    libyaml-cpp-dev
)

sudo apt-get update
sudo apt-get install ${dep_packages[@]} -y
make thirdparty/gtest
make -C thirdparty/src/opencc build
sudo env "PATH=$PATH" make -C thirdparty/src/opencc install
