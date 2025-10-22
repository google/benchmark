#ifndef BENCHMARK_COLORPRINT_H_
#define BENCHMARK_COLORPRINT_H_

#include <cstdarg>
#include <iostream>
#include <string>

namespace benchmark {
enum LogColor {
  COLOR_DEFAULT,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW,
  COLOR_BLUE,
  COLOR_MAGENTA,
  COLOR_CYAN,
  COLOR_WHITE
};

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_FORMAT_STRING_FUNC(format_arg, first_idx) \
  __attribute__((format(printf, format_arg, first_idx)))
#elif defined(__MINGW32__)
#define PRINTF_FORMAT_STRING_FUNC(format_arg, first_idx) \
  __attribute__((format(__MINGW_PRINTF_FORMAT, format_arg, first_idx)))
#else
#define PRINTF_FORMAT_STRING_FUNC(format_arg, first_idx)
#endif

PRINTF_FORMAT_STRING_FUNC(1, 0)
std::string FormatString(const char* msg, va_list args);
PRINTF_FORMAT_STRING_FUNC(1, 2) std::string FormatString(const char* msg, ...);

PRINTF_FORMAT_STRING_FUNC(3, 0)
void ColorPrintf(std::ostream& out, LogColor color, const char* fmt,
                 va_list args);
PRINTF_FORMAT_STRING_FUNC(3, 4)
void ColorPrintf(std::ostream& out, LogColor color, const char* fmt, ...);

// Returns true if stdout appears to be a terminal that supports colored
// output, false otherwise.
bool IsColorTerminal();

}  // end namespace benchmark

#endif  // BENCHMARK_COLORPRINT_H_
