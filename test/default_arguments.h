#ifndef TEST_DEFAULT_ARGUMENTS_H
#define TEST_DEFAULT_ARGUMENTS_H

#include <iostream>
#include <vector>

// AddTestArguments modifies argc and argv to include default test arguments.
// This allows tests to run without requiring command-line arguments to be
// passed externally, reducing confusion and maintenance burden.
//
// Usage:
//   int main(int argc, char** argv) {
//     AddTestArguments(argc, argv, {"--benchmark_counters_tabular=true"});
//     benchmark::MaybeReenterWithoutASLR(argc, argv);
//     RunOutputTests(argc, argv);
//   }
//
// Parameters:
//   argc: Reference to argument count (will be modified)
//   argv: Reference to argument vector (will be modified)
//   args: Additional test-specific arguments to include
void AddTestArguments(int& argc, char**& argv,
                      std::initializer_list<const char*> args = {}) {
  if (argc > 1) {
    std::cerr << "Warning: Tests should not require command line arguments\n";
  }

  static std::vector<char*> new_argv;
  new_argv.clear();

  // Copy existing arguments
  for (int i = 0; i < argc; ++i) {
    new_argv.push_back(argv[i]);
  }

  // Add test-specific arguments
  for (const char* arg : args) {
    new_argv.push_back(const_cast<char*>(arg));
  }

  // Add default minimum time for faster test execution
  new_argv.push_back(const_cast<char*>("--benchmark_min_time=0.01s"));

  argv = new_argv.data();
  argc = static_cast<int>(new_argv.size());
}

#endif  // TEST_DEFAULT_ARGUMENTS_H