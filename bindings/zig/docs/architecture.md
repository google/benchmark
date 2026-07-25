# Architecture

## Layer Diagram

```
┌─────────────────────────────────────────────────┐
│                  Zig Application                 │
│  benchmark.registerBenchmark("BM_Foo", foo)     │
│  benchmark.run()                                │
└───────────────────┬─────────────────────────────┘
                    │ @cImport / extern functions
┌───────────────────▼─────────────────────────────┐
│              benchmark.zig (Zig API)             │
│  State, Benchmark, TimeUnit — idiomatic wrappers │
│  comptime trampoline for callbacks               │
└───────────────────┬─────────────────────────────┘
                    │ C function calls
┌───────────────────▼─────────────────────────────┐
│           zig_api.h / zig_api.cc (C adapter)    │
│  extern "C" functions wrapping C++ methods       │
│  void* opaque pointers for State & Benchmark     │
└───────────────────┬─────────────────────────────┘
                    │ #include "benchmark/benchmark.h"
┌───────────────────▼─────────────────────────────┐
│           libbenchmark.so / .a (C++)             │
│  benchmark::Initialize, RunSpecifiedBenchmarks,  │
│  RegisterBenchmark, State, Benchmark classes     │
└─────────────────────────────────────────────────┘
```

## Why Three Layers?

The google-benchmark library is pure C++. Zig has zero-cost C interop but cannot call C++ directly (name mangling, classes, exceptions, templates). The C adapter layer:

1. Provides an `extern "C"` interface that Zig can call via `@cImport`
2. Casts `void*` back to C++ types internally
3. Keeps the Zig side simple — no C++ knowledge required from the user

This is the same pattern used by the Rust bindings (which use `cxx` for similar reasons).

## Callback Trampoline

When a benchmark function is registered, the flow is:

```
Zig: registerBenchmark("BM_foo", my_fn)
  → Zig generates a comptime trampoline: S.trampoline
  → calls c.benchmark_zig_register_benchmark("BM_foo", &S.trampoline)

C++ adapter:
  → benchmark::RegisterBenchmark("BM_foo", lambda)
  → lambda captures the C function pointer
  → when benchmark runs, lambda calls fn(&state) with State* cast to void*

Zig trampoline:
  → receives void* (the State*)
  → wraps it in Zig State struct
  → calls user's function
```

The trampoline is generated at compile time per benchmark function, avoiding heap allocation and dynamic dispatch.

## Opaque Pointers

`State` and `Benchmark` are C++ classes with complex internal state. Rather than replicating their layout in Zig (which would be fragile and tie the binding to a specific library version), we pass them as `void*` through the C boundary:

- Zig: `State { ptr: *anyopaque }` — thin wrapper, methods call C functions
- C adapter: `static_cast<benchmark::State*>(s)->Method()` — safe downcast
- The pointers are never dereferenced from Zig code

This makes the binding resilient to internal changes in libbenchmark.

## String Conversion

Zig strings are `(pointer, length)` pairs. C strings are null-terminated. At the boundary:

- **Zig → C**: Functions accept `[*:0]const u8` (sentinel-terminated). The caller ensures null termination.
- **C → Zig**: `benchmark_zig_benchmark_name()` returns `const char*`. The Zig wrapper uses `std.mem.span()` to convert.

For user-provided strings (e.g., `registerBenchmark`, `addCustomContext`), the convention is to accept `[*:0]const u8`, which Zig enforces as null-terminated at compile time.

## Build System Flow

```
zig build
  → cmake -S ../../ -B cmake-build (builds libbenchmark)
  → compiles zig_api.cc as C++
  → links libbenchmark.a + zig_api.o → libzig_api.a
  → compiles benchmark.zig + benchmark_test.zig
  → produces test binary
```

Alternatively, `-Dbenchmark_path=/path` skips the CMake step and links against a pre-installed library.

## Thread Safety

- google-benchmark's `State` is thread-local by design — each thread gets its own `State` instance
- The C adapter uses no global mutable state
- The Zig trampoline function is `comptime`-generated per benchmark, so no shared state
- Safe to register benchmarks from multiple threads (though this is uncommon)

## Error Handling

- C++ exceptions from libbenchmark are caught at the C adapter boundary (the adapter compiles with `-fno-exceptions` if the library is built without exceptions)
- `SkipWithError` translates to a Zig-level skip (no error propagation needed)
- Allocation failures in Zig are handled with `catch` at the call site
