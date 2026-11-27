# Benchmark

[![build-and-test](https://github.com/google/benchmark/workflows/build-and-test/badge.svg)](https://github.com/google/benchmark/actions?query=workflow%3Abuild-and-test)
[![bazel](https://github.com/google/benchmark/actions/workflows/bazel.yml/badge.svg)](https://github.com/google/benchmark/actions/workflows/bazel.yml)
[![test-bindings](https://github.com/google/benchmark/workflows/test-bindings/badge.svg)](https://github.com/google/benchmark/actions?query=workflow%3Atest-bindings)
[![Coverage Status](https://coveralls.io/repos/google/benchmark/badge.svg)](https://coveralls.io/r/google/benchmark)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/google/benchmark/badge)](https://securityscorecards.dev/viewer/?uri=github.com/google/benchmark)

[![Discord](https://discordapp.com/api/guilds/1125694995928719494/widget.png?style=shield)](https://discord.gg/cz7UX7wKC2)

A library to benchmark code snippets, similar to unit tests. Example:

```c++
#include <benchmark/registration.h>
#include <benchmark/state.h>

static void BM_SomeFunction(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    SomeFunction();
  }
}
// Register the function as a benchmark
BENCHMARK(BM_SomeFunction);
// Run the benchmark
BENCHMARK_MAIN();
```

## Getting Started

To get started, see [Requirements](#requirements) and
[Installation](#installation). See [Usage](#usage) for a full example and the
[User Guide](docs/user_guide.md) for a more comprehensive feature overview.

It may also help to read the [Google Test documentation](https://github.com/google/googletest/blob/main/docs/primer.md)
as some of the structural aspects of the APIs are similar.

## Resources

[Discussion group](https://groups.google.com/d/forum/benchmark-discuss)

IRC channels:
* [libera](https://libera.chat) #benchmark

[Additional Tooling Documentation](docs/tools.md)

[Assembly Testing Documentation](docs/AssemblyTests.md)

[Building and installing Python bindings](docs/python_bindings.md)

## Requirements

The library can be used with C++11. However, it requires C++17 to build,
including compiler and standard library support.

_See [dependencies.md](docs/dependencies.md) for more details regarding supported
compilers and standards._

If you have need for a particular compiler to be supported, patches are very welcome.

See [Platform-Specific Build Instructions](docs/platform_specific_build_instructions.md).

## Installation

This describes the installation process using cmake. As pre-requisites, you'll
need git and cmake installed.

_See [dependencies.md](docs/dependencies.md) for more details regarding supported
versions of build tools._

```bash
# Check out the library.
$ git clone https://github.com/google/benchmark.git
# Go to the library root directory
$ cd benchmark
# Make a build directory to place the build output.
$ cmake -E make_directory "build"
# Generate build system files with cmake, and download any dependencies.
$ cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release -S . -B "build"
# Build the library.
$ cmake --build "build" --config Release
```
This builds the `benchmark` and `benchmark_main` libraries and tests.
On a unix system, the build directory should now look something like this:

```
/benchmark
  /build
    /src
      /libbenchmark.a
      /libbenchmark_main.a
    /test
      ...
```

Next, you can run the tests to check the build.

```bash
$ cmake -E chdir "build" ctest --build-config Release
```

If you want to install the library globally, also run:

```
sudo cmake --build "build" --config Release --target install
```

Note that Google Benchmark requires Google Test to build and run the tests. This
dependency can be provided two ways:

* Checkout the Google Test sources into `benchmark/googletest`.
* Otherwise, if `-DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON` is specified during
  configuration as above, the library will automatically download and build
  any required dependencies.

If you do not wish to build and run the tests, add `-DBENCHMARK_ENABLE_GTEST_TESTS=OFF`
to `CMAKE_ARGS`.

### Debug vs Release

By default, benchmark builds as a debug library. You will see a warning in the
output when this is the case. To build it as a release library instead, add
`-DCMAKE_BUILD_TYPE=Release` when generating the build system files, as shown
above. The use of `--config Release` in build commands is needed to properly
support multi-configuration tools (like Visual Studio for example) and can be
skipped for other build systems (like Makefile).

To enable link-time optimisation, also add `-DBENCHMARK_ENABLE_LTO=true` when
generating the build system files.

If you are using gcc, you might need to set `GCC_AR` and `GCC_RANLIB` cmake
cache variables, if autodetection fails.

If you are using clang, you may need to set `LLVMAR_EXECUTABLE`,
`LLVMNM_EXECUTABLE` and `LLVMRANLIB_EXECUTABLE` cmake cache variables.

To enable sanitizer checks (eg., `asan` and `tsan`), add:
```
 -DCMAKE_C_FLAGS="-g -O2 -fno-omit-frame-pointer -fsanitize=address -fsanitize=thread -fno-sanitize-recover=all"
 -DCMAKE_CXX_FLAGS="-g -O2 -fno-omit-frame-pointer -fsanitize=address -fsanitize=thread -fno-sanitize-recover=all "  
```

### Stable and Experimental Library Versions

The main branch contains the latest stable version of the benchmarking library;
the API of which can be considered largely stable, with source breaking changes
being made only upon the release of a new major version.

Newer, experimental, features are implemented and tested on the
[`v2` branch](https://github.com/google/benchmark/tree/v2). Users who wish
to use, test, and provide feedback on the new features are encouraged to try
this branch. However, this branch provides no stability guarantees and reserves
the right to change and break the API at any time.

## Usage

### Basic usage

Define a function that executes the code to measure, register it as a benchmark
function using the `BENCHMARK` macro, and ensure an appropriate `main` function
is available:

```c++
#include <benchmark/benchmark.h>

static void BM_StringCreation(benchmark::State& state) {
  for (auto _ : state)
    std::string empty_string;
}
// Register the function as a benchmark
BENCHMARK(BM_StringCreation);

// Define another benchmark
static void BM_StringCopy(benchmark::State& state) {
  std::string x = "hello";
  for (auto _ : state)
    std::string copy(x);
}
BENCHMARK(BM_StringCopy);

BENCHMARK_MAIN();
```

To run the benchmark, compile and link against the `benchmark` library
(libbenchmark.a/.so). If you followed the build steps above, this library will 
be under the build directory you created.

```bash
# Example on linux after running the build steps above. Assumes the
# `benchmark` and `build` directories are under the current directory.
$ g++ mybenchmark.cc -std=c++11 -isystem benchmark/include \
  -Lbenchmark/build/src -lbenchmark -lpthread -o mybenchmark
```

Alternatively, link against the `benchmark_main` library and remove
`BENCHMARK_MAIN();` above to get the same behavior.

The compiled executable will run all benchmarks by default. Pass the `--help`
flag for option information or see the [User Guide](docs/user_guide.md).

### Usage with CMake

If using CMake, it is recommended to link against the project-provided
`benchmark::benchmark` or `benchmark::benchmark_main` targets using
`target_link_libraries`. Link to `benchmark::benchmark` when your target
defines its own `main` function, or link to `benchmark::benchmark_main` to use
the default benchmark entry point. The `benchmark::benchmark_main` target links
`benchmark::benchmark` transitively.
It is possible to use ```find_package``` to import an installed version of the
library.
```cmake
find_package(benchmark REQUIRED)
```
Alternatively, ```add_subdirectory``` will incorporate the library directly in
to one's CMake project.
```cmake
add_subdirectory(benchmark)
```
Either way, link to the library as follows.
```cmake
target_link_libraries(MyTarget benchmark::benchmark)
# Or, when you do not define your own main:
target_link_libraries(MyTarget benchmark::benchmark_main)
```
When benchmark sources are shared through an intermediate CMake target, choose
an object library instead of a static library:

```cmake
add_library(shared_benchmarks OBJECT bench.cc)
target_link_libraries(shared_benchmarks benchmark::benchmark_main)
add_executable(runnable_benchmarks)
target_link_libraries(runnable_benchmarks shared_benchmarks)
```

This links the object file that contains `BENCHMARK` registrations into the
final executable. If those registrations are placed only in an intermediate
`STATIC` library, the linker may not copy static registration symbols, and thus
benchmarks will not be part of the final executable.

#### Embedding Google Benchmark in another CMake project

There are two common ways to consume Google Benchmark from a CMake project:

* Use an installed or package-managed copy, for example from a system package
  manager or vcpkg, and import it with `find_package(benchmark REQUIRED)`.
* Add this repository to the source tree, for example as a submodule or
  `FetchContent` dependency, and call `add_subdirectory`.

The installed form keeps Google Benchmark's build separate from the parent
project and is usually the simplest choice for system packages and vcpkg. The
source-tree form is useful when the parent project wants to pin a specific
commit or build Google Benchmark as part of its normal CMake configure step.

When embedding from source, most projects should turn off Google Benchmark's
tests and install rules:

```cmake
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/benchmark)
target_link_libraries(MyTarget benchmark::benchmark)
```

If Google Test is not already provided by the parent build, either check out the
Google Test sources under `benchmark/googletest` or configure with
`BENCHMARK_DOWNLOAD_DEPENDENCIES=ON`. For projects that only link the benchmark
library and do not build Google Benchmark's tests, disabling
`BENCHMARK_ENABLE_GTEST_TESTS` avoids the Google Test dependency.

Google Benchmark follows CMake's `BUILD_SHARED_LIBS` setting when selecting
static or shared library output. On Windows, keep this setting consistent with
the rest of the project and make sure the same runtime library configuration is
used across the benchmark library and the targets that link it.

### Usage with Bazel

If using Bazel with Bzlmod, add Google Benchmark to your `MODULE.bazel` file:

```starlark
bazel_dep(name = "google_benchmark", version = "<VERSION>")
```

Replace `<VERSION>` with the Google Benchmark release version you want to use.

Then link a `cc_binary` or `cc_test` against one of the provided targets:

```starlark
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "my_benchmark",
    srcs = ["my_benchmark.cc"],
    deps = ["@google_benchmark//:benchmark_main"],
)
```

Use `@google_benchmark//:benchmark` when your target defines its own `main`
function, including through `BENCHMARK_MAIN()`. Use
`@google_benchmark//:benchmark_main` to use the default Google Benchmark entry
point.

For WORKSPACE setup and more examples, see [Bazel](docs/bazel.md).
