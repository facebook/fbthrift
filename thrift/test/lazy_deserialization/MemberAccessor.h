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

namespace apache::thrift::test {

template <template <auto> class Tag, auto MemberPtr, class U>
constexpr auto&& member_accessor(Tag<MemberPtr>, U&& u) {
  return static_cast<U&&>(u).*MemberPtr;
}

#define FBTHRIFT_DEFINE_MEMBER_ACCESSOR(FUNC, CLASS, MEMBER)                \
  constexpr auto&& FUNC(const CLASS&);                                      \
  template <auto>                                                           \
  struct FUNC##CLASS##MEMBER##TAG;                                          \
  template <>                                                               \
  struct FUNC##CLASS##MEMBER##TAG<&CLASS::MEMBER> {                         \
    friend constexpr auto&& FUNC(const CLASS& FUNC##obj) {                  \
      FUNC##CLASS##MEMBER##TAG FUNC##tag;                                   \
      return ::apache::thrift::test::member_accessor(FUNC##tag, FUNC##obj); \
    }                                                                       \
  }

} // namespace apache::thrift::test
