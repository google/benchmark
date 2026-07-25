#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Lifecycle ----

void benchmark_zig_initialize(int* argc, char** argv);
size_t benchmark_zig_run(void);
void benchmark_zig_clear_registered_benchmarks(void);
void benchmark_zig_add_custom_context(const char* key, const char* value);

// ---- Benchmark registration ----

typedef void (*benchmark_zig_fn)(void* state);
void* benchmark_zig_register_benchmark(const char* name, benchmark_zig_fn fn);

// ---- Benchmark configuration (void return — Zig returns self for chaining) ----

void benchmark_zig_benchmark_arg(void* benchmark, int64_t x);
void benchmark_zig_benchmark_range(void* benchmark, int64_t start, int64_t limit);
void benchmark_zig_benchmark_dense_range(void* benchmark, int64_t start, int64_t limit, int step);
void benchmark_zig_benchmark_args(void* benchmark, const int64_t* args, size_t len);
void benchmark_zig_benchmark_unit(void* benchmark, int unit);
void benchmark_zig_benchmark_threads(void* benchmark, int t);
void benchmark_zig_benchmark_thread_range(void* benchmark, int min_threads, int max_threads);
void benchmark_zig_benchmark_min_time(void* benchmark, double t);
void benchmark_zig_benchmark_iterations(void* benchmark, int64_t n);
void benchmark_zig_benchmark_repetitions(void* benchmark, int n);
void benchmark_zig_benchmark_use_real_time(void* benchmark);
void benchmark_zig_benchmark_use_manual_time(void* benchmark);
void benchmark_zig_benchmark_complexity(void* benchmark, int complexity);
const char* benchmark_zig_benchmark_name(void* benchmark);

// ---- State methods ----

bool benchmark_zig_state_keep_running(void* state);
bool benchmark_zig_state_keep_running_batch(void* state, int64_t n);
void benchmark_zig_state_pause_timing(void* state);
void benchmark_zig_state_resume_timing(void* state);
void benchmark_zig_state_skip_with_error(void* state, const char* msg);
void benchmark_zig_state_set_bytes_processed(void* state, int64_t bytes);
void benchmark_zig_state_set_items_processed(void* state, int64_t items);
void benchmark_zig_state_set_label(void* state, const char* label);
void benchmark_zig_state_set_complexity_n(void* state, int64_t n);
int64_t benchmark_zig_state_range(void* state, size_t pos);
int64_t benchmark_zig_state_iterations(void* state);
int benchmark_zig_state_threads(void* state);
int benchmark_zig_state_thread_index(void* state);

#ifdef __cplusplus
}
#endif
