//===---------------------------------------------------------------------===//
// string_util_test - Unit tests for src/string_util.cc
//===---------------------------------------------------------------------===//

#include <tuple>

#include "../src/internal_macros.h"
#include "../src/string_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
TEST(StringUtilTest, stoul) {
  {
    size_t pos = 0;
    EXPECT_EQ(0UL, benchmark::stoul("0", &pos));
    EXPECT_EQ(1UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(7UL, benchmark::stoul("7", &pos));
    EXPECT_EQ(1UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(135UL, benchmark::stoul("135", &pos));
    EXPECT_EQ(3UL, pos);
  }
#if ULONG_MAX == 0xFFFFFFFFul
  {
    size_t pos = 0;
    EXPECT_EQ(0xFFFFFFFFul, benchmark::stoul("4294967295", &pos));
    EXPECT_EQ(10ul, pos);
  }
#elif ULONG_MAX == 0xFFFFFFFFFFFFFFFFul
  {
    size_t pos = 0;
    EXPECT_EQ(0xFFFFFFFFFFFFFFFFUL,
              benchmark::stoul("18446744073709551615", &pos));
    EXPECT_EQ(20UL, pos);
  }
#endif
  {
    size_t pos = 0;
    EXPECT_EQ(10UL, benchmark::stoul("1010", &pos, 2));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(520UL, benchmark::stoul("1010", &pos, 8));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(1010UL, benchmark::stoul("1010", &pos, 10));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(4112UL, benchmark::stoul("1010", &pos, 16));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(0xBEEFUL, benchmark::stoul("BEEF", &pos, 16));
    EXPECT_EQ(4UL, pos);
  }
#ifndef BENCHMARK_HAS_NO_EXCEPTIONS
  {
    ASSERT_THROW(std::ignore = benchmark::stoul("this is a test"),
                 std::invalid_argument);
  }
#endif
}

TEST(StringUtilTest, stoi) {
  {
    size_t pos = 0;
    EXPECT_EQ(0, benchmark::stoi("0", &pos));
    EXPECT_EQ(1UL, pos);
  }  // namespace
  {
    size_t pos = 0;
    EXPECT_EQ(-17, benchmark::stoi("-17", &pos));
    EXPECT_EQ(3UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(1357, benchmark::stoi("1357", &pos));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(10, benchmark::stoi("1010", &pos, 2));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(520, benchmark::stoi("1010", &pos, 8));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(1010, benchmark::stoi("1010", &pos, 10));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(4112, benchmark::stoi("1010", &pos, 16));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(0xBEEF, benchmark::stoi("BEEF", &pos, 16));
    EXPECT_EQ(4UL, pos);
  }
#ifndef BENCHMARK_HAS_NO_EXCEPTIONS
  {
    ASSERT_THROW(std::ignore = benchmark::stoi("this is a test"),
                 std::invalid_argument);
  }
#endif
}

TEST(StringUtilTest, stod) {
  {
    size_t pos = 0;
    EXPECT_EQ(0.0, benchmark::stod("0", &pos));
    EXPECT_EQ(1UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(-84.0, benchmark::stod("-84", &pos));
    EXPECT_EQ(3UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(1234.0, benchmark::stod("1234", &pos));
    EXPECT_EQ(4UL, pos);
  }
  {
    size_t pos = 0;
    EXPECT_EQ(1.5, benchmark::stod("1.5", &pos));
    EXPECT_EQ(3UL, pos);
  }
  {
    size_t pos = 0;
    /* Note: exactly representable as double */
    EXPECT_EQ(-1.25e+9, benchmark::stod("-1.25e+9", &pos));
    EXPECT_EQ(8UL, pos);
  }
#ifndef BENCHMARK_HAS_NO_EXCEPTIONS
  {
    ASSERT_THROW(std::ignore = benchmark::stod("this is a test"),
                 std::invalid_argument);
  }
#endif
}

TEST(StringUtilTest, StrSplit) {
  EXPECT_EQ(benchmark::StrSplit("", ','), std::vector<std::string>{});
  EXPECT_EQ(benchmark::StrSplit("hello", ','),
            std::vector<std::string>({"hello"}));
  EXPECT_EQ(benchmark::StrSplit("hello,there,is,more", ','),
            std::vector<std::string>({"hello", "there", "is", "more"}));
}

using HumanReadableFixture = ::testing::TestWithParam<
    std::tuple<double, benchmark::Counter::OneK, std::string>>;

INSTANTIATE_TEST_SUITE_P(
    HumanReadableTests, HumanReadableFixture,
    ::testing::Values(
        std::make_tuple(0.0, benchmark::Counter::kIs1024, "0"),
        std::make_tuple(999.0, benchmark::Counter::kIs1024, "999"),
        std::make_tuple(1000.0, benchmark::Counter::kIs1024, "1000"),
        std::make_tuple(1024.0, benchmark::Counter::kIs1024, "1Ki"),
        std::make_tuple(1000 * 1000.0, benchmark::Counter::kIs1024,
                        "976\\.56.Ki"),
        std::make_tuple(1024 * 1024.0, benchmark::Counter::kIs1024, "1Mi"),
        std::make_tuple(1000 * 1000 * 1000.0, benchmark::Counter::kIs1024,
                        "953\\.674Mi"),
        std::make_tuple(1024 * 1024 * 1024.0, benchmark::Counter::kIs1024,
                        "1Gi"),
        std::make_tuple(0.0, benchmark::Counter::kIs1000, "0"),
        std::make_tuple(999.0, benchmark::Counter::kIs1000, "999"),
        std::make_tuple(1000.0, benchmark::Counter::kIs1000, "1k"),
        std::make_tuple(1024.0, benchmark::Counter::kIs1000, "1.024k"),
        std::make_tuple(1000 * 1000.0, benchmark::Counter::kIs1000, "1M"),
        std::make_tuple(1024 * 1024.0, benchmark::Counter::kIs1000,
                        "1\\.04858M"),
        std::make_tuple(1000 * 1000 * 1000.0, benchmark::Counter::kIs1000,
                        "1G"),
        std::make_tuple(1024 * 1024 * 1024.0, benchmark::Counter::kIs1000,
                        "1\\.07374G")));

TEST_P(HumanReadableFixture, HumanReadableNumber) {
  std::string str = benchmark::HumanReadableNumber(std::get<0>(GetParam()),
                                                   std::get<1>(GetParam()));
  ASSERT_THAT(str, ::testing::MatchesRegex(std::get<2>(GetParam())));
}

TEST(StringUtilTest, CsvEscape) {
  // Empty string
  EXPECT_EQ(benchmark::internal::CsvEscape(""), "\"\"");

  // Standard safe strings
  EXPECT_EQ(benchmark::internal::CsvEscape("BM_basic"), "\"BM_basic\"");
  EXPECT_EQ(benchmark::internal::CsvEscape("hello world"), "\"hello world\"");

  // Embedded double-quotes
  EXPECT_EQ(benchmark::internal::CsvEscape("foo\"bar"), "\"foo\"\"bar\"");

  // Formula prefixes neutralized with leading single-quote
  EXPECT_EQ(benchmark::internal::CsvEscape("=1+1"), "\"'=1+1\"");
  EXPECT_EQ(benchmark::internal::CsvEscape("=SUM(A1:A2)"), "\"'=SUM(A1:A2)\"");
  EXPECT_EQ(benchmark::internal::CsvEscape("+cmd"), "\"'+cmd\"");
  EXPECT_EQ(benchmark::internal::CsvEscape("-10"), "\"'-10\"");
  EXPECT_EQ(benchmark::internal::CsvEscape("@admin"), "\"'@admin\"");

  // Formula prefix with embedded quotes and carriage return
  EXPECT_EQ(benchmark::internal::CsvEscape("=cmd|' /C calc'!A0"),
            "\"'=cmd|' /C calc'!A0\"");
  EXPECT_EQ(benchmark::internal::CsvEscape("=foo\r\n\"bar\""),
            "\"'=foo\r\n\"\"bar\"\"\"");
}

TEST(StringUtilTest, JsonStrEscape) {
  // Empty string
  EXPECT_EQ(benchmark::internal::JsonStrEscape(""), "");

  // Safe strings
  EXPECT_EQ(benchmark::internal::JsonStrEscape("hello world"), "hello world");

  // Standard short escapes
  EXPECT_EQ(benchmark::internal::JsonStrEscape("\"quoted\\backslash\""),
            "\\\"quoted\\\\backslash\\\"");
  EXPECT_EQ(benchmark::internal::JsonStrEscape(
                "tab\tnewline\nreturn\rbackspace\bformfeed\f"),
            "tab\\tnewline\\nreturn\\rbackspace\\bformfeed\\f");

  // C0 control characters (RFC 8259 Section 7: 0x00 to 0x1F)
  EXPECT_EQ(benchmark::internal::JsonStrEscape(std::string("\x00", 1)),
            "\\u0000");
  EXPECT_EQ(benchmark::internal::JsonStrEscape("\x01"), "\\u0001");
  EXPECT_EQ(benchmark::internal::JsonStrEscape("\x07"), "\\u0007");
  EXPECT_EQ(benchmark::internal::JsonStrEscape("\x0b"), "\\u000b");
  EXPECT_EQ(benchmark::internal::JsonStrEscape("\x1b"), "\\u001b");
  EXPECT_EQ(benchmark::internal::JsonStrEscape("\x1f"), "\\u001f");

  // ANSI escape sequences in skip/error messages
  EXPECT_EQ(benchmark::internal::JsonStrEscape("\x1b[31mred text\x1b[0m"),
            "\\u001b[31mred text\\u001b[0m");

  // Printable ASCII and UTF-8 multi-byte characters remain unescaped
  EXPECT_EQ(benchmark::internal::JsonStrEscape(" 0123456789!@#$%^&*()~`"),
            " 0123456789!@#$%^&*()~`");
  EXPECT_EQ(benchmark::internal::JsonStrEscape("中文测试"), "中文测试");
}

}  // end namespace
