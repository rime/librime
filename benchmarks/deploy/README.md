# Deployment performance benchmark

Measures the difference between the serial and the parallel dictionary
compilation paths in `CompileScheduler` (`src/rime/lever/compile_unit.cc`),
and verifies that both produce byte-identical artifacts.

## How to run

```sh
# 1. build the serial and the parallel rime_deployer from the SAME source.
#    The only difference is the ENABLE_THREADING compile switch (OFF defines
#    RIME_NO_THREADING, which runs the same compile units sequentially). Each
#    build dir gets its own librime.so.
benchmarks/deploy/build.sh build-old build-new

# 2. run the benchmark (generates a synthetic shared data dir by default).
benchmarks/deploy/run_bench.sh --old build-old --new build-new --out deploy_report.md
```

`run_bench.sh` options:

| option | default | meaning |
|---|---|---|
| `--old`, `--new` | required | build dirs (contain `bin/rime_deployer` + `lib/librime.so`) |
| `--data` | generated | shared data dir; pass your own to test real data |
| `--modify-dict` | first `*.dict.yaml` | dictionary used by the modified scenario |
| `--rounds` | 3 | timing runs per scenario, best kept |
| `--out` | `deploy_bench_report.md` | markdown report |
| `--reuse-cold` | off | reuse the last cold user dir for warm/modified (saves two cold deploys) |
| `--fail-on-diff` | off | exit non-zero if cold artifacts differ between old and new |

To benchmark your own data (e.g. an existing user directory), point `--data`
at a copy of the directory containing `*.schema.yaml` / `*.dict.yaml` and
`default.yaml` with a `schema_list`. The modified scenario appends to one
dictionary, so do not pass a symlink farm or your live user directory.

### Kongmingma mixed workloads

`data/kongmingma/` contains the real Kongmingma schemes used for large-dictionary
benchmarks. `gen_data.py` can mix them with the generated schemes:

```sh
# real schemas + simulated dictionaries
python3 benchmarks/deploy/gen_data.py --out /tmp/km-sim \
    --schemas 6 --entries 150000 --kongmingma sim
benchmarks/deploy/run_bench.sh --old build-old --new build-new \
    --data /tmp/km-sim --modify-dict s1.dict.yaml

# real schemas + tracked real dictionaries
python3 benchmarks/deploy/gen_data.py --out /tmp/km-real \
    --schemas 6 --entries 150000 --kongmingma real
benchmarks/deploy/run_bench.sh --old build-old --new build-new \
    --data /tmp/km-real --modify-dict s1.dict.yaml --rounds 1
```

`--kongmingma sim` copies the real schema YAML and generates smaller dictionaries;
`real` copies the tracked dictionary files. The Linux CI in
`.github/workflows/deploy-benchmark.yml` runs both mixes with the built-in test
schemes. `report-kongmingma.md` records local mixed-workload results; CI uploads
fresh reports. The Kongmingma fixture and workflow are benchmark-only and are
not intended for upstream.

## What it measures

| scenario | description |
|---|---|
| cold | first deploy into an empty user dir; every dictionary compiles |
| warm | redeploy with nothing changed; incremental checks only |
| one dict modified | redeploy after appending to one `*.dict.yaml` so its content checksum changes and that dictionary recompiles |

Incremental reuse in rime is driven by a content checksum stored in the
compiled schema (`build_info/checksum`), not by file mtime — so merely
`touch`-ing a dict file triggers nothing. The modified scenario changes file
content on purpose.

## Interpreting the results

* **cold** — the case that benefits from parallelism: many independent
  dictionaries compile at the same time. Expect a ~2× speedup on a multi-core
  machine with several comparable-size dictionaries.
* **warm** — dominated by checksum/incremental checks; parallelism adds nothing,
  both paths are already near-instant. If your real user directory is
  up-to-date, **you should see no difference** — that is expected.
* **one dict modified** — only the changed dictionary recompiles; its compile
  time (single-threaded) dominates the wall time either way, so old and new are
  comparable. This scenario mainly guards against regressions.

## Why a real user directory may show no difference

* The directory was already deployed → every deployment is the *warm* case.
  Nothing recompiles, so parallel vs serial is identical (and both are fast).
* One dictionary dominates the build (e.g. a huge `luna_pinyin`) → wall time is
  that dictionary's compile time, which is single-threaded regardless.
* Most schemas share the same dictionary (with different prisms) → the
  artifacts overlap, so the parallel path serializes the units that share
  those files to avoid writing the same table concurrently.

## Memory limiting

The parallel path does **not** blindly compile everything at once. Dictionary
compilation is memory-heavy (the entry collector holds all entries while
building the trie/prism), so the set of in-flight units is constrained by a
memory budget:

* each dictionary's estimated footprint = source file bytes × 32;
* the in-flight set may not exceed 1/8 of detected memory (512 MB fallback if
  the limit cannot be detected);
* detected memory is the smaller of physical memory and the Linux cgroup limit;
* concurrency is ≤ min(hardware cores, 4);
* a single dictionary larger than the budget is compiled alone on the calling
  thread, so its peak memory matches the serial path.

This means the observed speedup depends on machine RAM as well as core count:
the same data set shows a smaller gain on a low-memory machine, and a lone huge
dictionary is never parallelized (it becomes the serial tail).

## Notes

* Serial vs parallel is a pure compile-time switch on the same source:
  `-DENABLE_THREADING=OFF` defines `RIME_NO_THREADING`, which runs the same
  compile-unit graph with a single thread; `ON` (default) uses the worker pool.
  No separate commits or git worktrees are needed.
* Each build dir keeps its own `librime.so`; the deployer resolves it via its
  RPATH, so the two binaries are fully independent.
* The synthetic data generator (`gen_data.py`) is deterministic (fixed seed).
