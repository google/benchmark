use std::pin::Pin;

#[cxx::bridge]
mod ffi {
    // Opaque handle to benchmark::State owned by the C++ runner.
    #[namespace = "benchmark"]
    unsafe extern "C++" {
        type State;
    }

    // Thin wrapper functions declared in bridge.h / bridge.cc.
    // These exist because State::KeepRunning() is always-inlined in the C++
    // header and therefore has no linkable symbol for cxx to call directly.
    #[namespace = "benchmark_bridge"]
    unsafe extern "C++" {
        include!("bridge.h");

        fn bm_initialize(args: &[&str]);
        fn bm_keep_running(state: Pin<&mut State>) -> bool;
        fn bm_skip_with_error(state: Pin<&mut State>, msg: &str);
        fn bm_register(name: &str, f: fn(Pin<&mut State>));
        fn bm_run() -> usize;
    }
}

pub use ffi::State;

/// Safe wrapper around a `benchmark::State` reference for the duration of one
/// benchmark invocation.
pub struct BenchState<'a>(Pin<&'a mut State>);

impl<'a> BenchState<'a> {
    pub fn from_pin(state: Pin<&'a mut State>) -> Self {
        BenchState(state)
    }

    /// Returns `true` while the benchmark runner wants more iterations.
    pub fn keep_running(&mut self) -> bool {
        ffi::bm_keep_running(self.0.as_mut())
    }

    /// Abort this benchmark run with an error message.
    pub fn skip_with_error(&mut self, msg: &str) {
        ffi::bm_skip_with_error(self.0.as_mut(), msg);
    }
}

/// Parse benchmark flags from `args` (typically `std::env::args()`).
pub fn initialize(args: &[&str]) {
    ffi::bm_initialize(args);
}

/// Register a benchmark function under `name`.
///
/// `f` must be a non-capturing function or a closure that coerces to a
/// function pointer (i.e. it captures nothing from the environment).
/// Use [`BenchState::from_pin`] inside `f` to access timing controls.
pub fn register_benchmark(name: &str, f: fn(Pin<&mut State>)) {
    ffi::bm_register(name, f);
}

/// Run all registered benchmarks.  Returns the number of benchmarks run.
pub fn run_specified_benchmarks() -> usize {
    ffi::bm_run()
}
