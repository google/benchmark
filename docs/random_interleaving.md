<a name="interleaving" />

# Random Interleaving

[Random Interleaving](https://github.com/google/benchmark/issues/1051) is a
technique to lower run-to-run variance. It breaks the execution of a
microbenchmark into multiple chunks and randomly interleaves them with chunks
from other microbenchmarks in the same benchmark test. Data shows it is able to
lower run-to-run variance by
[40%](https://github.com/google/benchmark/issues/1051) on average.

To use, set `--benchmark_enable_random_interleaving=true`.

It's a known issue that random interleaving may increase the benchmark execution
time, if:

1.  A benchmark has costly setup and / or teardown. Random interleaving will run
    setup and teardown many times and may increase test execution time
    significantly.
2.  The time to run a single benchmark iteration is larger than the desired time
    per repetition (i.e., `benchmark_min_time / benchmark_repetitions`).

The overhead of random interleaving can be controlled by
`--benchmark_random_interleaving_max_overhead`. The default value is 0.4 meaning
the total execution time under random interlaving is limited by 1.4 x original
total execution time. Set it to `inf` for unlimited overhead.
