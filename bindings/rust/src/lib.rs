pub mod ffi;

use std::ffi::CString;
use std::os::raw::c_char;
use std::pin::Pin;

pub struct State<'a> {
    #[doc(hidden)]
    pub inner: Pin<&'a mut ffi::ffi::State>,
}

impl<'a> State<'a> {
    /// Returns true if the benchmark should continue running.
    ///
    /// **Note:** `keep_running()` currently has a small per-iteration overhead due to the FFI boundary.
    /// In the future, this could be optimized using `KeepRunningBatch` under the hood.
    #[inline]
    pub fn keep_running(&mut self) -> bool {
        self.inner.as_mut().KeepRunning()
    }

    pub fn skip_with_error(&mut self, msg: &str) {
        unsafe {
            ffi::ffi::SkipWithError(self.inner.as_mut(), msg);
        }
    }
}

/// Initialize the benchmark library.
/// This should be called before `run_specified_benchmarks`.
pub fn initialize(args: &Vec<String>) {
    let mut c_args: Vec<CString> = args.iter()
        .map(|arg| CString::new(arg.as_str()).unwrap())
        .collect();
        
    let mut c_ptrs: Vec<*mut c_char> = c_args.iter_mut()
        .map(|c| c.as_ptr() as *mut c_char)
        .collect();
        
    let mut argc = c_ptrs.len() as i32;
    let argv = c_ptrs.as_mut_ptr();

    unsafe {
        ffi::ffi::Initialize(&mut argc as *mut _, argv as usize);
    }
}

#[macro_export]
macro_rules! register_benchmark {
    ($name:expr, $func:path) => {
        {
            fn trampoline(mut state: std::pin::Pin<&mut $crate::ffi::ffi::State>) {
                let mut wrapped = $crate::State { inner: state.as_mut() };
                $func(&mut wrapped);
            }
            unsafe {
                $crate::ffi::ffi::RegisterBenchmark($name, trampoline);
            }
        }
    };
}

/// Run all registered benchmarks.
pub fn run_specified_benchmarks() -> usize {
    ffi::ffi::RunSpecifiedBenchmarks()
}
