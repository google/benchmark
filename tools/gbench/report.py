import json
import os
import tempfile
import subprocess
import sys

# Benchmark Colors Enumeration
BC_MAGENTA = '\033[95m'
BC_CYAN = '\033[96m'
BC_OKBLUE = '\033[94m'
BC_HEADER = '\033[92m'
BC_WARNING = '\033[93m'
BC_WHITE = '\033[97m'
BC_FAIL = '\033[91m'
BC_ENDC = '\033[0m'
BC_BOLD = '\033[1m'
BC_UNDERLINE = '\033[4m'


def report_difference(json1, json2):
    def calculate_change(old_val, new_val):
        return float(new_val - old_val) / abs(old_val)
    longest_name = 1
    for bc in json1['benchmarks']:
        if len(bc['name']) > longest_name:
            longest_name = len(bc['name'])

    def find_test(name):
        for b in json2['benchmarks']:
            if b['name'] == name:
                return b
        return None
    for bn in json1['benchmarks']:
        other_bench = find_test(bn['name'])
        if not other_bench:
            continue

        def get_color(res):
            if res > 0.05:
                return BC_FAIL
            elif res > -0.07:
                return BC_WHITE
            else:
                return BC_CYAN

        fmt_str = "{}{:<{}s}{endc}    {}{:+.2f}{endc}     {}{:+.2f}{endc}"
        tres = calculate_change(bn['real_time'], other_bench['real_time'])
        cpures = calculate_change(bn['cpu_time'], other_bench['cpu_time'])
        print fmt_str.format(
            BC_HEADER, bn['name'], longest_name + 3,
            get_color(tres), tres, get_color(cpures), cpures,
            endc=BC_ENDC)