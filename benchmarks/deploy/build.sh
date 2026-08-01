#!/usr/bin/env bash
# Build a serial and a parallel rime_deployer from the SAME source for
# deployment benchmarks. Each goes into its own directory with its own
# librime.so (they are dynamically linked; do not mix them).
#
# Serial vs parallel is selected purely by the ENABLE_THREADING compile
# switch: OFF defines RIME_NO_THREADING, which takes the serial branch in
# WorkspaceUpdate (see src/rime/lever/deployment_tasks.cc); ON (default) takes
# the parallel branch. No separate commits/worktrees are needed.
#
# Usage:
#   benchmarks/deploy/build.sh <old_dir> <new_dir>
#
# Both dirs are created (and emptied) under the repo root. Requires cmake,
# ninja and the same dependencies used by the main build.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OLD_DIR="${1:?usage: build.sh <old_dir> <new_dir>}"
NEW_DIR="${2:?usage: build.sh <old_dir> <new_dir>}"
JOBS="${JOBS:-$(nproc)}"

build_in_dir() {
  local dir="$1"
  local threading="$2"
  rm -rf "$dir"
  mkdir -p "$dir"
  cmake -S "$REPO_ROOT" -B "$dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TEST=OFF \
    -DENABLE_THREADING="$threading" \
    -DCMAKE_CXX_FLAGS_RELEASE=-O3 >/dev/null
  ninja -C "$dir" -j"$JOBS" rime_deployer
}

echo "==> building serial (ENABLE_THREADING=OFF) into $OLD_DIR"
build_in_dir "$OLD_DIR" OFF

echo "==> building parallel (ENABLE_THREADING=ON) into $NEW_DIR"
build_in_dir "$NEW_DIR" ON

echo "done."
