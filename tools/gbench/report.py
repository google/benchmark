# type: ignore

"""
report.py - Utilities for reporting statistics about benchmark results
"""

import copy
import math
import os
import random
import re
import unittest
import unittest.mock

from numpy import array, median
from scipy.stats import binom, gmean, mannwhitneyu


class BenchmarkColor:
    def __init__(self, name, code):
        self.name = name
        self.code = code

    def __repr__(self):
        return "%s%r" % (self.__class__.__name__, (self.name, self.code))

    def __format__(self, format):
        return self.code


# Benchmark Colors Enumeration
BC_NONE = BenchmarkColor("NONE", "")
BC_MAGENTA = BenchmarkColor("MAGENTA", "\033[95m")
BC_CYAN = BenchmarkColor("CYAN", "\033[96m")
BC_OKBLUE = BenchmarkColor("OKBLUE", "\033[94m")
BC_OKGREEN = BenchmarkColor("OKGREEN", "\033[32m")
BC_HEADER = BenchmarkColor("HEADER", "\033[92m")
BC_WARNING = BenchmarkColor("WARNING", "\033[93m")
BC_WHITE = BenchmarkColor("WHITE", "\033[97m")
BC_FAIL = BenchmarkColor("FAIL", "\033[91m")
BC_ENDC = BenchmarkColor("ENDC", "\033[0m")
BC_BOLD = BenchmarkColor("BOLD", "\033[1m")
BC_UNDERLINE = BenchmarkColor("UNDERLINE", "\033[4m")

UTEST_MIN_REPETITIONS = 2
UTEST_OPTIMAL_REPETITIONS = 9  # Lowest reasonable number, More is better.
UTEST_COL_NAME = "_pvalue"

_TIME_UNIT_TO_SECONDS_MULTIPLIER = {
    "s": 1.0,
    "ms": 1e-3,
    "us": 1e-6,
    "ns": 1e-9,
}


def color_format(use_color, fmt_str, *args, **kwargs):
    """
    Return the result of 'fmt_str.format(*args, **kwargs)' after transforming
    'args' and 'kwargs' according to the value of 'use_color'. If 'use_color'
    is False then all color codes in 'args' and 'kwargs' are replaced with
    the empty string.
    """
    assert use_color is True or use_color is False
    if not use_color:
        args = [
            arg if not isinstance(arg, BenchmarkColor) else BC_NONE
            for arg in args
        ]
        kwargs = {
            key: arg if not isinstance(arg, BenchmarkColor) else BC_NONE
            for key, arg in kwargs.items()
        }
    return fmt_str.format(*args, **kwargs)


def get_color(res):
    """
    Return the color for the relative change 'res'; None means that there is
    no change to report.
    """
    if res is None:
        return BC_NONE
    if res > 0.05:
        return BC_FAIL
    elif res > -0.07:
        return BC_WHITE
    else:
        return BC_CYAN


def find_longest_name(benchmark_list):
    """
    Return the length of the longest benchmark name in a given list of
    benchmark JSON objects
    """
    longest_name = 1
    for bc in benchmark_list:
        if len(bc["name"]) > longest_name:
            longest_name = len(bc["name"])
    return longest_name


def calculate_change(old_val, new_val):
    """
    Return a float representing the decimal change between old_val and new_val.
    """
    if old_val == 0 and new_val == 0:
        return 0.0
    if old_val == 0:
        return float(new_val - old_val) / (float(old_val + new_val) / 2)
    return float(new_val - old_val) / abs(old_val)


def filter_benchmark(json_orig, family, replacement=""):
    """
    Apply a filter to the json, and only leave the 'family' of benchmarks.
    """
    regex = re.compile(family)
    filtered = {}
    filtered["benchmarks"] = []
    for be in json_orig["benchmarks"]:
        if not regex.search(be["name"]):
            continue
        filteredbench = copy.deepcopy(be)  # Do NOT modify the old name!
        filteredbench["name"] = regex.sub(replacement, filteredbench["name"])
        filtered["benchmarks"].append(filteredbench)
    return filtered


def get_unique_benchmark_names(json):
    """
    While *keeping* the order, give all the unique 'names' used for benchmarks.
    """
    seen = set()
    uniqued = [
        x["name"]
        for x in json["benchmarks"]
        if x["name"] not in seen and (seen.add(x["name"]) or True)
    ]
    return uniqued


def intersect(list1, list2):
    """
    Given two lists, get a new list consisting of the elements only contained
    in *both of the input lists*, while preserving the ordering.
    """
    return [x for x in list1 if x in list2]


def is_potentially_comparable_benchmark(x):
    return "time_unit" in x and "real_time" in x and "cpu_time" in x


def partition_benchmarks(json1, json2):
    """
    While preserving the ordering, find benchmarks with the same names in
    both of the inputs, and group them.
    (i.e. partition/filter into groups with common name)
    """
    json1_unique_names = get_unique_benchmark_names(json1)
    json2_unique_names = get_unique_benchmark_names(json2)
    names = intersect(json1_unique_names, json2_unique_names)
    partitions = []
    for name in names:
        time_unit = None
        # Pick the time unit from the first entry of the lhs benchmark.
        # We should be careful not to crash with unexpected input.
        for x in json1["benchmarks"]:
            if x["name"] == name and is_potentially_comparable_benchmark(x):
                time_unit = x["time_unit"]
                break
        if time_unit is None:
            continue
        # Filter by name and time unit.
        # All the repetitions are assumed to be comparable.
        lhs = [
            x
            for x in json1["benchmarks"]
            if x["name"] == name
            and x["time_unit"] == time_unit
            and not x.get("skipped", False)
            and not x.get("error_occurred", False)
        ]
        rhs = [
            x
            for x in json2["benchmarks"]
            if x["name"] == name
            and x["time_unit"] == time_unit
            and not x.get("skipped", False)
            and not x.get("error_occurred", False)
        ]

        if len(lhs) == 0 or len(rhs) == 0:
            continue

        partitions.append([lhs, rhs])
    return partitions


def get_timedelta_field_as_seconds(benchmark, field_name):
    """
    Get value of field_name field of benchmark, which is time with time unit
    time_unit, as time in seconds.
    """
    timedelta = benchmark[field_name]
    time_unit = benchmark.get("time_unit", "s")
    return timedelta * _TIME_UNIT_TO_SECONDS_MULTIPLIER.get(time_unit)


def calculate_geomean(json):
    """
    Extract all real/cpu times from all the benchmarks as seconds,
    and calculate their geomean.
    """
    times = []
    for benchmark in json["benchmarks"]:
        if "run_type" in benchmark and benchmark["run_type"] == "aggregate":
            continue
        if benchmark.get("skipped", False) or benchmark.get(
            "error_occurred", False
        ):
            continue
        times.append(
            [
                get_timedelta_field_as_seconds(benchmark, "real_time"),
                get_timedelta_field_as_seconds(benchmark, "cpu_time"),
            ]
        )
    return gmean(times) if times else array([])


def extract_field(partition, field_name):
    # The count of elements may be different. We want *all* of them.
    lhs = [x[field_name] for x in partition[0]]
    rhs = [x[field_name] for x in partition[1]]
    return [lhs, rhs]


def calc_utest(timings_cpu, timings_time):
    min_rep_cnt = min(
        len(timings_time[0]),
        len(timings_time[1]),
        len(timings_cpu[0]),
        len(timings_cpu[1]),
    )

    # Does *everything* has at least UTEST_MIN_REPETITIONS repetitions?
    if min_rep_cnt < UTEST_MIN_REPETITIONS:
        return False, None, None

    time_pvalue = mannwhitneyu(
        timings_time[0], timings_time[1], alternative="two-sided"
    ).pvalue
    cpu_pvalue = mannwhitneyu(
        timings_cpu[0], timings_cpu[1], alternative="two-sided"
    ).pvalue

    return (min_rep_cnt >= UTEST_OPTIMAL_REPETITIONS), cpu_pvalue, time_pvalue


def print_utest(bc_name, utest, utest_alpha, first_col_width, use_color=True):
    def get_utest_color(pval):
        return BC_FAIL if pval >= utest_alpha else BC_OKGREEN

    # Check if we failed miserably with minimum required repetitions for utest
    if (
        not utest["have_optimal_repetitions"]
        and utest["cpu_pvalue"] is None
        and utest["time_pvalue"] is None
    ):
        return []

    dsc = "U Test, Repetitions: {} vs {}".format(
        utest["nr_of_repetitions"], utest["nr_of_repetitions_other"]
    )
    dsc_color = BC_OKGREEN

    # We still got some results to show but issue a warning about it.
    if not utest["have_optimal_repetitions"]:
        dsc_color = BC_WARNING
        dsc += (
            f". WARNING: Results unreliable! {UTEST_OPTIMAL_REPETITIONS}+"
            " repetitions recommended."
        )

    special_str = "{}{:<{}s}{endc}{}{:16.4f}{endc}{}{:16.4f}{endc}{}      {}"

    return [
        color_format(
            use_color,
            special_str,
            BC_HEADER,
            f"{bc_name}{UTEST_COL_NAME}",
            first_col_width,
            get_utest_color(utest["time_pvalue"]),
            utest["time_pvalue"],
            get_utest_color(utest["cpu_pvalue"]),
            utest["cpu_pvalue"],
            dsc_color,
            dsc,
            endc=BC_ENDC,
        )
    ]


def get_difference_report(json1, json2, utest=False):
    """
    Calculate and report the difference between each test of two benchmarks
    runs specified as 'json1' and 'json2'. Output is another json containing
    relevant details for each test run.
    """
    assert utest is True or utest is False

    diff_report = []
    partitions = partition_benchmarks(json1, json2)
    for partition in partitions:
        benchmark_name = partition[0][0]["name"]
        label = partition[0][0].get("label", "")
        time_unit = partition[0][0]["time_unit"]
        measurements = []
        utest_results = {}
        # Careful, we may have different repetition count.
        for i in range(min(len(partition[0]), len(partition[1]))):
            bn = partition[0][i]
            other_bench = partition[1][i]
            measurements.append(
                {
                    "real_time": bn["real_time"],
                    "cpu_time": bn["cpu_time"],
                    "real_time_other": other_bench["real_time"],
                    "cpu_time_other": other_bench["cpu_time"],
                    "time": calculate_change(
                        bn["real_time"], other_bench["real_time"]
                    ),
                    "cpu": calculate_change(
                        bn["cpu_time"], other_bench["cpu_time"]
                    ),
                }
            )

        # After processing the whole partition, if requested, do the U test.
        if utest:
            timings_cpu = extract_field(partition, "cpu_time")
            timings_time = extract_field(partition, "real_time")
            have_optimal_repetitions, cpu_pvalue, time_pvalue = calc_utest(
                timings_cpu, timings_time
            )
            if cpu_pvalue is not None and time_pvalue is not None:
                utest_results = {
                    "have_optimal_repetitions": have_optimal_repetitions,
                    "cpu_pvalue": cpu_pvalue,
                    "time_pvalue": time_pvalue,
                    "nr_of_repetitions": len(timings_cpu[0]),
                    "nr_of_repetitions_other": len(timings_cpu[1]),
                }

        # Store only if we had any measurements for given benchmark.
        # E.g. partition_benchmarks will filter out the benchmarks having
        # time units which are not compatible with other time units in the
        # benchmark suite.
        if measurements:
            run_type = partition[0][0].get("run_type", "")
            aggregate_name = (
                partition[0][0]["aggregate_name"]
                if run_type == "aggregate"
                and "aggregate_name" in partition[0][0]
                else ""
            )
            diff_report.append(
                {
                    "name": benchmark_name,
                    "label": label,
                    "measurements": measurements,
                    "time_unit": time_unit,
                    "run_type": run_type,
                    "aggregate_name": aggregate_name,
                    "utest": utest_results,
                }
            )

    lhs_gmean = calculate_geomean(json1)
    rhs_gmean = calculate_geomean(json2)
    if lhs_gmean.any() and rhs_gmean.any():
        diff_report.append(
            {
                "name": "OVERALL_GEOMEAN",
                "label": "",
                "measurements": [
                    {
                        "real_time": lhs_gmean[0],
                        "cpu_time": lhs_gmean[1],
                        "real_time_other": rhs_gmean[0],
                        "cpu_time_other": rhs_gmean[1],
                        "time": calculate_change(lhs_gmean[0], rhs_gmean[0]),
                        "cpu": calculate_change(lhs_gmean[1], rhs_gmean[1]),
                    }
                ],
                "time_unit": "s",
                "run_type": "aggregate",
                "aggregate_name": "geomean",
                "utest": {},
            }
        )

    return diff_report


def print_difference_report(
    json_diff_report,
    include_aggregates_only=False,
    utest=False,
    utest_alpha=0.05,
    use_color=True,
):
    """
    Calculate and report the difference between each test of two benchmarks
    runs specified as 'json1' and 'json2'.
    """
    assert utest is True or utest is False

    first_col_width = find_longest_name(json_diff_report)
    first_col_width = max(first_col_width, len("Benchmark"))
    first_col_width += len(UTEST_COL_NAME)
    fmt_str = (
        "{:<{}s}Time             CPU      Time Old      Time New       CPU Old"
        "       CPU New"
    )
    first_line = fmt_str.format("Benchmark", 12 + first_col_width)
    output_strs = [first_line, "-" * len(first_line)]

    fmt_str = (
        "{}{:<{}s}{endc}{}{:+16.4f}{endc}{}{:+16.4f}{endc}{:14.0f}{:14.0f}"
        "{endc}{:14.0f}{:14.0f}"
    )
    for benchmark in json_diff_report:
        # *If* we were asked to only include aggregates,
        # and if it is non-aggregate, then don't print it.
        if (
            not include_aggregates_only
            or "run_type" not in benchmark
            or benchmark["run_type"] == "aggregate"
        ):
            for measurement in benchmark["measurements"]:
                output_strs += [
                    color_format(
                        use_color,
                        fmt_str,
                        BC_HEADER,
                        benchmark["name"],
                        first_col_width,
                        get_color(measurement["time"]),
                        measurement["time"],
                        get_color(measurement["cpu"]),
                        measurement["cpu"],
                        measurement["real_time"],
                        measurement["real_time_other"],
                        measurement["cpu_time"],
                        measurement["cpu_time_other"],
                        endc=BC_ENDC,
                    )
                ]

        # After processing the measurements, if requested and
        # if applicable (e.g. u-test exists for given benchmark),
        # print the U test.
        if utest and benchmark["utest"]:
            output_strs += print_utest(
                benchmark["name"],
                benchmark["utest"],
                utest_alpha=utest_alpha,
                first_col_width=first_col_width,
                use_color=use_color,
            )

    return output_strs


def format_with_suffix(val):
    """
    Format a numeric value with {:.2f} and a decimal SI suffix (K, M, G, etc.).
    Returns 'N/A' if val is None.
    """
    if val is None:
        return "N/A"
    suffixes = ("", "K", "M", "G", "T", "P", "E")
    v = float(val)
    abs_v = abs(v)
    idx = 0
    while round(abs_v, 2) >= 1000.0 and idx < len(suffixes) - 1:
        abs_v /= 1000.0
        v /= 1000.0
        idx += 1
    return f"{v:.2f}{suffixes[idx]}"


def format_time_s(seconds):
    """
    Format a duration in seconds using SI time units (s, ms, us, ns).
    """
    if seconds is None:
        return "N/A"
    # Pick the unit and the precision from the rounded value, so that a value
    # just below a boundary is not printed as e.g. '1000.0ms' or '100.00us'.
    for unit, per_second in (("ns", 1e9), ("us", 1e6), ("ms", 1e3), ("s", 1)):
        v = seconds * per_second
        digits = 2 if round(abs(v), 2) < 100 else 1
        if round(abs(v), digits) < 1000 or unit == "s":
            return f"{v:.{digits}f}{unit}"


def summarize_sample(values, confidence=0.95):
    """
    Return (median_value, pct_range_str) using a non-parametric binomial
    quantile confidence interval around the sample median (like benchstat).
    """
    vals = sorted(float(v) for v in values)
    n = len(vals)
    med = float(median(vals))
    if n < 2:
        return med, None
    tail = (1.0 - confidence) / 2.0
    lo_i = int(binom.ppf(tail, n, 0.5)) - 1
    hi_i = int(binom.isf(tail, n, 0.5))
    lo, hi = (
        (vals[0], vals[-1])
        if (lo_i < 0 or hi_i >= n)
        else (vals[lo_i], vals[hi_i])
    )
    if med == 0:
        return med, ("0%" if lo == hi == 0 else "?")
    if (med > 0) != (lo > 0) or (med > 0) != (hi > 0):
        return med, "?"
    pct = max(hi / med - 1.0, 1.0 - lo / med)
    return med, f"{100.0 * pct:.0f}%"


def _format_vs_base(old_med, new_med, lhs_vals, rhs_vals, utest, utest_alpha):
    n1, n2 = len(lhs_vals), len(rhs_vals)
    n_str = f"n={n1}" if n1 == n2 else f"n={n1}+{n2}"
    change = calculate_change(old_med, new_med)
    if not utest or min(n1, n2) < UTEST_MIN_REPETITIONS:
        return f"{change:+.2%}", f"({n_str})", change
    pval = mannwhitneyu(lhs_vals, rhs_vals, alternative="two-sided").pvalue
    # Depending on the scipy version, the p-value is NaN if all the values are
    # identical. There is no difference to detect then.
    if math.isnan(pval):
        pval = 1.0
    if pval > utest_alpha:
        return "~", f"(p={pval:.3f} {n_str})", None
    return f"{change:+.2%}", f"(p={pval:.3f} {n_str})", change


def _extract_metric_values(runs, extractor):
    return [v for r in runs if (v := extractor(r)) is not None]


def _collect_benchstat_rows(partitions, extractor, fmt_fn, utest, utest_alpha):
    rows = []
    pos_meds = []
    for lhs, rhs in partitions:
        l_vals = _extract_metric_values(lhs, extractor)
        r_vals = _extract_metric_values(rhs, extractor)
        if not l_vals or not r_vals:
            continue
        old_m, old_ci = summarize_sample(l_vals)
        new_m, new_ci = summarize_sample(r_vals)
        old_s = f"{fmt_fn(old_m)} ± {old_ci}" if old_ci else fmt_fn(old_m)
        new_s = f"{fmt_fn(new_m)} ± {new_ci}" if new_ci else fmt_fn(new_m)
        delta_s, p_s, chg = _format_vs_base(
            old_m, new_m, l_vals, r_vals, utest, utest_alpha
        )
        rows.append((lhs[0]["name"], old_s, new_s, delta_s, p_s, chg))
        if old_m > 0 and new_m > 0:
            pos_meds.append((old_m, new_m))
    return rows, pos_meds


def _render_benchstat_table(title, rows, pos_meds, fmt_fn, use_color):
    name_w = max(len("geomean"), *(len(r[0]) for r in rows))
    col_w = max(len(title), *(max(len(r[1]), len(r[2])) for r in rows))
    header = (
        f"{'name':<{name_w}s}  {title:>{col_w}s}  {title:>{col_w}s}  vs base"
    )
    lines = [header]
    row_fmt = "{}{:<{}s}{endc}  {:>{}s}  {:>{}s}  {}{:>8s}{endc} {}"
    for bname, old_s, new_s, delta_s, p_s, chg in rows:
        lines.append(
            color_format(
                use_color,
                row_fmt,
                BC_HEADER,
                bname,
                name_w,
                old_s,
                col_w,
                new_s,
                col_w,
                get_color(chg),
                delta_s,
                p_s,
                endc=BC_ENDC,
            )
        )
    if pos_meds:
        gm_old = float(gmean([m[0] for m in pos_meds]))
        gm_new = float(gmean([m[1] for m in pos_meds]))
        gm_chg = calculate_change(gm_old, gm_new)
        gm_fmt = "{}{:<{}s}{endc}  {:>{}s}  {:>{}s}  {}{:+8.2%}{endc}"
        lines.append(
            color_format(
                use_color,
                gm_fmt,
                BC_HEADER,
                "geomean",
                name_w,
                fmt_fn(gm_old),
                col_w,
                fmt_fn(gm_new),
                col_w,
                get_color(gm_chg),
                gm_chg,
                endc=BC_ENDC,
            )
        )
    return lines


# The numeric fields that the JSON reporter itself writes for a run. Any other
# numeric field is a user counter, e.g. one of --benchmark_perf_counters.
_NON_COUNTER_FIELDS = frozenset(
    (
        "family_index",
        "per_family_instance_index",
        "repetitions",
        "repetition_index",
        "threads",
        "iterations",
        "real_time",
        "cpu_time",
        "cpu_coefficient",
        "real_coefficient",
        "rms",
        "memory_iterations",
        "allocs_per_iter",
        "max_bytes_used",
        "total_allocated_bytes",
        "net_heap_growth",
    )
)


def _get_counter_names(runs):
    """
    While *keeping* the order, give the names of all the user counters
    reported by the given runs.
    """
    return list(
        dict.fromkeys(
            key
            for run in runs
            for key, value in run.items()
            if key not in _NON_COUNTER_FIELDS
            and isinstance(value, (int, float))
            and not isinstance(value, bool)
        )
    )


def _select_benchstat_runs(json1, json2):
    """
    Give the runs of both inputs to summarize: the individual repetitions, or,
    if an input only has the aggregates (e.g. because of
    --benchmark_report_aggregates_only), the means. The other aggregates
    (median, stddev, cv, ...) are not benchmarks of their own, and are never
    compared.
    """

    def select(json, wanted):
        return {"benchmarks": [b for b in json["benchmarks"] if wanted(b)]}

    def is_repetition(run):
        return run.get("run_type") != "aggregate"

    def is_mean(run):
        return not is_repetition(run) and run.get("aggregate_name") == "mean"

    selected = [select(json, is_repetition) for json in (json1, json2)]
    if not all(s["benchmarks"] for s in selected):
        selected = [select(json, is_mean) for json in (json1, json2)]
    return selected


def print_benchstat_report(
    json1, json2, utest=True, utest_alpha=0.05, use_color=True
):
    """
    Format benchmark comparisons as compact benchstat per-metric tables
    (cpu/op, time/op, one per user counter reported by both inputs,
    allocs/op, peak-mem(B)) with median ± 95% CI, Mann-Whitney U-test
    p-values, and geomean.
    """
    assert utest is True or utest is False
    partitions = partition_benchmarks(*_select_benchstat_runs(json1, json2))
    counters = intersect(
        _get_counter_names(run for lhs, _ in partitions for run in lhs),
        _get_counter_names(run for _, rhs in partitions for run in rhs),
    )
    metrics = [
        (
            "cpu/op",
            lambda r: get_timedelta_field_as_seconds(r, "cpu_time"),
            format_time_s,
        ),
        (
            "time/op",
            lambda r: get_timedelta_field_as_seconds(r, "real_time"),
            format_time_s,
        ),
    ]
    metrics += [
        (name, lambda r, name=name: r.get(name), format_with_suffix)
        for name in counters
    ]
    metrics += [
        ("allocs/op", lambda r: r.get("allocs_per_iter"), format_with_suffix),
        ("peak-mem(B)", lambda r: r.get("max_bytes_used"), format_with_suffix),
    ]
    output_strs = []
    for title, extractor, fmt_fn in metrics:
        rows, pos_meds = _collect_benchstat_rows(
            partitions, extractor, fmt_fn, utest, utest_alpha
        )
        if rows:
            if output_strs:
                output_strs.append("")
            output_strs.extend(
                _render_benchstat_table(
                    title, rows, pos_meds, fmt_fn, use_color
                )
            )
    return output_strs


###############################################################################
# Unit tests


class TestGetUniqueBenchmarkNames(unittest.TestCase):
    def load_results(self):
        import json

        testInputs = os.path.join(
            os.path.dirname(os.path.realpath(__file__)), "Inputs"
        )
        testOutput = os.path.join(testInputs, "test3_run0.json")
        with open(testOutput) as f:
            json = json.load(f)
        return json

    def test_basic(self):
        expect_lines = [
            "BM_One",
            "BM_Two",
            "short",  # These two are not sorted
            "medium",  # These two are not sorted
        ]
        json = self.load_results()
        output_lines = get_unique_benchmark_names(json)
        print("\n")
        print("\n".join(output_lines))
        self.assertEqual(len(output_lines), len(expect_lines))
        for i, output_line in enumerate(output_lines):
            self.assertEqual(expect_lines[i], output_line)


class TestReportDifference(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def load_results():
            import json

            testInputs = os.path.join(
                os.path.dirname(os.path.realpath(__file__)), "Inputs"
            )
            testOutput1 = os.path.join(testInputs, "test1_run1.json")
            testOutput2 = os.path.join(testInputs, "test1_run2.json")
            with open(testOutput1) as f:
                json1 = json.load(f)
            with open(testOutput2) as f:
                json2 = json.load(f)
            return json1, json2

        json1, json2 = load_results()
        cls.json_diff_report = get_difference_report(json1, json2)

    def test_json_diff_report_pretty_printing(self):
        expect_lines = [
            ["BM_SameTimes", "+0.0000", "+0.0000", "10", "10", "10", "10"],
            ["BM_2xFaster", "-0.5000", "-0.5000", "50", "25", "50", "25"],
            ["BM_2xSlower", "+1.0000", "+1.0000", "50", "100", "50", "100"],
            [
                "BM_1PercentFaster",
                "-0.0100",
                "-0.0100",
                "100",
                "99",
                "100",
                "99",
            ],
            [
                "BM_1PercentSlower",
                "+0.0100",
                "+0.0100",
                "100",
                "101",
                "100",
                "101",
            ],
            [
                "BM_10PercentFaster",
                "-0.1000",
                "-0.1000",
                "100",
                "90",
                "100",
                "90",
            ],
            [
                "BM_10PercentSlower",
                "+0.1000",
                "+0.1000",
                "100",
                "110",
                "100",
                "110",
            ],
            [
                "BM_100xSlower",
                "+99.0000",
                "+99.0000",
                "100",
                "10000",
                "100",
                "10000",
            ],
            [
                "BM_100xFaster",
                "-0.9900",
                "-0.9900",
                "10000",
                "100",
                "10000",
                "100",
            ],
            [
                "BM_10PercentCPUToTime",
                "+0.1000",
                "-0.1000",
                "100",
                "110",
                "100",
                "90",
            ],
            ["BM_ThirdFaster", "-0.3333", "-0.3334", "100", "67", "100", "67"],
            ["BM_NotBadTimeUnit", "-0.9000", "+0.2000", "0", "0", "0", "1"],
            ["BM_hasLabel", "+0.0000", "+0.0000", "1", "1", "1", "1"],
            ["OVERALL_GEOMEAN", "-0.8113", "-0.7779", "0", "0", "0", "0"],
        ]
        output_lines_with_header = print_difference_report(
            self.json_diff_report, use_color=False
        )
        output_lines = output_lines_with_header[2:]
        print("\n")
        print("\n".join(output_lines_with_header))
        self.assertEqual(len(output_lines), len(expect_lines))
        for i, output_line in enumerate(output_lines):
            parts = [x for x in output_line.split(" ") if x]
            self.assertEqual(len(parts), 7)
            self.assertEqual(expect_lines[i], parts)

    def test_json_diff_report_output(self):
        expected_output = [
            {
                "name": "BM_SameTimes",
                "label": "",
                "measurements": [
                    {
                        "time": 0.0000,
                        "cpu": 0.0000,
                        "real_time": 10,
                        "real_time_other": 10,
                        "cpu_time": 10,
                        "cpu_time_other": 10,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_2xFaster",
                "label": "",
                "measurements": [
                    {
                        "time": -0.5000,
                        "cpu": -0.5000,
                        "real_time": 50,
                        "real_time_other": 25,
                        "cpu_time": 50,
                        "cpu_time_other": 25,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_2xSlower",
                "label": "",
                "measurements": [
                    {
                        "time": 1.0000,
                        "cpu": 1.0000,
                        "real_time": 50,
                        "real_time_other": 100,
                        "cpu_time": 50,
                        "cpu_time_other": 100,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_1PercentFaster",
                "label": "",
                "measurements": [
                    {
                        "time": -0.0100,
                        "cpu": -0.0100,
                        "real_time": 100,
                        "real_time_other": 98.9999999,
                        "cpu_time": 100,
                        "cpu_time_other": 98.9999999,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_1PercentSlower",
                "label": "",
                "measurements": [
                    {
                        "time": 0.0100,
                        "cpu": 0.0100,
                        "real_time": 100,
                        "real_time_other": 101,
                        "cpu_time": 100,
                        "cpu_time_other": 101,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_10PercentFaster",
                "label": "",
                "measurements": [
                    {
                        "time": -0.1000,
                        "cpu": -0.1000,
                        "real_time": 100,
                        "real_time_other": 90,
                        "cpu_time": 100,
                        "cpu_time_other": 90,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_10PercentSlower",
                "label": "",
                "measurements": [
                    {
                        "time": 0.1000,
                        "cpu": 0.1000,
                        "real_time": 100,
                        "real_time_other": 110,
                        "cpu_time": 100,
                        "cpu_time_other": 110,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_100xSlower",
                "label": "",
                "measurements": [
                    {
                        "time": 99.0000,
                        "cpu": 99.0000,
                        "real_time": 100,
                        "real_time_other": 10000,
                        "cpu_time": 100,
                        "cpu_time_other": 10000,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_100xFaster",
                "label": "",
                "measurements": [
                    {
                        "time": -0.9900,
                        "cpu": -0.9900,
                        "real_time": 10000,
                        "real_time_other": 100,
                        "cpu_time": 10000,
                        "cpu_time_other": 100,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_10PercentCPUToTime",
                "label": "",
                "measurements": [
                    {
                        "time": 0.1000,
                        "cpu": -0.1000,
                        "real_time": 100,
                        "real_time_other": 110,
                        "cpu_time": 100,
                        "cpu_time_other": 90,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_ThirdFaster",
                "label": "",
                "measurements": [
                    {
                        "time": -0.3333,
                        "cpu": -0.3334,
                        "real_time": 100,
                        "real_time_other": 67,
                        "cpu_time": 100,
                        "cpu_time_other": 67,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_NotBadTimeUnit",
                "label": "",
                "measurements": [
                    {
                        "time": -0.9000,
                        "cpu": 0.2000,
                        "real_time": 0.4,
                        "real_time_other": 0.04,
                        "cpu_time": 0.5,
                        "cpu_time_other": 0.6,
                    }
                ],
                "time_unit": "s",
                "utest": {},
            },
            {
                "name": "BM_hasLabel",
                "label": "a label",
                "measurements": [
                    {
                        "time": 0.0000,
                        "cpu": 0.0000,
                        "real_time": 1,
                        "real_time_other": 1,
                        "cpu_time": 1,
                        "cpu_time_other": 1,
                    }
                ],
                "time_unit": "s",
                "utest": {},
            },
            {
                "name": "OVERALL_GEOMEAN",
                "label": "",
                "measurements": [
                    {
                        "real_time": 3.1622776601683826e-06,
                        "cpu_time": 3.2130844755623912e-06,
                        "real_time_other": 1.9768988699420897e-07,
                        "cpu_time_other": 2.397447755209533e-07,
                        "time": -0.8112976497120911,
                        "cpu": -0.7778551721181174,
                    }
                ],
                "time_unit": "s",
                "run_type": "aggregate",
                "aggregate_name": "geomean",
                "utest": {},
            },
        ]
        self.assertEqual(len(self.json_diff_report), len(expected_output))
        for out, expected in zip(
            self.json_diff_report, expected_output, strict=True
        ):
            self.assertEqual(out["name"], expected["name"])
            self.assertEqual(out["label"], expected["label"])
            self.assertEqual(out["time_unit"], expected["time_unit"])
            assert_utest(self, out, expected)
            assert_measurements(self, out, expected)


class TestReportDifferenceBetweenFamilies(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def load_result():
            import json

            testInputs = os.path.join(
                os.path.dirname(os.path.realpath(__file__)), "Inputs"
            )
            testOutput = os.path.join(testInputs, "test2_run.json")
            with open(testOutput) as f:
                json = json.load(f)
            return json

        json = load_result()
        json1 = filter_benchmark(json, "BM_Z.ro", ".")
        json2 = filter_benchmark(json, "BM_O.e", ".")
        cls.json_diff_report = get_difference_report(json1, json2)

    def test_json_diff_report_pretty_printing(self):
        expect_lines = [
            [".", "-0.5000", "-0.5000", "10", "5", "10", "5"],
            ["./4", "-0.5000", "-0.5000", "40", "20", "40", "20"],
            ["Prefix/.", "-0.5000", "-0.5000", "20", "10", "20", "10"],
            ["Prefix/./3", "-0.5000", "-0.5000", "30", "15", "30", "15"],
            ["OVERALL_GEOMEAN", "-0.5000", "-0.5000", "0", "0", "0", "0"],
        ]
        output_lines_with_header = print_difference_report(
            self.json_diff_report, use_color=False
        )
        output_lines = output_lines_with_header[2:]
        print("\n")
        print("\n".join(output_lines_with_header))
        self.assertEqual(len(output_lines), len(expect_lines))
        for i, output_line in enumerate(output_lines):
            parts = [x for x in output_line.split(" ") if x]
            self.assertEqual(len(parts), 7)
            self.assertEqual(expect_lines[i], parts)

    def test_json_diff_report(self):
        expected_output = [
            {
                "name": ".",
                "measurements": [
                    {
                        "time": -0.5,
                        "cpu": -0.5,
                        "real_time": 10,
                        "real_time_other": 5,
                        "cpu_time": 10,
                        "cpu_time_other": 5,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "./4",
                "measurements": [
                    {
                        "time": -0.5,
                        "cpu": -0.5,
                        "real_time": 40,
                        "real_time_other": 20,
                        "cpu_time": 40,
                        "cpu_time_other": 20,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "Prefix/.",
                "measurements": [
                    {
                        "time": -0.5,
                        "cpu": -0.5,
                        "real_time": 20,
                        "real_time_other": 10,
                        "cpu_time": 20,
                        "cpu_time_other": 10,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "Prefix/./3",
                "measurements": [
                    {
                        "time": -0.5,
                        "cpu": -0.5,
                        "real_time": 30,
                        "real_time_other": 15,
                        "cpu_time": 30,
                        "cpu_time_other": 15,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "OVERALL_GEOMEAN",
                "measurements": [
                    {
                        "real_time": 2.213363839400641e-08,
                        "cpu_time": 2.213363839400641e-08,
                        "real_time_other": 1.1066819197003185e-08,
                        "cpu_time_other": 1.1066819197003185e-08,
                        "time": -0.5000000000000009,
                        "cpu": -0.5000000000000009,
                    }
                ],
                "time_unit": "s",
                "run_type": "aggregate",
                "aggregate_name": "geomean",
                "utest": {},
            },
        ]
        self.assertEqual(len(self.json_diff_report), len(expected_output))
        for out, expected in zip(
            self.json_diff_report, expected_output, strict=True
        ):
            self.assertEqual(out["name"], expected["name"])
            self.assertEqual(out["time_unit"], expected["time_unit"])
            assert_utest(self, out, expected)
            assert_measurements(self, out, expected)


class TestReportDifferenceWithUTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def load_results():
            import json

            testInputs = os.path.join(
                os.path.dirname(os.path.realpath(__file__)), "Inputs"
            )
            testOutput1 = os.path.join(testInputs, "test3_run0.json")
            testOutput2 = os.path.join(testInputs, "test3_run1.json")
            with open(testOutput1) as f:
                json1 = json.load(f)
            with open(testOutput2) as f:
                json2 = json.load(f)
            return json1, json2

        json1, json2 = load_results()
        cls.json_diff_report = get_difference_report(json1, json2, utest=True)

    def test_json_diff_report_pretty_printing(self):
        expect_lines = [
            ["BM_One", "-0.1000", "+0.1000", "10", "9", "100", "110"],
            ["BM_Two", "+0.1111", "-0.0111", "9", "10", "90", "89"],
            ["BM_Two", "-0.1250", "-0.1628", "8", "7", "86", "72"],
            [
                "BM_Two_pvalue",
                "1.0000",
                "0.6667",
                "U",
                "Test,",
                "Repetitions:",
                "2",
                "vs",
                "2.",
                "WARNING:",
                "Results",
                "unreliable!",
                "9+",
                "repetitions",
                "recommended.",
            ],
            ["short", "-0.1250", "-0.0625", "8", "7", "80", "75"],
            ["short", "-0.4325", "-0.1351", "8", "5", "77", "67"],
            [
                "short_pvalue",
                "0.7671",
                "0.2000",
                "U",
                "Test,",
                "Repetitions:",
                "2",
                "vs",
                "3.",
                "WARNING:",
                "Results",
                "unreliable!",
                "9+",
                "repetitions",
                "recommended.",
            ],
            ["medium", "-0.3750", "-0.3375", "8", "5", "80", "53"],
            ["OVERALL_GEOMEAN", "+1.6405", "-0.6985", "0", "0", "0", "0"],
        ]
        output_lines_with_header = print_difference_report(
            self.json_diff_report, utest=True, utest_alpha=0.05, use_color=False
        )
        output_lines = output_lines_with_header[2:]
        print("\n")
        print("\n".join(output_lines_with_header))
        self.assertEqual(len(output_lines), len(expect_lines))
        for i, output_line in enumerate(output_lines):
            parts = [x for x in output_line.split(" ") if x]
            self.assertEqual(expect_lines[i], parts)

    def test_json_diff_report_pretty_printing_aggregates_only(self):
        expect_lines = [
            ["BM_One", "-0.1000", "+0.1000", "10", "9", "100", "110"],
            [
                "BM_Two_pvalue",
                "1.0000",
                "0.6667",
                "U",
                "Test,",
                "Repetitions:",
                "2",
                "vs",
                "2.",
                "WARNING:",
                "Results",
                "unreliable!",
                "9+",
                "repetitions",
                "recommended.",
            ],
            ["short", "-0.1250", "-0.0625", "8", "7", "80", "75"],
            ["short", "-0.4325", "-0.1351", "8", "5", "77", "67"],
            [
                "short_pvalue",
                "0.7671",
                "0.2000",
                "U",
                "Test,",
                "Repetitions:",
                "2",
                "vs",
                "3.",
                "WARNING:",
                "Results",
                "unreliable!",
                "9+",
                "repetitions",
                "recommended.",
            ],
            ["OVERALL_GEOMEAN", "+1.6405", "-0.6985", "0", "0", "0", "0"],
        ]
        output_lines_with_header = print_difference_report(
            self.json_diff_report,
            include_aggregates_only=True,
            utest=True,
            utest_alpha=0.05,
            use_color=False,
        )
        output_lines = output_lines_with_header[2:]
        print("\n")
        print("\n".join(output_lines_with_header))
        self.assertEqual(len(output_lines), len(expect_lines))
        for i, output_line in enumerate(output_lines):
            parts = [x for x in output_line.split(" ") if x]
            self.assertEqual(expect_lines[i], parts)

    def test_json_diff_report(self):
        expected_output = [
            {
                "name": "BM_One",
                "measurements": [
                    {
                        "time": -0.1,
                        "cpu": 0.1,
                        "real_time": 10,
                        "real_time_other": 9,
                        "cpu_time": 100,
                        "cpu_time_other": 110,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_Two",
                "measurements": [
                    {
                        "time": 0.1111111111111111,
                        "cpu": -0.011111111111111112,
                        "real_time": 9,
                        "real_time_other": 10,
                        "cpu_time": 90,
                        "cpu_time_other": 89,
                    },
                    {
                        "time": -0.125,
                        "cpu": -0.16279069767441862,
                        "real_time": 8,
                        "real_time_other": 7,
                        "cpu_time": 86,
                        "cpu_time_other": 72,
                    },
                ],
                "time_unit": "ns",
                "utest": {
                    "have_optimal_repetitions": False,
                    "cpu_pvalue": 0.6666666666666666,
                    "time_pvalue": 1.0,
                },
            },
            {
                "name": "short",
                "measurements": [
                    {
                        "time": -0.125,
                        "cpu": -0.0625,
                        "real_time": 8,
                        "real_time_other": 7,
                        "cpu_time": 80,
                        "cpu_time_other": 75,
                    },
                    {
                        "time": -0.4325,
                        "cpu": -0.13506493506493514,
                        "real_time": 8,
                        "real_time_other": 4.54,
                        "cpu_time": 77,
                        "cpu_time_other": 66.6,
                    },
                ],
                "time_unit": "ns",
                "utest": {
                    "have_optimal_repetitions": False,
                    "cpu_pvalue": 0.2,
                    "time_pvalue": 0.7670968684102772,
                },
            },
            {
                "name": "medium",
                "measurements": [
                    {
                        "time": -0.375,
                        "cpu": -0.3375,
                        "real_time": 8,
                        "real_time_other": 5,
                        "cpu_time": 80,
                        "cpu_time_other": 53,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "OVERALL_GEOMEAN",
                "measurements": [
                    {
                        "real_time": 8.48528137423858e-09,
                        "cpu_time": 8.441336246629233e-08,
                        "real_time_other": 2.2405267593145244e-08,
                        "cpu_time_other": 2.5453661413660466e-08,
                        "time": 1.6404861082353634,
                        "cpu": -0.6984640740519662,
                    }
                ],
                "time_unit": "s",
                "run_type": "aggregate",
                "aggregate_name": "geomean",
                "utest": {},
            },
        ]
        self.assertEqual(len(self.json_diff_report), len(expected_output))
        for out, expected in zip(
            self.json_diff_report, expected_output, strict=True
        ):
            self.assertEqual(out["name"], expected["name"])
            self.assertEqual(out["time_unit"], expected["time_unit"])
            assert_utest(self, out, expected)
            assert_measurements(self, out, expected)


class TestReportDifferenceWithUTestWhileDisplayingAggregatesOnly(
    unittest.TestCase
):
    @classmethod
    def setUpClass(cls):
        def load_results():
            import json

            testInputs = os.path.join(
                os.path.dirname(os.path.realpath(__file__)), "Inputs"
            )
            testOutput1 = os.path.join(testInputs, "test3_run0.json")
            testOutput2 = os.path.join(testInputs, "test3_run1.json")
            with open(testOutput1) as f:
                json1 = json.load(f)
            with open(testOutput2) as f:
                json2 = json.load(f)
            return json1, json2

        json1, json2 = load_results()
        cls.json_diff_report = get_difference_report(json1, json2, utest=True)

    def test_json_diff_report_pretty_printing(self):
        expect_lines = [
            ["BM_One", "-0.1000", "+0.1000", "10", "9", "100", "110"],
            ["BM_Two", "+0.1111", "-0.0111", "9", "10", "90", "89"],
            ["BM_Two", "-0.1250", "-0.1628", "8", "7", "86", "72"],
            [
                "BM_Two_pvalue",
                "1.0000",
                "0.6667",
                "U",
                "Test,",
                "Repetitions:",
                "2",
                "vs",
                "2.",
                "WARNING:",
                "Results",
                "unreliable!",
                "9+",
                "repetitions",
                "recommended.",
            ],
            ["short", "-0.1250", "-0.0625", "8", "7", "80", "75"],
            ["short", "-0.4325", "-0.1351", "8", "5", "77", "67"],
            [
                "short_pvalue",
                "0.7671",
                "0.2000",
                "U",
                "Test,",
                "Repetitions:",
                "2",
                "vs",
                "3.",
                "WARNING:",
                "Results",
                "unreliable!",
                "9+",
                "repetitions",
                "recommended.",
            ],
            ["medium", "-0.3750", "-0.3375", "8", "5", "80", "53"],
            ["OVERALL_GEOMEAN", "+1.6405", "-0.6985", "0", "0", "0", "0"],
        ]
        output_lines_with_header = print_difference_report(
            self.json_diff_report, utest=True, utest_alpha=0.05, use_color=False
        )
        output_lines = output_lines_with_header[2:]
        print("\n")
        print("\n".join(output_lines_with_header))
        self.assertEqual(len(output_lines), len(expect_lines))
        for i, output_line in enumerate(output_lines):
            parts = [x for x in output_line.split(" ") if x]
            self.assertEqual(expect_lines[i], parts)

    def test_json_diff_report(self):
        expected_output = [
            {
                "name": "BM_One",
                "measurements": [
                    {
                        "time": -0.1,
                        "cpu": 0.1,
                        "real_time": 10,
                        "real_time_other": 9,
                        "cpu_time": 100,
                        "cpu_time_other": 110,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            },
            {
                "name": "BM_Two",
                "measurements": [
                    {
                        "time": 0.1111111111111111,
                        "cpu": -0.011111111111111112,
                        "real_time": 9,
                        "real_time_other": 10,
                        "cpu_time": 90,
                        "cpu_time_other": 89,
                    },
                    {
                        "time": -0.125,
                        "cpu": -0.16279069767441862,
                        "real_time": 8,
                        "real_time_other": 7,
                        "cpu_time": 86,
                        "cpu_time_other": 72,
                    },
                ],
                "time_unit": "ns",
                "utest": {
                    "have_optimal_repetitions": False,
                    "cpu_pvalue": 0.6666666666666666,
                    "time_pvalue": 1.0,
                },
            },
            {
                "name": "short",
                "measurements": [
                    {
                        "time": -0.125,
                        "cpu": -0.0625,
                        "real_time": 8,
                        "real_time_other": 7,
                        "cpu_time": 80,
                        "cpu_time_other": 75,
                    },
                    {
                        "time": -0.4325,
                        "cpu": -0.13506493506493514,
                        "real_time": 8,
                        "real_time_other": 4.54,
                        "cpu_time": 77,
                        "cpu_time_other": 66.6,
                    },
                ],
                "time_unit": "ns",
                "utest": {
                    "have_optimal_repetitions": False,
                    "cpu_pvalue": 0.2,
                    "time_pvalue": 0.7670968684102772,
                },
            },
            {
                "name": "medium",
                "measurements": [
                    {
                        "real_time_other": 5,
                        "cpu_time": 80,
                        "time": -0.375,
                        "real_time": 8,
                        "cpu_time_other": 53,
                        "cpu": -0.3375,
                    }
                ],
                "utest": {},
                "time_unit": "ns",
                "aggregate_name": "",
            },
            {
                "name": "OVERALL_GEOMEAN",
                "measurements": [
                    {
                        "real_time": 8.48528137423858e-09,
                        "cpu_time": 8.441336246629233e-08,
                        "real_time_other": 2.2405267593145244e-08,
                        "cpu_time_other": 2.5453661413660466e-08,
                        "time": 1.6404861082353634,
                        "cpu": -0.6984640740519662,
                    }
                ],
                "time_unit": "s",
                "run_type": "aggregate",
                "aggregate_name": "geomean",
                "utest": {},
            },
        ]
        self.assertEqual(len(self.json_diff_report), len(expected_output))
        for out, expected in zip(
            self.json_diff_report, expected_output, strict=True
        ):
            self.assertEqual(out["name"], expected["name"])
            self.assertEqual(out["time_unit"], expected["time_unit"])
            assert_utest(self, out, expected)
            assert_measurements(self, out, expected)


class TestReportDifferenceForPercentageAggregates(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def load_results():
            import json

            testInputs = os.path.join(
                os.path.dirname(os.path.realpath(__file__)), "Inputs"
            )
            testOutput1 = os.path.join(testInputs, "test4_run0.json")
            testOutput2 = os.path.join(testInputs, "test4_run1.json")
            with open(testOutput1) as f:
                json1 = json.load(f)
            with open(testOutput2) as f:
                json2 = json.load(f)
            return json1, json2

        json1, json2 = load_results()
        cls.json_diff_report = get_difference_report(json1, json2, utest=True)

    def test_json_diff_report_pretty_printing(self):
        expect_lines = [["whocares", "-0.5000", "+0.5000", "0", "0", "0", "0"]]
        output_lines_with_header = print_difference_report(
            self.json_diff_report, utest=True, utest_alpha=0.05, use_color=False
        )
        output_lines = output_lines_with_header[2:]
        print("\n")
        print("\n".join(output_lines_with_header))
        self.assertEqual(len(output_lines), len(expect_lines))
        for i, output_line in enumerate(output_lines):
            parts = [x for x in output_line.split(" ") if x]
            self.assertEqual(expect_lines[i], parts)

    def test_json_diff_report(self):
        expected_output = [
            {
                "name": "whocares",
                "measurements": [
                    {
                        "time": -0.5,
                        "cpu": 0.5,
                        "real_time": 0.01,
                        "real_time_other": 0.005,
                        "cpu_time": 0.10,
                        "cpu_time_other": 0.15,
                    }
                ],
                "time_unit": "ns",
                "utest": {},
            }
        ]
        self.assertEqual(len(self.json_diff_report), len(expected_output))
        for out, expected in zip(
            self.json_diff_report, expected_output, strict=True
        ):
            self.assertEqual(out["name"], expected["name"])
            self.assertEqual(out["time_unit"], expected["time_unit"])
            assert_utest(self, out, expected)
            assert_measurements(self, out, expected)


class TestReportSorting(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def load_result():
            import json

            testInputs = os.path.join(
                os.path.dirname(os.path.realpath(__file__)), "Inputs"
            )
            testOutput = os.path.join(testInputs, "test4_run.json")
            with open(testOutput) as f:
                json = json.load(f)
            return json

        cls.json = load_result()

    def test_json_diff_report_pretty_printing(self):
        import util

        expected_names = [
            "99 family 0 instance 0 repetition 0",
            "98 family 0 instance 0 repetition 1",
            "97 family 0 instance 0 aggregate",
            "96 family 0 instance 1 repetition 0",
            "95 family 0 instance 1 repetition 1",
            "94 family 0 instance 1 aggregate",
            "93 family 1 instance 0 repetition 0",
            "92 family 1 instance 0 repetition 1",
            "91 family 1 instance 0 aggregate",
            "90 family 1 instance 1 repetition 0",
            "89 family 1 instance 1 repetition 1",
            "88 family 1 instance 1 aggregate",
        ]

        for _n in range(len(self.json["benchmarks"]) ** 2):
            random.shuffle(self.json["benchmarks"])
            sorted_benchmarks = util.sort_benchmark_results(self.json)[
                "benchmarks"
            ]
            self.assertEqual(len(expected_names), len(sorted_benchmarks))
            for out, expected in zip(
                sorted_benchmarks, expected_names, strict=True
            ):
                self.assertEqual(out["name"], expected)


class TestReportDifferenceWithUTestWhileDisplayingAggregatesOnly2(
    unittest.TestCase
):
    @classmethod
    def setUpClass(cls):
        def load_results():
            import json

            testInputs = os.path.join(
                os.path.dirname(os.path.realpath(__file__)), "Inputs"
            )
            testOutput1 = os.path.join(testInputs, "test5_run0.json")
            testOutput2 = os.path.join(testInputs, "test5_run1.json")
            with open(testOutput1) as f:
                json1 = json.load(f)
                json1["benchmarks"] = [
                    json1["benchmarks"][0] for i in range(1000)
                ]
            with open(testOutput2) as f:
                json2 = json.load(f)
                json2["benchmarks"] = [
                    json2["benchmarks"][0] for i in range(1000)
                ]
            return json1, json2

        json1, json2 = load_results()
        cls.json_diff_report = get_difference_report(json1, json2, utest=True)

    def test_json_diff_report_pretty_printing(self):
        expect_line = [
            "BM_ManyRepetitions_pvalue",
            "0.0000",
            "0.0000",
            "U",
            "Test,",
            "Repetitions:",
            "1000",
            "vs",
            "1000",
        ]
        output_lines_with_header = print_difference_report(
            self.json_diff_report, utest=True, utest_alpha=0.05, use_color=False
        )
        output_lines = output_lines_with_header[2:]
        found = False
        for output_line in output_lines:
            parts = [x for x in output_line.split(" ") if x]
            found = expect_line == parts
            if found:
                break
        self.assertTrue(found)

    def test_json_diff_report(self):
        expected_output = [
            {
                "name": "BM_ManyRepetitions",
                "label": "",
                "time_unit": "s",
                "run_type": "",
                "aggregate_name": "",
                "utest": {
                    "have_optimal_repetitions": True,
                    "cpu_pvalue": 0.0,
                    "time_pvalue": 0.0,
                    "nr_of_repetitions": 1000,
                    "nr_of_repetitions_other": 1000,
                },
            },
            {
                "name": "OVERALL_GEOMEAN",
                "label": "",
                "measurements": [
                    {
                        "real_time": 1.0,
                        "cpu_time": 1000.000000000069,
                        "real_time_other": 1000.000000000069,
                        "cpu_time_other": 1.0,
                        "time": 999.000000000069,
                        "cpu": -0.9990000000000001,
                    }
                ],
                "time_unit": "s",
                "run_type": "aggregate",
                "aggregate_name": "geomean",
                "utest": {},
            },
        ]
        self.assertEqual(len(self.json_diff_report), len(expected_output))
        for out, expected in zip(
            self.json_diff_report, expected_output, strict=True
        ):
            self.assertEqual(out["name"], expected["name"])
            self.assertEqual(out["time_unit"], expected["time_unit"])
            assert_utest(self, out, expected)


def assert_utest(unittest_instance, lhs, rhs):
    if lhs["utest"]:
        unittest_instance.assertAlmostEqual(
            lhs["utest"]["cpu_pvalue"], rhs["utest"]["cpu_pvalue"]
        )
        unittest_instance.assertAlmostEqual(
            lhs["utest"]["time_pvalue"], rhs["utest"]["time_pvalue"]
        )
        unittest_instance.assertEqual(
            lhs["utest"]["have_optimal_repetitions"],
            rhs["utest"]["have_optimal_repetitions"],
        )
    else:
        # lhs is empty. assert if rhs is not.
        unittest_instance.assertEqual(lhs["utest"], rhs["utest"])


def assert_measurements(unittest_instance, lhs, rhs):
    for m1, m2 in zip(lhs["measurements"], rhs["measurements"], strict=False):
        unittest_instance.assertEqual(m1["real_time"], m2["real_time"])
        unittest_instance.assertEqual(m1["cpu_time"], m2["cpu_time"])
        # m1['time'] and m1['cpu'] hold values which are being calculated,
        # and therefore we must use almost-equal pattern.
        unittest_instance.assertAlmostEqual(m1["time"], m2["time"], places=4)
        unittest_instance.assertAlmostEqual(m1["cpu"], m2["cpu"], places=4)


class TestBenchstatFormatHelpers(unittest.TestCase):
    def test_get_color(self):
        self.assertIs(get_color(None), BC_NONE)
        self.assertIs(get_color(0.06), BC_FAIL)
        self.assertIs(get_color(0.0), BC_WHITE)
        self.assertIs(get_color(-0.08), BC_CYAN)

    def test_format_with_suffix(self):
        self.assertEqual(format_with_suffix(None), "N/A")
        self.assertEqual(format_with_suffix(0), "0.00")
        self.assertEqual(format_with_suffix(12), "12.00")
        self.assertEqual(format_with_suffix(999.9), "999.90")
        self.assertEqual(format_with_suffix(1000), "1.00K")
        self.assertEqual(format_with_suffix(8256525), "8.26M")
        self.assertEqual(format_with_suffix(4868373980), "4.87G")
        self.assertEqual(format_with_suffix(1234567890123), "1.23T")

    def test_format_time_s(self):
        self.assertEqual(format_time_s(None), "N/A")
        self.assertEqual(format_time_s(0.0), "0.00ns")
        self.assertEqual(format_time_s(4.25e-8), "42.50ns")
        self.assertEqual(format_time_s(1.5e-4), "150.0us")
        self.assertEqual(format_time_s(0.1147), "114.7ms")
        self.assertEqual(format_time_s(3.071), "3.07s")
        # Rounding up to a boundary moves on to the next unit / precision.
        self.assertEqual(format_time_s(9.99996e-7), "1.00us")
        self.assertEqual(format_time_s(0.99996), "1.00s")
        self.assertEqual(format_time_s(9.9999e-5), "100.0us")


class TestBenchstatReport(unittest.TestCase):
    def _make_run(
        self,
        name,
        rep_idx,
        real_ns,
        cpu_ns,
        allocs=None,
        peak_mem=None,
        counters=None,
    ):
        entry = {
            "name": name,
            "family_index": 0,
            "per_family_instance_index": 0,
            "run_name": name,
            "run_type": "iteration",
            "repetitions": 10,
            "repetition_index": rep_idx,
            "threads": 1,
            "iterations": 1,
            "real_time": real_ns,
            "cpu_time": cpu_ns,
            "time_unit": "ns",
        }
        entry.update(counters or {})
        if allocs is not None or peak_mem is not None:
            entry["memory_iterations"] = 1
        if allocs is not None:
            entry["allocs_per_iter"] = allocs
        if peak_mem is not None:
            entry["max_bytes_used"] = peak_mem
        return entry

    def _make_aggregate(self, name, aggregate_name, value, unit="time"):
        return {
            "name": f"{name}_{aggregate_name}",
            "run_name": name,
            "run_type": "aggregate",
            "aggregate_name": aggregate_name,
            "aggregate_unit": unit,
            "real_time": value,
            "cpu_time": value,
            "time_unit": "ns",
        }

    def _table_titles(self, lines):
        return [ln.split()[1] for ln in lines if ln.startswith("name ")]

    def test_multi_metric_tables_and_aggregates_ignored(self):
        json1 = {"benchmarks": []}
        json2 = {"benchmarks": []}
        for i in range(10):
            json1["benchmarks"].append(
                self._make_run(
                    "BM_Kernel",
                    i,
                    real_ns=100_000_000 + i * 100_000,
                    cpu_ns=200_000_000 + i * 100_000,
                    allocs=15_000 + i * 12,
                    peak_mem=1_000_000_000,
                    counters={
                        "INSTRUCTIONS": 500_000_000 + i * 1_000,
                        "CYCLES": 250_000_000 + i * 1_000,
                    },
                )
            )
            json2["benchmarks"].append(
                self._make_run(
                    "BM_Kernel",
                    i,
                    real_ns=100_050_000 + i * 100_000,
                    cpu_ns=150_000_000 + i * 100_000,
                    allocs=30_000 + i * 12,
                    peak_mem=500_000_000,
                    counters={
                        "INSTRUCTIONS": 400_000_000 + i * 1_000,
                        "CYCLES": 200_000_000 + i * 1_000,
                    },
                )
            )
        # Aggregate rows without memory fields must not hide iteration metrics.
        for j in (json1, json2):
            j["benchmarks"].append(
                self._make_aggregate("BM_Kernel", "mean", 100_000_000)
            )

        lines = print_benchstat_report(
            json1, json2, utest=True, utest_alpha=0.05, use_color=False
        )
        joined = "\n".join(lines)
        self.assertEqual(
            self._table_titles(lines),
            [
                "cpu/op",
                "time/op",
                "INSTRUCTIONS",
                "CYCLES",
                "allocs/op",
                "peak-mem(B)",
            ],
        )
        self.assertNotIn("BM_Kernel_mean", joined)
        # Non-significant real_time change renders '~'
        self.assertIn("~ (p=", joined)
        # Significant allocs doubling (+99.64%) and memory halving (-50.00%)
        self.assertIn("15.05K ±", joined)
        self.assertIn("30.05K ±", joined)
        self.assertIn("+99.64%", joined)
        self.assertIn("1.00G ± 0%", joined)
        self.assertIn("500.00M ± 0%", joined)
        self.assertIn("-50.00%", joined)
        self.assertEqual(joined.count("geomean"), 6)

    def test_optional_tables_omitted_when_absent(self):
        json1 = {
            "benchmarks": [self._make_run("BM_TimeOnly", 0, 1000.0, 2000.0)]
        }
        json2 = {
            "benchmarks": [self._make_run("BM_TimeOnly", 0, 500.0, 1000.0)]
        }
        lines = print_benchstat_report(
            json1, json2, utest=True, use_color=False
        )
        self.assertEqual(self._table_titles(lines), ["cpu/op", "time/op"])
        self.assertIn("-50.00% (n=1)", "\n".join(lines))

    def test_table_per_user_counter_in_both_inputs(self):
        json1 = {
            "benchmarks": [
                self._make_run(
                    "BM_Counters",
                    i,
                    100.0,
                    100.0,
                    counters={"cache-misses": 50 + i, "OnlyInBaseline": 1},
                )
                for i in range(5)
            ]
        }
        json2 = {
            "benchmarks": [
                self._make_run(
                    "BM_Counters",
                    i,
                    100.0,
                    100.0,
                    counters={"OnlyInContender": 1, "cache-misses": 70 + i},
                )
                for i in range(5)
            ]
        }
        lines = print_benchstat_report(json1, json2, use_color=False)
        self.assertEqual(
            self._table_titles(lines), ["cpu/op", "time/op", "cache-misses"]
        )
        self.assertIn("52.00 ± 4%", "\n".join(lines))
        self.assertIn("72.00 ± 3%", "\n".join(lines))

    def test_aggregates_only_inputs_compare_means(self):
        def make_json(mean, stddev):
            return {
                "benchmarks": [
                    self._make_aggregate("BM_X", "mean", mean),
                    self._make_aggregate("BM_X", "median", mean),
                    self._make_aggregate("BM_X", "stddev", stddev),
                    self._make_aggregate(
                        "BM_X", "cv", stddev / mean, unit="percentage"
                    ),
                ]
            }

        lines = print_benchstat_report(
            make_json(100, 5), make_json(90, 9), use_color=False
        )
        self.assertEqual(
            [ln.split()[0] for ln in lines if ln],
            ["name", "BM_X_mean", "geomean"] * 2,
        )
        self.assertEqual("\n".join(lines).count("-10.00% (n=1)"), 2)

    def test_identical_samples_of_different_sizes(self):
        json1 = {
            "benchmarks": [
                self._make_run("BM_Same", i, 100.0, 100.0) for i in range(3)
            ]
        }
        json2 = {
            "benchmarks": [
                self._make_run("BM_Same", i, 100.0, 100.0) for i in range(4)
            ]
        }

        def count_insignificant():
            lines = print_benchstat_report(json1, json2, use_color=False)
            return "\n".join(lines).count("~ (p=1.000 n=3+4)")

        self.assertEqual(count_insignificant(), 2)
        # Depending on its version, scipy gives a NaN p-value for such samples.
        with unittest.mock.patch(f"{__name__}.mannwhitneyu") as utest_mock:
            utest_mock.return_value.pvalue = float("nan")
            self.assertEqual(count_insignificant(), 2)


if __name__ == "__main__":
    unittest.main()

# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
# kate: tab-width: 4; replace-tabs on; indent-width 4; tab-indents: off;
# kate: indent-mode python; remove-trailing-spaces modified;
