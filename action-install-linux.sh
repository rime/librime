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

sudo apt update -y
# fix a package dependency bug in Ubuntu 22.04
# https://bugs.launchpad.net/ubuntu/+source/google-glog/+bug/1991919
# https://github.com/kadalu-tech/pkgs/pull/2/files#r1001042597
sudo apt install -y libunwind-dev ninja-build ${dep_packages[@]}

make deps/gtest
make -C deps/opencc build
sudo env "PATH=$PATH" make -C deps/opencc install
