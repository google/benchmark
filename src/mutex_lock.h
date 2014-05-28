#ifndef BENCHMARK_MUTEX_LOCK_H_
#define BENCHMARK_MUTEX_LOCK_H_

#include "port.h"

#if defined HAVE_PTHREAD_H
#include <pthread.h>
#endif

#if defined OS_WINDOWS
#include "windows/pthread.h"
#endif

namespace benchmark {
class mutex_lock {
 public:
  explicit mutex_lock(pthread_mutex_t* mu) : mu_(mu) {
    pthread_mutex_lock(mu_);
  }

  ~mutex_lock() { pthread_mutex_unlock(mu_); }

 private:
  pthread_mutex_t* mu_;
};
}  // end namespace benchmark

#endif  // BENCHMARK_MUTEX_LOCK_H_
