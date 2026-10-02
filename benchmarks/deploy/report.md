# Rime deployment benchmark

Best of 1 runs. Old: serial. New: parallel dictionary compilation.
Shared data: /tmp/rime_deploy_bench.CphSh8/shared

| scenario | old (ms) | new (ms) | speedup |
|---|---|---|---|
| cold (full rebuild) | 15245 | 7080 | 2.15 |
| warm (nothing changed) | 73 | 27 | 2.70 |
| one dict modified | 2846 | 2845 | 1.00 |

## Artifact consistency (cold build outputs)

All deployed artifacts are byte-identical between old and new.
