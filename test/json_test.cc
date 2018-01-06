//===---------------------------------------------------------------------===//
// json_test - Unit tests for benchmark/json.h
//===---------------------------------------------------------------------===//

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

namespace {
TEST(JSONTest, BreathingTest) {
  using benchmark::json;
  using namespace benchmark;
  json obj{{"name", "foo"}, {"value", 42}, {"list", {1, 2, 3}}};
  auto expect_json = R"(
  {
    "name": "foo",
    "value": 42,
    "list": [1, 2, 3]
  })"_json;
  EXPECT_EQ(obj, expect_json);
  EXPECT_EQ(obj.at("name"), "foo");
  EXPECT_EQ(obj.at("value"), 42);
  EXPECT_EQ(obj.at("list"), (json::array_t{1, 2, 3}));
  auto list = obj.at("list");
  auto expect_list = expect_json.at("list");
  for (auto It = list.begin(), EIt = expect_list.begin(); It != list.end();
       ++It, ++EIt) {
    EXPECT_EQ(*It, *EIt);
  }
}

TEST(JSONTest, JSONInputTest) {
  using namespace benchmark;
  RegisterBenchmark("test1",
                    [](State& st) {
                      json obj = st.GetInput();
                      assert(!obj.is_null());
                      switch (obj.at("case").get<int>()) {
                        case 1:
                          assert(obj.at("name") == "foo");
                          assert(obj.at("a") == 42);
                          break;
                        case 2:
                          assert(obj.count("name") == 0);
                          assert(obj.at("b") == 101);
                          break;
                        default:
                          assert(false && "in default case");
                      }
                      for (auto _ : st) {
                      }
                    })
      ->WithInput({{"case", 1}, {"name", "foo"}, {"a", 42}})
      ->WithInput({{"case", 2}, {"b", 101}});
  RunSpecifiedBenchmarks();
}

}  // end namespace
