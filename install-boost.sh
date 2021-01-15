#!/bin/bash
set -ev

RIME_ROOT="$(cd "$(dirname "$0")"; pwd)"

boost_version="${boost_version=1.75.0}"
boost_x_y_z="${boost_version//./_}"

BOOST_ROOT="${BOOST_ROOT=${RIME_ROOT}/thirdparty/src/boost_${boost_x_y_z}}"

boost_tarball="boost_${boost_x_y_z}.tar.bz2"
download_url="https://dl.bintray.com/boostorg/release/${boost_version}/source/${boost_tarball}"
boost_tarball_sha256sum_1_75_0='953db31e016db7bb207f11432bef7df100516eeb746843fa0486a222e3fd49cb  boost_1_75_0.tar.bz2'
boost_tarball_sha256sum="${boost_tarball_sha256sum=${boost_tarball_sha256sum_1_75_0}}"

download_boost_source() {
    cd "${RIME_ROOT}/thirdparty/src"
    if ! [[ -f "${boost_tarball}" ]]; then
        curl -LO "${download_url}"
    fi
    echo "${boost_tarball_sha256sum}" | shasum -a 256 -c
    tar --bzip2 -xf "${boost_tarball}"
    [[ -f "${BOOST_ROOT}/bootstrap.sh" ]]
}

boost_libs="${boost_libs=filesystem,regex,system}"
boost_cxxflags='-arch arm64 -arch x86_64'

build_boost_macos() {
    cd "${BOOST_ROOT}"
    ./bootstrap.sh --with-toolset=clang-darwin --with-libraries="${boost_libs}"
    ./b2 -q -a link=static architecture=combined cxxflags="${boost_cxxflags}" stage
}

if [[ "$OSTYPE" =~ 'darwin' ]]; then
   if ! [[ -f "${BOOST_ROOT}/bootstrap.sh" ]]; then
       download_boost_source
   fi
   build_boost_macos
fi
