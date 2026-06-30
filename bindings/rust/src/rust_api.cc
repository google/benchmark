#include "rust_api.h"

#include <string>

namespace benchmark {
namespace rust_api {

void RegisterBenchmark(rust::Str name, rust::Fn<void(benchmark::State&)> func);
void Initialize(int* argc, size_t argv);
void SkipWithError(benchmark::State& state, rust::Str msg);

void RegisterBenchmark(rust::Str name, rust::Fn<void(benchmark::State&)> func) {
  ::benchmark::RegisterBenchmark(std::string(name).c_str(),
                                 [func](benchmark::State& st) { func(st); });
}

void Initialize(int* argc, size_t argv) {
  char** argv_ptr = reinterpret_cast<char**>(argv);
  if (argc != nullptr && *argc > 0 && argv_ptr != nullptr &&
      argv_ptr[0] != nullptr) {
    static std::string executable_name;
    executable_name = argv_ptr[0];
    argv_ptr[0] = executable_name.data();
  }
  ::benchmark::Initialize(argc, argv_ptr);
}

void SkipWithError(benchmark::State& state, rust::Str msg) {
  state.SkipWithError(std::string(msg).c_str());
}

}  // namespace rust_api
}  // namespace benchmark
