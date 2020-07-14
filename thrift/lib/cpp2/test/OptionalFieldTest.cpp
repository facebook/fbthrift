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

#include <thrift/lib/cpp2/OptionalField.h>
#include <folly/Portability.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/FieldRefHash.h>

#include <algorithm>
#include <type_traits>

namespace apache {
namespace thrift {
template <class T, class U>
void heterogeneousComparisonsTest(T opt1, U opt2) {
  EXPECT_TRUE(opt1(4) == int64_t(4));
  EXPECT_FALSE(opt1(8) == int64_t(4));
  EXPECT_FALSE(opt1() == int64_t(4));

  EXPECT_TRUE(int64_t(4) == opt1(4));
  EXPECT_FALSE(int64_t(4) == opt1(8));
  EXPECT_FALSE(int64_t(4) == opt1());

  EXPECT_FALSE(opt1(4) != int64_t(4));
  EXPECT_TRUE(opt1(8) != int64_t(4));
  EXPECT_TRUE(opt1() != int64_t(4));

  EXPECT_FALSE(int64_t(4) != opt1(4));
  EXPECT_TRUE(int64_t(4) != opt1(8));
  EXPECT_TRUE(int64_t(4) != opt1());

  EXPECT_TRUE(opt1() < int64_t(4));
  EXPECT_TRUE(opt1(4) < int64_t(8));
  EXPECT_FALSE(opt1(4) < int64_t(4));
  EXPECT_FALSE(opt1(8) < int64_t(4));

  EXPECT_FALSE(opt1() > int64_t(4));
  EXPECT_FALSE(opt1(4) > int64_t(8));
  EXPECT_FALSE(opt1(4) > int64_t(4));
  EXPECT_TRUE(opt1(8) > int64_t(4));

  EXPECT_TRUE(opt1() <= int64_t(4));
  EXPECT_TRUE(opt1(4) <= int64_t(8));
  EXPECT_TRUE(opt1(4) <= int64_t(4));
  EXPECT_FALSE(opt1(8) <= int64_t(4));

  EXPECT_FALSE(opt1() >= int64_t(4));
  EXPECT_FALSE(opt1(4) >= int64_t(8));
  EXPECT_TRUE(opt1(4) >= int64_t(4));
  EXPECT_TRUE(opt1(8) >= int64_t(4));

  EXPECT_TRUE(int64_t(4) < opt1(8));
  EXPECT_FALSE(int64_t(4) < opt1(4));
  EXPECT_FALSE(int64_t(8) < opt1(4));
  EXPECT_FALSE(int64_t(4) < opt1());

  EXPECT_FALSE(int64_t(4) > opt1(8));
  EXPECT_FALSE(int64_t(4) > opt1(4));
  EXPECT_TRUE(int64_t(8) > opt1(4));
  EXPECT_TRUE(int64_t(4) > opt1());

  EXPECT_TRUE(int64_t(4) <= opt1(8));
  EXPECT_TRUE(int64_t(4) <= opt1(4));
  EXPECT_FALSE(int64_t(8) <= opt1(4));
  EXPECT_FALSE(int64_t(4) <= opt1());

  EXPECT_FALSE(int64_t(4) >= opt1(8));
  EXPECT_TRUE(int64_t(4) >= opt1(4));
  EXPECT_TRUE(int64_t(8) >= opt1(4));
  EXPECT_TRUE(int64_t(4) >= opt1());

  EXPECT_TRUE(opt1() == opt2());
  EXPECT_TRUE(opt1(4) == opt2(4));
  EXPECT_FALSE(opt1(8) == opt2(4));
  EXPECT_FALSE(opt1() == opt2(4));
  EXPECT_FALSE(opt1(4) == opt2());

  EXPECT_FALSE(opt1() != opt2());
  EXPECT_FALSE(opt1(4) != opt2(4));
  EXPECT_TRUE(opt1(8) != opt2(4));
  EXPECT_TRUE(opt1() != opt2(4));
  EXPECT_TRUE(opt1(4) != opt2());

  EXPECT_TRUE(opt1() < opt2(4));
  EXPECT_TRUE(opt1(4) < opt2(8));
  EXPECT_FALSE(opt1() < opt2());
  EXPECT_FALSE(opt1(4) < opt2(4));
  EXPECT_FALSE(opt1(8) < opt2(4));
  EXPECT_FALSE(opt1(4) < opt2());

  EXPECT_FALSE(opt1() > opt2(4));
  EXPECT_FALSE(opt1(4) > opt2(8));
  EXPECT_FALSE(opt1() > opt2());
  EXPECT_FALSE(opt1(4) > opt2(4));
  EXPECT_TRUE(opt1(8) > opt2(4));
  EXPECT_TRUE(opt1(4) > opt2());

  EXPECT_TRUE(opt1() <= opt2(4));
  EXPECT_TRUE(opt1(4) <= opt2(8));
  EXPECT_TRUE(opt1() <= opt2());
  EXPECT_TRUE(opt1(4) <= opt2(4));
  EXPECT_FALSE(opt1(8) <= opt2(4));
  EXPECT_FALSE(opt1(4) <= opt2());

  EXPECT_FALSE(opt1() >= opt2(4));
  EXPECT_FALSE(opt1(4) >= opt2(8));
  EXPECT_TRUE(opt1() >= opt2());
  EXPECT_TRUE(opt1(4) >= opt2(4));
  EXPECT_TRUE(opt1(8) >= opt2(4));
  EXPECT_TRUE(opt1(4) >= opt2());
}

TEST(OptionalFieldRefTest, HeterogeneousComparisons) {
  auto genOptionalFieldRef = [i_ = int8_t(0), b_ = false](auto... i) mutable {
    return optional_field_ref<const int8_t&>(i_ = int(i...), b_ = sizeof...(i));
  };
  heterogeneousComparisonsTest(genOptionalFieldRef, genOptionalFieldRef);
}

TEST(FieldRefTest, HeterogeneousComparisons) {
  auto genFieldRef = [i_ = int8_t(0), b_ = false](auto... i) mutable {
    return field_ref<const int8_t&>(i_ = int(i...), b_ = sizeof...(i));
  };
  heterogeneousComparisonsTest(genFieldRef, genFieldRef);
}

TEST(RequiredFieldRefTest, HeterogeneousComparisons) {
  auto genRequiredFieldRef = [i_ = int8_t(0)](auto... i) mutable {
    return required_field_ref<const int8_t&>(i_ = int(i...));
  };
  heterogeneousComparisonsTest(genRequiredFieldRef, genRequiredFieldRef);
}
} // namespace thrift
} // namespace apache

struct Tag {};

namespace std {
// @lint-ignore HOWTOEVEN MisplacedTemplateSpecialization
template <>
struct hash<Tag> {
  explicit hash(size_t i) : i(i) {}
  size_t operator()(Tag) const {
    return i;
  }
  size_t i;
};
} // namespace std

namespace folly {
// @lint-ignore HOWTOEVEN MisplacedTemplateSpecialization
template <>
struct hasher<Tag> {
  explicit hasher(size_t i) : i(i) {}
  size_t operator()(Tag) const {
    return i;
  }
  size_t i;
};
} // namespace folly

namespace apache {
namespace thrift {
template <template <class...> class Hasher, template <class> class Optional>
void StatelessHashTest() {
  Hasher<Optional<int&>> hash;
  int x = 0;
  bool y = false;
  Optional<int&> f(x, y);
  if (std::is_same<Optional<int&>, apache::thrift::field_ref<int&>>::value) {
    EXPECT_EQ(hash(f), Hasher<int>()(0));
  } else {
    EXPECT_EQ(
        hash(f), apache::thrift::detail::kHashValueForNonExistsOptionalField);
  }
  y = true;
  for (x = 0; x < 1000; x++) {
    EXPECT_EQ(hash(f), Hasher<int>()(x));
  }
}

template <template <class...> class Hasher, template <class> class Optional>
void StatefulHashTest() {
  Hasher<Optional<Tag&>> hash(42);
  Tag x;
  bool y = false;
  Optional<Tag&> f(x, y);
  if (std::is_same<Optional<int&>, apache::thrift::field_ref<int&>>::value) {
    EXPECT_EQ(hash(f), 42);
  } else {
    EXPECT_EQ(
        hash(f), apache::thrift::detail::kHashValueForNonExistsOptionalField);
  }
  y = true;
  EXPECT_EQ(hash(f), 42);
}

// @lint-ignore HOWTOEVEN BadImplicitCast
TEST(optional_field_ref_test, Hash) {
  StatelessHashTest<std::hash, field_ref>();
  StatelessHashTest<std::hash, optional_field_ref>();
  StatelessHashTest<folly::hasher, field_ref>();
  StatelessHashTest<folly::hasher, optional_field_ref>();
  StatefulHashTest<std::hash, field_ref>();
  StatefulHashTest<std::hash, optional_field_ref>();
  StatefulHashTest<folly::hasher, field_ref>();
  StatefulHashTest<folly::hasher, optional_field_ref>();
}
} // namespace thrift
} // namespace apache
