#ifndef BENCHMARK_STRING_UTIL_H_
#define BENCHMARK_STRING_UTIL_H_

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "benchmark/export.h"
#include "check.h"
#include "internal_macros.h"

namespace benchmark {

void AppendHumanReadable(int n, std::string* str);

std::string HumanReadableNumber(double n, double one_k = 1024.0);

BENCHMARK_EXPORT
#if defined(__MINGW32__)
__attribute__((format(__MINGW_PRINTF_FORMAT, 1, 2)))
#elif defined(__GNUC__)
__attribute__((format(printf, 1, 2)))
#endif
std::string
StrFormat(const char* format, ...);

inline std::ostream& StrCatImp(std::ostream& out) BENCHMARK_NOEXCEPT {
  return out;
}

template <class First, class... Rest>
inline std::ostream& StrCatImp(std::ostream& out, First&& f, Rest&&... rest) {
  out << std::forward<First>(f);
  return StrCatImp(out, std::forward<Rest>(rest)...);
}

template <class... Args>
inline std::string StrCat(Args&&... args) {
  std::ostringstream ss;
  StrCatImp(ss, std::forward<Args>(args)...);
  return ss.str();
}

BENCHMARK_EXPORT
std::vector<std::string> StrSplit(const std::string& str, char delim);

// Disable lint checking for this block since it re-implements C functions.
// NOLINTBEGIN
#ifdef BENCHMARK_STL_ANDROID_GNUSTL
/*
 * GNU STL in Android NDK lacks support for some C++11 functions, including
 * stoul, stoi, stod. We reimplement them here using C functions strtoul,
 * strtol, strtod. Note that reimplemented functions are in benchmark::
 * namespace, not std:: namespace.
 */
unsigned long stoul(const std::string& str, size_t* pos = nullptr,
                    int base = 10);
int stoi(const std::string& str, size_t* pos = nullptr, int base = 10);
double stod(const std::string& str, size_t* pos = nullptr);
#else
using std::stod;   // NOLINT(misc-unused-using-decls)
using std::stoi;   // NOLINT(misc-unused-using-decls)
using std::stoul;  // NOLINT(misc-unused-using-decls)
#endif
// NOLINTEND

/**
 * Gets the human readable format for a given base10 value.
 * In other words converts 1_000 to 1k, 40_000_000 to 40m etc
 * @param arg the positive value to convert
 * @return human readable formatted string
 */
std::string Base10HumanReadableFormat(const int64_t& arg);

/**
 * Gets the human readable format for a given base2 value.
 * In other words converts 64 to 2^6, 1024 to 2^10 etc
 * @param arg the positive value to convert
 * @return human readable formatted string
 */
std::string Base2HumanReadableFormat(const int64_t& arg);

/**
 * Formats an argument into a human readable format.
 * @param arg the argument to format
 * @return the argument formatted as human readable
 */
std::string FormatHumanReadable(const int64_t& arg);

}  // end namespace benchmark

#endif  // BENCHMARK_STRING_UTIL_H_
