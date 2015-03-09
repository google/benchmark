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

#include <gtest/gtest.h>
#include "../src/re.h"

TEST(Regex, RegexSimple) {
    benchmark::Regex re;
    EXPECT_TRUE(re.Init("a+", NULL));

    EXPECT_FALSE(re.Match(""));
    EXPECT_TRUE(re.Match("a"));
    EXPECT_TRUE(re.Match("aa"));
    EXPECT_TRUE(re.Match("baa"));
    EXPECT_FALSE(re.Match("b"));
}

TEST(Regex, RegexWildcard) {
    benchmark::Regex re;
    EXPECT_TRUE(re.Init("^a*$", NULL));

    EXPECT_TRUE(re.Match(""));
    EXPECT_TRUE(re.Match("a"));
    EXPECT_TRUE(re.Match("aa"));
    EXPECT_FALSE(re.Match("baa"));
    EXPECT_FALSE(re.Match("b"));
}

TEST(Regex, RegexAny) {
    benchmark::Regex re;
    EXPECT_TRUE(re.Init(".", NULL));

    EXPECT_FALSE(re.Match(""));
    EXPECT_TRUE(re.Match("a"));
    EXPECT_TRUE(re.Match("aa"));
}

TEST(Regex, RegexExact) {
    benchmark::Regex re;
    EXPECT_TRUE(re.Init("^.$", NULL));

    EXPECT_FALSE(re.Match(""));
    EXPECT_TRUE(re.Match("a"));
    EXPECT_FALSE(re.Match("aa"));
}

TEST(Regex, RegexComplicated) {
    benchmark::Regex re;
    EXPECT_TRUE(re.Init("([0-9]+ )?(mon|low)key(s)?", NULL));

    EXPECT_TRUE(re.Match("something monkey hands"));
    EXPECT_TRUE(re.Match("1 lowkey"));
    EXPECT_TRUE(re.Match("19 monkeys"));
    EXPECT_FALSE(re.Match("09 a"));
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
