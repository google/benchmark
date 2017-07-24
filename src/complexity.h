// Copyright 2016 Ismael Jimenez Martinez. All rights reserved.
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

// Source project : https://github.com/ismaelJimenez/cpp.leastsq
// Adapted to be used with google benchmark

#ifndef COMPLEXITY_H_
#define COMPLEXITY_H_

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <numeric>

#include "benchmark/benchmark.h"

namespace benchmark {

// Return a vector containing the mean and standard devation information for
// the specified list of reports. If 'reports' contains less than two
// non-errored runs an empty vector is returned
std::vector<BenchmarkReporter::Run> ComputeStats(
    const std::vector<BenchmarkReporter::Run>& reports);

// Return a vector containing the bigO and RMS information for the specified
// list of reports. If 'reports.size() < 2' an empty vector is returned.
std::vector<BenchmarkReporter::Run> ComputeBigO(
    const std::vector<BenchmarkReporter::Run>& reports);

// This data structure will contain the result returned by MinimalLeastSq
//   - coef        : Estimated coeficient for the high-order term as
//                   interpolated from data.
//   - rms         : Normalized Root Mean Squared Error.
//   - complexity  : Scalability form (e.g. oN, oNLogN). In case a scalability
//                   form has been provided to MinimalLeastSq this will return
//                   the same value. In case BigO::oAuto has been selected, this
//                   parameter will return the best fitting curve detected.

struct LeastSq {
  LeastSq() : coef(0.0), rms(0.0), complexity(oNone) {}

  double coef;
  double rms;
  BigO complexity;
};

// Function to return an string for the calculated complexity
std::string GetBigOString(BigO complexity);

auto StatisticsSum = [](const std::vector<double>& v) {
  return std::accumulate(v.begin(), v.end(), double());
};

auto StatisticsMean = [](const std::vector<double>& v) {
  if (v.size() == 0) return double();
  return StatisticsSum(v) * (1.0 / v.size());
};

auto StatisticsMedian = [](const std::vector<double>& v) {
  if (v.size() < 3) return StatisticsMean(v);
  std::vector<double> partial;
  // we need roundDown(count/2)+1 slots
  partial.resize(1 + (v.size() / 2));
  std::partial_sort_copy(v.begin(), v.end(), partial.begin(), partial.end());
  double median;
  // did we have odd number of samples?
  // if yes, then the last element of partially-sorted vector is the median
  // it no, then the average of the last two elements is the median
  if(v.size() % 2 == 1)
    median = partial.back();
  else
    median = (partial[partial.size() - 2] + partial[partial.size() - 1]) / 2.0;
  return median;
};

// Return the sum of the squares of this sample set
auto SumSquares = [](const std::vector<double>& v) {
  return std::inner_product(v.begin(), v.end(), v.begin(), double());
};

auto Sqr = [](const double dat) { return dat * dat; };
auto Sqrt = [](const double dat) {
  // Avoid NaN due to imprecision in the calculations
  if (dat < 0.0) return 0.0;
  return std::sqrt(dat);
};

auto StatisticsStdDev = [](const std::vector<double>& v) {
  const auto mean = StatisticsMean(v);
  if (v.size() == 0) return mean;

  // Sample standard deviation is undefined for n = 1
  if (v.size() == 1)
    return double();

  const double avg_squares = SumSquares(v) * (1.0 / v.size());
  return Sqrt(v.size() / (v.size() - 1.0) * (avg_squares - Sqr(mean)));
};

}  // end namespace benchmark
#endif  // COMPLEXITY_H_
