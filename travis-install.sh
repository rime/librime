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

if [[ "$TRAVIS_OS_NAME" == linux ]]; then
  sudo apt-get update
  sudo apt-get install ${dep_packages[@]} -y
  make thirdparty/gtest
  make -C thirdparty/src/opencc build
  sudo env "PATH=$PATH" make -C thirdparty/src/opencc install
elif [[ "$TRAVIS_OS_NAME" == osx ]]; then
  make -f xcode.mk thirdparty
fi

if [[ -n "${RIME_PLUGINS}" ]]; then
    # intentionally not quoted: ${RIME_PLUGIN} is a space separated list of slugs
    bash ./travis-install-plugins.sh ${RIME_PLUGINS}
fi
