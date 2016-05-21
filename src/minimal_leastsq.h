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
// Addapted to be used with google benchmark

#if !defined(MINIMAL_LEASTSQ_H_)
#define MINIMAL_LEASTSQ_H_

#include "benchmark/benchmark_api.h"

#include <vector>

// This data structure will contain the result returned vy minimalLeastSq
//   - coef        : Estimated coeficient for the high-order term as interpolated from data.
//   - rms         : Normalized Root Mean Squared Error.
//   - complexity  : Scalability form (e.g. O_N, O_N_log_N). In case a scalability form has been provided to minimalLeastSq
//                   this will return the same value. In case BigO::O_Auto has been selected, this parameter will return the 
//                   best fitting curve detected.

struct LeastSq {
	LeastSq() :
		coef(0),
		rms(0),
		complexity(benchmark::O_None) {}

	double coef;
	double rms;
	benchmark::BigO   complexity;
};

// Find the coefficient for the high-order term in the running time, by minimizing the sum of squares of relative error.
LeastSq minimalLeastSq(const std::vector<int>& N, const std::vector<double>& Time, const benchmark::BigO Complexity = benchmark::O_Auto);

#endif
