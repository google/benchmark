// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <regex>
#include <string>
int main() {
  const std::string str = "test0159";
  std::regex re;
  re = std::regex("^[a-z]+[0-9]+$",
       std::regex_constants::extended | std::regex_constants::nosubs);
  return std::regex_search(str, re) ? 0 : -1;
}

