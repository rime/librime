FROM ubuntu:18.04
MAINTAINER a8568730

RUN apt update && apt install -y cmake python git

# Build librime
WORKDIR /
RUN git clone --recursive https://github.com/rime/librime.git
WORKDIR librime/

RUN sed -i 's/sudo //g' travis-install-linux.sh
RUN bash -eux travis-install-linux.sh

RUN make
RUN make debug
