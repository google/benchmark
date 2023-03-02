#include <thread>

#include "../src/perf_counters.h"
#include "../src/timers.h"
#include "gtest/gtest.h"

#ifndef GTEST_SKIP
struct MsgHandler {
  void operator=(std::ostream&) {}
};
#define GTEST_SKIP() return MsgHandler() = std::cout
#endif

using benchmark::internal::PerfCounters;
using benchmark::internal::PerfCountersMeasurement;
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
  EXPECT_TRUE(PerfCounters::Create({kGenericPerfEvent1}));
}

TEST(PerfCountersTest, NegativeTest) {
  if (!PerfCounters::kSupported) {
    EXPECT_FALSE(PerfCounters::Initialize());
    return;
  }
  EXPECT_TRUE(PerfCounters::Initialize());
  EXPECT_FALSE(PerfCounters::Create({}));
  EXPECT_FALSE(PerfCounters::Create({""}));
  EXPECT_FALSE(PerfCounters::Create({"not a counter name"}));
  {
    EXPECT_TRUE(PerfCounters::Create(
        {kGenericPerfEvent1, kGenericPerfEvent2, kGenericPerfEvent3}));
  }
  EXPECT_FALSE(
      PerfCounters::Create({kGenericPerfEvent2, "", kGenericPerfEvent1}));
  EXPECT_FALSE(PerfCounters::Create(
      {kGenericPerfEvent3, "not a counter name", kGenericPerfEvent1}));
  {
    EXPECT_TRUE(PerfCounters::Create(
        {kGenericPerfEvent1, kGenericPerfEvent2, kGenericPerfEvent3}));
  }
  EXPECT_FALSE(PerfCounters::Create({kGenericPerfEvent1, kGenericPerfEvent2,
                                     kGenericPerfEvent3,
                                     "MISPREDICTED_BRANCH_RETIRED"}));
}

TEST(PerfCountersTest, Read1Counter) {
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "Test skipped because libpfm is not supported.\n";
  }
  EXPECT_TRUE(PerfCounters::Initialize());
  auto counters = PerfCounters::Create({kGenericPerfEvent1});
  EXPECT_TRUE(counters);
  PerfCounterValues values1(1);
  EXPECT_TRUE(counters->Snapshot(&values1));
  EXPECT_GT(values1[0], 0);
  PerfCounterValues values2(1);
  EXPECT_TRUE(counters->Snapshot(&values2));
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
  EXPECT_TRUE(counters);
  PerfCounterValues values1(2);
  EXPECT_TRUE(counters->Snapshot(&values1));
  EXPECT_GT(values1[0], 0);
  EXPECT_GT(values1[1], 0);
  PerfCounterValues values2(2);
  EXPECT_TRUE(counters->Snapshot(&values2));
  EXPECT_GT(values2[0], 0);
  EXPECT_GT(values2[1], 0);
}

TEST(PerfCountersTest, ReopenExistingCounters) {
  // This test works as you can have multiple repeated counters
  // in different performance counter groups
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "Test skipped because libpfm is not supported.\n";
  }
  EXPECT_TRUE(PerfCounters::Initialize());
  std::vector<std::shared_ptr<PerfCounters>> counters;
  counters.reserve(6);
  for (int i = 0; i < 6; i++)
    counters.push_back(PerfCounters::Create({kGenericPerfEvent1}));
  PerfCounterValues values(1);
  EXPECT_TRUE(counters[0]->Snapshot(&values));
  EXPECT_TRUE(counters[4]->Snapshot(&values));
  EXPECT_TRUE(counters[5]->Snapshot(&values));
}

TEST(PerfCountersTest, CreateExistingMeasurements) {
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "Test skipped because libpfm is not supported.\n";
  }
  EXPECT_TRUE(PerfCounters::Initialize());

  const int kMaxCounters = 1;
  const std::vector<std::string> kMetrics{"cycles"};

  std::vector<PerfCountersMeasurement> perf_counter_measurements(kMaxCounters,
                                                                 kMetrics);
  std::vector<std::pair<std::string, double>> measurements;

  // Start all together
  int max_counters = kMaxCounters;
  for (int i = 0; i < kMaxCounters; ++i) {
    PerfCountersMeasurement& counter(perf_counter_measurements[i]);
    EXPECT_TRUE(counter.IsValid());
    EXPECT_EQ(counter.num_counters(), 1);
    if (!counter.IsValid()) {
      max_counters = i;
      break;
    }
    counter.Start();
  }

  ASSERT_GE(max_counters, 1);

  // Start all together
  for (int i = 0; i < max_counters; ++i) {
    PerfCountersMeasurement& counter(perf_counter_measurements[i]);
    EXPECT_TRUE(counter.Stop(measurements));
  }

  // Start/stop individually
  for (int i = 0; i < max_counters; ++i) {
    PerfCountersMeasurement& counter(perf_counter_measurements[i]);
    measurements.clear();
    counter.Start();
    EXPECT_TRUE(counter.Stop(measurements));
  }
}

using benchmark::ChronoClockNow;

size_t do_work() {
  const double kMinimumElapsedSeconds = 0.25;
  double now = ChronoClockNow();
  double finish = now + kMinimumElapsedSeconds;
  size_t res = 0;
  size_t counter = 0;
  do {
    now = ChronoClockNow();
    counter++;
    res += counter * counter;
  } while (now < finish);
  return res;
}

void measure(size_t threadcount, PerfCounterValues* before,
             PerfCounterValues* after) {
  BM_CHECK_NE(before, nullptr);
  BM_CHECK_NE(after, nullptr);
  std::vector<std::thread> threads(threadcount);
  auto work = [&]() { BM_CHECK(do_work() > 1000); };

  // We need to first set up the counters, then start the threads, so the
  // threads would inherit the counters. But later, we need to first destroy
  // the thread pool (so all the work finishes), then measure the counters. So
  // the scopes overlap, and we need to explicitly control the scope of the
  // threadpool.
  auto counters =
      PerfCounters::Create({kGenericPerfEvent1, kGenericPerfEvent3});
  for (auto& t : threads) t = std::thread(work);
  counters->Snapshot(before);
  for (auto& t : threads) t.join();
  counters->Snapshot(after);
}

TEST(PerfCountersTest, MultiThreaded) {
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "Test skipped because libpfm is not supported.";
  }
  EXPECT_TRUE(PerfCounters::Initialize());
  PerfCounterValues before(2);
  PerfCounterValues after(2);

  measure(2, &before, &after);
  std::vector<double> Elapsed2Threads{
      static_cast<double>(after[0] - before[0]),
      static_cast<double>(after[1] - before[1])};

  measure(4, &before, &after);
  std::vector<double> Elapsed4Threads{
      static_cast<double>(after[0] - before[0]),
      static_cast<double>(after[1] - before[1])};

  // Some extra work will happen on the main thread - like joining the threads
  // - so the ratio won't be quite 2.0, but very close.
  EXPECT_GE(Elapsed4Threads[0], 1.9 * Elapsed2Threads[0]);
  EXPECT_GE(Elapsed4Threads[1], 1.9 * Elapsed2Threads[1]);
}

TEST(PerfCountersTest, HardwareLimits) {
  // The test works (i.e. causes read to fail) for the assumptions
  // about hardware capabilities (i.e. small number (3-4) hardware
  // counters) at this date,
  // the same as previous test ReopenExistingCounters.
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "Test skipped because libpfm is not supported.\n";
  }
  EXPECT_TRUE(PerfCounters::Initialize());

  // Taken straight from `perf list` on x86-64
  // Got all hardware names since these are the problematic ones
  std::vector<std::string> counter_names{"cycles",  // leader
                                         "instructions",
                                         "branches",
                                         "L1-dcache-loads",
                                         "L1-dcache-load-misses",
                                         "L1-dcache-prefetches",
                                         "L1-icache-load-misses",  // leader
                                         "L1-icache-loads",
                                         "branch-load-misses",
                                         "branch-loads",
                                         "dTLB-load-misses",
                                         "dTLB-loads",
                                         "iTLB-load-misses",  // leader
                                         "iTLB-loads",
                                         "branch-instructions",
                                         "branch-misses",
                                         "cache-misses",
                                         "cache-references",
                                         "stalled-cycles-backend",  // leader
                                         "stalled-cycles-frontend"};

  // In the off-chance that some of these values are not supported,
  // we filter them out so the test will complete without failure
  // albeit it might not actually test the grouping on that platform
  std::vector<std::string> valid_names;
  for (const std::string& name : counter_names) {
    if (PerfCounters::IsCounterSupported(name)) {
      valid_names.push_back(name);
    }
  }
  PerfCountersMeasurement counter(valid_names);

  std::vector<std::pair<std::string, double>> measurements;

  counter.Start();
  EXPECT_TRUE(counter.Stop(measurements));
}

}  // namespace
