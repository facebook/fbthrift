/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "thrift/lib/cpp2/test/Matcher.h"
#include "thrift/lib/cpp2/test/gen-cpp2/Matcher_types.h"

#include <folly/portability/GTest.h>

using apache::thrift::test::Person;
using apache::thrift::test::ThriftField;
using testing::Eq;

TEST(MatcherTest, ThriftField) {
  auto p = Person();
  p.name_ref() = "Zaphod";
  EXPECT_THAT(p, ThriftField(&Person::name_ref<>, Eq("Zaphod")));
}
