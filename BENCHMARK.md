# UserDictionary Cache Optimization Benchmark

## Overview

Benchmark comparing the **sorted-array cache** (`CacheLookup`) vs the original **LevelDB forward-scan** (`DfsLookup`) approach for UserDictionary queries.

The two configurations under test:

| Configuration | Build flag | Code path |
|---------------|-----------|-----------|
| `RIME_USER_DICT_CACHE_ENABLED=1` | `-DRIME_USER_DICT_CACHE_ENABLED=1` (or default) | `CacheLookup` (binary search in sorted array) |
| **undefined** (`=0`) | `-DRIME_USER_DICT_CACHE_ENABLED=0` | `DfsLookup` (LevelDB forward scan) — identical to `rime/master` |

When the macro is **not defined**, the code compiles to the same `DfsLookup` path as
`rime/master`; enabling the cache (`=1`) is a purely opt-in change. This benchmark
therefore serves as a direct A/B comparison between the PR and the upstream baseline.

## Conditional Compilation Switch

Defined in `src/rime/dict/user_dictionary.h`:

```cpp
#ifndef RIME_USER_DICT_CACHE_ENABLED
#define RIME_USER_DICT_CACHE_ENABLED 1
#endif
```

- **Set to 1** (or default): Cache enabled — uses `CacheLookup` (binary search in sorted array).
- **Set to 0** (or undefined): Cache disabled — uses `DfsLookup` (LevelDB forward scan), same as `rime/master`. The in-memory cache is not built and `pending_` is not maintained, so this variant is a true upstream-equivalent baseline for both time and memory.

### Automated Comparison (recommended)

The whole flow — build both variants, run the benchmarks, render the report —
is automated:

```bash
# One shot: build both variants, run, and print the report
./benchmark_compare.sh

# Reuse previously built binaries, just run + report
./benchmark_compare.sh --skip-build --no-mem

# Also write a machine-readable JSON report
./benchmark_compare.sh --json

# Collect 10 fresh-process samples for the memory benchmark (default: 5)
./benchmark_compare.sh --mem-repeats 10
```

Build artifacts live under `build_bench/` (git-ignored):

- `build_bench/cache` — `RIME_USER_DICT_CACHE_ENABLED=1`
- `build_bench/nocache` — `RIME_USER_DICT_CACHE_ENABLED=0`
- `build_bench/report.md` — the comparison report

### Building for Comparison (manual)

```bash
# Cache ON (=1)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_USER_DICT_BENCHMARK=ON
cd build && make -j$(nproc) rime_test

# Cache OFF (undefined / =0, rime/master-equivalent)
cmake -B build_nocache -DCMAKE_BUILD_TYPE=Release -DBUILD_USER_DICT_BENCHMARK=ON \
  -DCMAKE_CXX_FLAGS="-DRIME_USER_DICT_CACHE_ENABLED=0 -O3 -DNDEBUG"
cd build_nocache && make -j$(nproc) rime_test
```

## Prerequisites

The benchmark uses the standard test dictionary `dictionary_test.dict.yaml`.
It is copied into the build tree automatically (see `test/CMakeLists.txt`).

## Running the Benchmark (manual)

Tests are prefixed with `DISABLED_` (not run by default) and gated behind
`BUILD_USER_DICT_BENCHMARK=ON`. Execute from the build test directory:

```bash
cd build/test && ./rime_test --gtest_filter='*Benchmark*' --gtest_also_run_disabled_tests
```

To run LevelDB-specific tests only:
```bash
./rime_test --gtest_filter='*BenchmarkLdb*' --gtest_also_run_disabled_tests
```

To exclude LevelDB tests:
```bash
./rime_test --gtest_filter='*Benchmark*:-*BenchmarkLdb*' --gtest_also_run_disabled_tests
```

## Results

Measured on 2026-08-01, Release build (`-O3 -DNDEBUG`), x86_64, via
`./benchmark_compare.sh --no-mem`. LevelDB disk I/O varies run to run.

### LevelDB (Real-World, Disk I/O)

| Benchmark | +cache (=1) | undefined/master (=0) | Speedup |
|-----------|------------|-----------------------|:-------:|
| **Lookup nhm** (3-letter) | ~1.9 µs | ~700 µs | **~370×** |
| **Lookup nhmsh** (5-letter) | ~1.3 µs | ~820 µs | **~650×** |
| **Lookup nhmshsh** (7-letter) | ~3.3 µs | ~600 µs | **~180×** |
| **Load** (10K entries) | ~0.4 ms | ~0.01 ms | cache build cost |
| **Load** (50K entries) | ~3.3 ms* | ~0.01 ms | cache build cost |
| **Load** (100K entries) | ~5.8 ms | ~0.03 ms | cache build cost |
| **UpdateEntry** (5000 entries) | 5 µs/entry | 5 µs/entry | ~1× |
| **UpdateBatch** (2000 entries, with txn) | 1 µs/entry | 1 µs/entry | ~1× |
| **Reload** (50K entries) | 2.2 ms | <0.001 ms | cache slower, rarely triggered |

\* Single LevelDB disk-I/O sample; Load/Reload numbers vary run to run.
The `-cache` column no longer builds the in-memory cache, so `Load` now
reflects the true cache-build cost.

### TextDb (In-Memory, No Disk I/O)

| Benchmark | +cache (=1) | undefined/master (=0) | Speedup |
|-----------|-------------|-----------------------|:-------:|
| **Lookup nhm** | 1.1 µs | 0.4 µs | ~1× |
| **Lookup nhmsh** | 0.6 µs | 0.4 µs | ~1× |
| **Lookup nhmshsh** | 0.4 µs | 0.6 µs | ~1× |
| **Load** (any size) | ~0.01 ms | ~0.01 ms | ~1× |
| **UpdateEntry** | 2 µs/entry | 1 µs/entry | ~1× |
| **UpdateBatch** | 1 µs/entry | 2 µs/entry | ~1× |
| **Reload** | 0.003 ms | <0.001 ms | ~1× |

## Key Takeaways

1. **LevelDB query speedup: ~180–650× (two to three orders of magnitude).** The cache replaces expensive LevelDB forward-scans (disk seeks) with in-memory binary search over a flattened, sorted array. When `RIME_USER_DICT_CACHE_ENABLED` is undefined (the `rime/master` baseline), lookups cost ~600–800 µs of disk I/O; with the cache they drop to ~1–3 µs.
2. **TextDb queries are neutral.** Both configs run at ~0.4–1 µs. TextDb is already in-memory with fast sequential access, so the cache adds no overhead — and no benefit — in this case. Most real-world deployments use LevelDB.
3. **Write performance unaffected.** `UpdateEntry` follows the same DB write path regardless of cache state.
4. **Load has a one-time cache-build cost** — roughly 0.4 ms at 10K entries and 5.8 ms at 100K. It is paid once when the user dictionary is loaded and is offset by the ~1000× lookup speedup.
5. **Reload is a few ms slower** with cache (rebuilds the sorted array from LevelDB), but this is only triggered on manual import/merge — a rare operation.

## Multi-Dictionary Memory Benchmark

`UserDictBenchmarkMultiDictMemory` emulates a scheme with several Translators,
each bound to a different user dictionary, all loaded in one process. It
populates 8 LevelDB user dictionaries (20K entries each), then reports process
RSS deltas (MB) for each phase:

| Metric | What it shows |
|--------|---------------|
| `MultiDict open` | Cost of opening all LevelDB databases. The block cache is process-wide (`SharedBlockCache`), so it does not scale with the number of dictionaries. |
| `MultiDict load` | Cost of building one in-memory `UserDictionary::cache_` per dictionary. This **does** scale with the number of dictionaries. |
| `MultiDict lookup` | First lookups; in `-cache` they warm the shared LevelDB block cache, in `+cache` they only read the in-memory array. |
| `MultiDict total` | Sum of the above, relative to the post-population baseline. |

This is intended to answer whether multiple user dictionaries duplicate the
LevelDB block cache (they do not) or the new in-memory cache (each
`UserDictionary` instance has its own). Results are also parsed by
`tools/bench_report.py` and shown as a memory table in `build_bench/report.md`.

Each sample is a fresh process; the report shows the median with min–max and
sample count. `benchmark_compare.sh` defaults to 5 samples per variant and
accepts `--mem-repeats N` to change that.

Example (Linux x86_64, 8 dicts × 20K entries, median of 5, min–max):

| Phase | +cache | -cache |
|-------|-------:|-------:|
| open (8 LevelDB dbs) | +1.7 MB (1.7–2.1) | +1.7 MB (1.6–1.9) |
| load (build 8 in-memory caches) | +3.9 MB (3.8–3.9) | +0.0 MB (0.0–0.0) |
| first lookup (warm block cache) | +0.0 MB (0.0–0.0) | +3.9 MB (3.8–3.9) |
| **total** | **+5.6 MB (5.5–5.9)** | **+5.5 MB (5.5–5.7)** |

Opening many dictionaries does not multiply the block cache. The in-memory
cache is the per-dictionary cost, but the no-cache path spends a comparable
amount on LevelDB block-cache/iterator memory during the first scan, so peak
RSS ends up similar while lookups are ~1000× faster.

## Test Coverage

### Benchmarks

| Test Suite | Metrics | Backend |
|------------|---------|---------|
| `UserDictBenchmarkMultiDictMemory` | RSS for open / load / lookup with 8 coexisting dicts | LevelDB |
| `UserDictBenchmarkLoad` | Open + Load time | TextDb |
| `UserDictBenchmarkLoadLdb` | Open + Load time | LevelDB |
| `UserDictQueryBench` | Lookup time (3/5/7-letter input) | TextDb |
| `UserDictLdbQueryBench` | Lookup time (3/5/7-letter input) | LevelDB |
| `UserDictBenchmarkSave` | UpdateEntry throughput | TextDb |
| `UserDictBenchmarkSaveLdb` | UpdateEntry throughput | LevelDB |
| `UserDictBenchmarkTransaction` | Batch write throughput | TextDb |
| `UserDictBenchmarkTxnLdb` | Batch write throughput (with txn) | LevelDB |
| `UserDictBenchmarkReload` | Reload time | TextDb |
| `UserDictBenchmarkReloadLdb` | Reload time | LevelDB |

### Regression Tests (`user_dictionary_test.cc`)

| Test | What it verifies |
|------|------------------|
| `ExactMatchFields` | text, commit_count after `AddEntry` + `Reload()` + `LookupWords` |
| `ExactMatchMultipleEntries` | Multiple entries for same code |
| `UpdateEntryAfterLookup` | Learn feedback cycle: `LookupWords` → `UpdateEntry` → `LookupWords` |
| `AddNewEntryViaUpdateEntry` | New phrase via `UpdateEntry` (pending_ path) |
| `DeleteEntry` | Mark as deleted (commits < 0), verify hidden |
| `DeleteThenRevive` | Delete → re-add with positive commits |
| `PredictiveLookup` | Multi-syllable completion via `Lookup(gs, 0, 0, depth)` |
| `BatchAddThenLookup` | 50 entries via `UpdateEntry`, verify via predictive `LookupWords` |
| `MultipleCodeSyllables` | Two-syllable code exact match |
| `UpdateEntryRoundTrip` | `LookupWords` → `UpdateEntry` (with code from `Lookup`) |
