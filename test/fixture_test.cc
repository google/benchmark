
#include "benchmark/benchmark.h"

#include <cassert>
#include <memory>

class MyFixture : public ::benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State&) {
    assert(data.get() == nullptr);
    data.reset(new int(42));
  }

  void TearDown() {
    assert(data.get() != nullptr);
    data.release();
  }

  ~MyFixture() {
    assert(data == nullptr);
  }

  std::unique_ptr<int> data;
};


BENCHMARK_F(MyFixture, Foo)(benchmark::State& st) {
  assert(data.get() != nullptr);
  assert(*data == 42);
  while (st.KeepRunning()) {
  }
}

BENCHMARK_DEFINE_F(MyFixture, Bar)(benchmark::State& st) {
  while (st.KeepRunning()) {
  }
  st.SetItemsProcessed(st.range_x());
}
BENCHMARK_REGISTER_F(MyFixture, Bar)->Arg(42);

BENCHMARK_MAIN()
