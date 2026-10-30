# Kongmingma benchmark fixture

Real Kongmingma scheme files used by the deployment benchmark:

- `kongmingma`: about 770k entries, 11 MB source;
- `kongmingmabig`: about 4.2M entries, 78 MB source.

These files are intentionally tracked in the benchmark-only commit so the Linux
CI job can exercise real large dictionaries together with the generated test
schemas. This commit is not intended to be merged upstream.
