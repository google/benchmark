# Bazel

Google Benchmark provides Bazel targets for both the benchmark library and the
optional default `main` function:

* `@google_benchmark//:benchmark` provides the benchmark library.
* `@google_benchmark//:benchmark_main` provides the default `main` function and
  depends on `@google_benchmark//:benchmark`.

Use `@google_benchmark//:benchmark` when the benchmark target defines its own
`main` function, including through `BENCHMARK_MAIN()`. Use
`@google_benchmark//:benchmark_main` when the benchmark target should use the
default Google Benchmark entry point.

## Bzlmod

With Bzlmod enabled, add Google Benchmark to your `MODULE.bazel` file:

```starlark
bazel_dep(name = "google_benchmark", version = "<VERSION>")
```

Replace `<VERSION>` with the Google Benchmark release version you want to use.

Then depend on the Bazel target from a `cc_binary` or `cc_test`:

```starlark
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "string_benchmark",
    srcs = ["string_benchmark.cc"],
    deps = ["@google_benchmark//:benchmark_main"],
)
```

The source file should register benchmarks, but it should not call
`BENCHMARK_MAIN()` when linking against `@google_benchmark//:benchmark_main`:

```c++
#include <benchmark/benchmark.h>
#include <string>

static void BM_StringCreation(benchmark::State& state) {
  for (auto _ : state) {
    std::string empty_string;
  }
}
BENCHMARK(BM_StringCreation);
```

Run the benchmark with Bazel:

```bash
bazel run //:string_benchmark
```

Pass Google Benchmark flags after Bazel's `--` separator:

```bash
bazel run //:string_benchmark -- --benchmark_filter=StringCreation
```

## WORKSPACE

Projects that still use `WORKSPACE` can declare Google Benchmark as an external
repository and load its dependencies from `bazel/benchmark_deps.bzl`:

```starlark
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "google_benchmark",
    strip_prefix = "benchmark-<VERSION>",
    urls = ["https://github.com/google/benchmark/archive/refs/tags/v<VERSION>.tar.gz"],
    # Add sha256 for reproducible builds.
)

load("@google_benchmark//:bazel/benchmark_deps.bzl", "benchmark_deps")

benchmark_deps()
```

Use the same `<VERSION>` value without the leading `v`; the archive URL adds the tag prefix explicitly.

After declaring the repository, use the same target labels shown above:
`@google_benchmark//:benchmark` or `@google_benchmark//:benchmark_main`.

## Perf Counters

When using Bazel, enable libpfm support by adding `--define pfm=1` to the build
or run command. See [Perf Counters](perf_counters.md) for more details.
