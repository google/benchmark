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

#include "minimal_leastsq.h"
#include "check.h"
#include <math.h>

// Internal function to calculate the different scalability forms
double FittingCurve(double n, benchmark::BigO complexity) {
  switch (complexity) {
    case benchmark::oN:
      return n;
    case benchmark::oNSquared:
      return pow(n, 2);
    case benchmark::oNCubed:
      return pow(n, 3);
    case benchmark::oLogN:
      return log2(n);
    case benchmark::oNLogN:
      return n * log2(n);
    case benchmark::o1:
    default:
      return 1;   
  }
}

// Internal function to find the coefficient for the high-order term in the running time, by minimizing the sum of squares of relative error.
//   - n          : Vector containing the size of the benchmark tests.
//   - time       : Vector containing the times for the benchmark tests.
//   - complexity : Fitting curve.
// For a deeper explanation on the algorithm logic, look the README file at http://github.com/ismaelJimenez/Minimal-Cpp-Least-Squared-Fit

LeastSq CalculateLeastSq(const std::vector<int>& n, const std::vector<double>& time, const benchmark::BigO complexity) {
  CHECK_NE(complexity, benchmark::oAuto);

  double sigma_gn = 0;
  double sigma_gn_squared = 0;
  double sigma_time = 0;
  double sigma_time_gn = 0;

  // Calculate least square fitting parameter
  for (size_t i = 0; i < n.size(); ++i) {
    double gn_i = FittingCurve(n[i], complexity);
    sigma_gn += gn_i;
    sigma_gn_squared += gn_i * gn_i;
    sigma_time += time[i];
    sigma_time_gn += time[i] * gn_i;
  }

  LeastSq result;
  result.complexity = complexity;

  // Calculate complexity. 
  // o1 is treated as an special case
  if (complexity != benchmark::o1)
    result.coef = sigma_time_gn / sigma_gn_squared;
  else
    result.coef = sigma_time / n.size();

  // Calculate RMS
  double rms = 0;
  for (size_t i = 0; i < n.size(); ++i) {
    double fit = result.coef * FittingCurve(n[i], complexity);
    rms += pow((time[i] - fit), 2);
  }

  double mean = sigma_time / n.size();

  result.rms = sqrt(rms / n.size()) / mean; // Normalized RMS by the mean of the observed values

  return result;
}

// Find the coefficient for the high-order term in the running time, by minimizing the sum of squares of relative error.
//   - n          : Vector containing the size of the benchmark tests.
//   - time       : Vector containing the times for the benchmark tests.
//   - complexity : If different than oAuto, the fitting curve will stick to this one. If it is oAuto, it will be calculated 
//                  the best fitting curve.

LeastSq MinimalLeastSq(const std::vector<int>& n, const std::vector<double>& time, const benchmark::BigO complexity) {
  CHECK_EQ(n.size(), time.size());
  CHECK_GE(n.size(), 2);  // Do not compute fitting curve is less than two benchmark runs are given
  CHECK_NE(complexity, benchmark::oNone);

  if(complexity == benchmark::oAuto) {
    std::vector<benchmark::BigO> fit_curves = { benchmark::oLogN, benchmark::oN, benchmark::oNLogN, benchmark::oNSquared, benchmark::oNCubed };

    LeastSq best_fit = CalculateLeastSq(n, time, benchmark::o1); // Take o1 as default best fitting curve

    // Compute all possible fitting curves and stick to the best one
    for (const auto& fit : fit_curves) {
      LeastSq current_fit = CalculateLeastSq(n, time, fit);
      if (current_fit.rms < best_fit.rms)
        best_fit = current_fit;
    }

    return best_fit;
  }
  else
    return CalculateLeastSq(n, time, complexity);
}