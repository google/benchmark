#ifndef CHECK_H_
#define CHECK_H_

#include <cassert>

#define CHECK(b)             \
  do {                       \
    if (!(b)) assert(false); \
  } while (0)
#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_LT(a, b) CHECK((a) < (b))

#endif  // CHECK_H_
