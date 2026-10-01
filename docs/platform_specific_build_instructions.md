# Platform Specific Build Instructions

## Building with GCC

When the library is built using GCC it is necessary to link with the pthread
library due to how GCC implements `std::thread`. Failing to link to pthread will
lead to runtime exceptions (unless you're using libc++), not linker errors. See
[issue #67](https://github.com/google/benchmark/issues/67) for more details. You
can link to pthread by adding `-pthread` to your linker command. Note, you can
also use `-lpthread`, but there are potential issues with ordering of command
line parameters if you use that.

On QNX, the pthread library is part of libc and usually included automatically
(see
[`pthread_create()`](https://www.qnx.com/developers/docs/7.1/index.html#com.qnx.doc.neutrino.lib_ref/topic/p/pthread_create.html)).
There's no separate pthread library to link.

## Building on Windows (Visual Studio, MinGW, Clang)

When building or linking against the static library on Windows (using Visual Studio, MinGW, or Clang), make sure to define `BENCHMARK_STATIC_DEFINE` in your preprocessor definitions / compiler flags (e.g. `-DBENCHMARK_STATIC_DEFINE` or under `[Configuration Properties > C/C++ > Preprocessor > Preprocessor Definitions]`). Without this definition, headers will default to `__declspec(dllimport)` and cause unresolved symbol errors during linking.

The `shlwapi` library (`-lshlwapi` or `Shlwapi.lib`) is required to support a call to `CPUInfo` which reads the registry. Either add `shlwapi.lib` under `[ Configuration Properties > Linker > Input ]`, add `-lshlwapi` to your linker command line, or use the following:

```
// Alternatively, can add libraries using linker options.

// First, Add the path to the generated library files (directory containing the `benchmark.lib`) in `[Configuration Properties > Linker > General > Additional Library Directories]`. Then do the following:
#ifdef _WIN32
#pragma comment ( lib, "Shlwapi.lib" )
#ifdef _DEBUG
#pragma comment ( lib, "benchmark.lib" )
#else
#pragma comment ( lib, "benchmark.lib" )
#endif
#endif
```

Can also use the graphical version of CMake:
* Open `CMake GUI`.
* Under `Where to build the binaries`, same path as source plus `build`.
* Under `CMAKE_INSTALL_PREFIX`, same path as source plus `install`.
* Click `Configure`, `Generate`, `Open Project`.
* If build fails, try deleting entire directory and starting again, or unticking options to build less.

## Building with Intel 2015 Update 1 or Intel System Studio Update 4

See instructions for building with Visual Studio. Once built, right click on the solution and change the build to Intel.

## Building on Solaris

If you're running benchmarks on solaris, you'll want the kstat library linked in
too (`-lkstat`).