// Copyright 2015 Google Inc. All rights reserved.
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

#include <benchmark/benchmark.h>

namespace benchmark {

BenchmarkName::BenchmarkName() : offsets_{} {}

BenchmarkName::BenchmarkName(std::string root)
    : name_(std::move(root)), offsets_{} {
  std::fill_n(offsets_, offset_count, name_.size());
}

BenchmarkName &BenchmarkName::append(Field field,
                                     const std::string &field_name) {
  for (auto e = 0u; e < enumerator_count; ++e) {
    if (is_set(field, e)) {
      // Insert the new name at the end of this field.
      auto offset = end_offset(e);
      auto inserted_characters_count = field_name.size();

      // Insert a leading separator unless we are right
      // at the beginning of the name.
      if (offset != 0) {
        name_.insert(offset++, 1, separator);
        ++inserted_characters_count;
      }

      name_.insert(offset, field_name);

      // Update all following offsets to account for the inserted characters.
      for (auto o = e; o < offset_count; ++o) {
        offsets_[o] += inserted_characters_count;
      }
    }
  }
  return *this;
}

std::string BenchmarkName::get(Field fields) const {
  std::string n;
  for (auto e = 0u; e < enumerator_count; ++e) {
    if (is_set(fields, e)) {
      auto begin = start_offset(e);
      const auto end = end_offset(e);

      if (begin == end) {
        continue;
      }

      // If we're about to start the string, drop the leading '/'.
      // Note we're iterating the fields in order, so the first
      // insert will always be at the front of 'n'.
      if (n.empty() && (name_.at(begin) == separator)) {
        ++begin;
      }

      assert(begin <= end);

      n += name_.substr(begin, end - begin);
    }
  }
  return n;
}

bool BenchmarkName::is_set(Field field, size_t enumerator_index) const {
  return field & (1u << enumerator_index);
}

size_t BenchmarkName::start_offset(size_t enumerator_index) const {
  return enumerator_index == 0 ? 0 : offsets_[enumerator_index - 1];
}

size_t BenchmarkName::end_offset(size_t enumerator_index) const {
  return enumerator_index + 1 == enumerator_count ? name_.size()
                                                  : offsets_[enumerator_index];
}

std::ostream &operator<<(std::ostream &os, const BenchmarkName &bmn) {
  return os << static_cast<std::string>(bmn);
}

BenchmarkName::Field operator|(BenchmarkName::Field lhs,
                               BenchmarkName::Field rhs) {
  using underlying_type =
      typename std::underlying_type<BenchmarkName::Field>::type;

  return static_cast<BenchmarkName::Field>(static_cast<underlying_type>(lhs) |
                                           static_cast<underlying_type>(rhs));
}

BenchmarkName::Field operator&(BenchmarkName::Field lhs,
                               BenchmarkName::Field rhs) {
  using underlying_type =
      typename std::underlying_type<BenchmarkName::Field>::type;

  return static_cast<BenchmarkName::Field>(static_cast<underlying_type>(lhs) &
                                           static_cast<underlying_type>(rhs));
}

BenchmarkName::Field operator~(BenchmarkName::Field component) {
  using underlying_type =
      typename std::underlying_type<BenchmarkName::Field>::type;

  return static_cast<BenchmarkName::Field>(
      ~static_cast<underlying_type>(component));
}

}  // namespace benchmark
