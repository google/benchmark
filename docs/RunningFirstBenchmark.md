# Running Your First Benchmark

This tutorial is aimed at people new to the library and shows you how to run benchmarks with the Google Benchmark library.

# Install the prerequisites

We need the following programs:

* git : to clone google benchmark and gtest libraries
* cmake : to build google benchmark
* make : to build google benchmark. Advanced users can use an alternative cmake backend like ninja.
* g++ : to compile your c++ code. Advanced users can use other compilers like clang instead.

On Fedora 28, the following command installs the requirements:

```
sudo dnf install --assumeyes wget git-core cmake make gcc-c++
```

At the time of writing, the versions were as follows:
* g++ : 8.1.1 20180502
* cmake : 3.11.2
* gnu make: 4.2.1
* git: 2.17.1

# Build and install Google Benchmark

Follow the instructions elsewhere to build and install the google-benchmark library.

On Fedora 28, these commands were sufficient:

```
git clone https://github.com/google/benchmark.git
cd benchmark
git clone https://github.com/google/googletest.git
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
sudo make install
```

`make install` should result in lines that look like:

```
-- Install configuration: "RELEASE"
-- Installing: /usr/local/lib/libbenchmark.a
-- Installing: /usr/local/lib/libbenchmark_main.a
-- Installing: /usr/local/include/benchmark
-- Installing: /usr/local/include/benchmark/benchmark.h
-- Installing: /usr/local/lib/cmake/benchmark/benchmarkConfig.cmake
-- Installing: /usr/local/lib/cmake/benchmark/benchmarkConfigVersion.cmake
-- Installing: /usr/local/lib/pkgconfig/benchmark.pc
-- Installing: /usr/local/lib/cmake/benchmark/benchmarkTargets.cmake
-- Installing: /usr/local/lib/cmake/benchmark/benchmarkTargets-release.cmake
```

Note the locations of the header file (`benchmark.h`) and the static library (`libbenchmark.a`); you might have to tell your compiler to look for libraries in those paths. Things worked out of the box in Fedora 28.


# Build your first benchmark

Save this file as `BenchmarkMain.cpp`:

```
#include <benchmark/benchmark.h>
#include <string>

static void BM_StringCopy(benchmark::State& state) {
    std::string x = "hello";
    for (auto _ : state)
        std::string copy(x);
}

BENCHMARK(BM_StringCopy);
BENCHMARK_MAIN();
```

Use this command to build the library:

```
g++ BenchMain.cpp -std=c++11 -lbenchmark -lpthread -O2 -o BenchMain
```

Notes:
* We use `-std=c++11` because the code sample uses range based for. You can specify `c++14` or `c++17` instead.
* We use `-lbenchmark` and `-lpthread` to use the benchmark library in our code. It is important to have `BenchMain.cpp` specified before you specify the `-lbenchmark` and `-lpthread` flags; otherwise you will get very confusing `undefined reference` type errors when building.
* We use `-O2` to compile the code with optimizations enabled. You want the code being benchmarked to be close to the code you will use in production, so you should specify any flags, including optimization flags that you compile your code with.
* Finally, we use `-o BenchMain` to tell the compiler to build an executable called `BenchMain`

Use this to run the created benchmark:

```
./BenchMain
```

This will produce an output that looks like:

```
2018-06-19 11:08:35
Running ./BenchMain
Run on (4 X 3000 MHz CPU s)
CPU Caches:
  L1 Data 32K (x2)
  L1 Instruction 32K (x2)
  L2 Unified 256K (x2)
  L3 Unified 3072K (x1)
-----------------------------------------------------
Benchmark              Time           CPU Iterations
-----------------------------------------------------
BM_StringCopy          5 ns          5 ns  127850761
```

Congratulations. You have just run your first benchmark.
