#include "benchmark/benchmark.h"

#include <cstdint>
#include <type_traits>

namespace {
#if defined(__GNUC__)
std::uint64_t double_up(const std::uint64_t x) __attribute__((const));
#endif
std::uint64_t double_up(const std::uint64_t x) { return x * 2; }
}

// Using MakeUnpredictable on types like BitRef seem to cause a lot of problems
// with the inline assembly on both GCC and Clang.
struct BitRef {
  int index;
  unsigned char &byte;

public:
  static BitRef Make() {
    static unsigned char arr[2] = {};
    BitRef b(1, arr[0]);
    return b;
  }
private:
  BitRef(int i, unsigned char& b) : index(i), byte(b) {}
};

struct MoveOnly {
  explicit MoveOnly(int xvalue) : value(xvalue) {}
  MoveOnly(MoveOnly&& other) : value(other.value) {
    other.value = -1;
  }
  int value;
};

using benchmark::MakeUnpredictable;

#define UNUSED (void)

void verify_compile() {
  // this test verifies compilation of MakeUnpredictable() for some types

  char buffer8[8];
  MakeUnpredictable(buffer8);

  char buffer20[20];
  MakeUnpredictable(buffer20);

  char buffer1024[1024];
  MakeUnpredictable(buffer1024);
  UNUSED MakeUnpredictable(&buffer1024[0]);

  int x = 123;
  MakeUnpredictable(x);
  UNUSED MakeUnpredictable(&x);
  UNUSED MakeUnpredictable(x += 42);

  UNUSED MakeUnpredictable(double_up(x));

  // These tests are to e
  UNUSED MakeUnpredictable(BitRef::Make());
  BitRef lval = BitRef::Make();
  MakeUnpredictable(lval);
}
#undef UNUSED

#define ASSERT_TYPE(Expr, Expect) \
  static_assert(std::is_same<decltype(MakeUnpredictable(Expr)), Expect>::value, "")

void verify_return_type() {
  {
    int lvalue = 42;
    ASSERT_TYPE(lvalue, int&);
    int &result = MakeUnpredictable(lvalue);
    assert(&result == &lvalue);
    assert(lvalue == 42);
  }
  {
    const int clvalue = 42;
    ASSERT_TYPE(clvalue, int);
    assert(MakeUnpredictable(clvalue) == 42);
  }
  {
    ASSERT_TYPE(42, int);
    assert(MakeUnpredictable(42) == 42);
  }
  {
    int rvalue = -1;
    ASSERT_TYPE(std::move(rvalue), int);
    int result = MakeUnpredictable(std::move(rvalue));
    assert(rvalue == -1);
    assert(result == -1);
  }
  {
    const int const_rvalue = 42;
    ASSERT_TYPE(std::move(const_rvalue), int);
    int result = MakeUnpredictable(std::move(const_rvalue));
    assert(const_rvalue == 42);
    assert(result == 42);
  }
  {
    MoveOnly mv(42);
    ASSERT_TYPE(std::move(mv), MoveOnly);
    MoveOnly result = MakeUnpredictable(std::move(mv));
    assert(result.value == 42);
    assert(mv.value == -1); // We moved from it during MakeUnpredictable
  }
}

int main() {
  verify_compile();
  verify_return_type();
}
