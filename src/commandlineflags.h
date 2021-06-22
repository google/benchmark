#ifndef BENCHMARK_COMMANDLINEFLAGS_H_
#define BENCHMARK_COMMANDLINEFLAGS_H_

#include <cstdint>
#include <map>
#include <string>

namespace benchmark {

// Parses a bool from the environment variable corresponding to the given flag.
//
// If the variable exists, returns IsTruthyFlagValue() value;  if not,
// returns the given default value.
bool BoolFromEnv(const char* flag, bool default_val);

// Parses an Int32 from the environment variable corresponding to the given
// flag.
//
// If the variable exists, returns ParseInt32() value;  if not, returns
// the given default value.
int32_t Int32FromEnv(const char* flag, int32_t default_val);

// Parses an Double from the environment variable corresponding to the given
// flag.
//
// If the variable exists, returns ParseDouble();  if not, returns
// the given default value.
double DoubleFromEnv(const char* flag, double default_val);

// Parses a string from the environment variable corresponding to the given
// flag.
//
// If variable exists, returns its value;  if not, returns
// the given default value.
const char* StringFromEnv(const char* flag, const char* default_val);

// Parses a set of kvpairs from the environment variable corresponding to the
// given flag.
//
// If variable exists, returns its value;  if not, returns
// the given default value.
std::map<std::string, std::string> KvPairsFromEnv(
    const char* flag, std::map<std::string, std::string> default_val);

// Parses a string for a bool flag, in the form of either
// "--flag=value" or "--flag".
//
// In the former case, the value is taken as true if it passes IsTruthyValue().
//
// In the latter case, the value is taken as true.
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseBoolFlag(const char* str, const char* flag, bool* value);

// Parses a string for an Int32 flag, in the form of "--flag=value".
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseInt32Flag(const char* str, const char* flag, int32_t* value);

// Parses a string for a Double flag, in the form of "--flag=value".
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseDoubleFlag(const char* str, const char* flag, double* value);

// Parses a string for a string flag, in the form of "--flag=value".
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseStringFlag(const char* str, const char* flag, std::string* value);

// Parses a string for a kvpairs flag in the form "--flag=key=value,key=value"
//
// On success, stores the value of the flag in *value and returns true. On
// failure returns false, though *value may have been mutated.
bool ParseKeyValueFlag(const char* str, const char* flag,
                       std::map<std::string, std::string>* value);

// Returns true if the string matches the flag.
bool IsFlag(const char* str, const char* flag);

// Returns true unless value starts with one of: '0', 'f', 'F', 'n' or 'N', or
// some non-alphanumeric character. Also returns false if the value matches
// one of 'no', 'false', 'off' (case-insensitive). As a special case, also
// returns true if value is the empty string.
bool IsTruthyFlagValue(const std::string& value);

}  // end namespace benchmark

#endif  // BENCHMARK_COMMANDLINEFLAGS_H_
