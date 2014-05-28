// Copyright 2014 Google Inc. All rights reserved.
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

#ifndef BENCHMARK_RE_H_
#define BENCHMARK_RE_H_

#include "port.h"

#if defined HAVE_GNUREGEX_H
#include <gnuregex.h>
#elif defined HAVE_REGEX_H
#include <regex.h>
#elif defined HAVE_REGEX
#include <regex>
#endif
#include <string>

namespace benchmark {

// A wrapper around the POSIX regular expression API that provides automatic
// cleanup
class Regex {
 public:
  Regex();
  ~Regex();

  // Compile a regular expression matcher from spec.  Returns true on success.
  //
  // On failure (and if error is not NULL), error is populated with a human
  // readable error message if an error occurs.
  bool Init(const std::string& spec, std::string* error);

  // Returns whether str matches the compiled regular expression.
  bool Match(const std::string& str);
 private:
  bool init_;
  // Underlying regular expression object
#if defined HAVE_REGEX
  std::regex re_;
#else
  regex_t re_;
#endif
};

}  // end namespace benchmark

#endif  // BENCHMARK_RE_H_
