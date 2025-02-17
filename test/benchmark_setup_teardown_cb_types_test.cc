#include <iostream>
#include <memory>

#include "benchmark/benchmark.h"

using benchmark::callback_function;
struct Functor {
  Functor() {}
  Functor(const Functor& /*unused*/) {}
  Functor(Functor&& /*unused*/) {}
  void operator()(const benchmark::State& /*unused*/) {}
};

void BM_DoSomething(benchmark::State& state) {
  for (auto _ : state) {
  }
}

void function(const benchmark::State& /*unused*/) {}

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  auto bm = benchmark::RegisterBenchmark("BM_DoSmth", BM_DoSomething);

  {
    Functor func;
    bm->Setup(func);
    bm->Teardown(func);
  }
  {
    Functor func;
    Functor& r_func{func};
    bm->Setup(r_func);
    bm->Teardown(r_func);
  }
  {
    Functor func;
    const Functor& cr_func{func};
    bm->Setup(cr_func);
    bm->Teardown(cr_func);
  }
  {
    Functor func1;
    Functor func2;
    bm->Setup(std::move(func1));
    bm->Teardown(std::move(func2));
  }
  {
    bm->Setup(Functor{});
    bm->Teardown(Functor{});
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
  }
  {
    callback_function func1 = [](const benchmark::State& /*unused*/) {};
    callback_function func2 = func1;
    bm->Setup(func1);
    bm->Teardown(func2);
  }
  {
    auto func = [](const benchmark::State& /*unused*/) {};
    bm->Setup(func);
    bm->Teardown(func);
  }
  {
    auto func1 = [](const benchmark::State& /*unused*/) {};
    auto func2 = func1;
    bm->Setup(std::move(func1));
    bm->Teardown(std::move(func2));
  }
  {
    bm->Setup(function);
    bm->Teardown(function);
  }
}
