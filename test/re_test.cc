// Copyright 2014 Google Inc. All rights reserved.
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

#include <gtest/gtest.h>
#include "re.h"

TEST(Regex, RegexSimple) {
    benchmark::Regex re;
    EXPECT_TRUE(re.Init("a+", NULL));

    EXPECT_FALSE(re.Match(""));
    EXPECT_TRUE(re.Match("a"));
    EXPECT_TRUE(re.Match("aa"));
    EXPECT_FALSE(re.Match("b"));
}

TEST(Regex, InvalidNoErrorMessage) {
    benchmark::Regex re;
    EXPECT_FALSE(re.Init("[", NULL));
}

TEST(Regex, Invalid) {
    std::string error;
    benchmark::Regex re;
    EXPECT_FALSE(re.Init("[", &error));

    EXPECT_NE("", error);
}
