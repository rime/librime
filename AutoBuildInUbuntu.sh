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

git clone https://github.com/BYVoid/OpenCC.git
cd OpenCC/
apt install -y doxygen
make
make install

cd /usr/src/gtest
cmake CMakeLists.txt
make
cp *.a /usr/lib

cd /
git clone https://github.com/rime/librime.git
cd librime/
make
make install
