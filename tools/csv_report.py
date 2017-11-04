#!/usr/bin/env python
"""
csv_report.py - Convert the JSON output from a benchmark into a csv.
"""

import argparse
from argparse import ArgumentParser
import sys
import gbench
from gbench import util, report
from gbench.util import *


def check_inputs(test, flags):
  """
  Perform checking on the user provided inputs and diagnose any abnormalities
  """
  test_kind, test_err = classify_input_file(test)
  output_file = find_benchmark_flag('--benchmark_out=', flags)
  output_type = find_benchmark_flag('--benchmark_out_format=', flags)
  if test_kind == IT_Executable and output_file:
    print(("WARNING: '--benchmark_out=%s' will be passed to benchmark causing "
           "it to be overwritten") % output_file)
  if test_kind == IT_JSON and len(flags) > 0:
    print("WARNING: passing --benchmark flags has no effect since input is "
          "JSON")
  if output_type is not None and output_type != 'json':
    print(("ERROR: passing '--benchmark_out_format=%s' to 'csv_report.py`"
      " is not supported.") % output_type)
    sys.exit(1)


def generate_csv_report(json):
  """
  Generate a CSV report from a benchmark result in 'json'.
  """
  columns = gbench.report.get_fields(json['benchmarks'])
  output_strs = [','.join(columns)]

  for bn in json['benchmarks']:
    output_strs += [','.join(map(lambda x: "{}".format(bn.get(x, "")), columns))]
  return output_strs


def main():
  parser = ArgumentParser(
      description='convert the output of a benchmark into a CSV')
  parser.add_argument(
      'test', metavar='test', type=str, nargs=1,
      help='A benchmark executable or JSON output file')
  parser.add_argument(
      'benchmark_options', metavar='benchmark_options', nargs=argparse.REMAINDER,
      help='Arguments to pass when running a benchmark executable'
  )
  args, unknown_args = parser.parse_known_args()
  # Parse the command line flags
  test = args.test[0]
  if unknown_args:
    # should never happen
    print("Unrecognized positional argument arguments: '%s'" % unknown_args)
    sys.exit(1)
  benchmark_options = args.benchmark_options
  check_inputs(test, benchmark_options)
  # Run the benchmarks and report the results
  json = gbench.util.run_or_load_benchmark(test, benchmark_options)
  output_lines = generate_csv_report(json)
  for ln in output_lines:
    print(ln)


if __name__ == '__main__':
  main()
