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
sudo apt install -y ${dep_packages[@]}
