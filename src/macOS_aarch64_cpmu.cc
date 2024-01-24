// WARNING: must be run as root on an M1 device
// WARNING: fragile, uses private apple APIs
// currently no command line interface, see variables at top of main

/*
  Based on https://github.com/travisdowns/robsize
  Henry Wong <henry@stuffedcow.net>
  http://blog.stuffedcow.net/2013/05/measuring-rob-capacity/
  2014-10-14
*/

#include "macOS_aarch64_cpmu.h"

#ifdef BENCHMARK_MACOS_AARCH64

#include <dlfcn.h>
#include <pthread.h>

#define KPERF_LIST                             \
  /*  ret, name, params */                     \
  F(int, kpc_get_counting, void)               \
  F(int, kpc_force_all_ctrs_set, int)          \
  F(int, kpc_set_counting, uint32_t)           \
  F(int, kpc_set_thread_counting, uint32_t)    \
  F(int, kpc_set_config, uint32_t, void *)     \
  F(int, kpc_get_config, uint32_t, void *)     \
  F(int, kpc_set_period, uint32_t, void *)     \
  F(int, kpc_get_period, uint32_t, void *)     \
  F(uint32_t, kpc_get_counter_count, uint32_t) \
  F(uint32_t, kpc_get_config_count, uint32_t)  \
  F(int, kperf_sample_get, int *)              \
  F(int, kpc_get_thread_counters, int, unsigned int, void *)

#define F(ret, name, ...)              \
  typedef ret name##proc(__VA_ARGS__); \
  static name##proc *name;
KPERF_LIST
#undef F

#define CFGWORD_EL0A32EN_MASK (0x10000)
#define CFGWORD_EL0A64EN_MASK (0x20000)
#define CFGWORD_EL1EN_MASK (0x40000)
#define CFGWORD_EL3EN_MASK (0x80000)
#define CFGWORD_ALLMODES_MASK (0xf0000)

#define CPMU_NONE 0
#define CPMU_CORE_CYCLE 0x02
#define CPMU_INST_A64 0x8c
#define CPMU_INST_BRANCH 0x8d
#define CPMU_SYNC_DC_LOAD_MISS 0xbf
#define CPMU_SYNC_DC_STORE_MISS 0xc0
#define CPMU_SYNC_DTLB_MISS 0xc1
#define CPMU_SYNC_ST_HIT_YNGR_LD 0xc4
#define CPMU_SYNC_BR_ANY_MISP 0xcb
#define CPMU_FED_IC_MISS_DEM 0xd3
#define CPMU_FED_ITLB_MISS 0xd4

#define KPC_CLASS_FIXED (0)
#define KPC_CLASS_CONFIGURABLE (1)
#define KPC_CLASS_POWER (2)
#define KPC_CLASS_RAWPMU (3)
#define KPC_CLASS_FIXED_MASK (1u << KPC_CLASS_FIXED)
#define KPC_CLASS_CONFIGURABLE_MASK (1u << KPC_CLASS_CONFIGURABLE)
#define KPC_CLASS_POWER_MASK (1u << KPC_CLASS_POWER)
#define KPC_CLASS_RAWPMU_MASK (1u << KPC_CLASS_RAWPMU)

#define COUNTERS_COUNT 10
#define CONFIG_COUNT 8
#define KPC_MASK (KPC_CLASS_CONFIGURABLE_MASK | KPC_CLASS_FIXED_MASK)
static uint64_t g_counters[COUNTERS_COUNT];
static uint64_t g_config[COUNTERS_COUNT];

bool configure_macOS_rdtsc() {
  if (kpc_set_config(KPC_MASK, g_config)) {
    return false;
  }

  if (kpc_force_all_ctrs_set(1)) {
    return false;
  }

  if (kpc_set_counting(KPC_MASK)) {
    return false;
  }

  if (kpc_set_thread_counting(KPC_MASK)) {
    return false;
  }

  return true;
}

bool init_macOS_rdtsc() {
  void *kperf = dlopen(
      "/System/Library/PrivateFrameworks/kperf.framework/Versions/A/kperf",
      RTLD_LAZY);
  if (!kperf) {
    return false;
  }
#define F(ret, name, ...)                                     \
  name = reinterpret_cast<name##proc *>(dlsym(kperf, #name)); \
  if (!name) {                                                \
    return false;                                             \
  }
  KPERF_LIST
#undef F

  if (kpc_get_counter_count(KPC_MASK) != COUNTERS_COUNT) {
    return false;
  }

  if (kpc_get_config_count(KPC_MASK) != CONFIG_COUNT) {
    return false;
  }

  // Not all counters can count all things:

  // CPMU_CORE_CYCLE           {0-7}
  // CPMU_FED_IC_MISS_DEM      {0-7}
  // CPMU_FED_ITLB_MISS        {0-7}

  // CPMU_INST_BRANCH          {3, 4, 5}
  // CPMU_SYNC_DC_LOAD_MISS    {3, 4, 5}
  // CPMU_SYNC_DC_STORE_MISS   {3, 4, 5}
  // CPMU_SYNC_DTLB_MISS       {3, 4, 5}
  // CPMU_SYNC_BR_ANY_MISP     {3, 4, 5}
  // CPMU_SYNC_ST_HIT_YNGR_LD  {3, 4, 5}
  // CPMU_INST_A64             {5}

  // using "CFGWORD_ALLMODES_MASK" is much noisier
  g_config[0] = CPMU_CORE_CYCLE | CFGWORD_EL0A64EN_MASK;
  //  g_config[3] = CPMU_INST_A64 | CFGWORD_EL0A64EN_MASK;
  //  g_config[4] = CPMU_INST_BRANCH | CFGWORD_EL0A64EN_MASK;
  //  g_config[5] = CPMU_SYNC_BR_ANY_MISP | CFGWORD_EL0A64EN_MASK;

  return configure_macOS_rdtsc();
}

unsigned long long int macOS_rdtsc() {
  if (kpc_get_thread_counters(0, COUNTERS_COUNT, g_counters)) {
    return 0;
  }
  return g_counters[0 + 2];
}

#endif  // BENCHMARK_MACOS_AARCH64
