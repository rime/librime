# Kongmingma mixed deployment benchmark

Host: local Linux, 8 cores / 7.7 GB. Old is serial; new uses the parallel
compile-unit scheduler. Both workloads include six generated schemas with
150,000 entries each, plus the two Kongmingma schemas.

The simulated workload uses the real Kongmingma schema files with generated
dictionaries (50,000 and 200,000 entries). Cold is best of two runs; warm and
modified are single runs reusing the last cold directory.

| scenario | old (ms) | new (ms) | speedup |
|---|---:|---:|---:|
| cold (full rebuild) | 20631 | 9034 | 2.28× |
| warm (nothing changed) | 91 | 39 | 2.33× |
| one generated dict modified | 2545 | 2449 | 1.04× |

The real workload uses the tracked Kongmingma dictionaries (about 770k and
4.2M entries). Cold, warm, and modified are single runs; the modified scenario
changes only generated `s1.dict.yaml`.

| scenario | old (ms) | new (ms) | speedup |
|---|---:|---:|---:|
| cold (full rebuild) | 113862 | 100968 | 1.13× |
| warm (nothing changed) | 282 | 235 | 1.20× |
| one generated dict modified | 2673 | 3171 | 0.84× |

All cold-build artifacts were byte-identical between serial and parallel in
both workloads. Timings are local single-host measurements; CI uploads fresh
reports and should be used for repeatable comparisons.
