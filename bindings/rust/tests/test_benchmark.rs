use google_benchmark_rs::{initialize, register_benchmark, run_specified_benchmarks, State};

fn my_benchmark(state: &mut State) {
    while state.keep_running() {
        // do nothing
    }
}

#[test]
fn test_bindings() {
    let args = vec!["--benchmark_format=console".to_string(), "--benchmark_min_time=0.01".to_string()];
    initialize(&args);
    register_benchmark!("BM_MyBenchmark", my_benchmark);
    let count = run_specified_benchmarks();
    assert!(count > 0);
}
