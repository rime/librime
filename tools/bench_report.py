#!/usr/bin/env python3
"""Generate a comparison report for user dictionary cache on/off benchmarks.

Reads build_bench/{cache,nocache}/bench.raw (gtest + glog output) and the
optional time.txt from GNU time, then emits a Markdown table (default) or
machine-readable JSON (--json).

The benchmark logs lines such as:
    ...] Lookup (nhm) [LevelDB] +cache: 5.9 us
    ...] UpdateEntry (5000 entries) [LevelDB] -cache: 5 us per entry, ...
"""

import json
import re
import statistics
import sys

LINE_RE = re.compile(r'^.*?\]\s+(.+?) \[LevelDB\] ([+-]cache): ([\d.]+) (us|ms)\b')
MEM_RE = re.compile(
    r'^.*?\]\s+(.+?) \[MEM\] ([+-]cache): (-?[\d.]+) (MB)\b')
RSS_RE = re.compile(r'Maximum resident set size \(kbytes\): (\d+)')


def parse_log(path, pattern=LINE_RE, to_value=None):
    """Return {metric: (value, unit)} averaged over runs."""
    values = {}
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            m = pattern.search(line)
            if not m:
                continue
            metric, variant, val, unit = m.groups()
            value = to_value(float(val), unit) if to_value else float(val)
            values.setdefault(metric, []).append((value, unit))
    result = {}
    for metric, samples in values.items():
        value = sum(s[0] for s in samples) / len(samples)
        result[metric] = (value, samples[0][1])
    return result


def to_us(value, unit):
    return value * 1000.0 if unit == 'ms' else value


def parse_mem_samples(path):
    """Return {metric: [MB, ...]} keeping every fresh-process sample."""
    samples = {}
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            m = MEM_RE.search(line)
            if not m:
                continue
            metric, variant, val, unit = m.groups()
            samples.setdefault(metric, []).append(float(val))
    return samples


def mem_stats(samples):
    """Return (median, min, max, count) for a list of RSS samples."""
    ordered = sorted(samples)
    return (statistics.median(ordered), ordered[0], ordered[-1],
            len(ordered))


def fmt_mem(samples):
    median, low, high, count = mem_stats(samples)
    if count == 1:
        return f'{median:.1f} MB'
    return f'{median:.1f} MB ({low:.1f}-{high:.1f}, n={count})'


def parse_rss(path):
    if not path:
        return None
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            for line in f:
                m = RSS_RE.search(line)
                if m:
                    return int(m.group(1)) / 1024.0  # MB
    except OSError:
        return None
    return None


def fmt_value(value_us, unit):
    if unit == 'ms' or value_us >= 1000:
        return f'{value_us / 1000.0:.3g} ms'
    return f'{value_us:.3g} us'


def fmt_speedup(nocache_us, cache_us):
    if not cache_us:
        return 'n/a'
    ratio = nocache_us / cache_us
    if abs(ratio - 1.0) < 0.05:
        return '1.0x'
    return f'{ratio:.1f}x'


def main():
    if len(sys.argv) < 2:
        print('usage: bench_report.py <build_bench_dir> [--markdown|--json]',
              file=sys.stderr)
        return 2
    out_dir = sys.argv[1]
    fmt = '--json' if '--json' in sys.argv else '--markdown'

    cache = parse_log(f'{out_dir}/cache/bench.raw', LINE_RE, to_us)
    nocache = parse_log(f'{out_dir}/nocache/bench.raw', LINE_RE, to_us)
    mem_cache = parse_mem_samples(f'{out_dir}/cache/bench.raw')
    mem_nocache = parse_mem_samples(f'{out_dir}/nocache/bench.raw')
    rss_cache = parse_rss(f'{out_dir}/cache/time.txt')
    rss_nocache = parse_rss(f'{out_dir}/nocache/time.txt')

    # stable ordering: keep cache log order for non-[LevelDB]-only metrics
    cache_order = list(cache.keys())
    mem_order = list(mem_cache.keys())

    if fmt == '--json':
        rows = []
        for metric in cache_order:
            if metric not in cache or metric not in nocache:
                continue
            cache_us, unit = cache[metric]
            nocache_us, _ = nocache[metric]
            rows.append({
                'metric': metric,
                'cache_us': cache_us,
                'nocache_us': nocache_us,
                'speedup': round(nocache_us / cache_us, 2) if cache_us else None,
            })
        mem_rows = []
        for metric in mem_order:
            if metric not in mem_cache or metric not in mem_nocache:
                continue
            cache_mb, cache_low, cache_high, cache_n = mem_stats(
                mem_cache[metric])
            nocache_mb, nocache_low, nocache_high, nocache_n = mem_stats(
                mem_nocache[metric])
            mem_rows.append({
                'metric': metric,
                'cache_mb': round(cache_mb, 2),
                'cache_min_mb': round(cache_low, 2),
                'cache_max_mb': round(cache_high, 2),
                'cache_samples': cache_n,
                'nocache_mb': round(nocache_mb, 2),
                'nocache_min_mb': round(nocache_low, 2),
                'nocache_max_mb': round(nocache_high, 2),
                'nocache_samples': nocache_n,
                'delta_mb': round(cache_mb - nocache_mb, 2),
            })
        report = {
            'metrics': rows,
            'memory': mem_rows,
            'peak_rss_mb': {
                'cache': rss_cache,
                'nocache': rss_nocache,
            },
        }
        print(json.dumps(report, indent=2))
        return 0

    print('# User Dictionary Cache Benchmark')
    print()
    print('Comparison of `RIME_USER_DICT_CACHE_ENABLED=1` (cache) vs '
          '`=0` (no cache). Generated by `benchmark_compare.sh`.')
    print()
    print('| Metric | Cache | No cache | Speedup |')
    print('|---|---|---|---|')
    for metric in cache_order:
        if metric not in cache or metric not in nocache:
            continue
        cache_us, unit = cache[metric]
        nocache_us, _ = nocache[metric]
        print(f'| {metric} | {fmt_value(cache_us, unit)} | '
              f'{fmt_value(nocache_us, unit)} | {fmt_speedup(nocache_us, cache_us)} |')
    print()
    if rss_cache and rss_nocache:
        print('| Peak RSS | '
              f'{rss_cache:.1f} MB | {rss_nocache:.1f} MB | '
              f'{fmt_speedup(rss_nocache, rss_cache)} |')
    print()
    if mem_order:
        print('### Memory (multiple user dictionaries coexisting)')
        print()
        print('Process RSS deltas (median, min-max, sample count) when '
              'several user dictionaries are opened and loaded in one '
              'process; every sample is a fresh process.')
        print()
        print('| Metric | Cache | No cache | Delta |')
        print('|---|---|---|---|')
        for metric in mem_order:
            if metric not in mem_nocache:
                continue
            cache_mb, _, _, _ = mem_stats(mem_cache[metric])
            nocache_mb, _, _, _ = mem_stats(mem_nocache[metric])
            print(f'| {metric} | {fmt_mem(mem_cache[metric])} | '
                  f'{fmt_mem(mem_nocache[metric])} | '
                  f'{cache_mb - nocache_mb:+.1f} MB |')
        print()
    print('*Higher speedup (>1x) means the cache variant is faster '
          'than the no-cache variant.*')
    return 0


if __name__ == '__main__':
    sys.exit(main())
