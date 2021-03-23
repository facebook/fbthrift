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
#include <thrift/lib/cpp2/IndirectionWrapper.h>
namespace apache {
namespace thrift {
namespace test {
class Foo_data;
class Foo : public IndirectionWrapper<Foo, Foo_data> {
 public:
  using IndirectionWrapper<Foo, Foo_data>::IndirectionWrapper;

  int sum();
};

class Bar_data;
class Bar : public IndirectionWrapper<Bar, Bar_data> {
 public:
  using IndirectionWrapper<Bar, Bar_data>::IndirectionWrapper;

  int sum();
};

class Baz : public IndirectionWrapper<Baz, Bar_data> {
 public:
  using IndirectionWrapper<Baz, Bar_data>::IndirectionWrapper;
};

class Seconds {
 public:
  FBTHRIFT_CPP_DEFINE_MEMBER_INDIRECTION_FN(value());

  Seconds() = default;
  explicit Seconds(int64_t seconds) noexcept : seconds_(seconds) {}

  int64_t value() const { return seconds_; }
  int64_t& value() { return seconds_; }

  bool operator==(Seconds that) const { return seconds_ == that.seconds_; }
  bool operator<(Seconds that) const { return seconds_ < that.seconds_; }

 private:
  int64_t seconds_{0};
};

} // namespace test
} // namespace thrift
} // namespace apache
