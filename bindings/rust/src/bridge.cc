#include "bridge.h"

#include <string>
#include <vector>

namespace benchmark_bridge {

void bm_initialize(::rust::Slice<const ::rust::Str> args) {
    std::vector<std::string> strings;
    strings.reserve(args.size());
    for (auto& arg : args) {
        strings.emplace_back(arg.data(), arg.size());
    }

    // argv[0] must outlive the Initialize call; keep a static copy.
    static std::string argv0;
    if (!strings.empty()) {
        argv0 = strings[0];
    }

    std::vector<char*> ptrs;
    ptrs.reserve(strings.size());
    for (auto& s : strings) {
        ptrs.push_back(const_cast<char*>(s.c_str()));
    }
    if (!ptrs.empty()) {
        ptrs[0] = const_cast<char*>(argv0.c_str());
    }

    int argc = static_cast<int>(ptrs.size());
    ::benchmark::Initialize(&argc, ptrs.data());
}

bool bm_keep_running(::benchmark::State& state) {
    return state.KeepRunning();
}

void bm_skip_with_error(::benchmark::State& state, ::rust::Str msg) {
    state.SkipWithError(std::string(msg.data(), msg.size()));
}

void bm_register(::rust::Str name, void (*f)(::benchmark::State&)) {
    ::benchmark::RegisterBenchmark(
        std::string(name.data(), name.size()), f);
}

::std::size_t bm_run() {
    return ::benchmark::RunSpecifiedBenchmarks();
}

}  // namespace benchmark_bridge
