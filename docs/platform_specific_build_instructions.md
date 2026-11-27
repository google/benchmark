# Platform Specific Build Instructions

The library can be built with CMake build system, and Bazel build system,

It can either be built as a subproject of a parent build system,
or can you build it separately, and then use the pkg-config `.pc` file
produced by the build system to link to the resulting library.
Additionally, CMake build system produces a CMake config.

These are all the supported cases. Unless there's an exceptional circumstances,
you should not need to use any alternative approaches.

If for some reason you can not use any of these approaches,
you'll need to manually inspect the `.pc` file and manually
adapt your compiler invocations with information encoded in that file.
