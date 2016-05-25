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

#if !defined(MINIMAL_LEASTSQ_H_)
#define MINIMAL_LEASTSQ_H_

#include "benchmark/benchmark_api.h"

#include <vector>
#include <functional>

// This data structure will contain the result returned by MinimalLeastSq
//   - coef        : Estimated coeficient for the high-order term as
//                   interpolated from data.
//   - rms         : Normalized Root Mean Squared Error.
//   - complexity  : Scalability form (e.g. oN, oNLogN). In case a scalability
//                   form has been provided to MinimalLeastSq this will return
//                   the same value. In case BigO::oAuto has been selected, this
//                   parameter will return the best fitting curve detected.

struct LeastSq {
  LeastSq() :
    coef(0),
    rms(0),
    complexity(benchmark::oNone),
    caption("") {}

  double coef;
  double rms;
  benchmark::BigO complexity;
  std::string caption;
};

// Find the coefficient for the high-order term in the running time, by
// minimizing the sum of squares of relative error.
LeastSq MinimalLeastSq(const std::vector<int>& n,
                       const std::vector<double>& time,
                       const benchmark::BigO complexity = benchmark::oAuto);

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
                         std::function<double(int)> fitting_curve);


#endif
