#!/bin/bash
apt update
apt install -y \
build-essential \
cmake \
libboost-dev \
libboost-filesystem-dev libboost-regex-dev libboost-system-dev libboost-locale-dev \
libgoogle-glog-dev \
libgtest-dev \
libyaml-cpp-dev \
libleveldb-dev \
libmarisa-dev \
vim \
openssh-server \
curl

curl -O https://capnproto.org/capnproto-c++-0.8.0.tar.gz
tar zxf capnproto-c++-0.8.0.tar.gz
cd capnproto-c++-0.8.0/
./configure
make -j2 check
make install

apt install -y git

# Manually install libopencc
git clone https://github.com/BYVoid/OpenCC.git
cd OpenCC/
apt install -y doxygen
make
make install

# Fix libgtest problem during compiling
cd /usr/src/gtest
cmake CMakeLists.txt
make
#copy or symlink libgtest.a and libgtest_main.a to your /usr/lib folder
cp *.a /usr/lib

# Build librime
cd /
git clone https://github.com/rime/librime.git
cd librime/
make
make install
