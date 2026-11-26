#include <locale>
#include <sstream>
#include <string>

#include "benchmark/reporter.h"
#include "gtest/gtest.h"

namespace {

class CommaDecimal : public std::numpunct<char> {
 protected:
  char do_decimal_point() const override { return ','; }
  char do_thousands_sep() const override { return '.'; }
  std::string do_grouping() const override { return "\3"; }
};

class JSONReporterLocaleTest : public testing::Test {
 protected:
  JSONReporterLocaleTest()
      : original_locale_(std::locale()),
        comma_locale_(std::locale::classic(), new CommaDecimal) {
    std::locale::global(std::locale::classic());
  }

  ~JSONReporterLocaleTest() override { std::locale::global(original_locale_); }

  void CheckLoadAverages(std::ios_base::fmtflags floatfield,
                         std::streamsize precision) {
    benchmark::BenchmarkReporter::Context context;
    if (context.cpu_info.load_avg.empty()) {
      GTEST_SKIP() << "Load averages are unavailable on this platform";
    }

    std::ostringstream out;
    out.imbue(comma_locale_);
    out.setf(floatfield, std::ios_base::floatfield);
    out.precision(precision);
    const auto flags = out.flags();

    std::ostringstream expected;
    expected.imbue(std::locale::classic());
    expected.flags(flags);
    expected.precision(precision);
    expected << "\"load_avg\": [";
    for (size_t i = 0; i < context.cpu_info.load_avg.size(); ++i) {
      if (i != 0) expected << ',';
      expected << context.cpu_info.load_avg[i];
    }
    expected << ']';

    benchmark::JSONReporter reporter;
    reporter.SetOutputStream(&out);
    ASSERT_TRUE(reporter.ReportContext(context));
    reporter.Finalize();

    EXPECT_NE(out.str().find(expected.str()), std::string::npos) << out.str();
    EXPECT_EQ(out.getloc(), comma_locale_);
    EXPECT_EQ(out.flags(), flags);
    EXPECT_EQ(out.precision(), precision);
    EXPECT_EQ(std::locale(), std::locale::classic());
  }

  const std::locale original_locale_;
  const std::locale comma_locale_;
};

TEST_F(JSONReporterLocaleTest, FloatingFieldsIgnoreGlobalLocale) {
  std::locale::global(comma_locale_);
  std::ostringstream out;
  out.imbue(std::locale::classic());

  benchmark::BenchmarkReporter::Run run;
  run.run_name.function_name = "locale_probe";
  run.family_index = 0;
  run.per_family_instance_index = 0;
  run.repetitions = 1;
  run.repetition_index = 0;
  run.time_unit = benchmark::kSecond;
  run.real_accumulated_time = 1.25;
  run.cpu_accumulated_time = 2.5;
  run.counters["fraction"] = 1234.5;

  benchmark::JSONReporter reporter;
  reporter.SetOutputStream(&out);
  reporter.ReportRuns({run});

  EXPECT_NE(out.str().find("\"real_time\": 1.2500000000000000e+00"),
            std::string::npos);
  EXPECT_NE(out.str().find("\"cpu_time\": 2.5000000000000000e+00"),
            std::string::npos);
  EXPECT_NE(out.str().find("\"fraction\": 1.2345000000000000e+03"),
            std::string::npos);
  EXPECT_EQ(out.getloc(), std::locale::classic());
  EXPECT_EQ(std::locale(), comma_locale_);
}

TEST_F(JSONReporterLocaleTest, LoadAveragesIgnoreStreamLocale) {
  CheckLoadAverages(std::ios_base::fmtflags(0), 6);
}

TEST_F(JSONReporterLocaleTest, LoadAveragesPreserveFormatting) {
  CheckLoadAverages(std::ios_base::fixed, 12);
}

}  // namespace
