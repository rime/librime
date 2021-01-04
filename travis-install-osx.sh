#!/bin/bash

if [[ -n "$BOOST_ROOT" ]]; then
    make xcode/thirdparty/boost
fi
make xcode/thirdparty
