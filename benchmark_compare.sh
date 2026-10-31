#!/usr/bin/env bash
#
# benchmark_compare.sh — compare user dictionary cache ON vs OFF performance.
#
# Builds two rime_test binaries (cache enabled / disabled) and runs the
# *Benchmark* tests in each, then produces a comparison report (Markdown).
#
# Build dirs:
#   build_bench/cache      RIME_USER_DICT_CACHE_ENABLED=1 (default)
#   build_bench/nocache    RIME_USER_DICT_CACHE_ENABLED=0
#
# Usage:
#   ./benchmark_compare.sh [options]
#
# Options:
#   -j, --jobs N       number of parallel build jobs (default: nproc)
#   --skip-build       reuse existing build_bench/* dirs, only run + report
#   --no-mem           skip peak-RSS measurement (needs GNU /usr/bin/time)
#   --json             also write a machine-readable report (build_bench/report.json)
#   -h, --help         show this help
#
# Output:
#   build_bench/report.md   (also printed to stdout)
#   build_bench/report.json (with --json)
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$ROOT/build_bench"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 4)"
SKIP_BUILD=0
MEASURE_MEM=1
WRITE_JSON=0

usage() {
  sed -n '2,40p' "$0" | sed 's/^# \{0,1\}//'
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--jobs) JOBS="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --no-mem) MEASURE_MEM=0; shift ;;
    --json) WRITE_JSON=1; shift ;;
    -h|--help) usage ;;
    *) echo "error: unknown option: $1" >&2; usage ;;
  esac
done

# Detect GNU /usr/bin/time (for peak-RSS measurement) once, up front.
USE_GNU_TIME=0
if (( MEASURE_MEM )) && command -v /usr/bin/time >/dev/null &&
   /usr/bin/time -v true 2>&1 | grep -m1 'Maximum resident set size' >/dev/null; then
  USE_GNU_TIME=1
else
  MEASURE_MEM=0
fi

command -v cmake >/dev/null || { echo "error: cmake not found" >&2; exit 1; }

# Reuse Opencc dict dir from an existing build cache if present; otherwise let
# cmake auto-detect it from the installed opencc prefix.
OPENCC_DICT_DIR=""
if [[ -f "$ROOT/build/CMakeCache.txt" ]]; then
  OPENCC_DICT_DIR="$(grep -m1 '^Opencc_DICT_DIR:PATH=' "$ROOT/build/CMakeCache.txt" | cut -d= -f2- || true)"
fi
OPENCC_ARGS=()
if [[ -n "$OPENCC_DICT_DIR" ]]; then
  OPENCC_ARGS+=("-DOpencc_DICT_DIR=$OPENCC_DICT_DIR")
fi

build_variant() {
  local dir="$1" cxx_flags="$2"
  if [[ -d "$dir" ]]; then
    # re-run cmake so option changes are picked up; incremental build after
    cmake -S "$ROOT" -B "$dir" \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_USER_DICT_BENCHMARK=ON \
      -DCMAKE_CXX_FLAGS="$cxx_flags" \
      "${OPENCC_ARGS[@]}" >/dev/null
  else
    cmake -S "$ROOT" -B "$dir" \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_USER_DICT_BENCHMARK=ON \
      -DCMAKE_CXX_FLAGS="$cxx_flags" \
      "${OPENCC_ARGS[@]}"
  fi
  cmake --build "$dir" --target rime_test -j "$JOBS"
}

run_bench() {
  local dir="$1"
  local raw="$dir/bench.raw"
  local time_file="$dir/time.txt"
  (
    cd "$dir/test"
    if (( USE_GNU_TIME )); then
      /usr/bin/time -v -o "$time_file" \
        ./rime_test --gtest_filter='*Benchmark*' --gtest_also_run_disabled_tests \
        > "$raw" 2>&1
    else
      ./rime_test --gtest_filter='*Benchmark*' --gtest_also_run_disabled_tests > "$raw" 2>&1
    fi
  )
}

if (( ! SKIP_BUILD )); then
  echo "==> configuring/building cache variant (default)"
  build_variant "$OUT/cache" "-O3 -DNDEBUG"
  echo "==> configuring/building no-cache variant"
  build_variant "$OUT/nocache" "-DRIME_USER_DICT_CACHE_ENABLED=0 -O3 -DNDEBUG"
fi

echo "==> running benchmark (cache)"
run_bench "$OUT/cache"
echo "==> running benchmark (no-cache)"
run_bench "$OUT/nocache"

echo "==> generating report"
"$ROOT/tools/bench_report.py" "$OUT" --markdown
"$ROOT/tools/bench_report.py" "$OUT" --markdown > "$OUT/report.md"
if (( WRITE_JSON )); then
  "$ROOT/tools/bench_report.py" "$OUT" --json > "$OUT/report.json"
fi
echo
echo "report written to $OUT/report.md"
