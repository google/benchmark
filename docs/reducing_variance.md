# Reducing Variance

<a name="disabling-cpu-frequency-scaling" />

## Disabling CPU Frequency Scaling

If you see this error:

```
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
```

you might want to disable the CPU frequency scaling while running the
benchmark, as well as consider other ways to stabilize the performance of
your system while benchmarking.

Exactly how to do this depends on the Linux distribution,
desktop environment, and installed programs.  Specific details are a moving
target, so we will not attempt to exhaustively document them here.

One simple option is to use the `cpupower` program to change the
performance governor to "performance".  This tool is maintained along with
the Linux kernel and provided by your distribution.

It must be run as root, like this:

```bash
sudo cpupower frequency-set --governor performance
```

After this you can verify that all CPUs are using the performance governor
by running this command:

```bash
cpupower frequency-info -o proc
```

The benchmarks you subsequently run will have less variance.

<a name="reducing-variance" />

## Disabling ASLR

If you see this error:

```
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
```

you might want to disable the ASLR security hardening feature while running the
benchmark.

The simplest way is to add
```
benchmark::MaybeReenterWithoutASLR(argc, argv);
```
as the first line of your `main()` function. It will try to disable ASLR
for the current processor, and, if successful, re-execute the binary.
Note that `personality(2)` may be forbidden by e.g. seccomp (which happens
by default if you are running in a Docker container).

Note that if you link to `benchmark_main` already does that for you.

To globally disable ASLR on Linux, run
```
echo 0 > /proc/sys/kernel/randomize_va_space
```

To run a single benchmark with ASLR disabled on Linux, do:
```
setarch `uname -m` -R ./a_benchmark
```

Note that for the information on how to disable ASLR on other operating systems,
please refer to their documentation.

## Reducing Variance in Benchmarks

The Linux CPU frequency governor [discussed
above](user_guide#disabling-cpu-frequency-scaling) is not the only source
of noise in benchmarks.  The advice in this section assumes a desktop or
server; on a mobile device most of it does not apply, and the dominant
sources of variance are different ones, so see [Reducing Variance on
Android](#reducing-variance-on-android).  Some, but not all, of the sources
of variance include:

1. On multi-core machines not all CPUs/CPU cores/CPU threads run the same
   speed, so running a benchmark one time and then again may give a
   different result depending on which CPU it ran on.
2. CPU scaling features that run on the CPU, like Intel's Turbo Boost and
   AMD Turbo Core and Precision Boost, can temporarily change the CPU
   frequency even when the using the "performance" governor on Linux.
3. Context switching between CPUs, or scheduling competition on the CPU the
   benchmark is running on.
4. Intel Hyperthreading or AMD SMT causing the same issue as above.
5. Cache effects caused by code running on other CPUs.
6. Non-uniform memory architectures (NUMA).

These can cause variance in benchmarks results within a single run
(`--benchmark_repetitions=N`) or across multiple runs of the benchmark
program.

Reducing sources of variance is OS and architecture dependent, which is one
reason some companies maintain machines dedicated to performance testing.

Some of the easier and effective ways of reducing variance on a typical
Linux workstation are:

1. Use the performance governor as [discussed
above](user_guide#disabling-cpu-frequency-scaling).
2. Disable processor boosting by:
   ```sh
   echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost
   ```
   See the Linux kernel's
   [boost.txt](https://www.kernel.org/doc/Documentation/cpu-freq/boost.txt)
   for more information.
3. Set the benchmark program's task affinity to a fixed cpu.  For example:
   ```sh
   taskset -c 0 ./mybenchmark
   ```
4. Increase the program's scheduling priority to minimize context switches using `nice` or `chrt`:
   ```sh
   sudo nice -n -20 ./mybenchmark
   sudo chrt -f 80 ./mybenchmark
   ```
5. Disabling Hyperthreading/SMT.  This can be done in the Bios or using the
   `/sys` file system (see the LLVM project's [Benchmarking
   tips](https://llvm.org/docs/Benchmarking.html)).
6. Close other programs that do non-trivial things based on timers, such as
   your web browser, desktop environment, etc.
7. Reduce the working set of your benchmark to fit within the L1 cache, but
   do be aware that this may lead you to optimize for an unrealistic
   situation.

Further resources on this topic:

1. The LLVM project's [Benchmarking
   tips](https://llvm.org/docs/Benchmarking.html).
1. The Arch Wiki [Cpu frequency
scaling](https://wiki.archlinux.org/title/CPU_frequency_scaling) page.

<a name="reducing-variance-on-android" />

## Reducing Variance on Android

On Android there is no `cpupower`, `scaling_governor` is often not readable or writable by
the `shell` user, and there is no BIOS in which to disable SMT. None of the following needs
root.

1. **Wait for background application compilation to finish.** Android compiles installed
   applications ahead of time in a background job gated on the device being idle and
   charging, which is exactly the state you create in order to benchmark it. The job runs
   several compiler threads at `nice` 10, so it yields the CPU readily and looks harmless,
   but a nice'd process does not yield memory bandwidth or cache. On one device a
   memory-bandwidth microbenchmark measured 6.35 GB/s while that job ran and 9.32 GB/s once
   it had finished, 32% lower, with repetition-to-repetition variation of 15% in both cases.
   Because the job runs for the whole measurement it depresses every repetition by a similar
   amount, so a check based on the spread between repetitions does not catch it. Check for
   it explicitly and record the result:
   ```sh
   adb shell pgrep dex2oat
   ```
   If that prints anything, wait. `pgrep` is deliberate here: listing every thread with
   `ps -A -T` is enough work on a device with a few thousand threads that it can leave
   `adbd` unresponsive, which is a poor way to find out your device is busy. The compiler
   threads belong to `dex2oat`, so matching the process is sufficient.

   On a retail device there is no reliable way to *stop* it:
   `cmd package cancel-bg-dexopt-job` and `cmd package bg-dexopt-job --disable` are not
   available on every build, and `cmd jobscheduler cancel android 800` cancels the AOSP job
   while vendor-scheduled compilation continues. A `userdebug` build or a rooted device can
   stop the framework outright with `adb shell stop`, which is the stronger remedy where it
   is available.

   You can, however, *wait* for it programmatically, which is what a harness should do
   rather than leaving it to the operator to remember. This blocks until the device has
   been quiet for three consecutive checks, and combines it with item 2 below:
   ```sh
   adb shell '
     busy() {  # percentage of total CPU capacity busy over a 3 second window
       set -- $(head -1 /proc/stat); t0=$(($2+$3+$4+$5+$6+$7+$8+$9)); i0=$(($5+$6))
       sleep 3
       set -- $(head -1 /proc/stat); t1=$(($2+$3+$4+$5+$6+$7+$8+$9)); i1=$(($5+$6))
       d=$((t1-t0)); echo $(( 100 * (d - (i1-i0)) / d ))
     }
     n=0
     while [ $n -lt 3 ]; do
       if [ -z "$(pgrep dex2oat)" ] && [ "$(busy)" -le 10 ]; then n=$((n+1)); else n=0; fi
     done'
   ```
2. **Measure whether the device is quiet rather than assuming it.** Sample `/proc/stat`
   twice and compute the fraction of CPU capacity that was busy over the window. Do not use
   the load average: on Android it counts threads blocked in `D` state, and it read 19 to 21
   on a device whose measured busy fraction was 3%.
3. **Record the device power mode, and do not assume what it does.**
   `settings put global low_power 1` can leave `scaling_max_freq` at its full value, so
   reading the frequency cap does not tell you the mode is on:
   ```sh
   adb shell settings get global low_power
   ```
   Record it and compare rather than assuming a direction. On the device above, measured on
   an idle machine with the state captured before and after every run, the mode made no
   difference worth reporting, although earlier runs on that same phone appeared to show it
   costing 33% before background compilation and post-boot settling were ruled out.
4. **Let the device settle after a reboot.** Measurements taken four minutes after a restart
   were markedly less reproducible than the same measurements on the same device later.
5. **Do not use every core.** Android needs cores for itself, so a benchmark with one thread
   per core competes with the system, and a workload whose threads synchronise at a barrier
   degrades sharply when one thread is descheduled. On an eight-core device a memory-bound
   benchmark peaked at six threads.
6. **Be careful with temperature.** `/sys/class/thermal` generally needs root, which leaves
   `dumpsys battery`, and battery temperature is a poor proxy for SoC temperature. On a
   device at full charge it tracks the battery finishing its charge: during six minutes of
   all-core compute it fell steadily while throughput stayed flat. If you report a
   temperature, say which sensor it is.
