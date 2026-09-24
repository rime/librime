#!/bin/bash
set -ex

RIME_ROOT="$(cd "$(dirname "$0")"; pwd)"

boost_version="${boost_version=1.92.0}"

BOOST_ROOT="${BOOST_ROOT=${RIME_ROOT}/deps/boost-${boost_version}}"

boost_tarball="boost_${boost_version//./_}.tar.gz"
download_url="https://archives.boost.io/release/${boost_version}/source/${boost_tarball}"
boost_tarball_sha256sum="c4a3b310ddd2472416e091067166b0713be97c63f38c212c484ada022fd296ce  ${boost_tarball}"

download_boost_source() {
    cd "${RIME_ROOT}/deps"
    if ! [[ -f "${boost_tarball}" ]]; then
        curl -LO "${download_url}"
    fi
    echo "${boost_tarball_sha256sum}" | shasum -a 256 -c
    tar -xzf "${boost_tarball}"
    mv "boost_${boost_version//./_}" "boost-${boost_version}"
    [[ -f "${BOOST_ROOT}/bootstrap.sh" ]]
}

boost_cxxflags='-arch arm64 -arch x86_64'

build_boost_macos() {
    cd "${BOOST_ROOT}"
    ./bootstrap.sh --with-toolset=clang --with-libraries="${boost_libs}"
    ./b2 -q -a link=static architecture=arm cxxflags="${boost_cxxflags}" stage
    for lib in stage/lib/*.a; do
        lipo $lib -info
    done
}

build_boost_linux() {
    local boost_toolset=
    if [[ "${CXX:-}" =~ clang ]]; then
        boost_toolset=clang
    elif [[ "${CXX:-}" =~ g\+\+|gcc ]]; then
        boost_toolset=gcc
    fi
    cd "${BOOST_ROOT}"
    if [[ -n "${boost_toolset}" ]]; then
        ./bootstrap.sh --with-toolset="${boost_toolset}" --with-libraries="${boost_libs}"
    else
        ./bootstrap.sh --with-libraries="${boost_libs}"
    fi
    ./b2 -q -a link=shared stage
}

if [[ $# -eq 0 || " $* " =~ ' --download ' ]]; then
    if [[ ! -f "${BOOST_ROOT}/bootstrap.sh" ]]; then
        download_boost_source
    else
        echo "found boost at ${BOOST_ROOT}"
    fi
    cd "${BOOST_ROOT}"
    ./bootstrap.sh
    ./b2 headers
fi
if [[ ($# -eq 0 || " $* " =~ ' --build ') && -n "${boost_libs}" ]]; then
    if [[ "$OSTYPE" =~ 'darwin' ]]; then
        build_boost_macos
    elif [[ "$OSTYPE" =~ 'linux' ]]; then
        build_boost_linux
    fi
fi
