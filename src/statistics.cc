// Copyright 2016 Ismael Jimenez Martinez. All rights reserved.
// Copyright 2017 Roman Lebedev. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "statistics.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "benchmark/reporter.h"
#include "benchmark/statistics.h"
#include "benchmark/types.h"
#include "check.h"

namespace benchmark {

const auto StatisticsSum = [](const std::vector<double>& v) {
  return std::accumulate(v.begin(), v.end(), 0.0);
};

double StatisticsMean(const std::vector<double>& v) {
  if (v.empty()) {
    return 0.0;
  }
  return StatisticsSum(v) * (1.0 / static_cast<double>(v.size()));
}

double StatisticsMedian(const std::vector<double>& v) {
  if (v.size() < 3) {
    return StatisticsMean(v);
  }
  std::vector<double> copy(v);

  auto center = copy.begin() + v.size() / 2;
  std::nth_element(copy.begin(), center, copy.end());

  // Did we have an odd number of samples?  If yes, then center is the median.
  // If not, then we are looking for the average between center and the value
  // before.  Instead of resorting, we just look for the max value before it,
  // which is not necessarily the element immediately preceding `center` Since
  // `copy` is only partially sorted by `nth_element`.
  if (v.size() % 2 == 1) {
    return *center;
  }
  auto center2 = std::max_element(copy.begin(), center);
  return (*center + *center2) / 2.0;
}

// Return the sum of the squares of this sample set
const auto SumSquares = [](const std::vector<double>& v) {
  return std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
};

const auto Sqr = [](const double dat) { return dat * dat; };
const auto Sqrt = [](const double dat) {
  // Avoid NaN due to imprecision in the calculations
  if (dat < 0.0) {
    return 0.0;
  }
  return std::sqrt(dat);
};

double StatisticsStdDev(const std::vector<double>& v) {
  const auto mean = StatisticsMean(v);
  if (v.empty()) {
    return mean;
  }

  // Sample standard deviation is undefined for n = 1
  if (v.size() == 1) {
    return 0.0;
  }

  const double avg_squares =
      SumSquares(v) * (1.0 / static_cast<double>(v.size()));
  return Sqrt(static_cast<double>(v.size()) /
              (static_cast<double>(v.size()) - 1.0) *
              (avg_squares - Sqr(mean)));
}

double StatisticsCV(const std::vector<double>& v) {
  if (v.size() < 2) {
    return 0.0;
  }

  const auto stddev = StatisticsStdDev(v);
  const auto mean = StatisticsMean(v);

  if (std::fpclassify(mean) == FP_ZERO) {
    return 0.0;
  }

  return stddev / mean;
}

std::vector<BenchmarkReporter::Run> ComputeAggregateReports(
    const BenchmarkReporter::Run& base,
    const std::vector<internal::Statistics>& statistics,
    const std::vector<double>& real_times, const std::vector<double>& cpu_times,
    const std::map<std::string, AggregateCounterSamples>& counters,
    IterationCount iterations_per_sample, const std::string& name_prefix,
    int64_t repetition_index, const std::string& report_label) {
  typedef BenchmarkReporter::Run Run;
  std::vector<Run> results;
  if (real_times.empty()) {
    return results;
  }

  const IterationCount rescale_iterations =
      iterations_per_sample == 0 ? 1 : iterations_per_sample;
  const double iteration_rescale_factor =
      static_cast<double>(real_times.size()) /
      static_cast<double>(rescale_iterations);

  for (const auto& Stat : statistics) {
    Run data;
    data.run_name = base.run_name;
    data.family_index = base.family_index;
    data.per_family_instance_index = base.per_family_instance_index;
    data.run_type = BenchmarkReporter::Run::RT_Aggregate;
    data.threads = base.threads;
    data.repetitions = base.repetitions;
    data.repetition_index = repetition_index;
    data.aggregate_name = name_prefix + Stat.name_;
    data.aggregate_unit = Stat.unit_;
    data.report_label = report_label;
    // Report the number of samples, not the iterations already averaged
    // inside each sample. See ComputeStats for the same rule.
    data.iterations = static_cast<IterationCount>(real_times.size());
    data.real_accumulated_time = Stat.compute_(real_times);
    data.cpu_accumulated_time = Stat.compute_(cpu_times);
    if (data.aggregate_unit == StatisticUnit::kTime) {
      data.real_accumulated_time *= iteration_rescale_factor;
      data.cpu_accumulated_time *= iteration_rescale_factor;
    }
    data.time_unit = base.time_unit;
    for (const auto& kv : counters) {
      // Do *NOT* rescale the custom counters. They are already properly scaled.
      const auto uc_stat = Stat.compute_(kv.second.values);
      data.counters[kv.first] =
          Counter(uc_stat, kv.second.counter.flags, kv.second.counter.oneK);
    }
    results.push_back(std::move(data));
  }
  return results;
}

std::vector<BenchmarkReporter::Run> ComputeStats(
    const std::vector<BenchmarkReporter::Run>& reports) {
  typedef BenchmarkReporter::Run Run;
  std::vector<Run> results;

  const auto is_successful = [](Run const& run) {
    return run.skipped == internal::NotSkipped;
  };
  auto successful_run =
      std::find_if(reports.begin(), reports.end(), is_successful);
  const auto successful_count = static_cast<size_t>(
      std::count_if(reports.begin(), reports.end(), is_successful));

  if (successful_count < 2) {
    // We don't report aggregated data if there was a single run.
    return results;
  }

  // Accumulators.
  std::vector<double> real_accumulated_time_stat;
  std::vector<double> cpu_accumulated_time_stat;

  real_accumulated_time_stat.reserve(successful_count);
  cpu_accumulated_time_stat.reserve(successful_count);

  // All repetitions should be run with the same number of iterations so we
  // can take this information from the first benchmark.
  const IterationCount run_iterations = successful_run->iterations;
  // create stats for user counters
  std::map<std::string, AggregateCounterSamples> counter_stats;
  for (Run const& r : reports) {
    if (!is_successful(r)) {
      continue;
    }
    for (auto const& cnt : r.counters) {
      auto it = counter_stats.find(cnt.first);
      if (it == counter_stats.end()) {
        it = counter_stats
                 .emplace(cnt.first,
                          AggregateCounterSamples{cnt.second, {}})
                 .first;
        it->second.values.reserve(successful_count);
      } else {
        BM_CHECK_EQ(it->second.counter.flags, cnt.second.flags);
      }
    }
  }

  // Populate the accumulators.
  for (Run const& run : reports) {
    BM_CHECK_EQ(successful_run->benchmark_name(), run.benchmark_name());
    if (!is_successful(run)) {
      continue;
    }
    BM_CHECK_EQ(run_iterations, run.iterations);
    real_accumulated_time_stat.emplace_back(run.real_accumulated_time);
    cpu_accumulated_time_stat.emplace_back(run.cpu_accumulated_time);
    // user counters
    for (auto const& cnt : run.counters) {
      auto it = counter_stats.find(cnt.first);
      BM_CHECK_NE(it, counter_stats.end());
      it->second.values.emplace_back(cnt.second);
    }
  }

  // Only add label if it is same for all runs
  std::string report_label = successful_run->report_label;
  for (const Run& run : reports) {
    if (!is_successful(run)) {
      continue;
    }
    if (run.report_label != report_label) {
      report_label = "";
      break;
    }
  }

  // It is incorrect to say that an aggregate is computed over a run's
  // iterations, because those iterations already got averaged. Time statistics
  // are rescaled by (sample count / iterations) inside ComputeAggregateReports
  // so the reported unit stays "per iteration" while iterations counts samples.
  return ComputeAggregateReports(
      *successful_run, *successful_run->statistics, real_accumulated_time_stat,
      cpu_accumulated_time_stat, counter_stats, run_iterations,
      /*name_prefix=*/"", Run::no_repetition_index, report_label);
}

}  // end namespace benchmark
