     1|# User Guide
     2|
     3|## Command Line
     4|
     5|[Output Formats](#output-formats)
     6|
     7|[Output Files](#output-files)
     8|
     9|[Running Benchmarks](#running-benchmarks)
    10|
    11|[Running a Subset of Benchmarks](#running-a-subset-of-benchmarks)
    12|
    13|[Result Comparison](#result-comparison)
    14|
    15|[Extra Context](#extra-context)
    16|
    17|## Library
    18|
    19|[Runtime and Reporting Considerations](#runtime-and-reporting-considerations)
    20|
    21|[Setup/Teardown](#setupteardown)
    22|
    23|[Passing Arguments](#passing-arguments)
    24|
    25|[Custom Benchmark Name](#custom-benchmark-name)
    26|
    27|[Calculating Asymptotic Complexity](#asymptotic-complexity)
    28|
    29|[Templated Benchmarks](#templated-benchmarks)
    30|
    31|[Templated Benchmarks that take arguments](#templated-benchmarks-with-arguments)
    32|
    33|[Fixtures](#fixtures)
    34|
    35|[Custom Counters](#custom-counters)
    36|
    37|[Multithreaded Benchmarks](#multithreaded-benchmarks)
    38|
    39|[CPU Timers](#cpu-timers)
    40|
    41|[Manual Timing](#manual-timing)
    42|
    43|[Setting the Time Unit](#setting-the-time-unit)
    44|
    45|[Random Interleaving](random_interleaving.md)
    46|
    47|[User-Requested Performance Counters](perf_counters.md)
    48|
    49|[Preventing Optimization](#preventing-optimization)
    50|
    51|[Reporting Statistics](#reporting-statistics)
    52|
    53|[Custom Statistics](#custom-statistics)
    54|
    55|[Memory Usage](#memory-usage)
    56|
    57|[Using RegisterBenchmark](#using-register-benchmark)
    58|
    59|[Exiting with an Error](#exiting-with-an-error)
    60|
    61|[A Faster `KeepRunning` Loop](#a-faster-keep-running-loop)
    62|
    63|## Benchmarking Tips
    64|
    65|[Disabling CPU Frequency Scaling](#disabling-cpu-frequency-scaling)
    66|
    67|[Reducing Variance in Benchmarks](reducing_variance.md)
    68|
    69|<a name="output-formats" />
    70|
    71|## Output Formats
    72|
    73|The library supports multiple output formats. Use the
    74|`--benchmark_format=<console|json|csv>` flag (or set the
    75|`BENCHMARK_FORMAT=<console|json|csv>` environment variable) to set
    76|the format type. `console` is the default format.
    77|
    78|The Console format is intended to be a human readable format. By default
    79|the format generates color output. Context is output on stderr and the
    80|tabular data on stdout. Example tabular output looks like:
    81|
    82|```
    83|Benchmark                               Time(ns)    CPU(ns) Iterations
    84|----------------------------------------------------------------------
    85|BM_SetInsert/1024/1                        28928      29349      23853  133.097kiB/s   33.2742k items/s
    86|BM_SetInsert/1024/8                        32065      32913      21375  949.487kiB/s   237.372k items/s
    87|BM_SetInsert/1024/10                       33157      33648      21431  1.13369MiB/s   290.225k items/s
    88|```
    89|
    90|The JSON format outputs human readable JSON split into two top level
    91|attributes: `context` and `benchmarks`. This format is useful for tools that
    92|need to consume benchmark results without parsing console output.
    93|
    94|The `context` object contains information about the run in general, including
    95|the date, host, CPU, caches, load average, benchmark library version, and
    96|`json_schema_version`. Extra context added with `benchmark::AddCustomContext` or
    97|`--benchmark_context` is emitted as additional string fields in `context`.
    98|
    99|The `benchmarks` array contains an object for each benchmark result. Iteration
   100|results commonly include fields such as `name`, `run_name`, `run_type`,
   101|`iterations`, `real_time`, `cpu_time`, `time_unit`, and `threads`. Depending on
   102|benchmark configuration, result objects can also include aggregate fields,
   103|asymptotic complexity fields, skip/error fields, memory metrics, labels, user
   104|counters, and user-requested performance counters.
   105|
   106|User counters, including rates such as `bytes_per_second` and
   107|`items_per_second`, are emitted as additional numeric fields on the benchmark
   108|object. User-requested performance counters are reported the same way.
   109|
   110|The JSON output may gain new fields over time. Consumers should ignore unknown
   111|fields and tolerate optional fields being absent. This allows the format to be
   112|extended while preserving compatibility for existing consumers.
   113|
   114|An abbreviated example JSON output looks like:
   115|
   116|```json
   117|{
   118|  "context": {
   119|    "date": "2015/03/17-18:40:25",
   120|    "host_name": "my-host",
   121|    "num_cpus": 40,
   122|    "mhz_per_cpu": 2801,
   123|    "cpu_scaling_enabled": false,
   124|    "caches": [
   125|      {
   126|        "type": "Data",
   127|        "level": 1,
   128|        "size": 32768,
   129|        "num_sharing": 2
   130|      }
   131|    ],
   132|    "load_avg": [],
   133|    "library_version": "vX.Y.Z",
   134|    "library_build_type": "debug",
   135|    "json_schema_version": 1
   136|  },
   137|  "benchmarks": [
   138|    {
   139|      "name": "BM_SetInsert/1024/1",
   140|      "run_name": "BM_SetInsert/1024/1",
   141|      "run_type": "iteration",
   142|      "iterations": 94877,
   143|      "real_time": 29275,
   144|      "cpu_time": 29836,
   145|      "time_unit": "ns",
   146|      "bytes_per_second": 134066,
   147|      "items_per_second": 33516
   148|    },
   149|    {
   150|      "name": "BM_SetInsert/1024/8",
   151|      "run_name": "BM_SetInsert/1024/8",
   152|      "run_type": "iteration",
   153|      "iterations": 21609,
   154|      "real_time": 32317,
   155|      "cpu_time": 32429,
   156|      "time_unit": "ns",
   157|      "bytes_per_second": 986770,
   158|      "items_per_second": 246693
   159|    },
   160|    {
   161|      "name": "BM_SetInsert/1024/10",
   162|      "run_name": "BM_SetInsert/1024/10",
   163|      "run_type": "iteration",
   164|      "iterations": 21393,
   165|      "real_time": 32724,
   166|      "cpu_time": 33355,
   167|      "time_unit": "ns",
   168|      "bytes_per_second": 1199226,
   169|      "items_per_second": 299807
   170|    }
   171|  ]
   172|}
   173|```
   174|
   175|The CSV format outputs comma-separated values. The `context` is output on stderr
   176|and the CSV itself on stdout. Example CSV output looks like:
   177|
   178|```
   179|name,iterations,real_time,cpu_time,bytes_per_second,items_per_second,label
   180|"BM_SetInsert/1024/1",65465,17890.7,8407.45,475768,118942,
   181|"BM_SetInsert/1024/8",116606,18810.1,9766.64,3.27646e+06,819115,
   182|"BM_SetInsert/1024/10",106365,17238.4,8421.53,4.74973e+06,1.18743e+06,
   183|```
   184|
   185|<a name="output-files" />
   186|
   187|## Output Files
   188|
   189|Write benchmark results to a file with the `--benchmark_out=<filename>` option
   190|(or set `BENCHMARK_OUT`). Specify the output format with
   191|`--benchmark_out_format={json|console|csv}` (or set
   192|`BENCHMARK_OUT_FORMAT={json|console|csv}`). Note that the 'csv' reporter is
   193|deprecated and the saved `.csv` file
   194|[is not parsable](https://github.com/google/benchmark/issues/794) by csv
   195|parsers.
   196|
   197|Specifying `--benchmark_out` does not suppress the console output.
   198|
   199|<a name="running-benchmarks" />
   200|
   201|## Running Benchmarks
   202|
   203|Benchmarks are executed by running the produced binaries. Benchmarks binaries,
   204|by default, accept options that may be specified either through their command
   205|line interface or by setting environment variables before execution. For every
   206|`--option_flag=<value>` CLI switch, a corresponding environment variable
   207|`OPTION_FLAG=<value>` exist and is used as default if set (CLI switches always
   208| prevails). A complete list of CLI options is available running benchmarks
   209| with the `--help` switch.
   210|
   211|### Dry runs
   212|
   213|To confirm that benchmarks can run successfully without needing to wait for
   214|multiple repetitions and iterations, the `--benchmark_dry_run` flag can be
   215|used.  This will run the benchmarks as normal, but for 1 iteration and 1
   216|repetition only.
   217|


<a name=\"command-line-options\" />

## Command-Line Options

Benchmarks accept a variety of command-line options that control their behavior. These options can be specified either through command-line switches or by setting environment variables before execution. For every `--option_flag=<value>` CLI switch, a corresponding environment variable `OPTION_FLAG=<value>` exists and is used as default if set (CLI switches always prevail).

A complete list of all available command-line options is provided below:

### Output Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--benchmark_format` | string | `console` | The format to use for console output. Valid values are `console`, `json`, or `csv`. |
| `--benchmark_out` | string | `""` | The file to write additional output to. If specified, results are written to this file in addition to console output. |
| `--benchmark_out_format` | string | `json` | The format to use for file output. Valid values are `console`, `json`, or `csv`. |
| `--benchmark_color` | string | `auto` | Whether to use colors in the output. Valid values: `true`/`yes`/`1`, `false`/`no`/`0`, and `auto`. `auto` means to use colors if the output is being sent to a terminal and the TERM environment variable is set to a terminal type that supports colors. |
| `--benchmark_counters_tabular` | bool | `false` | Whether to use tabular format when printing user counters to the console. Valid values: `true`/`yes`/`1`, `false`/`no`/`0`. |

### Benchmark Selection and Execution

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--benchmark_list_tests` | bool | `false` | Print a list of benchmarks and exit. This option overrides all other options. |
| `--benchmark_filter` | string | `""` | A regular expression that specifies the set of benchmarks to execute. If this flag is empty, or if this flag is the string `all`, all benchmarks linked into the binary are run. |
| `--benchmark_dry_run` | bool | `false` | If enabled, forces each benchmark to execute exactly one iteration and one repetition, bypassing any configured `MinTime()`/`MinWarmUpTime()`/`Iterations()`/`Repetitions()`. Useful for confirming that benchmarks can run successfully without waiting for multiple repetitions and iterations. |
| `--benchmark_enable_random_interleaving` | bool | `false` | If set, enable random interleaving of repetitions of all benchmarks. See [Random Interleaving](random_interleaving.md) for details. |

### Timing and Repetition

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--benchmark_min_time` | string | `0.5s` | Specification of how long to run the benchmark. It can be either an exact number of iterations (specified as `<integer>x`), or a minimum number of seconds (specified as `<float>s`). If the latter format (i.e., min seconds) is used, the system may run the benchmark longer until the results are considered significant. For backward compatibility, the `s` suffix may be omitted, in which case the specified number is interpreted as the number of seconds. |
| `--benchmark_min_warmup_time` | double | `0.0` | Minimum number of seconds a benchmark should be run before results should be taken into account. This can be necessary for benchmarks of code which needs to fill some form of cache before performance is of interest. Note: results gathered within this period are discarded and not used for reported result. |
| `--benchmark_repetitions` | int32 | `1` | The number of runs of each benchmark. If greater than 1, the mean and standard deviation of the runs will be reported. |
| `--benchmark_time_unit` | string | `""` | Set the default time unit to use for reports. Valid values are `ns`, `us`, `ms`, or `s`. If not specified, the library automatically selects an appropriate time unit. |

### Reporting Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--benchmark_report_aggregates_only` | bool | `false` | Report the result of each benchmark repetitions. When `true` is specified only the mean, standard deviation, and other statistics are reported for repeated benchmarks. Affects all reporters. |
| `--benchmark_display_aggregates_only` | bool | `false` | Display the result of each benchmark repetitions. When `true` is specified only the mean, standard deviation, and other statistics are displayed for repeated benchmarks. Unlike `benchmark_report_aggregates_only`, only affects the display reporter, but not file reporter, which will still contain all the output. |

### Performance Counters and Context

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--benchmark_perf_counters` | string | `""` | List of additional perf counters to collect, in libpfm format. For more information about libpfm: https://man7.org/linux/man-pages/man3/libpfm.3.html |
| `--benchmark_context` | kvpairs | `{}` | Extra context to include in the output formatted as comma-separated key-value pairs (e.g., `--benchmark_context=compiler=clang,mode=release`). |

### Other Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--v` | int32 | `0` | The level of verbose logging to output. Higher values produce more verbose output. |

### Environment Variables

All command-line options can also be set via environment variables. The environment variable name is the uppercase version of the option name without the `--benchmark_` prefix. For example:

- `--benchmark_filter=BM_memcpy` can be set as `BENCHMARK_FILTER=BM_memcpy`
- `--benchmark_repetitions=5` can be set as `BENCHMARK_REPETITIONS=5`
- `--benchmark_color=true` can be set as `BENCHMARK_COLOR=true`

Environment variables are used as defaults if set, but command-line switches always prevail.

### Examples

```bash
# Run only benchmarks matching a regex
$ ./benchmark --benchmark_filter=BM_memcpy/32

# Run benchmarks with 5 repetitions
$ ./benchmark --benchmark_repetitions=5

# Run benchmarks with JSON output to a file
$ ./benchmark --benchmark_out=results.json --benchmark_out_format=json

# Run benchmarks with custom context
$ ./benchmark --benchmark_context=compiler=clang,mode=release

# Run benchmarks with custom time unit
$ ./benchmark --benchmark_time_unit=us

# Run benchmarks with performance counters
$ ./benchmark --benchmark_perf_counters=cycles,instructions

# Dry run to verify benchmarks work
$ ./benchmark --benchmark_dry_run

# List all available benchmarks
$ ./benchmark --benchmark_list_tests
```

### Getting Help

To see a complete list of command-line options with their current values, run the benchmark binary with the `--help` flag:

```bash
$ ./benchmark --help
```



   218|<a name="running-a-subset-of-benchmarks" />
   219|
   220|## Running a Subset of Benchmarks
   221|
   222|The `--benchmark_filter=<regex>` option (or `BENCHMARK_FILTER=<regex>`
   223|environment variable) can be used to only run the benchmarks that match
   224|the specified `<regex>`. For example:
   225|
   226|```bash
   227|$ ./run_benchmarks.x --benchmark_filter=BM_memcpy/32
   228|Run on (1 X 2300 MHz CPU )
   229|2016-06-25 19:34:24
   230|Benchmark              Time           CPU Iterations
   231|----------------------------------------------------
   232|BM_memcpy/32          11 ns         11 ns   79545455
   233|BM_memcpy/32k       2181 ns       2185 ns     324074
   234|BM_memcpy/32          12 ns         12 ns   54687500
   235|BM_memcpy/32k       1834 ns       1837 ns     357143
   236|```
   237|
   238|## Disabling Benchmarks
   239|
   240|It is possible to temporarily disable benchmarks by renaming the benchmark
   241|function to have the prefix "DISABLED_". This will cause the benchmark to
   242|be skipped at runtime.
   243|
   244|<a name="result-comparison" />
   245|
   246|## Result comparison
   247|
   248|It is possible to compare the benchmarking results.
   249|See [Additional Tooling Documentation](tools.md)
   250|
   251|<a name="extra-context" />
   252|
   253|## Extra Context
   254|
   255|Sometimes it's useful to add extra context to the content printed before the
   256|results. By default this section includes information about the CPU on which
   257|the benchmarks are running. If you do want to add more context, you can use
   258|the `benchmark_context` command line flag:
   259|
   260|```bash
   261|$ ./run_benchmarks --benchmark_context=pwd=`pwd`
   262|Run on (1 x 2300 MHz CPU)
   263|pwd: /home/user/benchmark/
   264|Benchmark              Time           CPU Iterations
   265|----------------------------------------------------
   266|BM_memcpy/32          11 ns         11 ns   79545455
   267|BM_memcpy/32k       2181 ns       2185 ns     324074
   268|```
   269|
   270|You can get the same effect with the API:
   271|
   272|```c++
   273|  benchmark::AddCustomContext("foo", "bar");
   274|```
   275|
   276|Note that attempts to add a second value with the same key will fail with an
   277|error message.
   278|
   279|<a name="runtime-and-reporting-considerations" />
   280|
   281|## Runtime and Reporting Considerations
   282|
   283|When the benchmark binary is executed, each benchmark function is run serially.
   284|The number of iterations to run is determined dynamically by running the
   285|benchmark a few times and measuring the time taken and ensuring that the
   286|ultimate result will be statistically stable. As such, faster benchmark
   287|functions will be run for more iterations than slower benchmark functions, and
   288|the number of iterations is thus reported.
   289|
   290|In all cases, the number of iterations for which the benchmark is run is
   291|governed by the amount of time the benchmark takes. Concretely, the number of
   292|iterations is at least one, not more than 1e9, until CPU time is greater than
   293|the minimum time, or the wallclock time is 5x minimum time. The minimum time is
   294|set per benchmark by calling `MinTime` on the registered benchmark object.
   295|
   296|The minimum time can also be set for all benchmarks with the
   297|`--benchmark_min_time=<value>` command-line option. This flag supports two
   298|forms:
   299|
   300|* `--benchmark_min_time=<float>s` sets the minimum running time for each
   301|  benchmark repetition in seconds.
   302|* `--benchmark_min_time=<integer>x` runs each benchmark repetition for an
   303|  explicit number of iterations instead of using the dynamic time-based
   304|  iteration selection. This applies to benchmarks that do not already specify
   305|  an explicit iteration count in code.
   306|
   307|For compatibility, bare numeric values such as `--benchmark_min_time=0.5` are
   308|also interpreted as seconds, but the explicit `s` suffix is preferred for
   309|clarity.
   310|
   311|For example:
   312|
   313|```bash
   314|$ ./run_benchmarks.x --benchmark_min_time=0.5s
   315|$ ./run_benchmarks.x --benchmark_min_time=100x
   316|```
   317|
   318|If a benchmark specifies its own `MinTime()` or `Iterations()` in code, those
   319|per-benchmark settings take precedence over the corresponding
   320|`--benchmark_min_time` command-line forms.
   321|
   322|Furthermore warming up a benchmark might be necessary in order to get
   323|stable results because of e.g caching effects of the code under benchmark.
   324|Warming up means running the benchmark a given amount of time, before
   325|results are actually taken into account. The amount of time for which
   326|the warmup should be run can be set per benchmark by calling
   327|`MinWarmUpTime` on the registered benchmark object or for all benchmarks
   328|using the `--benchmark_min_warmup_time` command-line option. Note that
   329|`MinWarmUpTime` will overwrite the value of `--benchmark_min_warmup_time`
   330|for the single benchmark. How many iterations the warmup run of each
   331|benchmark takes is determined the same way as described in the paragraph
   332|above. Per default the warmup phase is set to 0 seconds and is therefore
   333|disabled.
   334|
   335|Average timings are then reported over the iterations run. If multiple
   336|repetitions are requested using the `--benchmark_repetitions` command-line
   337|option, or at registration time, the benchmark function will be run several
   338|times and statistical results across these repetitions will also be reported.
   339|
   340|As well as the per-benchmark entries, a preamble in the report will include
   341|information about the machine on which the benchmarks are run.
   342|
   343|<a name="setup-teardown" />
   344|
   345|## Setup/Teardown
   346|
   347|Global setup/teardown specific to each benchmark can be done by
   348|passing a callback to Setup/Teardown:
   349|
   350|The setup/teardown callbacks will be invoked once for each benchmark. If the
   351|benchmark is multi-threaded (will run in k threads), they will be invoked
   352|exactly once before each run with k threads.
   353|
   354|If the benchmark uses different size groups of threads, the above will be true
   355|for each size group.
   356|
   357|Eg.,
   358|
   359|```c++
   360|static void DoSetup(const benchmark::State& state) {
   361|}
   362|
   363|static void DoTeardown(const benchmark::State& state) {
   364|}
   365|
   366|static void BM_func(benchmark::State& state) {...}
   367|
   368|BENCHMARK(BM_func)->Arg(1)->Arg(3)->Threads(16)->Threads(32)->Setup(DoSetup)->Teardown(DoTeardown);
   369|
   370|```
   371|
   372|In this example, `DoSetup` and `DoTearDown` will be invoked 4 times each,
   373|specifically, once for each of this family:
   374| - BM_func_Arg_1_Threads_16, BM_func_Arg_1_Threads_32
   375| - BM_func_Arg_3_Threads_16, BM_func_Arg_3_Threads_32
   376|
   377|<a name="passing-arguments" />
   378|
   379|## Passing Arguments
   380|
   381|Sometimes a family of benchmarks can be implemented with just one routine that
   382|takes an extra argument to specify which one of the family of benchmarks to
   383|run. For example, the following code defines a family of benchmarks for
   384|measuring the speed of `memcpy()` calls of different lengths:
   385|
   386|```c++
   387|static void BM_memcpy(benchmark::State& state) {
   388|  char* src = new char[state.range(0)];
   389|  char* dst = new char[state.range(0)];
   390|  memset(src, 'x', state.range(0));
   391|  for (auto _ : state)
   392|    memcpy(dst, src, state.range(0));
   393|  state.SetBytesProcessed(int64_t(state.iterations()) *
   394|                          int64_t(state.range(0)));
   395|  delete[] src;
   396|  delete[] dst;
   397|}
   398|BENCHMARK(BM_memcpy)->Arg(8)->Arg(64)->Arg(512)->Arg(4<<10)->Arg(8<<10);
   399|```
   400|
   401|The preceding code is quite repetitive, and can be replaced with the following
   402|short-hand. The following invocation will pick a few appropriate arguments in
   403|the specified range and will generate a benchmark for each such argument.
   404|
   405|```c++
   406|BENCHMARK(BM_memcpy)->Range(8, 8<<10);
   407|```
   408|
   409|By default the arguments in the range are generated in multiples of eight and
   410|the command above selects [ 8, 64, 512, 4k, 8k ]. In the following code the
   411|range multiplier is changed to multiples of two.
   412|
   413|```c++
   414|BENCHMARK(BM_memcpy)->RangeMultiplier(2)->Range(8, 8<<10);
   415|```
   416|
   417|Now arguments generated are [ 8, 16, 32, 64, 128, 256, 512, 1024, 2k, 4k, 8k ].
   418|
   419|The preceding code shows a method of defining a sparse range.  The following
   420|example shows a method of defining a dense range. It is then used to benchmark
   421|the performance of `std::vector` initialization for uniformly increasing sizes.
   422|
   423|```c++
   424|static void BM_DenseRange(benchmark::State& state) {
   425|  for(auto _ : state) {
   426|    std::vector<int> v(state.range(0), state.range(0));
   427|    auto data = v.data();
   428|    benchmark::DoNotOptimize(data);
   429|    benchmark::ClobberMemory();
   430|  }
   431|}
   432|BENCHMARK(BM_DenseRange)->DenseRange(0, 1024, 128);
   433|```
   434|
   435|Now arguments generated are [ 0, 128, 256, 384, 512, 640, 768, 896, 1024 ].
   436|
   437|You might have a benchmark that depends on two or more inputs. For example, the
   438|following code defines a family of benchmarks for measuring the speed of set
   439|insertion.
   440|
   441|```c++
   442|static void BM_SetInsert(benchmark::State& state) {
   443|  std::set<int> data;
   444|  for (auto _ : state) {
   445|    state.PauseTiming();
   446|    data = ConstructRandomSet(state.range(0));
   447|    state.ResumeTiming();
   448|    for (int j = 0; j < state.range(1); ++j)
   449|      data.insert(RandomNumber());
   450|  }
   451|}
   452|BENCHMARK(BM_SetInsert)
   453|    ->Args({1<<10, 128})
   454|    ->Args({2<<10, 128})
   455|    ->Args({4<<10, 128})
   456|    ->Args({8<<10, 128})
   457|    ->Args({1<<10, 512})
   458|    ->Args({2<<10, 512})
   459|    ->Args({4<<10, 512})
   460|    ->Args({8<<10, 512});
   461|```
   462|
   463|The preceding code is quite repetitive, and can be replaced with the following
   464|short-hand. The following macro will pick a few appropriate arguments in the
   465|product of the two specified ranges and will generate a benchmark for each such
   466|pair.
   467|
   468|<!-- {% raw %} -->
   469|```c++
   470|BENCHMARK(BM_SetInsert)->Ranges({{1<<10, 8<<10}, {128, 512}});
   471|```
   472|<!-- {% endraw %} -->
   473|
   474|Some benchmarks may require specific argument values that cannot be expressed
   475|with `Ranges`. In this case, `ArgsProduct` offers the ability to generate a
   476|benchmark input for each combination in the product of the supplied vectors.
   477|
   478|<!-- {% raw %} -->
   479|```c++
   480|BENCHMARK(BM_SetInsert)
   481|    ->ArgsProduct({{1<<10, 3<<10, 8<<10}, {20, 40, 60, 80}})
   482|// would generate the same benchmark arguments as
   483|BENCHMARK(BM_SetInsert)
   484|    ->Args({1<<10, 20})
   485|    ->Args({3<<10, 20})
   486|    ->Args({8<<10, 20})
   487|    ->Args({3<<10, 40})
   488|    ->Args({8<<10, 40})
   489|    ->Args({1<<10, 40})
   490|    ->Args({1<<10, 60})
   491|    ->Args({3<<10, 60})
   492|    ->Args({8<<10, 60})
   493|    ->Args({1<<10, 80})
   494|    ->Args({3<<10, 80})
   495|    ->Args({8<<10, 80});
   496|```
   497|<!-- {% endraw %} -->
   498|
   499|For the most common scenarios, helper methods for creating a list of
   500|integers for a given sparse or dense range are provided.
   501|
   502|```c++
   503|BENCHMARK(BM_SetInsert)
   504|    ->ArgsProduct({
   505|      benchmark::CreateRange(8, 128, /*multi=*/2),
   506|      benchmark::CreateDenseRange(1, 4, /*step=*/1)
   507|    })
   508|// would generate the same benchmark arguments as
   509|BENCHMARK(BM_SetInsert)
   510|    ->ArgsProduct({
   511|      {8, 16, 32, 64, 128},
   512|      {1, 2, 3, 4}
   513|    });
   514|```
   515|
   516|For more complex patterns of inputs, passing a custom function to `Apply` allows
   517|programmatic specification of an arbitrary set of arguments on which to run the
   518|benchmark. The following example enumerates a dense range on one parameter,
   519|and a sparse range on the second.
   520|
   521|```c++
   522|static void CustomArguments(benchmark::Benchmark* b) {
   523|  for (int i = 0; i <= 10; ++i)
   524|    for (int j = 32; j <= 1024*1024; j *= 8)
   525|      b->Args({i, j});
   526|}
   527|BENCHMARK(BM_SetInsert)->Apply(CustomArguments);
   528|```
   529|
   530|### Naming Benchmark Arguments
   531|
   532|When a benchmark takes one or more numeric arguments, the generated benchmark
   533|names can be made easier to read by naming those arguments. Use `ArgName` for a
   534|single argument and `ArgNames` for multiple arguments.
   535|
   536|```c++
   537|BENCHMARK(BM_memcpy)->Range(8, 512)->ArgName("bytes");
   538|```
   539|
   540|This changes names such as `BM_memcpy/8` and `BM_memcpy/512` to
   541|`BM_memcpy/bytes:8` and `BM_memcpy/bytes:512`.
   542|
   543|For benchmarks with more than one argument, each name labels the corresponding
   544|argument position.
   545|
   546|<!-- {% raw %} -->
   547|```c++
   548|BENCHMARK(BM_SetInsert)
   549|    ->Args({100, 128})
   550|    ->Args({200, 512})
   551|    ->ArgNames({"size", "inserts"});
   552|```
   553|<!-- {% endraw %} -->
   554|
   555|This produces names such as `BM_SetInsert/size:100/inserts:128` and
   556|`BM_SetInsert/size:200/inserts:512`. Empty argument names are allowed and leave
   557|that argument value unlabeled, for example `ArgNames({"size", ""})` produces
   558|names like `BM_SetInsert/size:100/128`.
   559|
   560|### Passing Arbitrary Arguments to a Benchmark
   561|
   562|It is possible to define a benchmark that takes an arbitrary number
   563|of extra arguments. The `BENCHMARK_CAPTURE(func, test_case_name, ...args)`
   564|macro creates a benchmark that invokes `func`  with the `benchmark::State` as
   565|the first argument followed by the specified `args...`.
   566|The `test_case_name` is appended to the name of the benchmark and
   567|should describe the values passed.
   568|
   569|```c++
   570|template <class ...Args>
   571|void BM_takes_args(benchmark::State& state, Args&&... args) {
   572|  auto args_tuple = std::make_tuple(std::move(args)...);
   573|  for (auto _ : state) {
   574|    std::cout << std::get<0>(args_tuple) << ": " << std::get<1>(args_tuple)
   575|              << '\n';
   576|    [...]
   577|  }
   578|}
   579|// Registers a benchmark named "BM_takes_args/int_string_test" that passes
   580|// the specified values to `args`.
   581|BENCHMARK_CAPTURE(BM_takes_args, int_string_test, 42, std::string("abc"));
   582|
   583|// Registers the same benchmark "BM_takes_args/int_test" that passes
   584|// the specified values to `args`.
   585|BENCHMARK_CAPTURE(BM_takes_args, int_test, 42, 43);
   586|```
   587|
   588|Note that elements of `...args` may refer to global variables. Users should
   589|avoid modifying global state inside of a benchmark.
   590|
   591|### Naming a Benchmark Without Capturing Arguments
   592|
   593|If you only need to give a benchmark a custom name (without passing extra
   594|arguments), use `BENCHMARK_NAMED(func, test_case_name)`. Unlike
   595|`BENCHMARK_CAPTURE`, this macro does not create a lambda, which avoids
   596|compiler and linker scalability issues when registering thousands of
   597|benchmarks.
   598|
   599|```c++
   600|void BM_Foo(benchmark::State& state) {
   601|  for (auto _ : state) {}
   602|}
   603|// Registers a benchmark named "BM_Foo/my_variant"
   604|BENCHMARK_NAMED(BM_Foo, my_variant);
   605|```
   606|
   607|Use `BENCHMARK_CAPTURE` when you need to pass extra arguments; use
   608|`BENCHMARK_NAMED` when you only need the name.
   609|
   610|<a name="asymptotic-complexity" />
   611|
   612|## Calculating Asymptotic Complexity (Big O)
   613|
   614|Asymptotic complexity might be calculated for a family of benchmarks. The
   615|following code will calculate the coefficient for the high-order term in the
   616|running time and the normalized root-mean square error of string comparison.
   617|
   618|```c++
   619|static void BM_StringCompare(benchmark::State& state) {
   620|  std::string s1(state.range(0), '-');
   621|  std::string s2(state.range(0), '-');
   622|  for (auto _ : state) {
   623|    auto comparison_result = s1.compare(s2);
   624|    benchmark::DoNotOptimize(comparison_result);
   625|  }
   626|  state.SetComplexityN(state.range(0));
   627|}
   628|BENCHMARK(BM_StringCompare)
   629|    ->RangeMultiplier(2)->Range(1<<10, 1<<18)->Complexity(benchmark::oN);
   630|```
   631|
   632|As shown in the following invocation, asymptotic complexity might also be
   633|calculated automatically.
   634|
   635|```c++
   636|BENCHMARK(BM_StringCompare)
   637|    ->RangeMultiplier(2)->Range(1<<10, 1<<18)->Complexity();
   638|```
   639|
   640|The following code will specify asymptotic complexity with a lambda function,
   641|that might be used to customize high-order term calculation.
   642|
   643|```c++
   644|BENCHMARK(BM_StringCompare)->RangeMultiplier(2)
   645|    ->Range(1<<10, 1<<18)->Complexity([](benchmark::IterationCount n)->double{return n; });
   646|```
   647|
   648|<a name="custom-benchmark-name" />
   649|
   650|## Custom Benchmark Name
   651|
   652|You can change the benchmark's name as follows:
   653|
   654|```c++
   655|BENCHMARK(BM_memcpy)->Name("memcpy")->RangeMultiplier(2)->Range(8, 8<<10);
   656|```
   657|
   658|The invocation will execute the benchmark as before using `BM_memcpy` but changes
   659|the prefix in the report to `memcpy`.
   660|
   661|<a name="templated-benchmarks" />
   662|
   663|## Templated Benchmarks
   664|
   665|This example produces and consumes messages of size `sizeof(v)` `range_x`
   666|times. It also outputs throughput in the absence of multiprogramming.
   667|
   668|```c++
   669|template <class Q> void BM_Sequential(benchmark::State& state) {
   670|  Q q;
   671|  typename Q::value_type v;
   672|  for (auto _ : state) {
   673|    for (int i = state.range(0); i--; )
   674|      q.push(v);
   675|    for (int e = state.range(0); e--; )
   676|      q.Wait(&v);
   677|  }
   678|  // actually messages, not bytes:
   679|  state.SetBytesProcessed(
   680|      static_cast<int64_t>(state.iterations())*state.range(0));
   681|}
   682|
   683|// You can use the BENCHMARK macro with template parameters:
   684|BENCHMARK(BM_Sequential<WaitQueue<int>>)->Range(1<<0, 1<<10);
   685|
   686|// Old, legacy verbose C++03 syntax:
   687|BENCHMARK_TEMPLATE(BM_Sequential, WaitQueue<int>)->Range(1<<0, 1<<10);
   688|
   689|```
   690|
   691|Three macros are provided for adding benchmark templates.
   692|
   693|```c++
   694|#define BENCHMARK(func<...>) // Takes any number of parameters.
   695|#define BENCHMARK_TEMPLATE1(func, arg1)
   696|#define BENCHMARK_TEMPLATE2(func, arg1, arg2)
   697|```
   698|
   699|<a name="templated-benchmarks-with-arguments" />
   700|
   701|## Templated Benchmarks that take arguments
   702|
   703|Sometimes there is a need to template benchmarks, and provide arguments to them.
   704|
   705|```c++
   706|template <class Q> void BM_Sequential_With_Step(benchmark::State& state, int step) {
   707|  Q q;
   708|  typename Q::value_type v;
   709|  for (auto _ : state) {
   710|    for (int i = state.range(0); i-=step; )
   711|      q.push(v);
   712|    for (int e = state.range(0); e-=step; )
   713|      q.Wait(&v);
   714|  }
   715|  // actually messages, not bytes:
   716|  state.SetBytesProcessed(
   717|      static_cast<int64_t>(state.iterations())*state.range(0));
   718|}
   719|
   720|BENCHMARK_TEMPLATE1_CAPTURE(BM_Sequential, WaitQueue<int>, Step1, 1)->Range(1<<0, 1<<10);
   721|```
   722|
   723|<a name="fixtures" />
   724|
   725|## Fixtures
   726|
   727|Fixture tests are created by first defining a type that derives from
   728|`::benchmark::Fixture` and then creating/registering the tests using the
   729|following macros:
   730|
   731|* `BENCHMARK_F(ClassName, Method)`
   732|* `BENCHMARK_DEFINE_F(ClassName, Method)`
   733|* `BENCHMARK_REGISTER_F(ClassName, Method)`
   734|
   735|For Example:
   736|
   737|```c++
   738|class MyFixture : public benchmark::Fixture {
   739|public:
   740|  void SetUp(::benchmark::State& state) {
   741|  }
   742|
   743|  void TearDown(::benchmark::State& state) {
   744|  }
   745|};
   746|
   747|// Defines and registers `FooTest` using the class `MyFixture`.
   748|BENCHMARK_F(MyFixture, FooTest)(benchmark::State& st) {
   749|   for (auto _ : st) {
   750|     ...
   751|  }
   752|}
   753|
   754|// Only defines `BarTest` using the class `MyFixture`.
   755|BENCHMARK_DEFINE_F(MyFixture, BarTest)(benchmark::State& st) {
   756|   for (auto _ : st) {
   757|     ...
   758|  }
   759|}
   760|// `BarTest` is NOT registered.
   761|BENCHMARK_REGISTER_F(MyFixture, BarTest)->Threads(2);
   762|// `BarTest` is now registered.
   763|```
   764|
   765|### Templated Fixtures
   766|
   767|Also you can create templated fixture by using the following macros:
   768|
   769|* `BENCHMARK_TEMPLATE_F(ClassName, Method, ...)`
   770|* `BENCHMARK_TEMPLATE_DEFINE_F(ClassName, Method, ...)`
   771|
   772|For example:
   773|
   774|```c++
   775|template<typename T>
   776|class MyFixture : public benchmark::Fixture {};
   777|
   778|// Defines and registers `IntTest` using the class template `MyFixture<int>`.
   779|BENCHMARK_TEMPLATE_F(MyFixture, IntTest, int)(benchmark::State& st) {
   780|   for (auto _ : st) {
   781|     ...
   782|  }
   783|}
   784|
   785|// Only defines `DoubleTest` using the class template `MyFixture<double>`.
   786|BENCHMARK_TEMPLATE_DEFINE_F(MyFixture, DoubleTest, double)(benchmark::State& st) {
   787|   for (auto _ : st) {
   788|     ...
   789|  }
   790|}
   791|// `DoubleTest` is NOT registered.
   792|BENCHMARK_REGISTER_F(MyFixture, DoubleTest)->Threads(2);
   793|// `DoubleTest` is now registered.
   794|```
   795|
   796|If you want to use a method template for your fixtures,
   797|which you instantiate afterward, use the following macros:
   798|
   799|* `BENCHMARK_TEMPLATE_METHOD_F(ClassName, Method)`
   800|* `BENCHMARK_TEMPLATE_INSTANTIATE_F(ClassName, Method, ...)`
   801|
   802|With these macros you can define one method for several instantiations.
   803|Example (using `MyFixture` from above):
   804|
   805|```c++
   806|// Defines `Test` using the class template `MyFixture`.
   807|BENCHMARK_TEMPLATE_METHOD_F(MyFixture, Test)(benchmark::State& st) {
   808|   for (auto _ : st) {
   809|     ...
   810|  }
   811|}
   812|
   813|// Instantiates and registers the benchmark `MyFixture<int>::Test`.
   814|BENCHMARK_TEMPLATE_INSTANTIATE_F(MyFixture, Test, int)->Threads(2);
   815|// Instantiates and registers the benchmark `MyFixture<double>::Test`.
   816|BENCHMARK_TEMPLATE_INSTANTIATE_F(MyFixture, Test, double)->Threads(4);
   817|```
   818|
   819|Inside the method definition of `BENCHMARK_TEMPLATE_METHOD_F` the type `Base` refers
   820|to the type of the instantiated fixture.
   821|Accesses to members of the fixture must be prefixed by `this->`.
   822|
   823|`BENCHMARK_TEMPLATE_METHOD_F`and `BENCHMARK_TEMPLATE_INSTANTIATE_F` can only be used,
   824|if the fixture does not use non-type template parameters.
   825|If you want to pass values as template parameters, use e.g. `std::integral_constant`.
   826|For example:
   827|
   828|```c++
   829|template<typename Sz>
   830|class SizedFixture : public benchmark::Fixture {
   831|  static constexpr auto Size = Sz::value;
   832|  int myValue;
   833|};
   834|
   835|BENCHMARK_TEMPLATE_METHOD_F(SizedFixture, Test)(benchmark::State& st) {
   836|   for (auto _ : st) {
   837|     this->myValue = Base::Size;
   838|  }
   839|}
   840|
   841|BENCHMARK_TEMPLATE_INSTANTIATE_F(SizedFixture, Test, std::integral_constant<5>)->Threads(2);
   842|```
   843|
   844|<a name="custom-counters" />
   845|
   846|## Custom Counters
   847|
   848|You can add your own counters with user-defined names. The example below
   849|will add columns "Foo", "Bar" and "Baz" in its output:
   850|
   851|```c++
   852|static void UserCountersExample1(benchmark::State& state) {
   853|  double numFoos = 0, numBars = 0, numBazs = 0;
   854|  for (auto _ : state) {
   855|    // ... count Foo,Bar,Baz events
   856|  }
   857|  state.counters["Foo"] = numFoos;
   858|  state.counters["Bar"] = numBars;
   859|  state.counters["Baz"] = numBazs;
   860|}
   861|```
   862|
   863|The `state.counters` object is a `std::map` with `std::string` keys
   864|and `Counter` values. The latter is a `double`-like class, via an implicit
   865|conversion to `double&`. Thus you can use all of the standard arithmetic
   866|assignment operators (`=,+=,-=,*=,/=`) to change the value of each counter.
   867|
   868|The `Counter` constructor accepts three parameters: the value as a `double`
   869|; a bit flag which allows you to show counters as rates, and/or as per-thread
   870|iteration, and/or as per-thread averages, and/or iteration invariants,
   871|and/or finally inverting the result; and a flag specifying the 'unit' - i.e.
   872|is 1k a 1000 (default, `benchmark::Counter::OneK::kIs1000`), or 1024
   873|(`benchmark::Counter::OneK::kIs1024`)?
   874|
   875|```c++
   876|  // sets a simple counter
   877|  state.counters["Foo"] = numFoos;
   878|
   879|  // Set the counter as a rate. It will be presented divided
   880|  // by the duration of the benchmark.
   881|  // Meaning: per one second, how many 'foo's are processed?
   882|  state.counters["FooRate"] = Counter(numFoos, benchmark::Counter::kIsRate);
   883|
   884|  // Set the counter as a rate. It will be presented divided
   885|  // by the duration of the benchmark, and the result inverted.
   886|  // Meaning: how many seconds it takes to process one 'foo'?
   887|  state.counters["FooInvRate"] = Counter(numFoos, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
   888|
   889|  // Set the counter as a thread-average quantity. It will
   890|  // be presented divided by the number of threads.
   891|  state.counters["FooAvg"] = Counter(numFoos, benchmark::Counter::kAvgThreads);
   892|
   893|  // There's also a combined flag:
   894|  state.counters["FooAvgRate"] = Counter(numFoos,benchmark::Counter::kAvgThreadsRate);
   895|
   896|  // This says that we process with the rate of state.range(0) bytes every iteration:
   897|  state.counters["BytesProcessed"] = Counter(state.range(0), benchmark::Counter::kIsIterationInvariantRate, benchmark::Counter::OneK::kIs1024);
   898|```
   899|
   900|You can use `insert()` with `std::initializer_list`:
   901|
   902|<!-- {% raw %} -->
   903|```c++
   904|  state.counters.insert({{"Foo", numFoos}, {"Bar", numBars}, {"Baz", numBazs}});
   905|  // ... instead of:
   906|  state.counters["Foo"] = numFoos;
   907|  state.counters["Bar"] = numBars;
   908|  state.counters["Baz"] = numBazs;
   909|```
   910|<!-- {% endraw %} -->
   911|
   912|In multithreaded benchmarks, each counter is set on the calling thread only.
   913|When the benchmark finishes, the counters from each thread will be summed.
   914|Counters that are configured with `kIsRate`, will report the average rate across all threads, while `kAvgThreadsRate` counters will report the average rate per thread.
   915|
   916|### Counter Reporting
   917|
   918|When using the console reporter, by default, user counters are printed at
   919|the end after the table, the same way as ``bytes_processed`` and
   920|``items_processed``. This is best for cases in which there are few counters,
   921|or where there are only a couple of lines per benchmark. Here's an example of
   922|the default output:
   923|
   924|```
   925|------------------------------------------------------------------------------
   926|Benchmark                        Time           CPU Iterations UserCounters...
   927|------------------------------------------------------------------------------
   928|BM_UserCounter/threads:8      2248 ns      10277 ns      68808 Bar=16 Bat=40 Baz=24 Foo=8
   929|BM_UserCounter/threads:1      9797 ns       9788 ns      71523 Bar=2 Bat=5 Baz=3 Foo=1024m
   930|BM_UserCounter/threads:2      4924 ns       9842 ns      71036 Bar=4 Bat=10 Baz=6 Foo=2
   931|BM_UserCounter/threads:4      2589 ns      10284 ns      68012 Bar=8 Bat=20 Baz=12 Foo=4
   932|BM_UserCounter/threads:8      2212 ns      10287 ns      68040 Bar=16 Bat=40 Baz=24 Foo=8
   933|BM_UserCounter/threads:16     1782 ns      10278 ns      68144 Bar=32 Bat=80 Baz=48 Foo=16
   934|BM_UserCounter/threads:32     1291 ns      10296 ns      68256 Bar=64 Bat=160 Baz=96 Foo=32
   935|BM_UserCounter/threads:4      2615 ns      10307 ns      68040 Bar=8 Bat=20 Baz=12 Foo=4
   936|BM_Factorial                    26 ns         26 ns   26608979 40320
   937|BM_Factorial/real_time          26 ns         26 ns   26587936 40320
   938|BM_CalculatePiRange/1           16 ns         16 ns   45704255 0
   939|BM_CalculatePiRange/8           73 ns         73 ns    9520927 3.28374
   940|BM_CalculatePiRange/64         609 ns        609 ns    1140647 3.15746
   941|BM_CalculatePiRange/512       4900 ns       4901 ns     142696 3.14355
   942|```
   943|
   944|If this doesn't suit you, you can print each counter as a table column by
   945|passing the flag `--benchmark_counters_tabular=true` to the benchmark
   946|application. This is best for cases in which there are a lot of counters, or
   947|a lot of lines per individual benchmark. Note that this will trigger a
   948|reprinting of the table header any time the counter set changes between
   949|individual benchmarks. Here's an example of corresponding output when
   950|`--benchmark_counters_tabular=true` is passed:
   951|
   952|```
   953|---------------------------------------------------------------------------------------
   954|Benchmark                        Time           CPU Iterations    Bar   Bat   Baz   Foo
   955|---------------------------------------------------------------------------------------
   956|BM_UserCounter/threads:8      2198 ns       9953 ns      70688     16    40    24     8
   957|BM_UserCounter/threads:1      9504 ns       9504 ns      73787      2     5     3     1
   958|BM_UserCounter/threads:2      4775 ns       9550 ns      72606      4    10     6     2
   959|BM_UserCounter/threads:4      2508 ns       9951 ns      70332      8    20    12     4
   960|BM_UserCounter/threads:8      2055 ns       9933 ns      70344     16    40    24     8
   961|BM_UserCounter/threads:16     1610 ns       9946 ns      70720     32    80    48    16
   962|BM_UserCounter/threads:32     1192 ns       9948 ns      70496     64   160    96    32
   963|BM_UserCounter/threads:4      2506 ns       9949 ns      70332      8    20    12     4
   964|--------------------------------------------------------------
   965|Benchmark                        Time           CPU Iterations
   966|--------------------------------------------------------------
   967|BM_Factorial                    26 ns         26 ns   26392245 40320
   968|BM_Factorial/real_time          26 ns         26 ns   26494107 40320
   969|BM_CalculatePiRange/1           15 ns         15 ns   45571597 0
   970|BM_CalculatePiRange/8           74 ns         74 ns    9450212 3.28374
   971|BM_CalculatePiRange/64         595 ns        595 ns    1173901 3.15746
   972|BM_CalculatePiRange/512       4752 ns       4752 ns     147380 3.14355
   973|BM_CalculatePiRange/4k       37970 ns      37972 ns      18453 3.14184
   974|BM_CalculatePiRange/32k     303733 ns     303744 ns       2305 3.14162
   975|BM_CalculatePiRange/256k   2434095 ns    2434186 ns        288 3.1416
   976|BM_CalculatePiRange/1024k  9721140 ns    9721413 ns         71 3.14159
   977|BM_CalculatePi/threads:8      2255 ns       9943 ns      70936
   978|```
   979|
   980|Note above the additional header printed when the benchmark changes from
   981|``BM_UserCounter`` to ``BM_Factorial``. This is because ``BM_Factorial`` does
   982|not have the same counter set as ``BM_UserCounter``.
   983|
   984|<a name="multithreaded-benchmarks"/>
   985|
   986|## Multithreaded Benchmarks
   987|
   988|In a multithreaded test (benchmark invoked by multiple threads simultaneously),
   989|it is guaranteed that none of the threads will start until all have reached
   990|the start of the benchmark loop, and all will have finished before any thread
   991|exits the benchmark loop. (This behavior is also provided by the `KeepRunning()`
   992|API) As such, any global setup or teardown can be wrapped in a check against the thread
   993|index:
   994|
   995|```c++
   996|static void BM_MultiThreaded(benchmark::State& state) {
   997|  if (state.thread_index() == 0) {
   998|    // Setup code here.
   999|  }
  1000|  for (auto _ : state) {
  1001|    // Run the test as normal.
  1002|  }
  1003|  if (state.thread_index() == 0) {
  1004|    // Teardown code here.
  1005|  }
  1006|}
  1007|BENCHMARK(BM_MultiThreaded)->Threads(2);
  1008|```
  1009|
  1010|To run the benchmark across a range of thread counts, instead of `Threads`, use
  1011|`ThreadRange`. This takes two parameters (`min_threads` and `max_threads`) and
  1012|runs the benchmark once for values in the inclusive range. For example:
  1013|
  1014|```c++
  1015|BENCHMARK(BM_MultiThreaded)->ThreadRange(1, 8);
  1016|```
  1017|
  1018|will run `BM_MultiThreaded` with thread counts 1, 2, 4, and 8.
  1019|
  1020|If the benchmarked code itself uses threads and you want to compare it to
  1021|single-threaded code, you may want to use real-time ("wallclock") measurements
  1022|for latency comparisons:
  1023|
  1024|```c++
  1025|BENCHMARK(BM_test)->Range(8, 8<<10)->UseRealTime();
  1026|```
  1027|
  1028|Without `UseRealTime`, CPU time is used by default.
  1029|
  1030|### Manual Multithreaded Benchmarks
  1031|
  1032|Google/benchmark uses `std::thread` as multithreading environment per default.
  1033|If you want to use another multithreading environment (e.g. OpenMP), you can provide
  1034|a factory function to your benchmark using the `ThreadRunner` function.
  1035|The factory function takes the number of threads as argument and creates a custom class
  1036|derived from `benchmark::ThreadRunnerBase`.
  1037|This custom class must override the function
  1038|`void RunThreads(const std::function<void(int)>& fn)`.
  1039|`RunThreads` is called by the main thread and spawns the requested number of threads.
  1040|Each spawned thread must call `fn(thread_index)`, where `thread_index` is its own
  1041|thread index. Before `RunThreads` returns, all spawned threads must be joined.
  1042|```c++
  1043|class OpenMPThreadRunner : public benchmark::ThreadRunnerBase
  1044|{
  1045|  OpenMPThreadRunner(int num_threads)
  1046|  : num_threads_(num_threads)
  1047|  {}
  1048|
  1049|  void RunThreads(const std::function<void(int)>& fn) final
  1050|  {
  1051|#pragma omp parallel num_threads(num_threads_)
  1052|    fn(omp_get_thread_num());
  1053|  }
  1054|
  1055|private:
  1056|  int num_threads_;
  1057|};
  1058|
  1059|BENCHMARK(BM_MultiThreaded)
  1060|  ->ThreadRunner([](int num_threads) {
  1061|    return std::make_unique<OpenMPThreadRunner>(num_threads);
  1062|  })
  1063|  ->Threads(1)->Threads(2)->Threads(4);
  1064|```
  1065|The above example creates a parallel OpenMP region before it enters `BM_MultiThreaded`.
  1066|The actual benchmark code can remain the same and is therefore not tied to a specific
  1067|thread runner. The measurement does not include the time for creating and joining the
  1068|threads.
  1069|
  1070|<a name="cpu-timers" />
  1071|
  1072|## CPU Timers
  1073|
  1074|By default, the CPU timer only measures the time spent by the main thread.
  1075|If the benchmark itself uses threads internally, this measurement may not
  1076|be what you are looking for. Instead, there is a way to measure the total
  1077|CPU usage of the process, by all the threads.
  1078|
  1079|```c++
  1080|void callee(int i);
  1081|
  1082|static void MyMain(int size) {
  1083|#pragma omp parallel for
  1084|  for(int i = 0; i < size; i++)
  1085|    callee(i);
  1086|}
  1087|
  1088|static void BM_OpenMP(benchmark::State& state) {
  1089|  for (auto _ : state)
  1090|    MyMain(state.range(0));
  1091|}
  1092|
  1093|// Measure the time spent by the main thread, use it to decide for how long to
  1094|// run the benchmark loop. Depending on the internal implementation detail may
  1095|// measure to anywhere from near-zero (the overhead spent before/after work
  1096|// handoff to worker thread[s]) to the whole single-thread time.
  1097|BENCHMARK(BM_OpenMP)->Range(8, 8<<10);
  1098|
  1099|// Measure the user-visible time, the wall clock (literally, the time that
  1100|// has passed on the clock on the wall), use it to decide for how long to
  1101|// run the benchmark loop. This will always be meaningful, and will match the
  1102|// time spent by the main thread in single-threaded case, in general decreasing
  1103|// with the number of internal threads doing the work.
  1104|BENCHMARK(BM_OpenMP)->Range(8, 8<<10)->UseRealTime();
  1105|
  1106|// Measure the total CPU consumption, use it to decide for how long to
  1107|// run the benchmark loop. This will always measure to no less than the
  1108|// time spent by the main thread in single-threaded case.
  1109|BENCHMARK(BM_OpenMP)->Range(8, 8<<10)->MeasureProcessCPUTime();
  1110|
  1111|// A mixture of the last two. Measure the total CPU consumption, but use the
  1112|// wall clock to decide for how long to run the benchmark loop.
  1113|BENCHMARK(BM_OpenMP)->Range(8, 8<<10)->MeasureProcessCPUTime()->UseRealTime();
  1114|```
  1115|
  1116|### Controlling Timers
  1117|
  1118|Normally, the entire duration of the work loop (`for (auto _ : state) {}`)
  1119|is measured. But sometimes, it is necessary to do some work inside of
  1120|that loop, every iteration, but without counting that time to the benchmark time.
  1121|That is possible, although it is not recommended, since it has high overhead.
  1122|
  1123|<!-- {% raw %} -->
  1124|```c++
  1125|static void BM_SetInsert_With_Timer_Control(benchmark::State& state) {
  1126|  std::set<int> data;
  1127|  for (auto _ : state) {
  1128|    state.PauseTiming(); // Stop timers. They will not count until they are resumed.
  1129|    data = ConstructRandomSet(state.range(0)); // Do something that should not be measured
  1130|    state.ResumeTiming(); // And resume timers. They are now counting again.
  1131|    // The rest will be measured.
  1132|    for (int j = 0; j < state.range(1); ++j)
  1133|      data.insert(RandomNumber());
  1134|  }
  1135|}
  1136|BENCHMARK(BM_SetInsert_With_Timer_Control)->Ranges({{1<<10, 8<<10}, {128, 512}});
  1137|```
  1138|<!-- {% endraw %} -->
  1139|
  1140|For convenience, a `ScopedPauseTiming` class is provided to manage pausing and
  1141|resuming timers within a scope. This is less error-prone than manually calling
  1142|`PauseTiming` and `ResumeTiming`.
  1143|
  1144|<!-- {% raw %} -->
  1145|```c++
  1146|static void BM_SetInsert_With_Scoped_Timer_Control(benchmark::State& state) {
  1147|  std::set<int> data;
  1148|  for (auto _ : state) {
  1149|    {
  1150|      benchmark::ScopedPauseTiming pause(state); // Pauses timing
  1151|      data = ConstructRandomSet(state.range(0));
  1152|    } // Timing resumes automatically when 'pause' goes out of scope
  1153|
  1154|    // The rest will be measured.
  1155|    for (int j = 0; j < state.range(1); ++j)
  1156|      data.insert(RandomNumber());
  1157|  }
  1158|}
  1159|BENCHMARK(BM_SetInsert_With_Scoped_Timer_Control)->Ranges({{1<<10, 8<<10}, {128, 512}});
  1160|```
  1161|<!-- {% endraw %} -->
  1162|
  1163|<a name="manual-timing" />
  1164|
  1165|## Manual Timing
  1166|
  1167|For benchmarking something for which neither CPU time nor real-time are
  1168|correct or accurate enough, completely manual timing is supported using
  1169|the `UseManualTime` function.
  1170|
  1171|When `UseManualTime` is used, the benchmarked code must call
  1172|`SetIterationTime` once per iteration of the benchmark loop to
  1173|report the manually measured time.
  1174|
  1175|An example use case for this is benchmarking GPU execution (e.g. OpenCL
  1176|or CUDA kernels, OpenGL or Vulkan or Direct3D draw calls), which cannot
  1177|be accurately measured using CPU time or real-time. Instead, they can be
  1178|measured accurately using a dedicated API, and these measurement results
  1179|can be reported back with `SetIterationTime`.
  1180|
  1181|```c++
  1182|static void BM_ManualTiming(benchmark::State& state) {
  1183|  int microseconds = state.range(0);
  1184|  std::chrono::duration<double, std::micro> sleep_duration {
  1185|    static_cast<double>(microseconds)
  1186|  };
  1187|
  1188|  for (auto _ : state) {
  1189|    auto start = std::chrono::high_resolution_clock::now();
  1190|    // Simulate some useful workload with a sleep
  1191|    std::this_thread::sleep_for(sleep_duration);
  1192|    auto end = std::chrono::high_resolution_clock::now();
  1193|
  1194|    auto elapsed_seconds =
  1195|      std::chrono::duration_cast<std::chrono::duration<double>>(
  1196|        end - start);
  1197|
  1198|    state.SetIterationTime(elapsed_seconds.count());
  1199|  }
  1200|}
  1201|BENCHMARK(BM_ManualTiming)->Range(1, 1<<17)->UseManualTime();
  1202|```
  1203|
  1204|<a name="setting-the-time-unit" />
  1205|
  1206|## Setting the Time Unit
  1207|
  1208|If a benchmark runs a few milliseconds it may be hard to visually compare the
  1209|measured times, since the output data is given in nanoseconds per default. In
  1210|order to manually set the time unit, you can specify it manually:
  1211|
  1212|```c++
  1213|BENCHMARK(BM_test)->Unit(benchmark::kMillisecond);
  1214|```
  1215|
  1216|Additionally the default time unit can be set globally with the
  1217|`--benchmark_time_unit={ns|us|ms|s}` command line argument. The argument only
  1218|affects benchmarks where the time unit is not set explicitly.
  1219|
  1220|<a name="preventing-optimization" />
  1221|
  1222|## Preventing Optimization
  1223|
  1224|To prevent a value or expression from being optimized away by the compiler
  1225|the `benchmark::DoNotOptimize(...)` and `benchmark::ClobberMemory()`
  1226|functions can be used.
  1227|
  1228|```c++
  1229|static void BM_test(benchmark::State& state) {
  1230|  for (auto _ : state) {
  1231|      int x = 0;
  1232|      for (int i=0; i < 64; ++i) {
  1233|        benchmark::DoNotOptimize(x += i);
  1234|      }
  1235|  }
  1236|}
  1237|```
  1238|
  1239|`DoNotOptimize(<expr>)` forces the  *result* of `<expr>` to be stored in either
  1240|memory or a register. For GNU based compilers it acts as read/write barrier
  1241|for global memory. More specifically it forces the compiler to flush pending
  1242|writes to memory and reload any other values as necessary.
  1243|
  1244|Note that `DoNotOptimize(<expr>)` does not prevent optimizations on `<expr>`
  1245|in any way. `<expr>` may even be removed entirely when the result is already
  1246|known. For example:
  1247|
  1248|```c++
  1249|  // Example 1: `<expr>` is removed entirely.
  1250|  int foo(int x) { return x + 42; }
  1251|  while (...) DoNotOptimize(foo(0)); // Optimized to DoNotOptimize(42);
  1252|
  1253|  // Example 2: Result of '<expr>' is only reused.
  1254|  int bar(int) __attribute__((const));
  1255|  while (...) DoNotOptimize(bar(0)); // Optimized to:
  1256|  // int __result__ = bar(0);
  1257|  // while (...) DoNotOptimize(__result__);
  1258|```
  1259|
  1260|The second tool for preventing optimizations is `ClobberMemory()`. In essence
  1261|`ClobberMemory()` forces the compiler to perform all pending writes to global
  1262|memory. Memory managed by block scope objects must be "escaped" using
  1263|`DoNotOptimize(...)` before it can be clobbered. In the below example
  1264|`ClobberMemory()` prevents the call to `v.push_back(42)` from being optimized
  1265|away.
  1266|
  1267|```c++
  1268|static void BM_vector_push_back(benchmark::State& state) {
  1269|  for (auto _ : state) {
  1270|    std::vector<int> v;
  1271|    v.reserve(1);
  1272|    auto data = v.data();           // Allow v.data() to be clobbered. Pass as non-const
  1273|    benchmark::DoNotOptimize(data); // lvalue to avoid undesired compiler optimizations
  1274|    v.push_back(42);
  1275|    benchmark::ClobberMemory(); // Force 42 to be written to memory.
  1276|  }
  1277|}
  1278|```
  1279|
  1280|Note that `ClobberMemory()` is only available for GNU or MSVC based compilers.
  1281|
  1282|<a name="reporting-statistics" />
  1283|
  1284|## Statistics: Reporting the Mean, Median and Standard Deviation / Coefficient of variation of Repeated Benchmarks
  1285|
  1286|By default each benchmark is run once and that single result is reported.
  1287|However benchmarks are often noisy and a single result may not be representative
  1288|of the overall behavior. For this reason it's possible to repeatedly rerun the
  1289|benchmark.
  1290|
  1291|The number of runs of each benchmark is specified globally by the
  1292|`--benchmark_repetitions` flag or on a per benchmark basis by calling
  1293|`Repetitions` on the registered benchmark object. When a benchmark is run more
  1294|than once the mean, median, standard deviation and coefficient of variation
  1295|of the runs will be reported.
  1296|
  1297|Additionally the `--benchmark_report_aggregates_only={true|false}`,
  1298|`--benchmark_display_aggregates_only={true|false}` flags or
  1299|`ReportAggregatesOnly(bool)`, `DisplayAggregatesOnly(bool)` functions can be
  1300|used to change how repeated tests are reported. By default the result of each
  1301|repeated run is reported. When `report aggregates only` option is `true`,
  1302|only the aggregates (i.e. mean, median, standard deviation and coefficient
  1303|of variation, maybe complexity measurements if they were requested) of the runs
  1304|is reported, to both the reporters - standard output (console), and the file.
  1305|However when only the `display aggregates only` option is `true`,
  1306|only the aggregates are displayed in the standard output, while the file
  1307|output still contains everything.
  1308|Calling `ReportAggregatesOnly(bool)` / `DisplayAggregatesOnly(bool)` on a
  1309|registered benchmark object overrides the value of the appropriate flag for that
  1310|benchmark.
  1311|
  1312|<a name="custom-statistics" />
  1313|
  1314|## Custom Statistics
  1315|
  1316|While having these aggregates is nice, this may not be enough for everyone.
  1317|For example you may want to know what the largest observation is, e.g. because
  1318|you have some real-time constraints. This is easy. The following code will
  1319|specify a custom statistic to be calculated, defined by a lambda function.
  1320|
  1321|```c++
  1322|void BM_spin_empty(benchmark::State& state) {
  1323|  for (auto _ : state) {
  1324|    for (int x = 0; x < state.range(0); ++x) {
  1325|      benchmark::DoNotOptimize(x);
  1326|    }
  1327|  }
  1328|}
  1329|
  1330|BENCHMARK(BM_spin_empty)
  1331|  ->Repetitions(3) // or add option --benchmark_repetitions=3
  1332|  ->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
  1333|    return *(std::max_element(std::begin(v), std::end(v)));
  1334|  })
  1335|  ->Arg(512);
  1336|```
  1337|
  1338|While usually the statistics produce values in time units,
  1339|you can also produce percentages:
  1340|
  1341|```c++
  1342|void BM_spin_empty(benchmark::State& state) {
  1343|  for (auto _ : state) {
  1344|    for (int x = 0; x < state.range(0); ++x) {
  1345|      benchmark::DoNotOptimize(x);
  1346|    }
  1347|  }
  1348|}
  1349|
  1350|BENCHMARK(BM_spin_empty)
  1351|  ->Repetitions(3) // or add option --benchmark_repetitions=3
  1352|  ->ComputeStatistics("ratio", [](const std::vector<double>& v) -> double {
  1353|    return v.front() / v.back();
  1354|  }, benchmark::StatisticUnit::kPercentage)
  1355|  ->Arg(512);
  1356|```
  1357|
  1358|<a name="memory-usage" />
  1359|
  1360|## Memory Usage
  1361|
  1362|It's often useful to also track memory usage for benchmarks, alongside CPU
  1363|performance. For this reason, benchmark offers the `RegisterMemoryManager`
  1364|method that allows a custom `MemoryManager` to be injected.
  1365|
  1366|If set, the `MemoryManager::Start` and `MemoryManager::Stop` methods will be
  1367|called at the start and end of benchmark runs to allow user code to fill out
  1368|a report on the number of allocations, bytes used, etc.
  1369|
  1370|This data will then be reported alongside other performance data, currently
  1371|only when using JSON output.
  1372|
  1373|<a name="profiling" />
  1374|
  1375|## Profiling
  1376|
  1377|It's often useful to also profile benchmarks in particular ways, in addition to
  1378|CPU performance. For this reason, benchmark offers the `RegisterProfilerManager`
  1379|method that allows a custom `ProfilerManager` to be injected.
  1380|
  1381|If set, the `ProfilerManager::AfterSetupStart` and
  1382|`ProfilerManager::BeforeTeardownStop` methods will be called at the start and
  1383|end of a separate benchmark run to allow user code to collect and report
  1384|user-provided profile metrics.
  1385|
  1386|Output collected from this profiling run must be reported separately.
  1387|
  1388|<a name="using-register-benchmark" />
  1389|
  1390|## Using RegisterBenchmark(name, fn, args...)
  1391|
  1392|The `RegisterBenchmark(name, func, args...)` function provides an alternative
  1393|way to create and register benchmarks.
  1394|`RegisterBenchmark(name, func, args...)` creates, registers, and returns a
  1395|pointer to a new benchmark with the specified `name` that invokes
  1396|`func(st, args...)` where `st` is a `benchmark::State` object.
  1397|
  1398|Unlike the `BENCHMARK` registration macros, which can only be used at the global
  1399|scope, the `RegisterBenchmark` can be called anywhere. This allows for
  1400|benchmark tests to be registered programmatically.
  1401|
  1402|Additionally `RegisterBenchmark` allows any callable object to be registered
  1403|as a benchmark. Including capturing lambdas and function objects.
  1404|
  1405|For Example:
  1406|```c++
  1407|auto BM_test = [](benchmark::State& st, auto Inputs) { /* ... */ };
  1408|
  1409|int main(int argc, char** argv) {
  1410|  benchmark::MaybeReenterWithoutASLR(argc, argv);
  1411|  for (auto& test_input : { /* ... */ })
  1412|      benchmark::RegisterBenchmark(test_input.name(), BM_test, test_input);
  1413|  benchmark::Initialize(&argc, argv);
  1414|  benchmark::RunSpecifiedBenchmarks();
  1415|  benchmark::Shutdown();
  1416|}
  1417|```
  1418|
  1419|<a name="exiting-with-an-error" />
  1420|
  1421|## Exiting with an Error
  1422|
  1423|When errors caused by external influences, such as file I/O and network
  1424|communication, occur within a benchmark the
  1425|`State::SkipWithError(const std::string& msg)` function can be used to skip that run
  1426|of benchmark and report the error. Note that only future iterations of the
  1427|`KeepRunning()` are skipped. For the ranged-for version of the benchmark loop
  1428|Users must explicitly exit the loop, otherwise all iterations will be performed.
  1429|Users may explicitly return to exit the benchmark immediately.
  1430|
  1431|The `SkipWithError(...)` function may be used at any point within the benchmark,
  1432|including before and after the benchmark loop. Moreover, if `SkipWithError(...)`
  1433|has been used, it is not required to reach the benchmark loop and one may return
  1434|from the benchmark function early.
  1435|
  1436|For example:
  1437|
  1438|```c++
  1439|static void BM_test(benchmark::State& state) {
  1440|  auto resource = GetResource();
  1441|  if (!resource.good()) {
  1442|    state.SkipWithError("Resource is not good!");
  1443|    // KeepRunning() loop will not be entered.
  1444|  }
  1445|  while (state.KeepRunning()) {
  1446|    auto data = resource.read_data();
  1447|    if (!resource.good()) {
  1448|      state.SkipWithError("Failed to read data!");
  1449|      break; // Needed to skip the rest of the iteration.
  1450|    }
  1451|    do_stuff(data);
  1452|  }
  1453|}
  1454|
  1455|static void BM_test_ranged_fo(benchmark::State & state) {
  1456|  auto resource = GetResource();
  1457|  if (!resource.good()) {
  1458|    state.SkipWithError("Resource is not good!");
  1459|    return; // Early return is allowed when SkipWithError() has been used.
  1460|  }
  1461|  for (auto _ : state) {
  1462|    auto data = resource.read_data();
  1463|    if (!resource.good()) {
  1464|      state.SkipWithError("Failed to read data!");
  1465|      break; // REQUIRED to prevent all further iterations.
  1466|    }
  1467|    do_stuff(data);
  1468|  }
  1469|}
  1470|```
  1471|<a name="a-faster-keep-running-loop" />
  1472|
  1473|## A Faster KeepRunning Loop
  1474|
  1475|A ranged-based for loop should be used in preference to
  1476|the `KeepRunning` loop for running the benchmarks. For example:
  1477|
  1478|```c++
  1479|static void BM_Fast(benchmark::State &state) {
  1480|  for (auto _ : state) {
  1481|    FastOperation();
  1482|  }
  1483|}
  1484|BENCHMARK(BM_Fast);
  1485|```
  1486|
  1487|The reason the ranged-for loop is faster than using `KeepRunning`, is
  1488|because `KeepRunning` requires a memory load and store of the iteration count
  1489|ever iteration, whereas the ranged-for variant is able to keep the iteration count
  1490|in a register.
  1491|
  1492|For example, an empty inner loop of using the ranged-based for method looks like:
  1493|
  1494|```asm
  1495|# Loop Init
  1496|  mov rbx, qword ptr [r14 + 104]
  1497|  call benchmark::State::StartKeepRunning()
  1498|  test rbx, rbx
  1499|  je .LoopEnd
  1500|.LoopHeader: # =>This Inner Loop Header: Depth=1
  1501|  add rbx, -1
  1502|  jne .LoopHeader
  1503|.LoopEnd:
  1504|```
  1505|
  1506|Compared to an empty `KeepRunning` loop, which looks like:
  1507|
  1508|```asm
  1509|.LoopHeader: # in Loop: Header=BB0_3 Depth=1
  1510|  cmp byte ptr [rbx], 1
  1511|  jne .LoopInit
  1512|.LoopBody: # =>This Inner Loop Header: Depth=1
  1513|  mov rax, qword ptr [rbx + 8]
  1514|  lea rcx, [rax + 1]
  1515|  mov qword ptr [rbx + 8], rcx
  1516|  cmp rax, qword ptr [rbx + 104]
  1517|  jb .LoopHeader
  1518|  jmp .LoopEnd
  1519|.LoopInit:
  1520|  mov rdi, rbx
  1521|  call benchmark::State::StartKeepRunning()
  1522|  jmp .LoopBody
  1523|.LoopEnd:
  1524|```
  1525|
  1526|Unless C++03 compatibility is required, the ranged-for variant of writing
  1527|the benchmark loop should be preferred.
  1528|
  1529|<a name="disabling-cpu-frequency-scaling" />
  1530|
  1531|## Disabling CPU Frequency Scaling
  1532|
  1533|If you see this error:
  1534|
  1535|```
  1536|***WARNING*** CPU scaling is enabled, the benchmark real time measurements may
  1537|be noisy and will incur extra overhead.
  1538|```
  1539|
  1540|you might want to disable the CPU frequency scaling while running the
  1541|benchmark, as well as consider other ways to stabilize the performance of
  1542|your system while benchmarking.
  1543|
  1544|See [Reducing Variance](reducing_variance.md) for more information.
  1545|
  1546|