// Benchmark for Python.

#include <vector>
#include <string>

#include "benchmark/benchmark.h"

#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace {
namespace py = ::pybind11;

std::vector<std::string> Initialize(const std::vector<std::string>& argv) {
  // The `argv` pointers here become invalid when this function returns, but
  // benchmark holds the pointer to `argv[0]`. We create a static copy of it
  // so it persists, and replace the pointer below.
  static std::string executable_name(argv[0]);
  std::vector<char*> ptrs;
  ptrs.reserve(argv.size());
  for (auto& arg : argv) {
    ptrs.push_back(const_cast<char*>(arg.c_str()));
  }
  ptrs[0] = const_cast<char*>(executable_name.c_str());
  int argc = static_cast<int>(argv.size());
  benchmark::Initialize(&argc, ptrs.data());
  std::vector<std::string> remaining_argv;
  remaining_argv.reserve(argc);
  for (int i = 0; i < argc; ++i) {
    remaining_argv.emplace_back(ptrs[i]);
  }
  return remaining_argv;
}

void RegisterBenchmark(const char* name, py::function f) {
  benchmark::RegisterBenchmark(name,
                               [f](benchmark::State& state) { f(&state); });
}

PYBIND11_MODULE(_benchmark, m) {
  m.def("Initialize", Initialize);
  m.def("RegisterBenchmark", RegisterBenchmark);
  m.def("RunSpecifiedBenchmarks",
        []() { benchmark::RunSpecifiedBenchmarks(); });

  using benchmark::Counter;
  py::class_<Counter> py_counter(m, "Counter");

  py::enum_<Counter::Flags>(py_counter, "Flags")
      .value("kDefaults", Counter::Flags::kDefaults)
      .value("kIsRate", Counter::Flags::kIsRate)
      .value("kAvgThreads", Counter::Flags::kAvgThreads)
      .value("kAvgThreadsRate", Counter::Flags::kAvgThreadsRate)
      .value("kIsIterationInvariant", Counter::Flags::kIsIterationInvariant)
      .value("kIsIterationInvariantRate",
             Counter::Flags::kIsIterationInvariantRate)
      .value("kAvgIterations", Counter::Flags::kAvgIterations)
      .value("kAvgIterationsRate", Counter::Flags::kAvgIterationsRate)
      .value("kInvert", Counter::Flags::kInvert)
      .export_values()
      .def(py::self | py::self);

  py::enum_<Counter::OneK>(py_counter, "OneK")
      .value("kIs1000", Counter::OneK::kIs1000)
      .value("kIs1024", Counter::OneK::kIs1024)
      .export_values();

  py_counter  //
      .def(py::init<double, Counter::Flags, Counter::OneK>(),
           py::arg("value") = 0., py::arg("flags") = Counter::kDefaults,
           py::arg("k") = Counter::kIs1000)
      .def_readwrite("value", &Counter::value)
      .def_readwrite("flags", &Counter::flags)
      .def_readwrite("oneK", &Counter::oneK);

  py::class_<benchmark::State>(m, "State")
      .def("__bool__", &benchmark::State::KeepRunning)
      .def_property_readonly("keep_running", &benchmark::State::KeepRunning)
      .def("skip_with_error", &benchmark::State::SkipWithError);
};
}  // namespace
