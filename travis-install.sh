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
  make xcode/thirdparty
fi

if [[ -n "${RIME_PLUGINS}" ]]; then
    # intentionally unquoted: ${RIME_PLUGINS} is a space separated list of slugs
    bash ./install-plugins.sh ${RIME_PLUGINS}
    for plugin_dir in plugins/*; do
        if [[ -e "${plugin_dir}/travis-install.sh" ]]; then
	    bash "${plugin_dir}/travis-install.sh"
        fi
    done
fi
