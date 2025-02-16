#include <iostream>
#include <memory>

#include "benchmark/benchmark.h"

using benchmark::callback_function;

namespace counters {
static int functor_calls = 0;
static int lambda_calls = 0;
static int ctr_copy_calls = 0;
static int ctr_move_calls = 0;
};  // namespace counters

struct Functor {
  Functor() {}
  Functor(const Functor& /*unused*/) { counters::ctr_copy_calls++; }
  Functor(Functor&& /*unused*/) { counters::ctr_move_calls++; }
  void operator()(const benchmark::State& /*unused*/) {}
};

void BM_DoSomething(benchmark::State& state) {
  for (auto _ : state) {
  }
}

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  auto bm = benchmark::RegisterBenchmark("BM_CtrCopy", BM_DoSomething);

  {
    Functor func;
    bm->Setup(func);
    bm->Teardown(func);
    assert(counters::ctr_copy_calls == 2);
  }
  {
    Functor func;
    Functor& r_func{func};
    bm->Setup(r_func);
    bm->Teardown(r_func);
    assert(counters::ctr_copy_calls == 4);
  }
  {
    Functor func;
    const Functor& cr_func{func};
    bm->Setup(cr_func);
    bm->Teardown(cr_func);
    assert(counters::ctr_copy_calls == 6);
  }
  {
    Functor func1;
    Functor func2;
    bm->Setup(std::move(func1));
    bm->Teardown(std::move(func2));
    assert(counters::ctr_move_calls == 2);
  }
  {
    bm->Setup(Functor{});
    bm->Teardown(Functor{});
    assert(counters::ctr_move_calls == 4);
  }
  {
    bm->Setup([](const benchmark::State& /*unused*/) {});
    bm->Teardown([](const benchmark::State& /*unused*/) {});
  }
  {
    callback_function func1 = [](const benchmark::State& /*unused*/) {};
    callback_function func2 = func1;
    bm->Setup(std::move(func1));
    bm->Teardown(std::move(func2));
    assert(func1 == nullptr);
    assert(func2 == nullptr);
  }
  {
    callback_function func1 = [](const benchmark::State& /*unused*/) {};
    callback_function func2 = func1;
    bm->Setup(func1);
    bm->Teardown(func2);
    assert(func1 != nullptr);
    assert(func2 != nullptr);
  }
  {
    auto func = [](const benchmark::State& /*unused*/) {};
    bm->Setup(func);
    bm->Teardown(func);
    assert(func != nullptr);
  }
  {
    auto func1 = [](const benchmark::State& /*unused*/) {};
    auto func2 = func1;
    bm->Setup(std::move(func1));
    bm->Teardown(std::move(func2));
  }
}
