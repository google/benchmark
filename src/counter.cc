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
    : name_(nullptr),
      value_(0.),
      flags_(kDefaults),
      name_buf_(),
      name_mem_(nullptr),
      mem_size_(0) {
}

Counter::Counter(const char* name)
    : name_(nullptr),
      value_(0.),
      flags_(kDefaults),
      name_buf_(),
      name_mem_(nullptr),
      mem_size_(0) {
  SetName_(name);
}

Counter::Counter(const char* name, double v)
    : name_(nullptr),
      value_(v),
      flags_(kDefaults),
      name_buf_(),
      name_mem_(nullptr),
      mem_size_(0) {
  SetName_(name);
}

Counter::Counter(const char* name, double v, uint32_t f)
    : name_(nullptr),
      value_(v),
      flags_(f),
      name_buf_(),
      name_mem_(nullptr),
      mem_size_(0) {
  SetName_(name);
}

Counter::Counter(Counter const& that)
    : name_(nullptr),
      value_(that.value_),
      flags_(that.flags_),
      name_buf_(),
      name_mem_(nullptr),
      mem_size_(0) {
  SetName_(that.name_);
}
Counter& Counter::operator=(Counter const& that) {
  if (&that == this) return *this;
  SetName_(that.name_);
  value_ = that.value_;
  flags_ = that.flags_;
  return *this;
}

#ifdef BENCHMARK_HAS_CXX11
Counter::Counter(Counter&& that)
    : name_(that.name_ == that.name_mem_ ? that.name_mem_ : this->name_buf_),
      value_(that.value_),
      flags_(that.flags_),
      name_buf_(),
      name_mem_(that.name_mem_),
      mem_size_(that.mem_size_) {
  memcpy(name_buf_, that.name_buf_, sizeof(name_buf_));
  that.name_mem_ = nullptr;
  that.mem_size_ = 0;
  that.name_ = nullptr;
}
Counter& Counter::operator=(Counter&& that) {
  if (&that == this) return *this;
  SetName_(nullptr);
  memcpy(name_buf_, that.name_buf_, sizeof(name_buf_));
  name_mem_ = that.name_mem_;
  mem_size_ = that.mem_size_;
  name_ = that.name_ == that.name_mem_ ? that.name_mem_ : this->name_buf_;
  value_ = that.value_;
  flags_ = that.flags_;
  that.name_mem_ = nullptr;
  that.mem_size_ = 0;
  that.name_ = nullptr;
  return *this;
}
#endif  // BENCHMARK_HAS_CXX11

Counter::~Counter() { SetName_(nullptr); }

void Counter::SetName_(const char* n) {
  if (n == nullptr) {
    if (name_mem_ != nullptr) {
      delete[] name_mem_;
    }
    name_buf_[0] = '\0';
    name_mem_ = nullptr;
    mem_size_ = 0;
    name_ = nullptr;
  } else {
    size_t sz = strlen(n);
    if (sz < sizeof(name_buf_)) {  // new name fits in fixed buffer
      name_ = name_buf_;
      // keep any allocated memory as it is
    } else {  // new name does not fit fixed buffer; must use allocated memory
      if (mem_size_ == 0) {  // allocate mem
        mem_size_ = sz + 1;
        name_mem_ = new char[mem_size_];
      } else if (mem_size_ <= sz) {  // resize mem
        delete[] name_mem_;
        mem_size_ = sz + 1;
        name_mem_ = new char[mem_size_];
      }
      name_ = name_mem_;
    }
    strcpy(name_, n);  // finally, copy the string to its right place.
  }
}

bool Counter::SameName(const char* n) const {
  if (!n || !name_) return false;
  int stat = strcmp(name_, n);
  return stat == 0;
}
void Counter::Finish(double cpu_time, double num_threads) {
  if (flags_ & kIsRate) {
    value_ /= cpu_time;
  }
  if (flags_ & kAvgThreads) {
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
  Reserve_(16);  // minimize reallocations
}

BenchmarkCounters::~BenchmarkCounters() {
  Reserve_(0);
}

#ifdef BENCHMARK_HAS_CXX11
BenchmarkCounters::BenchmarkCounters(BenchmarkCounters&& that)
    : counters_(that.counters_),
      num_counters_(that.num_counters_),
      capacity_(that.capacity_) {
  that.counters_ = nullptr;
  that.num_counters_ = 0;
  that.capacity_ = 0;
}
BenchmarkCounters& BenchmarkCounters::operator=(BenchmarkCounters&& that) {
  if (&that == this) return *this;
  Reserve_(0);  // free existing memory
  counters_ = that.counters_;
  num_counters_ = that.num_counters_;
  capacity_ = that.capacity_;
  that.counters_ = nullptr;
  that.num_counters_ = 0;
  that.capacity_ = 0;
  return *this;
}
#endif  // BENCHMARK_HAS_CXX11

BenchmarkCounters::BenchmarkCounters(BenchmarkCounters const& that)
    : counters_(nullptr), num_counters_(0), capacity_(0) {
  Reserve_(that.num_counters_);
  num_counters_ = that.num_counters_;  // reserve does not change num_counters_
                                       // unless it's cleaning up
  for (size_t i = 0; i < num_counters_; ++i) {
    Counter* slot = counters_ + i;
    new (slot) Counter(that.counters_[i]);
  }
}
BenchmarkCounters& BenchmarkCounters::operator=(BenchmarkCounters const& that) {
  if (&that == this) return *this;
  Reserve_(that.num_counters_);
  num_counters_ = that.num_counters_;  // reserve does not change num_counters_
                                       // unless it's cleaning up
  for (size_t i = 0; i < num_counters_; ++i) {
    Counter* slot = counters_ + i;
    new (slot) Counter(that.counters_[i]);
  }
  return *this;
}

void BenchmarkCounters::Reserve_(size_t sz) {
  if (sz == 0) {
    // call the destructor in the counters
    for (size_t i = 0; i < num_counters_; ++i) {
      counters_[i].~Counter();
    }
    delete[] counters_;
    num_counters_ = 0;
    capacity_ = 0;
    counters_ = nullptr;
  } else if (sz > capacity_) {
    Counter* tmp = new Counter[sz];
    if (counters_) {
      size_t min = sz < num_counters_ ? sz : num_counters_;
      for (size_t i = 0; i < min; ++i) {
        new (tmp + i) Counter(std::move(counters_[i])); // placement new with move ctor
      }
      delete[] counters_;
    }
    counters_ = tmp;
    capacity_ = sz;
  }
}

// this just creates memory space - it does not invoke the constructor
size_t BenchmarkCounters::Add_() {
  if (capacity_ == 0) {
    Reserve_(16);
  } else if (num_counters_ == capacity_ - 1) {
    Reserve_(2 * capacity_);
  }
  size_t pos = num_counters_++;
  return pos;
}

Counter& BenchmarkCounters::operator[](size_t id) {
  assert("invalid counter handle" && id < num_counters_);
  //this is slow.... CHECK_LT(id, num_counters_) << "Invalid counter handle: " << id;
  return counters_[id];
}
Counter const& BenchmarkCounters::operator[](size_t id) const {
  assert("invalid counter handle" && id < num_counters_);
  //this is slow.... CHECK_LT(id, num_counters_) << "Invalid counter handle: " << id;
  return counters_[id];
}
Counter const& BenchmarkCounters::operator[](const char* name) const {
  size_t id = Find(name);
  CHECK_EQ(Exists(name), true) << "Counter not found: " << name;
  return counters_[id];
}
Counter& BenchmarkCounters::operator[](const char* name) {
  size_t id = Find(name);
  if (id < num_counters_) {
    return counters_[id];
  } else {
    id = Add_();  // get a slot
    Counter* slot = counters_ + id;
    new (slot) Counter(name);  // construct in place using placement new
    return *slot;
  }
}

size_t BenchmarkCounters::Insert(const char* name) {
  size_t id = Find(name);
  if (id < num_counters_) {
    return id;
  } else {
    id = Add_();  // get a slot
    Counter* slot = counters_ + id;
    new (slot) Counter(name);  // construct in place using placement new
    return id;
  }
}
size_t BenchmarkCounters::Insert(const char* name, double value) {
  size_t id = Find(name);
  if (id < num_counters_) {
    return id;
  } else {
    id = Add_();  // get a slot
    Counter* slot = counters_ + id;
    new (slot) Counter(name, value);  // construct in place using placement new
    return id;
  }
}
size_t BenchmarkCounters::Insert(const char* name, double value,
                                 uint32_t flags) {
  size_t id = Find(name);
  if (id < num_counters_) {
    return id;
  } else {
    id = Add_();  // get a slot
    Counter* slot = counters_ + id;
    new (slot)
        Counter(name, value, flags);  // construct in place using placement new
    return id;
  }
}
size_t BenchmarkCounters::Insert(Counter const& c) {
  size_t id = Find(c.Name());
  if (id < num_counters_) {
    return id;
  } else {
    id = Add_();  // get a slot
    Counter* slot = counters_ + id;
    new (slot) Counter(c);  // construct in place using placement new
    return id;
  }
}

bool BenchmarkCounters::SameNames(BenchmarkCounters const& that) const {
  if (this == &that) return true;
  if (that.num_counters_ != num_counters_) {
    return false;
  }
  for (size_t i = 0; i < num_counters_; ++i) {
    Counter const& c = counters_[i];
    if (that.Find(c.Name()) == that.num_counters_) {
      return false;
    }
  }
  return true;
}

// returns the index where name is located,
// or size() if the name was not found
size_t BenchmarkCounters::Find(const char* name) const {
  size_t pos = 0;
  for (size_t i = 0; i < num_counters_; ++i) {
    Counter const& c = counters_[i];
    if (c.SameName(name)) {
      break;
    }
    ++pos;
  }
  return pos;
}

void BenchmarkCounters::Sum(BenchmarkCounters const& that) {
  // add counters present in both or just in this
  for (size_t i = 0; i < num_counters_; ++i) {
    Counter& c = counters_[i];
    size_t j = that.Find(c.Name());
    if (j < that.num_counters_) {
      c.Set(c.Value() + that.counters_[j].Value());
    }
  }
  // add counters present in that, but not in this
  for (size_t i = 0; i < that.num_counters_; ++i) {
    Counter const& tc = that.counters_[i];
    size_t j = Find(tc.Name());
    if (j >= num_counters_) {
      (*this)[tc.Name()] = tc;
    }
  }
}

void BenchmarkCounters::Finish(double cpu_time, double num_threads) {
  for (size_t i = 0; i < num_counters_; ++i) {
    Counter& c = counters_[i];
    c.Finish(cpu_time, num_threads);
  }
}

}  // end namespace benchmark
