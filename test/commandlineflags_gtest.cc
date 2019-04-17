#include <cstdlib>

#include "../src/commandlineflags.h"
#include "../src/internal_macros.h"
#include "gtest/gtest.h"

namespace benchmark {
namespace {

#if defined(BENCHMARK_OS_WINDOWS)
int setenv(const char* name, const char* value, int overwrite) {
  if (!overwrite) {
    // NOTE: getenv_s is far superior but not available under mingw.
    char* env_value = getenv(name);
    if (env_value == nullptr) {
      return -1;
    }
  }
  return _putenv_s(name, value);
}

int unsetenv(const char* name) {
  return _putenv_s(name, "");
}

#endif  // BENCHMARK_OS_WINDOWS

TEST(BoolFromEnv, Default) {
  ASSERT_EQ(unsetenv("BENCHMARK_NOT_IN_ENV"), 0);
  EXPECT_EQ(BoolFromEnv("not_in_env", true), true);
}

TEST(BoolFromEnv, False) {
  ASSERT_EQ(setenv("BENCHMARK_IN_ENV", "0", 1), 0);
  EXPECT_EQ(BoolFromEnv("in_env", true), false);
  unsetenv("BENCHMARK_IN_ENV");
}

TEST(BoolFromEnv, True) {
  ASSERT_EQ(setenv("BENCHMARK_IN_ENV", "1", 1), 0);
  EXPECT_EQ(BoolFromEnv("in_env", false), true);
  unsetenv("BENCHMARK_IN_ENV");

  ASSERT_EQ(setenv("BENCHMARK_IN_ENV", "foo", 1), 0);
  EXPECT_EQ(BoolFromEnv("in_env", false), true);
  unsetenv("BENCHMARK_IN_ENV");
}

TEST(Int32FromEnv, NotInEnv) {
  ASSERT_EQ(unsetenv("BENCHMARK_NOT_IN_ENV"), 0);
  EXPECT_EQ(Int32FromEnv("not_in_env", 42), 42);
}

TEST(Int32FromEnv, InvalidInteger) {
  ASSERT_EQ(setenv("BENCHMARK_IN_ENV", "foo", 1), 0);
  EXPECT_EQ(Int32FromEnv("in_env", 42), 42);
  ASSERT_EQ(unsetenv("BENCHMARK_IN_ENV"), 0);
}

TEST(Int32FromEnv, ValidInteger) {
  ASSERT_EQ(setenv("BENCHMARK_IN_ENV", "42", 1), 0);
  EXPECT_EQ(Int32FromEnv("in_env", 64), 42);
  unsetenv("BENCHMARK_IN_ENV");
}

TEST(StringFromEnv, Default) {
  ASSERT_EQ(unsetenv("BENCHMARK_NOT_IN_ENV"), 0);
  EXPECT_STREQ(StringFromEnv("not_in_env", "foo"), "foo");
}

TEST(StringFromEnv, Valid) {
  ASSERT_EQ(setenv("BENCHMARK_IN_ENV", "foo", 1), 0);
  EXPECT_STREQ(StringFromEnv("in_env", "bar"), "foo");
  unsetenv("BENCHMARK_IN_ENV");
}

}  // namespace
}  // namespace benchmark
