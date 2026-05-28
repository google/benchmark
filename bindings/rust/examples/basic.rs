use google_benchmark::{BenchState, State};
use std::pin::Pin;

fn bm_vector_push(state: Pin<&mut State>) {
    let mut s = BenchState::from_pin(state);
    while s.keep_running() {
        std::hint::black_box(Vec::<u32>::new());
    }
}

fn bm_string_copy(state: Pin<&mut State>) {
    let src = String::from("hello, benchmark");
    let mut s = BenchState::from_pin(state);
    while s.keep_running() {
        std::hint::black_box(src.clone());
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let args_ref: Vec<&str> = args.iter().map(String::as_str).collect();
    google_benchmark::initialize(&args_ref);

    google_benchmark::register_benchmark("BM_VectorPush", bm_vector_push);
    google_benchmark::register_benchmark("BM_StringCopy", bm_string_copy);

    google_benchmark::run_specified_benchmarks();
}
