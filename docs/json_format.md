# JSON Output Format

When you pass `--benchmark_format=json` (or `--benchmark_out_format=json` to
write JSON to a file while keeping console output), the library emits a single
JSON object with two top-level keys: **`context`** and **`benchmarks`**.

```
{
  "context": { ... },
  "benchmarks": [ ... ]
}
```

The current `json_schema_version` is **1**. The schema version is not tied to
the library version — it only bumps when the structure changes in a
backwards-incompatible way. New fields may be added at any time, so consumers
should ignore keys they don't recognize.

## Context Object

The `context` object describes the environment the benchmarks ran in.

| Field | Type | Description |
|---|---|---|
| `date` | string | Wall-clock timestamp when the run started, e.g. `"2015/03/17-18:40:25"`. |
| `host_name` | string | Machine hostname. |
| `executable` | string | Path to the benchmark binary. May be absent on some platforms. |
| `num_cpus` | integer | Number of logical CPUs detected. |
| `mhz_per_cpu` | integer | CPU frequency in MHz (rounded). |
| `cpu_scaling_enabled` | boolean | Whether CPU frequency scaling is on. Omitted if the library can't determine it (common on non-Linux). |
| `aslr_enabled` | boolean | Whether address-space layout randomization is on. Omitted when unknown. |
| `caches` | array | CPU cache hierarchy. Each entry is an object (see below). |
| `load_avg` | array | System load averages. Typically 3 floats on Linux/macOS, empty on Windows. |
| `library_version` | string | Benchmark library version, e.g. `"v1.9.1"`. |
| `library_build_type` | string | `"release"` or `"debug"`, depending on whether `NDEBUG` was defined at build time. |
| `json_schema_version` | integer | Currently `1`. |

Any custom context added via `benchmark::AddCustomContext("key", "value")` or
`--benchmark_context=key=value` shows up as extra string fields in this object.

### Cache entries

Each element in the `caches` array looks like:

| Field | Type | Description |
|---|---|---|
| `type` | string | Cache type, e.g. `"Data"`, `"Instruction"`, or `"Unified"`. |
| `level` | integer | Cache level (1, 2, 3, …). |
| `size` | integer | Cache size in bytes. |
| `num_sharing` | integer | Number of logical CPUs sharing this cache. |

## Benchmark Results

The `benchmarks` array contains one object per benchmark run. A single
registered benchmark can produce multiple objects if it uses repetitions (each
iteration run + the aggregate rows).

### Fields present on every result

| Field | Type | Description |
|---|---|---|
| `name` | string | Full display name of this result, including any suffixes like `_mean` or `/repeats:3`. |
| `family_index` | integer | Index of the benchmark family in registration order. |
| `per_family_instance_index` | integer | Index of this particular argument combination within its family. |
| `run_name` | string | Base benchmark name without aggregate suffixes. |
| `run_type` | string | Either `"iteration"` (a single measured run) or `"aggregate"` (a computed statistic across repetitions). |
| `repetitions` | integer | Total number of repetitions configured for this benchmark. |
| `threads` | integer | Number of threads the benchmark was run with. |

### Iteration-only fields

These appear when `run_type` is `"iteration"`:

| Field | Type | Description |
|---|---|---|
| `repetition_index` | integer | Zero-based index of this repetition. |

### Aggregate-only fields

These appear when `run_type` is `"aggregate"`:

| Field | Type | Description |
|---|---|---|
| `aggregate_name` | string | Name of the statistic: `"mean"`, `"median"`, `"stddev"`, `"cv"`, or a custom statistic name. |
| `aggregate_unit` | string | `"time"` if the aggregate is in the same time unit as the benchmark, or `"percentage"` for relative measures like CV. |

### Timing and iteration metrics

For normal (non-Big-O, non-RMS) results:

| Field | Type | Description |
|---|---|---|
| `iterations` | integer | Number of iterations run. |
| `real_time` | number | Wall-clock time per iteration, in `time_unit`. |
| `cpu_time` | number | CPU time per iteration, in `time_unit`. |
| `time_unit` | string | One of `"ns"`, `"us"`, `"ms"`, or `"s"`. |

### Asymptotic complexity fields

When a benchmark reports Big-O complexity, the result object contains these
instead of the normal timing fields:

| Field | Type | Description |
|---|---|---|
| `cpu_coefficient` | number | Fitted coefficient for CPU time. |
| `real_coefficient` | number | Fitted coefficient for wall-clock time. |
| `big_o` | string | Complexity string, e.g. `"N"`, `"NlgN"`, `"N^2"`, etc. |
| `time_unit` | string | Time unit used for the coefficients. |

When the RMS error row is reported:

| Field | Type | Description |
|---|---|---|
| `rms` | number | Root-mean-square relative error of the fit. |

### Error and skip fields

If a benchmark calls `state.SkipWithError(msg)`:

| Field | Type | Description |
|---|---|---|
| `error_occurred` | boolean | `true`. |
| `error_message` | string | The error message passed by the benchmark. |

If a benchmark calls `state.SkipWithMessage(msg)`:

| Field | Type | Description |
|---|---|---|
| `skipped` | boolean | `true`. |
| `skip_message` | string | The skip reason. |

### Memory fields

If a `MemoryManager` is registered, results include:

| Field | Type | Description |
|---|---|---|
| `allocs_per_iter` | number | Heap allocations per iteration. |
| `max_bytes_used` | integer | Peak heap usage in bytes. |
| `total_allocated_bytes` | integer | Total bytes allocated. Present only if the memory manager provides it. |
| `net_heap_growth` | integer | Net change in heap size. Present only if the memory manager provides it. |

### User counters

Any counters set via `state.counters["name"] = value` are emitted as additional
numeric fields directly on the result object. This includes the built-in
convenience counters like `bytes_per_second` and `items_per_second` (set through
`state.SetBytesProcessed()` and `state.SetItemsProcessed()`).

User-requested performance counters (see [perf_counters.md](perf_counters.md))
also appear the same way.

### Label

| Field | Type | Description |
|---|---|---|
| `label` | string | Custom label set via `state.SetLabel()`. Only present if a label was set. |

## Special floating-point values

Doubles are printed in scientific notation with full precision. The two
non-finite cases use unquoted tokens that aren't part of the JSON spec — this
matches the library's existing behavior:

* **NaN** → `NaN` (or `-NaN`)
* **Infinity** → `Infinity` (or `-Infinity`)

Consumers that need strict JSON compliance should handle these before parsing.

## List mode

When using `--benchmark_list_tests` with JSON format, the output is a simpler
object containing just the benchmark names:

```json
{
  "benchmarks": [
    { "name": "BM_Foo" },
    { "name": "BM_Bar/1/threads:4" }
  ]
}
```

No `context` is emitted in list mode.

## Full example

Here's a realistic output showing iteration results, aggregate statistics, user
counters, and a label:

```json
{
  "context": {
    "date": "2025-06-15T14:23:07+00:00",
    "host_name": "build-server",
    "executable": "./my_benchmarks",
    "num_cpus": 8,
    "mhz_per_cpu": 3200,
    "cpu_scaling_enabled": false,
    "caches": [
      {
        "type": "Data",
        "level": 1,
        "size": 32768,
        "num_sharing": 2
      },
      {
        "type": "Instruction",
        "level": 1,
        "size": 32768,
        "num_sharing": 2
      },
      {
        "type": "Unified",
        "level": 2,
        "size": 262144,
        "num_sharing": 2
      }
    ],
    "load_avg": [1.23, 0.98, 0.87],
    "library_version": "v1.9.1",
    "library_build_type": "release",
    "json_schema_version": 1
  },
  "benchmarks": [
    {
      "name": "BM_StringCopy/64/repeats:3",
      "family_index": 0,
      "per_family_instance_index": 0,
      "run_name": "BM_StringCopy/64/repeats:3",
      "run_type": "iteration",
      "repetitions": 3,
      "repetition_index": 0,
      "threads": 1,
      "iterations": 12451993,
      "real_time": 5.6214141412345678e+01,
      "cpu_time": 5.5892100000000000e+01,
      "time_unit": "ns",
      "bytes_per_second": 1.1448035200000000e+09,
      "items_per_second": 1.7890680000000000e+07
    },
    {
      "name": "BM_StringCopy/64/repeats:3",
      "family_index": 0,
      "per_family_instance_index": 0,
      "run_name": "BM_StringCopy/64/repeats:3",
      "run_type": "iteration",
      "repetitions": 3,
      "repetition_index": 1,
      "threads": 1,
      "iterations": 12451993,
      "real_time": 5.5987623400000003e+01,
      "cpu_time": 5.5701200000000000e+01,
      "time_unit": "ns",
      "bytes_per_second": 1.1487123400000000e+09,
      "items_per_second": 1.7951750000000000e+07
    },
    {
      "name": "BM_StringCopy/64/repeats:3",
      "family_index": 0,
      "per_family_instance_index": 0,
      "run_name": "BM_StringCopy/64/repeats:3",
      "run_type": "iteration",
      "repetitions": 3,
      "repetition_index": 2,
      "threads": 1,
      "iterations": 12451993,
      "real_time": 5.6102456700000001e+01,
      "cpu_time": 5.5800000000000004e+01,
      "time_unit": "ns",
      "bytes_per_second": 1.1465892300000000e+09,
      "items_per_second": 1.7918530000000000e+07
    },
    {
      "name": "BM_StringCopy/64/repeats:3_mean",
      "family_index": 0,
      "per_family_instance_index": 0,
      "run_name": "BM_StringCopy/64/repeats:3",
      "run_type": "aggregate",
      "repetitions": 3,
      "threads": 1,
      "aggregate_name": "mean",
      "aggregate_unit": "time",
      "iterations": 3,
      "real_time": 5.6101407170781896e+01,
      "cpu_time": 5.5797766666666668e+01,
      "time_unit": "ns",
      "bytes_per_second": 1.1467016966666666e+09,
      "items_per_second": 1.7920320000000000e+07
    },
    {
      "name": "BM_StringCopy/64/repeats:3_median",
      "family_index": 0,
      "per_family_instance_index": 0,
      "run_name": "BM_StringCopy/64/repeats:3",
      "run_type": "aggregate",
      "repetitions": 3,
      "threads": 1,
      "aggregate_name": "median",
      "aggregate_unit": "time",
      "iterations": 3,
      "real_time": 5.6102456700000001e+01,
      "cpu_time": 5.5800000000000004e+01,
      "time_unit": "ns",
      "bytes_per_second": 1.1465892300000000e+09,
      "items_per_second": 1.7918530000000000e+07
    },
    {
      "name": "BM_StringCopy/64/repeats:3_stddev",
      "family_index": 0,
      "per_family_instance_index": 0,
      "run_name": "BM_StringCopy/64/repeats:3",
      "run_type": "aggregate",
      "repetitions": 3,
      "threads": 1,
      "aggregate_name": "stddev",
      "aggregate_unit": "time",
      "iterations": 3,
      "real_time": 1.1345678901234568e-01,
      "cpu_time": 9.5700000000000000e-02,
      "time_unit": "ns",
      "bytes_per_second": 1.9567800000000000e+06,
      "items_per_second": 3.3612000000000000e+04
    },
    {
      "name": "BM_StringCopy/64/repeats:3_cv",
      "family_index": 0,
      "per_family_instance_index": 0,
      "run_name": "BM_StringCopy/64/repeats:3",
      "run_type": "aggregate",
      "repetitions": 3,
      "threads": 1,
      "aggregate_name": "cv",
      "aggregate_unit": "percentage",
      "iterations": 3,
      "real_time": 2.0223456789000001e-03,
      "cpu_time": 1.7152000000000001e-03,
      "time_unit": "ns"
    },
    {
      "name": "BM_Serialize",
      "family_index": 1,
      "per_family_instance_index": 0,
      "run_name": "BM_Serialize",
      "run_type": "iteration",
      "repetitions": 1,
      "repetition_index": 0,
      "threads": 1,
      "iterations": 890432,
      "real_time": 7.8912345678901236e+02,
      "cpu_time": 7.8500000000000000e+02,
      "time_unit": "ns",
      "label": "using_protobuf"
    }
  ]
}
```
