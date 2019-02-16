
#undef NDEBUG

#include <chrono>
#include <thread>
#include "benchmark/benchmark.h"
#include "output_test.h"

static const std::chrono::duration<double, std::milli> time_frame(50);
static const double time_frame_in_ns(
    std::chrono::duration_cast<std::chrono::nanoseconds>(time_frame).count());

void MyBusySpinwait() {
  const auto start = std::chrono::high_resolution_clock::now();

  while (true) {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto elapsed = now - start;

    if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed) >=
        std::chrono::duration<double, std::milli>(50))
      return;
  }
}

// ========================================================================= //
// --------------------------- TEST CASES BEGIN ---------------------------- //
// ========================================================================= //

ADD_CASES(TC_ConsoleOut, {{"^[-]+$", MR_Next},
                          {"^Benchmark %s Time %s CPU %s Iterations$", MR_Next},
                          {"^[-]+$", MR_Next}});
ADD_CASES(TC_CSVOut, {{"%csv_header"}});

// ========================================================================= //
// BM_MainThread

void BM_MainThread(benchmark::State& state) {
  for (auto _ : state) MyBusySpinwait();
}

BENCHMARK(BM_MainThread)->Iterations(1);
ADD_CASES(TC_ConsoleOut, {{"^BM_MainThread/iterations:1 %console_report$"}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_MainThread/iterations:1\",$"},
           {"\"run_name\": \"BM_MainThread/iterations:1\",$", MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"iterations\": 1,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\"$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_MainThread/iterations:1\",%csv_report$"}});
void CheckTestVariantZero(Results const& e) {
  // check that the values are within 10% of the expected values
  CHECK_FLOAT_RESULT_VALUE(e, "real_time", EQ, time_frame_in_ns, 0.1);
  CHECK_FLOAT_RESULT_VALUE(e, "cpu_time", EQ, time_frame_in_ns, 0.1);
}
CHECK_BENCHMARK_RESULTS("BM_MainThread/iterations:1$", &CheckTestVariantZero);

BENCHMARK(BM_MainThread)->Iterations(1)->UseRealTime();
ADD_CASES(TC_ConsoleOut,
          {{"^BM_MainThread/iterations:1/real_time %console_report$"}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_MainThread/iterations:1/real_time\",$"},
           {"\"run_name\": \"BM_MainThread/iterations:1/real_time\",$",
            MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"iterations\": 1,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\"$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_CSVOut,
          {{"^\"BM_MainThread/iterations:1/real_time\",%csv_report$"}});
CHECK_BENCHMARK_RESULTS("BM_MainThread/iterations:1/real_time",
                        &CheckTestVariantZero);

BENCHMARK(BM_MainThread)->Iterations(1)->MeasureProcessCPUTime();
ADD_CASES(TC_ConsoleOut,
          {{"^BM_MainThread/iterations:1/process_time %console_report$"}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_MainThread/iterations:1/process_time\",$"},
           {"\"run_name\": \"BM_MainThread/iterations:1/process_time\",$",
            MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"iterations\": 1,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\"$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_CSVOut,
          {{"^\"BM_MainThread/iterations:1/process_time\",%csv_report$"}});
CHECK_BENCHMARK_RESULTS("BM_MainThread/iterations:1/process_time",
                        &CheckTestVariantZero);

BENCHMARK(BM_MainThread)->Iterations(1)->MeasureProcessCPUTime()->UseRealTime();
ADD_CASES(
    TC_ConsoleOut,
    {{"^BM_MainThread/iterations:1/process_time/real_time %console_report$"}});
ADD_CASES(
    TC_JSONOut,
    {{"\"name\": \"BM_MainThread/iterations:1/process_time/real_time\",$"},
     {"\"run_name\": \"BM_MainThread/iterations:1/process_time/real_time\",$",
      MR_Next},
     {"\"run_type\": \"iteration\",$", MR_Next},
     {"\"iterations\": 1,$", MR_Next},
     {"\"real_time\": %float,$", MR_Next},
     {"\"cpu_time\": %float,$", MR_Next},
     {"\"time_unit\": \"ns\"$", MR_Next},
     {"}", MR_Next}});
ADD_CASES(
    TC_CSVOut,
    {{"^\"BM_MainThread/iterations:1/process_time/real_time\",%csv_report$"}});
CHECK_BENCHMARK_RESULTS("BM_MainThread/iterations:1/process_time/real_time",
                        &CheckTestVariantZero);

// ========================================================================= //
// BM_WorkerThread

void BM_WorkerThread(benchmark::State& state) {
  for (auto _ : state) {
    std::thread Worker(&MyBusySpinwait);
    Worker.join();
  }
}
BENCHMARK(BM_WorkerThread)->Iterations(1);
ADD_CASES(TC_ConsoleOut, {{"^BM_WorkerThread/iterations:1 %console_report$"}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_WorkerThread/iterations:1\",$"},
           {"\"run_name\": \"BM_WorkerThread/iterations:1\",$", MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"iterations\": 1,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\"$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_WorkerThread/iterations:1\",%csv_report$"}});
void CheckTestVariantOne(Results const& e) {
  // check that the value is within 10% of the expected
  CHECK_FLOAT_RESULT_VALUE(e, "real_time", EQ, time_frame_in_ns, 0.1);
  // check that the cpu time is between 0 and (wall time / 100)
  CHECK_FLOAT_RESULT_VALUE(e, "cpu_time", EQ, (time_frame_in_ns / 100.), 2.0);
}
CHECK_BENCHMARK_RESULTS("BM_WorkerThread/iterations:1$", &CheckTestVariantOne);

BENCHMARK(BM_WorkerThread)->Iterations(1)->UseRealTime();
ADD_CASES(TC_ConsoleOut,
          {{"^BM_WorkerThread/iterations:1/real_time %console_report$"}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_WorkerThread/iterations:1/real_time\",$"},
           {"\"run_name\": \"BM_WorkerThread/iterations:1/real_time\",$",
            MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"iterations\": 1,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\"$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_CSVOut,
          {{"^\"BM_WorkerThread/iterations:1/real_time\",%csv_report$"}});
CHECK_BENCHMARK_RESULTS("BM_WorkerThread/iterations:1/real_time$",
                        &CheckTestVariantOne);

BENCHMARK(BM_WorkerThread)->Iterations(1)->MeasureProcessCPUTime();
ADD_CASES(TC_ConsoleOut,
          {{"^BM_WorkerThread/iterations:1/process_time %console_report$"}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_WorkerThread/iterations:1/process_time\",$"},
           {"\"run_name\": \"BM_WorkerThread/iterations:1/process_time\",$",
            MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"iterations\": 1,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\"$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_CSVOut,
          {{"^\"BM_WorkerThread/iterations:1/process_time\",%csv_report$"}});
void CheckTestVariantTwo(Results const& e) {
  // check that the values are within 10% of the expected values
  CHECK_FLOAT_RESULT_VALUE(e, "real_time", EQ, time_frame_in_ns, 0.1);
  CHECK_FLOAT_RESULT_VALUE(e, "cpu_time", EQ, time_frame_in_ns, 0.1);
}
CHECK_BENCHMARK_RESULTS("BM_WorkerThread/iterations:1/process_time$",
                        &CheckTestVariantTwo);

BENCHMARK(BM_WorkerThread)
    ->Iterations(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
ADD_CASES(TC_ConsoleOut, {{"^BM_WorkerThread/iterations:1/process_time/"
                           "real_time %console_report$"}});
ADD_CASES(
    TC_JSONOut,
    {{"\"name\": \"BM_WorkerThread/iterations:1/process_time/real_time\",$"},
     {"\"run_name\": \"BM_WorkerThread/iterations:1/process_time/real_time\",$",
      MR_Next},
     {"\"run_type\": \"iteration\",$", MR_Next},
     {"\"iterations\": 1,$", MR_Next},
     {"\"real_time\": %float,$", MR_Next},
     {"\"cpu_time\": %float,$", MR_Next},
     {"\"time_unit\": \"ns\"$", MR_Next},
     {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_WorkerThread/iterations:1/process_time/"
                       "real_time\",%csv_report$"}});
CHECK_BENCHMARK_RESULTS("BM_WorkerThread/iterations:1/process_time/real_time$",
                        &CheckTestVariantTwo);

// ========================================================================= //
// BM_MainThreadAndWorkerThread

void BM_MainThreadAndWorkerThread(benchmark::State& state) {
  for (auto _ : state) {
    std::thread Worker(&MyBusySpinwait);
    MyBusySpinwait();
    Worker.join();
  }
}
BENCHMARK(BM_MainThreadAndWorkerThread)->Iterations(1);
ADD_CASES(TC_ConsoleOut,
          {{"^BM_MainThreadAndWorkerThread/iterations:1 %console_report$"}});
ADD_CASES(TC_JSONOut,
          {{"\"name\": \"BM_MainThreadAndWorkerThread/iterations:1\",$"},
           {"\"run_name\": \"BM_MainThreadAndWorkerThread/iterations:1\",$",
            MR_Next},
           {"\"run_type\": \"iteration\",$", MR_Next},
           {"\"iterations\": 1,$", MR_Next},
           {"\"real_time\": %float,$", MR_Next},
           {"\"cpu_time\": %float,$", MR_Next},
           {"\"time_unit\": \"ns\"$", MR_Next},
           {"}", MR_Next}});
ADD_CASES(TC_CSVOut,
          {{"^\"BM_MainThreadAndWorkerThread/iterations:1\",%csv_report$"}});
CHECK_BENCHMARK_RESULTS("BM_MainThreadAndWorkerThread/iterations:1$",
                        &CheckTestVariantTwo);

BENCHMARK(BM_MainThreadAndWorkerThread)->Iterations(1)->UseRealTime();
ADD_CASES(TC_ConsoleOut, {{"^BM_MainThreadAndWorkerThread/iterations:1/"
                           "real_time %console_report$"}});
ADD_CASES(
    TC_JSONOut,
    {{"\"name\": \"BM_MainThreadAndWorkerThread/iterations:1/real_time\",$"},
     {"\"run_name\": \"BM_MainThreadAndWorkerThread/iterations:1/real_time\",$",
      MR_Next},
     {"\"run_type\": \"iteration\",$", MR_Next},
     {"\"iterations\": 1,$", MR_Next},
     {"\"real_time\": %float,$", MR_Next},
     {"\"cpu_time\": %float,$", MR_Next},
     {"\"time_unit\": \"ns\"$", MR_Next},
     {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_MainThreadAndWorkerThread/iterations:1/"
                       "real_time\",%csv_report$"}});
CHECK_BENCHMARK_RESULTS("BM_MainThreadAndWorkerThread/iterations:1/real_time$",
                        &CheckTestVariantTwo);

BENCHMARK(BM_MainThreadAndWorkerThread)->Iterations(1)->MeasureProcessCPUTime();
ADD_CASES(TC_ConsoleOut, {{"^BM_MainThreadAndWorkerThread/iterations:1/"
                           "process_time %console_report$"}});
ADD_CASES(
    TC_JSONOut,
    {{"\"name\": \"BM_MainThreadAndWorkerThread/iterations:1/process_time\",$"},
     {"\"run_name\": "
      "\"BM_MainThreadAndWorkerThread/iterations:1/process_time\",$",
      MR_Next},
     {"\"run_type\": \"iteration\",$", MR_Next},
     {"\"iterations\": 1,$", MR_Next},
     {"\"real_time\": %float,$", MR_Next},
     {"\"cpu_time\": %float,$", MR_Next},
     {"\"time_unit\": \"ns\"$", MR_Next},
     {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_MainThreadAndWorkerThread/iterations:1/"
                       "process_time\",%csv_report$"}});
void CheckTestVariantThree(Results const& e) {
  // check that the values are within 10% of the expected values
  CHECK_FLOAT_RESULT_VALUE(e, "real_time", EQ, time_frame_in_ns, 0.1);
  CHECK_FLOAT_RESULT_VALUE(e, "cpu_time", EQ, 2.0 * time_frame_in_ns, 0.1);
}
CHECK_BENCHMARK_RESULTS(
    "BM_MainThreadAndWorkerThread/iterations:1/process_time$",
    &CheckTestVariantThree);

BENCHMARK(BM_MainThreadAndWorkerThread)
    ->Iterations(1)
    ->MeasureProcessCPUTime()
    ->UseRealTime();
ADD_CASES(TC_ConsoleOut, {{"^BM_MainThreadAndWorkerThread/iterations:1/"
                           "process_time/real_time %console_report$"}});
ADD_CASES(
    TC_JSONOut,
    {{"\"name\": "
      "\"BM_MainThreadAndWorkerThread/iterations:1/process_time/real_time\",$"},
     {"\"run_name\": "
      "\"BM_MainThreadAndWorkerThread/iterations:1/process_time/real_time\",$",
      MR_Next},
     {"\"run_type\": \"iteration\",$", MR_Next},
     {"\"iterations\": 1,$", MR_Next},
     {"\"real_time\": %float,$", MR_Next},
     {"\"cpu_time\": %float,$", MR_Next},
     {"\"time_unit\": \"ns\"$", MR_Next},
     {"}", MR_Next}});
ADD_CASES(TC_CSVOut, {{"^\"BM_MainThreadAndWorkerThread/iterations:1/"
                       "process_time/real_time\",%csv_report$"}});
CHECK_BENCHMARK_RESULTS(
    "BM_MainThreadAndWorkerThread/iterations:1/process_time/real_time$",
    &CheckTestVariantThree);

// ========================================================================= //
// ---------------------------- TEST CASES END ----------------------------- //
// ========================================================================= //

int main(int argc, char* argv[]) { RunOutputTests(argc, argv); }
