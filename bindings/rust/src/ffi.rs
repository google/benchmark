#[cxx::bridge]
pub mod ffi {
    #[namespace = "benchmark"]
    unsafe extern "C++" {
        include!("benchmark/benchmark.h");

        type State;

        fn KeepRunning(self: Pin<&mut State>) -> bool;
        fn RunSpecifiedBenchmarks() -> usize;
    }

    #[namespace = "benchmark::rust_api"]
    unsafe extern "C++" {
        include!("rust_api.h");

        unsafe fn SkipWithError(state: Pin<&mut State>, msg: &str);
        unsafe fn RegisterBenchmark(name: &str, func: fn(Pin<&mut State>));
        unsafe fn Initialize(argc: *mut i32, argv: usize);
    }
}
