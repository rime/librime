FROM debian:12.1

RUN apt update && apt install -y \
  git \
  build-essential \
  cmake \
  ninja-build \
  libboost-dev \
  libboost-regex-dev \
  libboost-locale-dev \
  libgtest-dev \
  libleveldb-dev \
  libmarisa-dev \
  libopencc-dev

COPY / /librime
WORKDIR /librime/plugins
RUN git clone https://github.com/rime/librime-charcode charcode && \
  git clone https://github.com/lotem/librime-octagram octagram

WORKDIR /librime
RUN cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DENABLE_LOGGING:BOOL=ON \
  -DBUILD_TEST:BOOL=ON \
  -DBUILD_STATIC:BOOL=OFF \
  -DBUILD_SHARED_LIBS:BOOL=ON
RUN cmake --build build

WORKDIR /librime/build
RUN ctest
