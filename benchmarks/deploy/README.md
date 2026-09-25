# Deployment performance benchmark

Measures the difference between the serial and the parallel dictionary
compilation paths in `WorkspaceUpdate` (`src/rime/lever/deployment_tasks.cc`),
and verifies that both produce byte-identical artifacts.

## How to run

```sh
# 1. build the serial and the parallel rime_deployer from the SAME source.
#    The only difference is the ENABLE_THREADING compile switch (OFF takes the
#    RIME_NO_THREADING serial branch in WorkspaceUpdate). Each build dir gets
#    its own librime.so.
benchmarks/deploy/build.sh build-old build-new

# 2. run the benchmark (generates a synthetic shared data dir by default).
benchmarks/deploy/run_bench.sh --old build-old --new build-new --out deploy_report.md
```

`run_bench.sh` options:

| option | default | meaning |
|---|---|---|
| `--old`, `--new` | required | build dirs (contain `bin/rime_deployer` + `lib/librime.so`) |
| `--data` | generated | shared data dir; pass your own to test real data |
| `--rounds` | 3 | timing runs per scenario, best kept |
| `--out` | `deploy_bench_report.md` | markdown report |

To benchmark your own data (e.g. an existing user directory), point `--data`
at the directory containing `*.schema.yaml` / `*.dict.yaml` and `default.yaml`
with a `schema_list`.

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
  artifacts overlap, so the parallel path deliberately falls back to serial
  compilation to avoid writing the same table concurrently.

## Memory limiting

The parallel path does **not** blindly compile everything at once. Dictionary
compilation is memory-heavy (the entry collector holds all entries while
building the trie/prism), so batching is constrained by a memory budget:

* each dictionary's estimated footprint = source file bytes × 16;
* a batch may not exceed 1/4 of physical memory (2 GB fallback if the total
  cannot be detected);
* concurrency per batch ≤ min(hardware cores, 8);
* dictionaries are batched by descending estimated size; a single dictionary
  larger than the budget is compiled alone.

This means the observed speedup depends on machine RAM as well as core count:
the same data set shows a smaller gain on a low-memory machine, and a lone huge
dictionary is never parallelized (it becomes the serial tail).

## Notes

* Serial vs parallel is a pure compile-time switch on the same source:
  `-DENABLE_THREADING=OFF` defines `RIME_NO_THREADING`, which selects the
  serial loop in `WorkspaceUpdate`; `ON` (default) selects the parallel path.
  No separate commits or git worktrees are needed.
* Each build dir keeps its own `librime.so`; the deployer resolves it via its
  RPATH, so the two binaries are fully independent.
* The synthetic data generator (`gen_data.py`) is deterministic (fixed seed).
