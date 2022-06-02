# User-Requested Performance Counters

When running benchmarks, the user may choose to request collection of
performance counters. This may be useful in investigation scenarios - narrowing
down the cause of a regression; or verifying that the underlying cause of a
performance improvement matches expectations.

This feature is available if:

* The benchmark is run on an architecture featuring a Performance Monitoring Unit (PMU),
* The benchmark is compiled with support for collecting counters. 


The feature does not require modifying benchmark code. Counter collection is
handled at the boundaries where timer collection is also handled. 

The counter values are reported back through the [User Counters](../README.md#custom-counters)
mechanism, meaning, they are available in all the formats (e.g. JSON) supported
by User Counters.

## MacOS
MacOS, on Apple Silicon and Intel, has built in support for per thread instruction 
and cycle counters. These counters can be queried by the by semi-undocumented API 
in libpthread `thread_selfcounts`. Benchmark support for these counters is always 
enabled as it requires no additional dependencies.

To use, pass a comma-separated list of counter names through the 
`--benchmark_perf_counters` flag. The only available counter names 
are `CYCLES` and `INSTRUCTIONS`.

## Linux
Currently, this requires [libpfm](http://perfmon2.sourceforge.net/) be available 
at build time.

To opt-in:

*  Install `libpfm4-dev`, e.g. `apt-get install libpfm4-dev`.
*  Enable the cmake flag BENCHMARK_ENABLE_LIBPFM.

To use, pass a comma-separated list of counter names through the
`--benchmark_perf_counters` flag. The names are decoded through libpfm - meaning,
they are platform specific, but some (e.g. `CYCLES` or `INSTRUCTIONS`) are
mapped by libpfm to platform-specifics - see libpfm
[documentation](http://perfmon2.sourceforge.net/docs.html) for more details.

