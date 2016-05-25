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

#include "benchmark/complexity.h"
#include "check.h"
#include <math.h>
#include <functional>

namespace benchmark {
  
// Internal function to calculate the different scalability forms
std::function<double(int)> FittingCurve(BigO complexity) {
  switch (complexity) {
    case oN:
      return [](int n) {return n; };
    case oNSquared:
      return [](int n) {return n*n; };
    case oNCubed:
      return [](int n) {return n*n*n; };
    case oLogN:
      return [](int n) {return log2(n); };
    case oNLogN:
      return [](int n) {return n * log2(n); };
    case o1:
    default:
      return [](int) {return 1; };
  }
}

// Function to to return an string for the calculated complexity
std::string GetBigOString(BigO complexity) {
  switch (complexity) {
    case oN:
      return "* N";
    case oNSquared:
      return "* N**2";
    case oNCubed:
      return "* N**3";
    case oLogN:
      return "* lgN";
    case oNLogN:
      return "* NlgN";
    case o1:
      return "* 1";
    default:
      return "";
  }
}

// Find the coefficient for the high-order term in the running time, by minimizing the sum of squares of relative error, for the fitting curve given on the lambda expresion.
//   - n             : Vector containing the size of the benchmark tests.
//   - time          : Vector containing the times for the benchmark tests.
//   - fitting_curve : lambda expresion (e.g. [](int n) {return n; };).
// For a deeper explanation on the algorithm logic, look the README file at
// http://github.com/ismaelJimenez/Minimal-Cpp-Least-Squared-Fit

// This interface is currently not used from the oustide, but it has been provided 
// for future upgrades. If in the future it is not needed to support Cxx03, then 
// all the calculations could be upgraded to use lambdas because they are more 
// powerful and provide a cleaner inferface than enumerators, but complete 
// implementation with lambdas will not work for Cxx03 (e.g. lack of std::function).
// In case lambdas are implemented, the interface would be like :
//   -> Complexity([](int n) {return n;};)
// and any arbitrary and valid  equation would be allowed, but the option to calculate
// the best fit to the most common scalability curves will still be kept.

LeastSq CalculateLeastSq(const std::vector<int>& n, 
                         const std::vector<double>& time, 
                         std::function<double(int)> fitting_curve) {
  double sigma_gn = 0;
  double sigma_gn_squared = 0;
  double sigma_time = 0;
  double sigma_time_gn = 0;

  // Calculate least square fitting parameter
  for (size_t i = 0; i < n.size(); ++i) {
    double gn_i = fitting_curve(n[i]);
    sigma_gn += gn_i;
    sigma_gn_squared += gn_i * gn_i;
    sigma_time += time[i];
    sigma_time_gn += time[i] * gn_i;
  }

  LeastSq result;

  // Calculate complexity.
  result.coef = sigma_time_gn / sigma_gn_squared;

  // Calculate RMS
  double rms = 0;
  for (size_t i = 0; i < n.size(); ++i) {
    double fit = result.coef * fitting_curve(n[i]);
    rms += pow((time[i] - fit), 2);
  }

  // Normalized RMS by the mean of the observed values
  double mean = sigma_time / n.size();
  result.rms = sqrt(rms / n.size()) / mean;

  return result;
}

// Find the coefficient for the high-order term in the running time, by
// minimizing the sum of squares of relative error.
//   - n          : Vector containing the size of the benchmark tests.
//   - time       : Vector containing the times for the benchmark tests.
//   - complexity : If different than oAuto, the fitting curve will stick to
//                  this one. If it is oAuto, it will be calculated the best
//                  fitting curve.
LeastSq MinimalLeastSq(const std::vector<int>& n,
                       const std::vector<double>& time,
                       const BigO complexity) {
  CHECK_EQ(n.size(), time.size());
  CHECK_GE(n.size(), 2);  // Do not compute fitting curve is less than two benchmark runs are given
  CHECK_NE(complexity, oNone);

  LeastSq best_fit;

  if(complexity == oAuto) {
    std::vector<BigO> fit_curves = {
      oLogN, oN, oNLogN, oNSquared, oNCubed };

    // Take o1 as default best fitting curve
    best_fit = CalculateLeastSq(n, time, FittingCurve(o1));
    best_fit.complexity = o1;

    // Compute all possible fitting curves and stick to the best one
    for (const auto& fit : fit_curves) {
      LeastSq current_fit = CalculateLeastSq(n, time, FittingCurve(fit));
      if (current_fit.rms < best_fit.rms) {
        best_fit = current_fit;
        best_fit.complexity = fit;
      }
    }
  } else {
    best_fit = CalculateLeastSq(n, time, FittingCurve(complexity));
    best_fit.complexity = complexity;
  }

  return best_fit;
}

}  // end namespace benchmark
