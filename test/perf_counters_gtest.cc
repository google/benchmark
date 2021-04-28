#include "../src/perf_counters.h"
#include "gtest/gtest.h"

#ifndef GTEST_SKIP
struct MsgHandler {
  void operator=(std::ostream&){}
};
#define GTEST_SKIP() return MsgHandler() = std::cout
#endif

using benchmark::internal::PerfCounters;
using benchmark::internal::PerfCounterValues;

namespace {
const char kGenericPerfEvent1[] = "CYCLES";
const char kGenericPerfEvent2[] = "BRANCHES";
const char kGenericPerfEvent3[] = "INSTRUCTIONS";

TEST(PerfCountersTest, Init) {
  EXPECT_EQ(PerfCounters::Initialize(), PerfCounters::kSupported);
}

TEST(PerfCountersTest, OneCounter) {
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "Performance counters not supported.\n";
  }
  EXPECT_TRUE(PerfCounters::Initialize());
  EXPECT_TRUE(PerfCounters::Create({kGenericPerfEvent1}).IsValid());
}

TEST(PerfCountersTest, NegativeTest) {
  if (!PerfCounters::kSupported) {
    EXPECT_FALSE(PerfCounters::Initialize());
    return;
  }
  EXPECT_TRUE(PerfCounters::Initialize());
  EXPECT_FALSE(PerfCounters::Create({}).IsValid());
  EXPECT_FALSE(PerfCounters::Create({""}).IsValid());
  EXPECT_FALSE(PerfCounters::Create({"not a counter name"}).IsValid());
  {
    EXPECT_TRUE(PerfCounters::Create({kGenericPerfEvent1, kGenericPerfEvent2,
                                      kGenericPerfEvent3})
                    .IsValid());
  }
  EXPECT_FALSE(
      PerfCounters::Create({kGenericPerfEvent2, "", kGenericPerfEvent1})
          .IsValid());
  EXPECT_FALSE(PerfCounters::Create({kGenericPerfEvent3, "not a counter name",
                                     kGenericPerfEvent1})
                   .IsValid());
  {
    EXPECT_TRUE(PerfCounters::Create({kGenericPerfEvent1, kGenericPerfEvent2,
                                      kGenericPerfEvent3})
                    .IsValid());
  }
  EXPECT_FALSE(
      PerfCounters::Create({kGenericPerfEvent1, kGenericPerfEvent2,
                            kGenericPerfEvent3, "MISPREDICTED_BRANCH_RETIRED"})
          .IsValid());
}

TEST(PerfCountersTest, Read1Counter) {
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "Test skipped because libpfm is not supported.\n";
  }
  EXPECT_TRUE(PerfCounters::Initialize());
  auto counters = PerfCounters::Create({kGenericPerfEvent1});
  EXPECT_TRUE(counters.IsValid());
  PerfCounterValues values1(1);
  EXPECT_TRUE(counters.Snapshot(&values1));
  EXPECT_GT(values1[0], 0);
  PerfCounterValues values2(1);
  EXPECT_TRUE(counters.Snapshot(&values2));
  EXPECT_GT(values2[0], 0);
  EXPECT_GT(values2[0], values1[0]);
}

TEST(PerfCountersTest, Read2Counters) {
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "Test skipped because libpfm is not supported.\n";
  }
  EXPECT_TRUE(PerfCounters::Initialize());
  auto counters =
      PerfCounters::Create({kGenericPerfEvent1, kGenericPerfEvent2});
  EXPECT_TRUE(counters.IsValid());
  PerfCounterValues values1(2);
  EXPECT_TRUE(counters.Snapshot(&values1));
  EXPECT_GT(values1[0], 0);
  EXPECT_GT(values1[1], 0);
  PerfCounterValues values2(2);
  EXPECT_TRUE(counters.Snapshot(&values2));
  EXPECT_GT(values2[0], 0);
  EXPECT_GT(values2[1], 0);
}
}  // namespace
