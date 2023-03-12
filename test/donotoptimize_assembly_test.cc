#include <benchmark/benchmark.h>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wreturn-type"
#endif

extern "C" {

extern int ExternInt;
extern int ExternInt2;
extern int ExternInt3;
extern int BigArray[2049];

const int ConstBigArray[2049]{};

inline int Add42(int x) { return x + 42; }

struct NotTriviallyCopyable {
  NotTriviallyCopyable();
  explicit NotTriviallyCopyable(int x) : value(x) {}
  NotTriviallyCopyable(NotTriviallyCopyable const &);
  int value;
};

struct Large {
  int value;
  int data[2];
};

struct ExtraLarge {
  int arr[2049];
};
}

extern ExtraLarge ExtraLargeObj;
const ExtraLarge ConstExtraLargeObj{};

// CHECK-LABEL: test_with_rvalue:
extern "C" void test_with_rvalue() {
  benchmark::DoNotOptimize(Add42(0));
  // CHECK: mov 2a,REG
  // CHECK: ret
}

// CHECK-LABEL: test_with_large_rvalue:
extern "C" void test_with_large_rvalue() {
  benchmark::DoNotOptimize(Large{ExternInt, {ExternInt, ExternInt}});
  // CHECK: mov OFFSET(RIP),REG
  // CHECK-NVHPC: mov (REG),REG
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: ret
}

// CHECK-LABEL: test_with_non_trivial_rvalue:
extern "C" void test_with_non_trivial_rvalue() {
  benchmark::DoNotOptimize(NotTriviallyCopyable(ExternInt));
  // CHECK: mov OFFSET(RIP),REG
  // CHECK-NVHPC: mov (REG),REG
  // CHECK-NVHPC: mov REG,OFFSET(REG)
  // CHECK-GNU:   mov REG,OFFSET(REG)
  // CHECK: ret
}

// CHECK-LABEL: test_with_lvalue:
extern "C" void test_with_lvalue() {
  int x = 101;
  benchmark::DoNotOptimize(x);
  // CHECK: mov 65,{{REG|OFFSET\(REG\)}}
  // CHECK: ret
}

// CHECK-LABEL: test_with_large_lvalue:
extern "C" void test_with_large_lvalue() {
  Large L{ExternInt, {ExternInt, ExternInt}};
  benchmark::DoNotOptimize(L);
  // CHECK: mov OFFSET(RIP),REG
  // CHECK-NVHPC: mov (REG),REG
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: ret
}

// CHECK-LABEL: test_with_extra_large_lvalue_with_op:
extern "C" void test_with_extra_large_lvalue_with_op() {
  ExtraLargeObj.arr[16] = 42;
  benchmark::DoNotOptimize(ExtraLargeObj);
  // CHECK-NVHPC: mov OFFSET(RIP),REG
  // CHECK-NVHPC: mov 2a,OFFSET(REG)
  // CHECK-CLANG: mov 2a,OFFSET(REG)
  // CHECK-GNU: mov 2a,OFFSET(RIP)
  // CHECK: ret
}

// CHECK-LABEL: test_with_big_array_with_op
extern "C" void test_with_big_array_with_op() {
  BigArray[16] = 42;
  benchmark::DoNotOptimize(BigArray);
  // CHECK-NVHPC: mov OFFSET(RIP),REG
  // CHECK-NVHPC: mov 2a,OFFSET(REG)
  // CHECK-CLANG: mov 2a,OFFSET(REG)
  // CHECK-GNU:   mov 2a,OFFSET(RIP)
  // CHECK: ret
}

// CHECK-LABEL: test_with_non_trivial_lvalue:
extern "C" void test_with_non_trivial_lvalue() {
  NotTriviallyCopyable NTC(ExternInt);
  benchmark::DoNotOptimize(NTC);
  // CHECK: mov OFFSET(RIP),REG
  // CHECK-NVHPC: mov (REG),REG
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: ret
}

// CHECK-LABEL: test_with_const_lvalue:
extern "C" void test_with_const_lvalue() {
  const int x = 123;
  benchmark::DoNotOptimize(x);
  // CHECK: mov 7b,REG
  // CHECK: ret
}

// CHECK-LABEL: test_with_large_const_lvalue:
extern "C" void test_with_large_const_lvalue() {
  const Large L{ExternInt, {ExternInt, ExternInt}};
  benchmark::DoNotOptimize(L);
  // CHECK: mov OFFSET(RIP),REG
  // CHECK-NVHPC: mov (REG),REG
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: mov REG,OFFSET(REG)
  // CHECK: ret
}

// CHECK-LABEL: test_with_const_extra_large_obj:
extern "C" void test_with_const_extra_large_obj() {
  benchmark::DoNotOptimize(ConstExtraLargeObj);
  // CHECK: ret
}

// CHECK-LABEL: test_with_const_big_array
extern "C" void test_with_const_big_array() {
  benchmark::DoNotOptimize(ConstBigArray);
  // CHECK-CLANG: lea OFFSET(RIP),REG
  // CHECK-CLANG: mov REG,OFFSET(REG)
  // CHECK: ret
}

// CHECK-LABEL: test_with_non_trivial_const_lvalue:
extern "C" void test_with_non_trivial_const_lvalue() {
  const NotTriviallyCopyable Obj(ExternInt);
  benchmark::DoNotOptimize(Obj);
  // CHECK: mov OFFSET(RIP),REG
  // CHECK-NVHPC: mov (REG),REG
  // CHECK-NVHPC: mov REG,OFFSET(REG)
  // CHECK: ret
}

// CHECK-LABEL: test_div_by_two:
extern "C" int test_div_by_two(int input) {
  int divisor = 2;
  benchmark::DoNotOptimize(divisor);
  return input / divisor;
  // CHECK: mov REG,REG
  // CHECK: mov 2,{{REG|OFFSET\(REG\)}}
  // CHECK: cltd
  // CHECK: idiv {{REG|OFFSET\(REG\)}}
  // CHECK: ret
}

// CHECK-LABEL: test_inc_integer:
extern "C" int test_inc_integer() {
  int x = 0;
  for (int i = 0; i < 5; ++i) benchmark::DoNotOptimize(++x);
  // CHECK:   mov 1,{{REG|OFFSET\(REG\)}}
  // CHECK:   {{inc |add 1,}}{{REG|OFFSET\(REG\)}}
  // CHECK:   {{inc |add 1,}}{{REG|OFFSET\(REG\)}}
  // CHECK:   {{inc |add 1,}}{{REG|OFFSET\(REG\)}}
  // CHECK:   {{inc |add 1,}}{{REG|OFFSET\(REG\)}}
  // CHECK: ret
  return x;
}

// CHECK-LABEL: test_pointer_rvalue
extern "C" void test_pointer_rvalue() {
  // CHECK-DAG: lea OFFSET(REG),REG
  // CHECK-DAG: mov 2a,OFFSET(REG)
  // CHECK-NVHPC: mov REG,OFFSET(REG)
  // CHECK-CLANG: mov REG,OFFSET(REG)
  // CHECK: ret
  int x = 42;
  benchmark::DoNotOptimize(&x);
}

// CHECK-LABEL: test_pointer_const_lvalue:
extern "C" void test_pointer_const_lvalue() {
  // CHECK-DAG: mov 2a,OFFSET(REG)
  // CHECK-DAG: lea OFFSET(REG),REG
  // CHECK-NVHPC: mov REG,OFFSET(REG)
  // CHECK-CLANG: mov REG,OFFSET(REG)
  // CHECK: ret
  int x = 42;
  int *const xp = &x;
  benchmark::DoNotOptimize(xp);
}

// CHECK-LABEL: test_pointer_lvalue:
extern "C" void test_pointer_lvalue() {
  // CHECK-DAG: mov 2a,OFFSET(REG)
  // CHECK-DAG: lea OFFSET(REG),REG
  // CHECK-NVHPC: mov REG,OFFSET(REG)
  // CHECK-CLANG: mov REG,OFFSET(REG)
  // CHECK: ret
  int x = 42;
  int *xp = &x;
  benchmark::DoNotOptimize(xp);
}
