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

#include "benchmark/benchmark.h"
#include "check.h"
#include "internal_macros.h"

#include <cstring>

namespace benchmark {

Counter::Counter()
  : name_(nullptr), value_(0.), flags_(kDefaults) {
}

Counter::Counter(const char* name)
  : name_(nullptr), value_(0.), flags_(kDefaults) {
  _SetName(name);
}

Counter::Counter(const char* name, double v)
  : name_(nullptr), value_(v), flags_(kDefaults) {
  _SetName(name);
}

Counter::Counter(const char* name, double v, uint32_t f)
  : name_(nullptr), value_(v), flags_(f) {
  _SetName(name);
}

Counter::Counter(Counter const& that)
  : name_(nullptr), value_(that.value_), flags_(that.flags_) {
  _SetName(that.name_);
}
Counter& Counter::operator= (Counter const& that) {
  if(&that == this) return *this;
  _SetName(nullptr);
  _SetName(that.name_);
  value_ = that.value_;
  flags_ = that.flags_;
  return *this;
}

#ifdef BENCHMARK_HAS_CXX11
Counter::Counter(Counter && that)
  : name_(that.name_), value_(that.value_), flags_(that.flags_) {
  that._SetName(nullptr);
}
Counter& Counter::operator= (Counter && that) {
  if(&that == this) return *this;
  _SetName(nullptr);
  name_ = that.name_;
  value_ = that.value_;
  flags_ = that.flags_;
  that._SetName(nullptr);
  return *this;
}
#endif // BENCHMARK_HAS_CXX11

Counter::~Counter() {
  _SetName(nullptr);
}

void Counter::_SetName(const char *n) {
  if(n) {
    size_t sz = strlen(n);
    name_ = new char [sz + 1];
    strncpy(name_, n, sz);
    name_[sz] = '\0';
  } else {
    if(name_) {
      delete [] name_;
      name_ = nullptr;
    }
  }
}

bool Counter::SameName(const char *n) const {
    int stat = strcmp(name_, n);
    return stat == 0;
}
void Counter::Finish(double cpu_time, double num_threads) {
  if(flags_ & kIsRate) {
    value_ /= cpu_time;
  }
  if(flags_ & kAvgThreads) {
    value_ /= num_threads;
  }
}
/*
std::string Counter::ToString() const {
  std::string s;
  if(flags & _kWasReported) {
    if(flags & kHumanReadable) {
      s = HumanReadableNumber(value);
      s += name;
    } else {
      char tmp[256];
      snprintf(tmp, sizeof(tmp), format.c_str(), value);
      s = tmp;
    }
  }
  return s;
}
*/


BenchmarkCounters::BenchmarkCounters()
 : counters_(nullptr), num_counters_(0), capacity_(0) {
  _Reserve(16); // minimize reallocations
}

BenchmarkCounters::~BenchmarkCounters() {
  _Reserve(0);
}

#ifdef BENCHMARK_HAS_CXX11
BenchmarkCounters::BenchmarkCounters(BenchmarkCounters && that)
 : counters_(that.counters_), num_counters_(that.num_counters_), capacity_(that.capacity_) {
  that.counters_ = nullptr;
  that.num_counters_ = 0;
  that.capacity_ = 0;
}
BenchmarkCounters& BenchmarkCounters::operator= (BenchmarkCounters && that) {
  counters_ = that.counters_;
  num_counters_ = that.num_counters_;
  capacity_ = that.capacity_;
  that.counters_ = nullptr;
  that.num_counters_ = 0;
  that.capacity_ = 0;
  return *this;
}
#endif // BENCHMARK_HAS_CXX11

BenchmarkCounters::BenchmarkCounters(BenchmarkCounters const& that)
 : counters_(nullptr), num_counters_(0), capacity_(0) {
  _Reserve(that.num_counters_);
  num_counters_ = that.num_counters_; // reserve does not change num_counters_ unless it's cleaning up
  memcpy(counters_, that.counters_, num_counters_ * sizeof(Counter));
}
BenchmarkCounters& BenchmarkCounters::operator= (BenchmarkCounters const& that) {
  _Reserve(that.num_counters_);
  num_counters_ = that.num_counters_; // reserve does not change num_counters_ unless it's cleaning up
  memcpy(counters_, that.counters_, num_counters_ * sizeof(Counter));
  return *this;
}

void BenchmarkCounters::_Reserve(size_t sz) {
  if(sz == 0) {
    // call the destructor in the counters
    for(size_t i = 0; i < num_counters_; ++i) {
      counters_[i].~Counter();
    }
    delete [] counters_;
    num_counters_ = 0;
    capacity_ = 0;
    counters_ = nullptr;
  } else if(sz != capacity_) {
    Counter *tmp = new Counter[sz];
    size_t min = sz < num_counters_ ? sz : num_counters_;
    if(counters_) {
      memcpy(tmp, counters_, min * sizeof(Counter));
      delete [] counters_;
    }
    counters_ = tmp;
    capacity_ = sz;
  }
}

// this just creates memory space - it does not invoke the constructor
size_t BenchmarkCounters::_Add() {
  if(capacity_ == 0) {
    _Reserve(16);
  } else if(num_counters_ == capacity_-1) {
    _Reserve(2 * capacity_);
  }
  size_t c = num_counters_++;
  return c;
}

size_t BenchmarkCounters::Set(Counter const& c) {
  size_t pos = Find(c.Name());
  if(pos < num_counters_) {
    counters_[pos] = c;
  } else {
    pos = _Add(); // get room
    Counter *tmp = counters_ + pos; // it's here
    tmp = new (tmp) Counter(c); // call the ctor using placement new
    (void)tmp; // prevent unused warning
  }
  return pos;
}

size_t BenchmarkCounters::Set(const char *n, double v, uint32_t f) {
  size_t pos = Find(n);
  if(pos < num_counters_) {
    counters_[pos].Set(v);
  } else {
    pos = _Add(); // get room
    Counter *tmp = counters_ + pos; // it's here
    tmp = new (tmp) Counter(n, v, f); // call the ctor placement new
    (void)tmp; // prevent unused warning
  }
  return pos;
}

Counter& BenchmarkCounters::Get(size_t id) {
  CHECK_LT(id, num_counters_) << "Invalid counter handle: " << id;
  return counters_[id];
}
Counter const& BenchmarkCounters::Get(size_t id) const {
  CHECK_LT(id, num_counters_) << "Invalid counter handle: " << id;
  return counters_[id];
}
Counter& BenchmarkCounters::Get(const char *name) {
  size_t id = Find(name);
  CHECK_EQ(Exists(name), true) << "Counter not found: " << name;
  return counters_[id];
}
Counter const& BenchmarkCounters::Get(const char *name) const {
  size_t id = Find(name);
  CHECK_EQ(Exists(name), true) << "Counter not found: " << name;
  return counters_[id];
}


bool BenchmarkCounters::SameNames(BenchmarkCounters const& that) const {
  if(this == &that) return true;
  if(that.num_counters_ != num_counters_) {
    return false;
  }
  for(size_t i = 0; i < num_counters_; ++i) {
    Counter const& c = counters_[i];
    if(that.Find(c.Name()) == that.num_counters_) {
      return false;
    }
  }
  return true;
}

// returns the index where name is located,
// or size() if the name was not found
size_t BenchmarkCounters::Find(const char *name) const {
  size_t pos = 0;
  for(size_t i = 0; i < num_counters_; ++i) {
    Counter const& c = counters_[i];
    if(c.SameName(name)) break;
    ++pos;
  }
  return pos;
}

void BenchmarkCounters::Sum(BenchmarkCounters const& that) {
  // add counters present in both or just in this
  for(size_t i = 0; i < num_counters_; ++i) {
    Counter & c = counters_[i];
    size_t j = that.Find(c.Name());
    if(j < that.num_counters_) {
      c.Set(c.Value() + that.counters_[j].Value());
    }
  }
  // add counters present in that, but not in this
  for(size_t i = 0; i < that.num_counters_; ++i) {
    Counter const& tc = that.counters_[i];
    size_t j = Find(tc.Name());
    if(j >= num_counters_) {
      Set(tc);
    }
  }
}

void BenchmarkCounters::Finish(double cpu_time, double num_threads) {
  for(size_t i = 0; i < num_counters_; ++i) {
    Counter & c = counters_[i];
    c.Finish(cpu_time, num_threads);
  }
}

void BenchmarkCounters::Clear() {
  for(size_t i = 0; i < num_counters_; ++i) {
    Counter & c = counters_[i];
    c.Set(0.);
  }
}

/*
// Common counters
Counter const& BenchmarkCounters::GetBytesPerSecond() const {
    size_t pos = _Find("B/s");
    CHECK_LT(pos, counters_.size()) << "Could not find counter \"B/s\"";
    return counters_[pos];
}
size_t  BenchmarkCounters::BytesPerSecond() const {
    size_t pos = _Find("B/s");
    return pos < counters_.size() ? counters_[pos].value : 0;
}
void    BenchmarkCounters::BytesPerSecond(size_t b) {
    size_t pos = _Find("B/s");
    if(pos < counters_.size()) {
        counters_[pos].value = double(b);
    } else {
        Add("B/s", double(b), "", Counter::kBytesProcessed);
    }
}

Counter const& BenchmarkCounters::GetItemsPerSecond() const {
    size_t pos = _Find("items/s");
    CHECK_LT(pos, counters_.size()) << "Could not find counter \" items/s\"";
    return counters_[pos];
}
size_t  BenchmarkCounters::ItemsPerSecond() const
{
    size_t pos = _Find("items/s");
    return pos < counters_.size() ? counters_[pos].value : 0;
}
void    BenchmarkCounters::ItemsPerSecond(size_t i) {
    size_t pos = _Find("items/s");
    if(pos < counters_.size()) {
        counters_[pos].value = double(i);
    } else {
        Add("items/s", double(i), "", Counter::kItemsProcessed);
    }
}
*/


} // end namespace benchmark
