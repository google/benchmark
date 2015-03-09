#ifndef BENCHMARK_STRING_UTIL_H_
#define BENCHMARK_STRING_UTIL_H_

#include <string>
#include <sstream>
#include <utility>

namespace benchmark {

void AppendHumanReadable(int n, std::string* str);

std::string HumanReadableNumber(double n);

std::string StringPrintF(const char* format, ...);

inline std::ostream&
StringCatImp(std::ostream& out) noexcept
{
  return out;
}

template <class First, class ...Rest>
inline std::ostream&
StringCatImp(std::ostream& out, First&& f, Rest&&... rest)
{
  out << std::forward<First>(f);
  return StringCatImp(out, std::forward<Rest>(rest)...);
}

template<class ...Args>
inline std::string StrCat(Args&&... args)
{
  std::ostringstream ss;
  StringCatImp(ss, std::forward<Args>(args)...);
  return ss.str();
}

} // end namespace benchmark

#endif // BENCHMARK_STRING_UTIL_H_
