#!/bin/bash
set -ex

RIME_ROOT="$(cd "$(dirname "$0")"; pwd)"

boost_version="${boost_version=1.84.0}"

BOOST_ROOT="${BOOST_ROOT=${RIME_ROOT}/deps/boost-${boost_version}}"

boost_tarball="boost-${boost_version}.tar.xz"
download_url="https://github.com/boostorg/boost/releases/download/boost-${boost_version}/${boost_tarball}"
boost_tarball_sha256sum="2e64e5d79a738d0fa6fb546c6e5c2bd28f88d268a2a080546f74e5ff98f29d0e  ${boost_tarball}"

download_boost_source() {
    cd "${RIME_ROOT}/deps"
    if ! [[ -f "${boost_tarball}" ]]; then
        curl -LO "${download_url}"
    fi
    echo "${boost_tarball_sha256sum}" | shasum -a 256 -c
    tar -xJf "${boost_tarball}"
    [[ -f "${BOOST_ROOT}/bootstrap.sh" ]]
}

if [[ $# -eq 0 || " $* " =~ ' --download ' ]]; then
    if [[ ! -f "${BOOST_ROOT}/bootstrap.sh" ]]; then
        download_boost_source
    else
        echo "found boost at ${BOOST_ROOT}"
    fi
fi
