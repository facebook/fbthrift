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

#include <random>
#include <folly/portability/GTest.h>

#include <folly/container/Array.h>
#include <thrift/lib/cpp2/BadFieldAccess.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/test/lazy_deserialization/gen-cpp2/simple_types.h>

constexpr int kIterationCount = folly::kIsDebug ? 50'000 : 500'000;
constexpr int kListMaxSize = 10;

namespace apache::thrift::test {

using FieldType = std::vector<int32_t>;
using FieldRefType = optional_field_ref<FieldType&>;

std::mt19937 rng;

std::vector<int32_t> randomField() {
  std::vector<int32_t> ret(rng() % kListMaxSize);
  std::generate(ret.begin(), ret.end(), std::ref(rng));
  return ret;
}

template <class Struct>
std::string randomSerializedStruct() {
  Struct s;
  s.field4_ref() = randomField();
  return CompactSerializer::serialize<std::string>(s);
}

auto create(const std::vector<int32_t>& field4) {
  std::pair<OptionalFoo, OptionalLazyFoo> ret;
  ret.first.field4_ref() = field4;
  ret.second.field4_ref() = field4;
  return ret;
}

class RandomTestWithSeed : public testing::TestWithParam<int> {};
TEST_P(RandomTestWithSeed, test) {
  rng.seed(GetParam());
  OptionalFoo foo;
  OptionalLazyFoo lazyFoo;
  for (int i = 0; i < kIterationCount; i++) {
    auto arg = randomField();
    auto methods = folly::make_array<std::function<void()>>(
        [&] { EXPECT_EQ(bool(foo.field4_ref()), bool(lazyFoo.field4_ref())); },
        [&] {
          EXPECT_EQ(
              foo.field4_ref().has_value(), lazyFoo.field4_ref().has_value());
        },
        [&] {
          EXPECT_EQ(
              foo.field4_ref().value_or(arg),
              lazyFoo.field4_ref().value_or(arg));
        },
        [&] {
          EXPECT_EQ(
              foo.field4_ref().emplace(arg), lazyFoo.field4_ref().emplace(arg));
        },
        [&] { foo.field4_ref() = arg, lazyFoo.field4_ref() = arg; },
        [&] { foo.field4_ref().reset(), lazyFoo.field4_ref().reset(); },
        [&] {
          if (foo.field4_ref()) {
            EXPECT_EQ(foo.field4_ref().value(), lazyFoo.field4_ref().value());
          } else {
            EXPECT_THROW(foo.field4_ref().value(), bad_field_access);
            EXPECT_THROW(lazyFoo.field4_ref().value(), bad_field_access);
          }
        },
        [&] {
          if (foo.field4_ref()) {
            EXPECT_EQ(*foo.field4_ref(), *lazyFoo.field4_ref());
          } else {
            EXPECT_THROW(*foo.field4_ref(), bad_field_access);
            EXPECT_THROW(*lazyFoo.field4_ref(), bad_field_access);
          }
        },
        [&] { EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref()); },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          EXPECT_EQ(foo < foo2, lazyFoo < lazyFoo2);
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          EXPECT_EQ(foo > foo2, lazyFoo > lazyFoo2);
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          EXPECT_EQ(foo <= foo2, lazyFoo <= lazyFoo2);
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          EXPECT_EQ(foo >= foo2, lazyFoo >= lazyFoo2);
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          EXPECT_EQ(foo == foo2, lazyFoo == lazyFoo2);
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          EXPECT_EQ(foo != foo2, lazyFoo != lazyFoo2);
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          foo = foo2;
          lazyFoo = lazyFoo2;
          EXPECT_EQ(foo, foo2);
          EXPECT_EQ(lazyFoo, lazyFoo2);
          EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
          EXPECT_EQ(foo2.field4_ref(), lazyFoo2.field4_ref());
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          foo2 = foo;
          lazyFoo2 = lazyFoo;
          EXPECT_EQ(foo, foo2);
          EXPECT_EQ(lazyFoo, lazyFoo2);
          EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
          EXPECT_EQ(foo2.field4_ref(), lazyFoo2.field4_ref());
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          foo = std::move(foo2);
          lazyFoo = std::move(lazyFoo2);
          EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
          EXPECT_EQ(foo2.field4_ref(), lazyFoo2.field4_ref());
        },
        [&] {
          auto [foo2, lazyFoo2] = create(arg);
          foo2 = std::move(foo);
          lazyFoo2 = std::move(lazyFoo);
          EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
          EXPECT_EQ(foo2.field4_ref(), lazyFoo2.field4_ref());
        },
        [&] {
          OptionalFoo foo2 = foo;
          OptionalLazyFoo lazyFoo2 = lazyFoo;
          EXPECT_EQ(foo, foo2);
          EXPECT_EQ(lazyFoo, lazyFoo2);
          EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
          EXPECT_EQ(foo2.field4_ref(), lazyFoo2.field4_ref());
        },
        [&] {
          OptionalFoo foo2 = std::move(foo);
          OptionalLazyFoo lazyFoo2 = std::move(lazyFoo);
          EXPECT_EQ(foo.field4_ref(), lazyFoo.field4_ref());
          EXPECT_EQ(foo2.field4_ref(), lazyFoo2.field4_ref());
        },
        [&] {
          auto s = randomSerializedStruct<OptionalFoo>();
          CompactSerializer::deserialize(s, foo);
          CompactSerializer::deserialize(s, lazyFoo);
        },
        [&] {
          auto s = randomSerializedStruct<OptionalLazyFoo>();
          CompactSerializer::deserialize(s, foo);
          CompactSerializer::deserialize(s, lazyFoo);
        },
        [&] {
          auto s = randomSerializedStruct<OptionalFoo>();
          foo = CompactSerializer::deserialize<OptionalFoo>(s);
          lazyFoo = CompactSerializer::deserialize<OptionalLazyFoo>(s);
        },
        [&] {
          auto s = randomSerializedStruct<OptionalLazyFoo>();
          foo = CompactSerializer::deserialize<OptionalFoo>(s);
          lazyFoo = CompactSerializer::deserialize<OptionalLazyFoo>(s);
        });

    // Choose a random method and call it
    methods[rng() % methods.size()]();
  }
}

INSTANTIATE_TEST_CASE_P(
    RandomTest,
    RandomTestWithSeed,
    testing::Range(0, folly::kIsDebug ? 10 : 1000));

} // namespace apache::thrift::test
