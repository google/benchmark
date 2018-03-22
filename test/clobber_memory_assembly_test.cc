#include <benchmark/benchmark.h>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wreturn-type"
#endif

extern "C" {

extern int ExternInt;
extern int ExternInt2;
extern int ExternInt3;

}

// CHECK-LABEL: test_basic:
extern "C" void test_basic() {
  int x;
  benchmark::DoNotOptimize(&x);
  x = 101;
  benchmark::ClobberMemory();
  // CHECK: leaq [[DEST:[^,]+]], %rax
  // CHECK: movl $101, [[DEST]]
  // CHECK: ret
}

// CHECK-LABEL: test_redundant_store:
extern "C" void test_redundant_store() {
  ExternInt = 3;
  benchmark::ClobberMemory();
  ExternInt = 51;
  // CHECK: movl	$3, ExternInt(%rip)
  // CHECK: movl	$51, ExternInt(%rip)
}

// CHECK-LABEL: test_redundant_read:
extern "C" void test_redundant_read() {
  int x;
  benchmark::DoNotOptimize(&x);
  x = ExternInt;
  benchmark::ClobberMemory();
  x = ExternInt2;
  // CHECK: leaq [[DEST:[^,]+]], %rax
  // CHECK: movl ExternInt(%rip), %eax
  // CHECK: movl %eax, [[DEST]]
  // CHECK-NOT: movl ExternInt2
  // CHECK: ret
}

// CHECK-LABEL: test_redundant_read2:
extern "C" void test_redundant_read2() {
  int x;
  benchmark::DoNotOptimize(&x);
  x = ExternInt;
  benchmark::ClobberMemory();
  x = ExternInt2;
  benchmark::ClobberMemory();
  // CHECK: leaq [[DEST:[^,]+]], %rax
  // CHECK: movl ExternInt(%rip), %eax
  // CHECK: movl %eax, [[DEST]]
  // CHECK: movl ExternInt2(%rip), %eax
  // CHECK: movl %eax, [[DEST]]
  // CHECK: ret
}
