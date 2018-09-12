
#undef NDEBUG
#include <cstdio>
#include <fstream>
#include <streambuf>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"
#include "output_test.h"

class TestBenchmarkFileReporter {
  std::string output;

 public:
  TestBenchmarkFileReporter(int argc, char* argv[]) {
    std::vector<char*> new_argv(argv, argv + argc);
    assert(static_cast<decltype(new_argv)::size_type>(argc) == new_argv.size());

    std::string tmp_file_name = std::tmpnam(nullptr);
    std::cout << "Will be using this as the tmp file: " << tmp_file_name
              << '\n';

    std::string tmp = "--benchmark_out=";
    tmp += tmp_file_name;
    new_argv.emplace_back(const_cast<char*>(tmp.c_str()));

    argc = int(new_argv.size());

    benchmark::Initialize(&argc, new_argv.data());
    benchmark::RunSpecifiedBenchmarks();

    // Read the output back from the file, and delete the file.
    std::ifstream tmp_stream(tmp_file_name);
    output = std::string((std::istreambuf_iterator<char>(tmp_stream)),
                         std::istreambuf_iterator<char>());
    std::remove(tmp_file_name.c_str());
  }

  const std::string& getOutput() const { return output; }
};
