# Building Benchmark

To build Benchmark we need to follow a series of simple steps:
*   Obtain Benchmark
*   Configure Benchmark
*   Build Benchmark

To obtain Benchmark we can simply clone the Github repository using
```bash
$ git clone https://github.com/google/benchmark.git
```
Now, Benchmark uses Google Test to run it's tests. So, if we wish to build and run
the tests we need to include googletest, which can be done either by cloning the googletest
repository or by asking benchmark in the configuration step to download this dependency.
We can clone the googletest repository in benchmark using
```bash
$ git clone https://github.com/google/googletest.git benchmark/googletest
```
To configure Benchmark we use CMake as follows:
```bash
$ cmake -G <generator> [options] -S benchmark -B build
```
If we haven't cloned the googletest repository as described above we can ask benchmark to download this dependency in the above configuration step by adding `-DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON`

Then to build Benchmark we use CMake again as follows:
```bash
$ cmake -build build
```
If we don't want the Benchmark tests to build and run we can add `-DBENCHMARK_ENABLE_GTEST_TESTS=OFF` in the configuration step.

## Using Benchmark

If we want to use Benchmark with a particular file, say `BenchmarkEverything.cpp`,
we can build Benchmark and use the following comands in the terminal.

### In Windows
```bash
> cl /EHsc /MD BenchmarkEverything.cpp /I<BENCHMARK_SOURCE_DIR>\include /link shlwapi.lib <BENCHMARK_BUILD_DIR>\src\Release\benchmark.lib
```
where `<BENCHMARK_SOURCE_DIR>` is the directory where Benchmark was cloned into and `<BENCHMARK_BUILD_DIR>` the directory where Benchmark was build. Here, we had build Benchmark in the Release configuration and hence we 
used `<BENCHMARK_BUILD_DIR>\src\Release\benchmark.lib`. We also used the `cl` compiler which is provided with Visual Studio and also linked the `shlwapi` library.

### In Linux
```bash
$ g++ -std=c++11 BenchmarkEverything.cpp -I<BENCHMARK_SOURCE_DIR>/include -pthread -L <BENCHMARK_BUILD_DIR>\src -lbenchmark
```
where `<BENCHMARK_SOURCE_DIR>` is the directory where Benchmark was cloned into and `<BENCHMARK_BUILD_DIR>` the directory where Benchmark was build. We also need to link the `pthread` library.

# Installing Benchmark

After building Benchmark we can choose to install it in our systems. 

For Ubuntu and Debian based systems we have to build Benchmark in the Release configuration by applying `-DCMAKE_BUILD_TYPE=RELEASE` in the configuration step. Then install it using `sudo make install` inside the build directory.

We could also use Microsoft's C++ Library Manager [`vcpkg`](https://github.com/Microsoft/vcpkg) to install Benchmark in our systems. This manager works in Windows, Linux and Mac OS and it is very easy to manage libraries with it. To install Benchmark we simply need to run the following commnd, provided we already have `vcpkg` installed in our system with user-wide integration. (NOTE: documentation for installation of `vcpkg` can be found [here](https://github.com/Microsoft/vcpkg))

For Windows
```bash
PS> .\vcpkg install benchmark
```
For Linux
```bash
$ ./vcpkg install benchmark
```

# Embedding Benchmark in other projects
 
Benchmark can easily be incuded in other projects. There are a few ways by which this can be done,
*   Clone the repository and build it inside the project and then use it in the project.
*   Add Benchmark as a git submodule and incorporate it as a part of the project.
*   Use CMake to download and build Benchmark as a part of the project and use it in the project.

For the first method we can follow the usual steps to build and use Benchmark.
The second and the third methods are described below


## Adding Benchmark as a git submodule

We can add Benchmark as a submodule in our CMake project, build it and use it in the project.
To demonstarte this method we consider a simple project `BenchmarkEverything` whose 
directory structure is as follows:

```bash
BenchmarkEverything
|   CMakeLists.txt
|   BenchmarkEverything.cpp
|   .git
```

We add Benchmark as a submodule to our git repository using
```bash
$ git submodule add http://github.com/google/benchmark.git extproject/benchmark
```
This will add Benchmark as a submodule in `/extproject/benchmark/`. In our 
`CMakeLists.txt` file we incorporate this submodule into our project using `add_subdirectory()`
```cmake
# setting minimum cmake version
cmake_minimum_required (VERSION 3.14)
# setting cxx standard
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
# project name
project (BenchmarkEverything)
# dissabling tests for benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
add_subdirectory(extprojects/benchmark)
# including benchmark header files
include_directories(extprojects/benchmark/include)
add_executable(BenchmarkEverything BenchmarkEverything.cpp)
target_link_libraries(BenchmarkEverything benchmark)
if(WIN32)
    target_link_libraries(BenchmarkEverything shlwapi)
elseif(LINUX)
    target_link_libraries(BenchmarkEverything pthread)
elseif(SOLARIS)
    target_link_libraries(BenchmarkEverything kstat)
endif()
```
Here we have disabled the build and run of tests for Benchmark by setting `BENCHMARK_ENABLE_TESTING` to `OFF`.
If we wish to build and run the tests we can either include googletest as a submodule or set `BENCHMARK_DOWNLOAD_DEPENDENCIES` to `ON`. In both cases we need to remove the line setting `BENCHMARK_ENABLE_TESTING` to `OFF`.

In the file `BenchmarkEverything.cpp` we add
```c++
// An simple example from https://github.com/google/benchmark/blob/master/README.md
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

We can then then build `BenchmarkEverything` from `BenchmarkEverything/` using CMake as follows:
```shell
$ cmake -S . -B <BUILD_DIR> -G <GENERATOR>
$ cmake --build <BUILD_DIR> --config <CONFIGURATION>
```
where `<BUILD_DIR>` is the directory where you would like to build `BenchmarkEverything`,
`<GENERATOR>` is the generator that you would like to use and `<CONFIGURATION>` the configuration that
you would like to build, like `Release` or `Debug`. To run `BenchmarkEverything` we just need to run 
the executable generated inside `<BUILD_DIR>`.

## Use CMake for embedding Benchmark

The Benchmark library can easily be embedded in other projects using CMake such that the project downloads Benchmark during its configuration(using CMake) and then adds it to the project using `add_subdirectory()`. This
method is similar to the method used to include googletest into an existing CMake project, [See Here](https://github.com/google/googletest/blob/master/googletest/README.md).

The construction of a simple project demonstrating such an embedding is described below.

We begin with a simple project `BenchmarkEverything` whose directory structure is as follows:

```shell
BenchmarkEverything
|   README.md
|___BenchmarkEverything
    |   CMakeLists.txt
    |   CMakeLists.txt.in
    |   BenchmarkIt.cpp
```

In the file `CMakeLists.txt` we have 

```cmake
# setting minimum version of cmake
cmake_minimum_required (VERSION 3.14)
# setting cxx standard
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
# project name
project (BenchmarkEverything)
# include the configurations from CMakeLists.txt.in
configure_file(CMakeLists.txt.in googlebenchmark-download/CMakeLists.txt)
# executing the configuration step
execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . 
    RESULT_VARIABLE results
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-download
)
# checking if the configuration step passed
if(results)
    message(FATAL_ERROR "Configuration step for Benchmark has Failed. ${results}")
endif()
# executing the build step
execute_process(
    COMMAND ${CMAKE_COMMAND} --build . --config Release
    RESULT_VARIABLE results
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-download
)
# checking if the build step passed
if(results)
    message(FATAL_ERROR "Build step for Benchmark has Failed. ${results}")
endif()
add_subdirectory(
    ${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-src
    ${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-build
    EXCLUDE_FROM_ALL
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-src/include)
add_executable(BenchmarkEverything BenchmarkIt.cpp)
target_link_libraries(BenchmarkEverything benchmark)
if (WIN32)
    target_link_libraries(BenchmarkEverything shlwapi)
elseif(LINUX)
    target_link_libraries(BenchmarkEverything pthread)
elseif(SOLARIS)
    target_link_libraries(BenchmarkEverything kstat)
endif()
```

Then in the `CMakeLists.txt.in` file we have

```cmake
# setting cmake minimum version
cmake_minimum_required(VERSION 3.14)
# setting a project name
project(googlebenchmark-download NONE)
# adding benchmark as an external project
include(ExternalProject)
ExternalProject_Add(googlebenchmark 
  GIT_REPOSITORY    https://github.com/google/benchmark.git
  GIT_TAG           master
  SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-src"
  BINARY_DIR        "${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-build"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      "" 
)
# adding googletest similarly
include(ExternalProject)
ExternalProject_Add(googletest DEPENDS googlebenchmark
  GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           master
  SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-src/googletest"
  BINARY_DIR        "${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-build/googletest"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      "" 
)
```

The `CMakeLists.txt` file above downloads both benchmark and googletest, configures and builds them during the build step and adds their directories into the project using `add_subdirectory()`. 

Here we manually add the download configuration for googletest. We can avoid that and ask Benchmark to automatically download this dependency. To do so we need to set `BENCHMARK_DOWNLOAD_DEPENDENCIES` to `ON` in the file `CMakeLists.txt`. If we do not want to run and build the tests we can add `BENCHMARK_ENABLE_TESTING` to `OFF` in `CMakeLists.txt`. In both cases we should remove the `ExternalProject` for googletest from the file `CMakeLists.txt.in`. Setting `BENCHMARK_DOWNLOAD_DEPENDENCIES` to `ON` could be done by simply adding the following line in the `CMakeLists.txt` file.
```cmake
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "Download Benchmark Dependencies" FORCE)
```
For setting `BENCHMARK_ENABLE_TESTING` to `OFF` a similar line could be added.

Now for the file `BenchmarkIt.cpp` we have

```c++
// An simple example from https://github.com/google/benchmark/blob/master/README.md
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

We can then run the following commandsfrom the parent `BenchmarkEverything/` directory to build `BenchmarkEverything`

```shell
$ cmake -S BenchmarkEverything -B <BUILD_DIR> -G <GENERATOR>
$ cmake --build <BUILD_DIR> --config <CONFIGURATION>
```
where `<BUILD_DIR>` is the directory where you would like to build `BenchmarkEverything`,
`<GENERATOR>` is the generator that you would like to use and `<CONFIGURATION>` the configuration that
you would like to build, like `Release` or `Debug`. To run `BenchmarkEverything` we just need to run 
the executable generated inside `<BUILD_DIR>`.