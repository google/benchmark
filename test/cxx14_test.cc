#include "benchmark/benchmark.h"

#if defined(_MSC_VER)
#define CXX_STD_VERSION _MSVC_LANG
#else  // non-MSVC
#define CXX_STD_VERSION __cplusplus
#endif

#if CXX_STD_VERSION != 201402L
#error "Trying to compile C++14 test with wrong C++ standard"
#endif
