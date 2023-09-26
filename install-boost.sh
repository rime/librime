#!/bin/bash
set -ex

RIME_ROOT="$(cd "$(dirname "$0")"; pwd)"

boost_version="${boost_version=1.83.0}"
boost_x_y_z="${boost_version//./_}"

BOOST_ROOT="${BOOST_ROOT=${RIME_ROOT}/deps/boost_${boost_x_y_z}}"

boost_tarball="boost_${boost_x_y_z}.tar.bz2"
download_url="https://boostorg.jfrog.io/artifactory/main/release/${boost_version}/source/${boost_tarball}"
boost_tarball_sha256sum_1_83_0='6478edfe2f3305127cffe8caf73ea0176c53769f4bf1585be237eb30798c3b8e  boost_1_83_0.tar.bz2'
boost_tarball_sha256sum="${boost_tarball_sha256sum=${boost_tarball_sha256sum_1_83_0}}"

download_boost_source() {
    cd "${RIME_ROOT}/deps"
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
    ./bootstrap.sh --with-toolset=clang --with-libraries="${boost_libs}"
    ./b2 -q -a link=static architecture=arm cxxflags="${boost_cxxflags}" stage
    for lib in stage/lib/*.a; do
        lipo $lib -info
    done
}

if [[ $# -eq 0 || " $* " =~ ' --download ' ]]; then
    if [[ ! -f "${BOOST_ROOT}/bootstrap.sh" ]]; then
        download_boost_source
    else
        echo "found boost at ${BOOST_ROOT}"
    fi
fi
if [[ $# -eq 0 || " $* " =~ ' --build ' ]]; then
    if [[ "$OSTYPE" =~ 'darwin' ]]; then
        build_boost_macos
    fi
fi
