#include "performance_events.h"
#ifdef BENCHMARK_HAS_PAPI
extern "C"
{
#include <papi.h>
}

namespace benchmark {
namespace internal {

namespace {

  bool Init()
  {
    auto inited = PAPI_is_initialized();
    switch (inited)
    {
    case PAPI_NOT_INITED:
      if (PAPI_VER_CURRENT != PAPI_library_init(PAPI_VER_CURRENT))
        return false;
      // fallthrough
    case PAPI_LOW_LEVEL_INITED:
    case PAPI_HIGH_LEVEL_INITED:
      if (PAPI_OK != PAPI_thread_init(pthread_self))
        return false;
      break;
    case PAPI_THREAD_LEVEL_INITED:
      break;
    }
    return true;
  }

}

PerformanceCounter::PerformanceCounter(const PerformanceEvents& events)
    : event_set_(PAPI_NULL)
    , counters_(events.size())
{
  if (!Init())
    return;
  if (PAPI_OK != PAPI_create_eventset(&event_set_))
    return;
  for (const auto event: events)
  {
    char name[PAPI_MAX_STR_LEN];
    if (PAPI_OK != PAPI_event_code_to_name(event, name))
      continue;
    event_names_.push_back(std::string(name).substr(5));
    if (PAPI_OK != PAPI_add_event(event_set_, event))
      continue;
  }
}

PerformanceCounter::~PerformanceCounter()
{
  PAPI_cleanup_eventset(event_set_);
  PAPI_destroy_eventset(&event_set_);
}

bool PerformanceCounter::Start()
{
  return PAPI_OK == PAPI_start(event_set_);
}

bool PerformanceCounter::Stop()
{
  if (PAPI_OK != PAPI_accum(event_set_, counters_.data()))
    return false;
  std::vector<long long> dummies(counters_.size());
  return PAPI_OK == PAPI_stop(event_set_, dummies.data());
}

void PerformanceCounter::IncrementCounters(UserCounters& counters) const
{
  for (std::size_t i = 0; i < counters_.size(); ++i)
  {
    auto it = counters.find(event_names_[i]);
    if (it == counters.end())
      counters.emplace(event_names_[i], Counter(counters_[i], Counter::kAvgIterations));
    else
      it->second.value += counters_[i];
  }
}

PerformanceEvents PerformanceCounter::ReadEvents(const std::string& input, std::ostream& err_stream)
{
  if (input.empty())
    return {};
  if (!Init())
  {
    err_stream << "***WARNING*** Could not init papi library\n";
    return {};
  }
  PerformanceEvents events;
  std::string::size_type start = 0;
  for (;;)
  {
    auto next = input.find(',', start);
    const auto name = "PAPI_" + input.substr(start, next-start);
    int code = 0;
    if (PAPI_OK != PAPI_event_name_to_code(name.data(), &code))
      err_stream << "***WARNING*** Skipping unknown PAPI event: '" << name << "', check output of papi_avail\n";
    else
      events.push_back(code);
    if (next >= input.size())
      break;
    start = next + 1;
  }
  return events;
}

}  // namespace internal
}  // namespace benchmark

#else

namespace benchmark {
namespace internal {

PerformanceCounter::PerformanceCounter(const PerformanceEvents&)
{
}

PerformanceCounter::~PerformanceCounter()
{
}

bool PerformanceCounter::Start()
{
  return true;
}

bool PerformanceCounter::Stop()
{
  return true;
}

void PerformanceCounter::IncrementCounters(UserCounters&) const
{
}

PerformanceEvents PerformanceCounter::ReadEvents(const std::string& events, std::ostream& err_stream)
{
  if (!events.empty())
    err_stream << "***WARNING*** PerformanceCounters not supported\n";
  return PerformanceEvents();
}

}  // namespace internal
}  // namespace benchmark

#endif
