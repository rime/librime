#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == linux ]]; then
  make debug
  pushd debug-build/test
  ./rime_test
  popd
elif [[ "$TRAVIS_OS_NAME" == osx ]]; then
  make -f Makefile.xcode debug
  pushd xdebug/test
  ./Debug/rime_test
  popd
fi
