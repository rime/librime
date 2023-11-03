#!/bin/bash

dep_packages=(
    libboost-locale-dev
    libboost-regex-dev
    libgoogle-glog-dev
    libleveldb-dev
    libmarisa-dev
    libyaml-cpp-dev
    libopencc-dev
    libgtest-dev
)

sudo apt update
# fix a package dependency bug in Ubuntu 22.04
# https://bugs.launchpad.net/ubuntu/+source/google-glog/+bug/1991919
# https://github.com/kadalu-tech/pkgs/pull/2/files#r1001042597
sudo apt install -y libunwind-dev ninja-build ${dep_packages[@]}
