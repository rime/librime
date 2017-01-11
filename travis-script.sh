#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == linux ]]; then
  make test
elif [[ "$TRAVIS_OS_NAME" == osx ]]; then
  make -f Makefile.xcode test
fi
