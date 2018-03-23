#include <benchmark/benchmark.h>

// CHECK-LABEL: test_div_by_two_lvalue:
extern "C" int test_div_by_two_lvalue(int input) {
  int divisor = 2;
  benchmark::MakeUnpredictable(divisor);
  return input / divisor;
  // CHECK: movl $2, [[DEST:.*]]
  // CHECK: idivl [[DEST]]
  // CHECK: ret
}

// CHECK-LABEL: test_div_by_two_rvalue:
extern "C" int test_div_by_two_rvalue(int input) {
  int divisor = benchmark::MakeUnpredictable(2);
  return input / divisor;
  // CHECK: movl $2, [[DEST:.*]]
  // CHECK: idivl [[DEST]]
  // CHECK: ret
}

// CHECK-LABEL: test_div_by_two_rvalue_2:
extern "C" int test_div_by_two_rvalue_2(int input) {
  return input / benchmark::MakeUnpredictable(2);
  // CHECK: movl $2, [[DEST:.*]]
  // CHECK: idivl [[DEST]]
  // CHECK: ret
}

