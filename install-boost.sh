#!/bin/bash
set -ex

RIME_ROOT="$(cd "$(dirname "$0")"; pwd)"

boost_version="${boost_version=1.88.0}"

BOOST_ROOT="${BOOST_ROOT=${RIME_ROOT}/deps/boost-${boost_version}}"

boost_tarball="boost_${boost_version//./_}.tar.gz"
download_url="https://archives.boost.io/release/${boost_version}/source/${boost_tarball}"
boost_tarball_sha256sum="3621533e820dcab1e8012afd583c0c73cf0f77694952b81352bf38c1488f9cb4  ${boost_tarball}"

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
    fi
fi
