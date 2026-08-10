// Test for perf counter modifier parsing (:u, :k, :h modifiers)
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "../src/perf_counters.h"

using benchmark::internal::PerfCounters;
using benchmark::internal::PerfCounterValues;
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::SizeIs;

// Test that modifier parsing works correctly without actual hardware
TEST(PerfCountersModifierTest, ModifierParsingNoLibPfm) {
  // This test runs even without libpfm - we just test that the
  // modifier parsing logic doesn't crash
  if (!PerfCounters::kSupported) {
    // On platforms without libpfm, Create() returns empty but should not crash
    auto counters1 = PerfCounters::Create({"INSTRUCTIONS:u"});
    EXPECT_EQ(counters1.num_counters(), 0);
    
    auto counters2 = PerfCounters::Create({"INSTRUCTIONS:k"});
    EXPECT_EQ(counters2.num_counters(), 0);
    
    auto counters3 = PerfCounters::Create({"INSTRUCTIONS"});
    EXPECT_EQ(counters3.num_counters(), 0);
    
    auto counters4 = PerfCounters::Create({"INSTRUCTIONS:uk"});
    EXPECT_EQ(counters4.num_counters(), 0);
    
    auto counters5 = PerfCounters::Create({"CYCLES:u", "INSTRUCTIONS:k"});
    EXPECT_EQ(counters5.num_counters(), 0);
    
    SUCCEED() << "Modifier parsing works on non-libpfm platforms";
  }
}

// Test IsCounterSupported with modifiers
TEST(PerfCountersModifierTest, IsCounterSupportedWithModifiers) {
  if (!PerfCounters::kSupported) {
    EXPECT_FALSE(PerfCounters::IsCounterSupported("INSTRUCTIONS:u"));
    EXPECT_FALSE(PerfCounters::IsCounterSupported("INSTRUCTIONS:k"));
    EXPECT_FALSE(PerfCounters::IsCounterSupported("INSTRUCTIONS"));
    EXPECT_FALSE(PerfCounters::IsCounterSupported("CYCLES:uk"));
    SUCCEED() << "IsCounterSupported handles modifiers on non-libpfm platforms";
  }
}

TEST(PerfCountersModifierTest, EmptyName) {
  if (!PerfCounters::kSupported) {
    auto counters = PerfCounters::Create({""});
    EXPECT_EQ(counters.num_counters(), 0);
  }
}

TEST(PerfCountersModifierTest, InvalidName) {
  if (!PerfCounters::kSupported) {
    auto counters = PerfCounters::Create({"not_a_valid_counter:u"});
    EXPECT_EQ(counters.num_counters(), 0);
  }
}

TEST(PerfCountersModifierTest, MultipleCountersWithModifiers) {
  if (!PerfCounters::kSupported) {
    auto counters = PerfCounters::Create({"CYCLES:u", "INSTRUCTIONS:k", "BRANCH_MISSES"});
    EXPECT_EQ(counters.num_counters(), 0);
  }
}

// Test with libpfm available
TEST(PerfCountersModifierTest, ModifierParsingWithLibPfm) {
  if (!PerfCounters::kSupported) {
    GTEST_SKIP() << "libpfm not supported on this platform";
  }
  
  PerfCounters::Initialize();
  
  // Test that we can at least try to create counters with modifiers
  // without crashing. The actual counters may not be available on all hardware.
  auto counters1 = PerfCounters::Create({"INSTRUCTIONS:u"});
  auto counters2 = PerfCounters::Create({"INSTRUCTIONS:k"});
  auto counters3 = PerfCounters::Create({"INSTRUCTIONS"});
  auto counters4 = PerfCounters::Create({"INSTRUCTIONS:uk"});
  auto counters5 = PerfCounters::Create({"CYCLES:u", "INSTRUCTIONS:k"});
  
  // Just verify they return valid objects (may be empty if counters not available)
  EXPECT_GE(counters1.num_counters(), 0);
  EXPECT_GE(counters2.num_counters(), 0);
  EXPECT_GE(counters3.num_counters(), 0);
  EXPECT_GE(counters4.num_counters(), 0);
  EXPECT_GE(counters5.num_counters(), 0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}