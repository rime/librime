FROM debian:13-slim

RUN DEBIAN_FRONTEND=noninteractive apt-get update \
  && apt-get install -y --no-install-recommends \
  ca-certificates \
  git \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  libboost-dev \
  libboost-regex-dev \
  libboost-locale-dev \
  libgoogle-glog-dev \
  libgtest-dev \
  libyaml-cpp-dev \
  libleveldb-dev \
  libmarisa-dev \
  libopencc-dev \
  liblua5.4-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /librime
COPY . .

RUN bash install-plugins.sh \
  rime/librime-charcode \
  hchunhui/librime-lua \
  lotem/librime-octagram \
  rime/librime-predict

RUN cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DENABLE_LOGGING:BOOL=ON \
  -DBUILD_TEST:BOOL=ON \
  -DBUILD_STATIC:BOOL=OFF \
  -DBUILD_SHARED_LIBS:BOOL=ON
RUN cmake --build build

RUN ctest --test-dir build --output-on-failure
