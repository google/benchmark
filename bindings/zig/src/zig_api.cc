#include "zig_api.h"

#include <string>
#include <vector>

#include "benchmark/benchmark.h"

// ---- Lifecycle ----

void benchmark_zig_initialize(int* argc, char** argv) {
  ::benchmark::Initialize(argc, argv);
}

size_t benchmark_zig_run(void) {
  return ::benchmark::RunSpecifiedBenchmarks();
}

void benchmark_zig_clear_registered_benchmarks(void) {
  ::benchmark::ClearRegisteredBenchmarks();
}

void benchmark_zig_add_custom_context(const char* key, const char* value) {
  ::benchmark::AddCustomContext(key, value);
}

// ---- Benchmark registration ----

void* benchmark_zig_register_benchmark(const char* name, benchmark_zig_fn fn) {
  return ::benchmark::RegisterBenchmark(
      name, [fn](benchmark::State& st) { fn(&st); });
}

// ---- Benchmark configuration ----

void* benchmark_zig_benchmark_arg(void* b, int64_t x) {
  return static_cast<benchmark::Benchmark*>(b)->Arg(x);
}

void* benchmark_zig_benchmark_range(void* b, int64_t start, int64_t limit) {
  return static_cast<benchmark::Benchmark*>(b)->Range(start, limit);
}

void* benchmark_zig_benchmark_dense_range(void* b, int64_t start, int64_t limit,
                                          int step) {
  return static_cast<benchmark::Benchmark*>(b)->DenseRange(start, limit, step);
}

void* benchmark_zig_benchmark_args(void* b, const int64_t* args, size_t len) {
  std::vector<int64_t> v(args, args + len);
  return static_cast<benchmark::Benchmark*>(b)->Args(v);
}

void* benchmark_zig_benchmark_unit(void* b, int unit) {
  return static_cast<benchmark::Benchmark*>(b)->Unit(
      static_cast<benchmark::TimeUnit>(unit));
}

void* benchmark_zig_benchmark_threads(void* b, int t) {
  return static_cast<benchmark::Benchmark*>(b)->Threads(t);
}

void* benchmark_zig_benchmark_thread_range(void* b, int min_threads,
                                           int max_threads) {
  return static_cast<benchmark::Benchmark*>(b)->ThreadRange(min_threads,
                                                            max_threads);
}

void* benchmark_zig_benchmark_min_time(void* b, double t) {
  return static_cast<benchmark::Benchmark*>(b)->MinTime(t);
}

void* benchmark_zig_benchmark_iterations(void* b, int64_t n) {
  return static_cast<benchmark::Benchmark*>(b)->Iterations(n);
}

void* benchmark_zig_benchmark_repetitions(void* b, int n) {
  return static_cast<benchmark::Benchmark*>(b)->Repetitions(n);
}

void* benchmark_zig_benchmark_use_real_time(void* b) {
  return static_cast<benchmark::Benchmark*>(b)->UseRealTime();
}

void* benchmark_zig_benchmark_use_manual_time(void* b) {
  return static_cast<benchmark::Benchmark*>(b)->UseManualTime();
}

void* benchmark_zig_benchmark_complexity(void* b, int complexity) {
  return static_cast<benchmark::Benchmark*>(b)->Complexity(
      static_cast<benchmark::BigO>(complexity));
}

const char* benchmark_zig_benchmark_name(void* b) {
  return static_cast<benchmark::Benchmark*>(b)->GetName();
}

// ---- State methods ----

bool benchmark_zig_state_keep_running(void* s) {
  return static_cast<benchmark::State*>(s)->KeepRunning();
}

bool benchmark_zig_state_keep_running_batch(void* s, int64_t n) {
  return static_cast<benchmark::State*>(s)->KeepRunningBatch(n);
}

void benchmark_zig_state_pause_timing(void* s) {
  static_cast<benchmark::State*>(s)->PauseTiming();
}

void benchmark_zig_state_resume_timing(void* s) {
  static_cast<benchmark::State*>(s)->ResumeTiming();
}

void benchmark_zig_state_skip_with_error(void* s, const char* msg) {
  static_cast<benchmark::State*>(s)->SkipWithError(msg);
}

void benchmark_zig_state_set_bytes_processed(void* s, int64_t bytes) {
  static_cast<benchmark::State*>(s)->SetBytesProcessed(bytes);
}

void benchmark_zig_state_set_items_processed(void* s, int64_t items) {
  static_cast<benchmark::State*>(s)->SetItemsProcessed(items);
}

void benchmark_zig_state_set_label(void* s, const char* label) {
  static_cast<benchmark::State*>(s)->SetLabel(label);
}

void benchmark_zig_state_set_complexity_n(void* s, int64_t n) {
  static_cast<benchmark::State*>(s)->SetComplexityN(n);
}

int64_t benchmark_zig_state_range(void* s, size_t pos) {
  return static_cast<benchmark::State*>(s)->range(pos);
}

int64_t benchmark_zig_state_iterations(void* s) {
  return static_cast<benchmark::State*>(s)->iterations();
}

int benchmark_zig_state_threads(void* s) {
  return static_cast<benchmark::State*>(s)->threads();
}

int benchmark_zig_state_thread_index(void* s) {
  return static_cast<benchmark::State*>(s)->thread_index();
}
