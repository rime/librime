# Rime deployment benchmark

Best of 5 runs. Old: serial. New: parallel dictionary compilation.
Shared data: /tmp/rime_deploy_bench.fzWI1M/shared

| scenario | old (ms) | new (ms) | speedup |
|---|---|---|---|
| cold (full rebuild) | 12612 | 5718 | 2.21 |
| warm (nothing changed) | 64 | 27 | 2.37 |
| one dict modified | 2337 | 2269 | 1.03 |

## Artifact consistency (cold build outputs)

All deployed artifacts are byte-identical between old and new.
