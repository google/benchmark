#!/usr/bin/env python

import sys
import gbench
import gbench.report as greport
import gbench.util as gutil

def main():
    # Parse the command line flags
    def usage():
        print('compare_bench.py <test1> <test2> [benchmark options]...')
        exit(1)
    if '--help' in sys.argv or len(sys.argv) < 3:
        usage()
    tests = sys.argv[1:3]
    bench_opts = sys.argv[3:]
    bench_opts = list(bench_opts)
    # Run the benchmarks and report the results
    json1 = gbench.util.run_or_load_benchmark(tests[0], bench_opts)
    json2 = gbench.util.run_or_load_benchmark(tests[1], bench_opts)
    gbench.report.report_difference(json1, json2)


if __name__ == '__main__':
    main()
