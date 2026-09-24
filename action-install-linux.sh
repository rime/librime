#!/bin/bash

dep_packages=(
    libgoogle-glog-dev
    libleveldb-dev
    libmarisa-dev
    libyaml-cpp-dev
    libopencc-dev
    libgtest-dev
)

sudo apt update
sudo apt install -y ${dep_packages[@]}

./install-boost.sh
