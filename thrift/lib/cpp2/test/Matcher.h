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

#pragma once

#include <folly/portability/GMock.h>
#include <thrift/lib/cpp2/FieldRef.h>

namespace apache::thrift::test {

// A Thrift field matcher.
//
// Given a Thrift struct
//
//   struct Person {
//     1: string name;
//   }
//
// the matcher can be used as follows:
//
//   auto p = Person();
//   p.name_ref() = "Zaphod";
//   EXPECT_THAT(p, ThriftField(&Person::name_ref<>, Eq("Zaphod")));
template <typename T, typename Struct, typename Matcher>
auto ThriftField(
    apache::thrift::field_ref<const T&> (Struct::*ref)() const&,
    const Matcher& matcher) {
  return testing::ResultOf(
      [=](const Struct& s) { return (s.*ref)(); }, matcher);
}
template <typename T, typename Struct, typename Matcher>
auto ThriftField(
    apache::thrift::optional_field_ref<const T&> (Struct::*ref)() const&,
    const Matcher& matcher) {
  return testing::ResultOf(
      [=](const Struct& s) { return (s.*ref)(); }, matcher);
}
} // namespace apache::thrift::test
