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
#include <folly/Optional.h>
#include <folly/Portability.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <algorithm>
#include <initializer_list>
#include <iomanip>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

using folly::none;
using std::shared_ptr;
using std::unique_ptr;

namespace apache {
namespace thrift {

namespace {

template <class V>
std::ostream& operator<<(
    std::ostream& os,
    const DeprecatedOptionalField<V>& v) {
  if (v) {
    os << "DeprecatedOptionalField(" << v.value() << ')';
  } else {
    os << "None";
  }
  return os;
}

struct NoDefault {
  NoDefault(int, int) {}
  char a, b, c;
};

} // namespace

static_assert(sizeof(DeprecatedOptionalField<char>) == 2, "");
static_assert(sizeof(DeprecatedOptionalField<int>) == 8, "");
static_assert(sizeof(DeprecatedOptionalField<NoDefault>) == 4, "");
static_assert(
    sizeof(DeprecatedOptionalField<char>) == sizeof(boost::optional<char>),
    "");
static_assert(
    sizeof(DeprecatedOptionalField<short>) == sizeof(boost::optional<short>),
    "");
static_assert(
    sizeof(DeprecatedOptionalField<int>) == sizeof(boost::optional<int>),
    "");
static_assert(
    sizeof(DeprecatedOptionalField<double>) == sizeof(boost::optional<double>),
    "");

TEST(DeprecatedOptionalField, ConstexprConstructible) {
  // Use FOLLY_STORAGE_CONSTEXPR to work around MSVC not taking this.
  static FOLLY_STORAGE_CONSTEXPR DeprecatedOptionalField<int> opt;
  // NOTE: writing `opt = none` instead of `opt(none)` causes gcc to reject this
  // code, claiming that the (non-constexpr) move ctor of
  // `DeprecatedOptionalField` is being invoked.
  static FOLLY_STORAGE_CONSTEXPR DeprecatedOptionalField<int> opt2;

  EXPECT_FALSE(opt.has_value());
  EXPECT_FALSE(opt2.has_value());
}

TEST(DeprecatedOptionalField, NoDefault) {
  DeprecatedOptionalField<NoDefault> x;
  EXPECT_FALSE(x);
  x.emplace(4, 5);
  EXPECT_TRUE(bool(x));
  x.reset();
  EXPECT_FALSE(x);
}

TEST(DeprecatedOptionalField, Emplace) {
  DeprecatedOptionalField<std::vector<int>> opt;
  auto& values1 = opt.emplace(3, 4);
  EXPECT_THAT(values1, testing::ElementsAre(4, 4, 4));
  auto& values2 = opt.emplace(2, 5);
  EXPECT_THAT(values2, testing::ElementsAre(5, 5));
}

TEST(DeprecatedOptionalField, EmplaceInitializerList) {
  DeprecatedOptionalField<std::vector<int>> opt;
  auto& values1 = opt.emplace({3, 4, 5});
  EXPECT_THAT(values1, testing::ElementsAre(3, 4, 5));
  auto& values2 = opt.emplace({4, 5, 6});
  EXPECT_THAT(values2, testing::ElementsAre(4, 5, 6));
}

TEST(DeprecatedOptionalField, Reset) {
  DeprecatedOptionalField<int> opt(3);
  opt.reset();
  EXPECT_FALSE(opt);
}

TEST(DeprecatedOptionalField, String) {
  DeprecatedOptionalField<std::string> maybeString;
  EXPECT_FALSE(maybeString);
  maybeString = "hello";
  EXPECT_TRUE(bool(maybeString));
}

TEST(DeprecatedOptionalField, Const) {
  { // default construct
    DeprecatedOptionalField<const int> opt;
    EXPECT_FALSE(bool(opt));
    opt.emplace(4);
    EXPECT_EQ(*opt, 4);
    opt.emplace(5);
    EXPECT_EQ(*opt, 5);
    opt.reset();
    EXPECT_FALSE(bool(opt));
  }
  { // copy-constructed
    const int x = 6;
    DeprecatedOptionalField<const int> opt(x);
    EXPECT_EQ(*opt, 6);
  }
  { // move-constructed
    const int x = 7;
    DeprecatedOptionalField<const int> opt(std::move(x));
    EXPECT_EQ(*opt, 7);
  }
  // no assignment allowed
}

TEST(DeprecatedOptionalField, Simple) {
  DeprecatedOptionalField<int> opt;
  EXPECT_FALSE(bool(opt));
  EXPECT_EQ(42, opt.value_or(42));
  opt = 4;
  EXPECT_TRUE(bool(opt));
  EXPECT_EQ(4, *opt);
  EXPECT_EQ(4, opt.value_or(42));
  opt = 5;
  EXPECT_EQ(5, *opt);
  opt.reset();
  EXPECT_FALSE(bool(opt));
}

namespace {

class MoveTester {
 public:
  /* implicit */ MoveTester(const char* s) : s_(s) {}
  MoveTester(const MoveTester&) = default;
  MoveTester(MoveTester&& other) noexcept {
    s_ = std::move(other.s_);
    other.s_ = "";
  }
  MoveTester& operator=(const MoveTester&) = default;
  MoveTester& operator=(MoveTester&& other) noexcept {
    s_ = std::move(other.s_);
    other.s_ = "";
    return *this;
  }

 private:
  friend bool operator==(const MoveTester& o1, const MoveTester& o2);
  std::string s_;
};

bool operator==(const MoveTester& o1, const MoveTester& o2) {
  return o1.s_ == o2.s_;
}

} // namespace

TEST(DeprecatedOptionalField, value_or_rvalue_arg) {
  DeprecatedOptionalField<MoveTester> opt;
  MoveTester dflt = "hello";
  EXPECT_EQ("hello", opt.value_or(dflt));
  EXPECT_EQ("hello", dflt);
  EXPECT_EQ("hello", opt.value_or(std::move(dflt)));
  EXPECT_EQ("", dflt);
  EXPECT_EQ("world", opt.value_or("world"));

  dflt = "hello";
  // Make sure that the const overload works on const objects
  const auto& optc = opt;
  EXPECT_EQ("hello", optc.value_or(dflt));
  EXPECT_EQ("hello", dflt);
  EXPECT_EQ("hello", optc.value_or(std::move(dflt)));
  EXPECT_EQ("", dflt);
  EXPECT_EQ("world", optc.value_or("world"));

  dflt = "hello";
  opt = "meow";
  EXPECT_EQ("meow", opt.value_or(dflt));
  EXPECT_EQ("hello", dflt);
  EXPECT_EQ("meow", opt.value_or(std::move(dflt)));
  EXPECT_EQ("hello", dflt); // only moved if used
}

TEST(DeprecatedOptionalField, value_or_noncopyable) {
  DeprecatedOptionalField<std::unique_ptr<int>> opt;
  std::unique_ptr<int> dflt(new int(42));
  EXPECT_EQ(42, *std::move(opt).value_or(std::move(dflt)));
}

struct ExpectingDeleter {
  explicit ExpectingDeleter(int expected_) : expected(expected_) {}
  int expected;
  void operator()(const int* ptr) {
    EXPECT_EQ(*ptr, expected);
    delete ptr;
  }
};

TEST(DeprecatedOptionalField, value_move) {
  auto ptr = DeprecatedOptionalField<std::unique_ptr<int, ExpectingDeleter>>(
                 {new int(42), ExpectingDeleter{1337}})
                 .value();
  *ptr = 1337;
}

TEST(DeprecatedOptionalField, dereference_move) {
  auto ptr = *DeprecatedOptionalField<std::unique_ptr<int, ExpectingDeleter>>(
      {new int(42), ExpectingDeleter{1337}});
  *ptr = 1337;
}

TEST(DeprecatedOptionalField, EmptyConstruct) {
  DeprecatedOptionalField<int> opt;
  EXPECT_FALSE(bool(opt));
  DeprecatedOptionalField<int> test1(opt);
  EXPECT_FALSE(bool(test1));
  DeprecatedOptionalField<int> test2(std::move(opt));
  EXPECT_FALSE(bool(test2));
}

TEST(DeprecatedOptionalField, Unique) {
  DeprecatedOptionalField<unique_ptr<int>> opt;

  opt.reset();
  EXPECT_FALSE(bool(opt));
  // empty->emplaced
  opt.emplace(new int(5));
  EXPECT_TRUE(bool(opt));
  EXPECT_EQ(5, **opt);

  opt.reset();
  // empty->moved
  opt = std::make_unique<int>(6);
  EXPECT_EQ(6, **opt);
  // full->moved
  opt = std::make_unique<int>(7);
  EXPECT_EQ(7, **opt);

  // move it out by move construct
  DeprecatedOptionalField<unique_ptr<int>> moved(std::move(opt));
  EXPECT_TRUE(bool(moved));
  EXPECT_FALSE(bool(opt));
  EXPECT_EQ(7, **moved);

  EXPECT_TRUE(bool(moved));
  opt = std::move(moved); // move it back by move assign
  EXPECT_FALSE(bool(moved));
  EXPECT_TRUE(bool(opt));
  EXPECT_EQ(7, **opt);
}

TEST(DeprecatedOptionalField, Shared) {
  shared_ptr<int> ptr;
  DeprecatedOptionalField<shared_ptr<int>> opt;
  EXPECT_FALSE(bool(opt));
  // empty->emplaced
  opt.emplace(new int(5));
  EXPECT_TRUE(bool(opt));
  ptr = opt.value();
  EXPECT_EQ(ptr.get(), opt->get());
  EXPECT_EQ(2, ptr.use_count());
  opt.reset();
  EXPECT_EQ(1, ptr.use_count());
  // full->copied
  opt = ptr;
  EXPECT_EQ(2, ptr.use_count());
  EXPECT_EQ(ptr.get(), opt->get());
  opt.reset();
  EXPECT_EQ(1, ptr.use_count());
  // full->moved
  opt = std::move(ptr);
  EXPECT_EQ(1, opt->use_count());
  EXPECT_EQ(nullptr, ptr.get());
  {
    DeprecatedOptionalField<shared_ptr<int>> copied(opt);
    EXPECT_EQ(2, opt->use_count());
    DeprecatedOptionalField<shared_ptr<int>> moved(std::move(opt));
    EXPECT_EQ(2, moved->use_count());
    moved.emplace(new int(6));
    EXPECT_EQ(1, moved->use_count());
    copied = moved;
    EXPECT_EQ(2, moved->use_count());
  }
}

TEST(DeprecatedOptionalField, Order) {
  std::vector<DeprecatedOptionalField<int>> vect{
      DeprecatedOptionalField<int>{},
      DeprecatedOptionalField<int>{3},
      DeprecatedOptionalField<int>{1},
      DeprecatedOptionalField<int>{},
      DeprecatedOptionalField<int>{2},
  };
  std::vector<DeprecatedOptionalField<int>> expected{
      DeprecatedOptionalField<int>{},
      DeprecatedOptionalField<int>{},
      DeprecatedOptionalField<int>{1},
      DeprecatedOptionalField<int>{2},
      DeprecatedOptionalField<int>{3},
  };
  std::sort(vect.begin(), vect.end());
  EXPECT_EQ(vect, expected);
}

TEST(DeprecatedOptionalField, Swap) {
  DeprecatedOptionalField<std::string> a;
  DeprecatedOptionalField<std::string> b;

  swap(a, b);
  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(b.has_value());

  a = "hello";
  EXPECT_TRUE(a.has_value());
  EXPECT_FALSE(b.has_value());
  EXPECT_EQ("hello", a.value());

  swap(a, b);
  EXPECT_FALSE(a.has_value());
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ("hello", b.value());

  a = "bye";
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ("bye", a.value());

  swap(a, b);
  EXPECT_TRUE(a.has_value());
  EXPECT_TRUE(b.has_value());
  EXPECT_EQ("hello", a.value());
  EXPECT_EQ("bye", b.value());
}

TEST(DeprecatedOptionalField, Comparisons) {
  DeprecatedOptionalField<int> o_;
  DeprecatedOptionalField<int> o1(1);
  DeprecatedOptionalField<int> o2(2);

  EXPECT_TRUE(o_ <= (o_));
  EXPECT_TRUE(o_ == (o_));
  EXPECT_TRUE(o_ >= (o_));

  EXPECT_TRUE(o1 < o2);
  EXPECT_TRUE(o1 <= o2);
  EXPECT_TRUE(o1 <= (o1));
  EXPECT_TRUE(o1 == (o1));
  EXPECT_TRUE(o1 != o2);
  EXPECT_TRUE(o1 >= (o1));
  EXPECT_TRUE(o2 >= o1);
  EXPECT_TRUE(o2 > o1);

  EXPECT_FALSE(o2 < o1);
  EXPECT_FALSE(o2 <= o1);
  EXPECT_FALSE(o2 <= o1);
  EXPECT_FALSE(o2 == o1);
  EXPECT_FALSE(o1 != (o1));
  EXPECT_FALSE(o1 >= o2);
  EXPECT_FALSE(o1 >= o2);
  EXPECT_FALSE(o1 > o2);
}

template <template <class> class Optional1, template <class> class Optional2>
void heterogeneousComparisonsTest() {
  using opt8 = Optional1<uint8_t>;
  using opt64 = Optional2<uint8_t>;
  EXPECT_TRUE(opt8(4) == uint64_t(4));
  EXPECT_FALSE(opt8(8) == uint64_t(4));
  EXPECT_FALSE(opt8() == uint64_t(4));

  EXPECT_TRUE(uint64_t(4) == opt8(4));
  EXPECT_FALSE(uint64_t(4) == opt8(8));
  EXPECT_FALSE(uint64_t(4) == opt8());

  EXPECT_FALSE(opt8(4) != uint64_t(4));
  EXPECT_TRUE(opt8(8) != uint64_t(4));
  EXPECT_TRUE(opt8() != uint64_t(4));

  EXPECT_FALSE(uint64_t(4) != opt8(4));
  EXPECT_TRUE(uint64_t(4) != opt8(8));
  EXPECT_TRUE(uint64_t(4) != opt8());

  EXPECT_TRUE(opt8() == opt64());
  EXPECT_TRUE(opt8(4) == opt64(4));
  EXPECT_FALSE(opt8(8) == opt64(4));
  EXPECT_FALSE(opt8() == opt64(4));
  EXPECT_FALSE(opt8(4) == opt64());

  EXPECT_FALSE(opt8() != opt64());
  EXPECT_FALSE(opt8(4) != opt64(4));
  EXPECT_TRUE(opt8(8) != opt64(4));
  EXPECT_TRUE(opt8() != opt64(4));
  EXPECT_TRUE(opt8(4) != opt64());

  EXPECT_TRUE(opt8() < opt64(4));
  EXPECT_TRUE(opt8(4) < opt64(8));
  EXPECT_FALSE(opt8() < opt64());
  EXPECT_FALSE(opt8(4) < opt64(4));
  EXPECT_FALSE(opt8(8) < opt64(4));
  EXPECT_FALSE(opt8(4) < opt64());

  EXPECT_FALSE(opt8() > opt64(4));
  EXPECT_FALSE(opt8(4) > opt64(8));
  EXPECT_FALSE(opt8() > opt64());
  EXPECT_FALSE(opt8(4) > opt64(4));
  EXPECT_TRUE(opt8(8) > opt64(4));
  EXPECT_TRUE(opt8(4) > opt64());

  EXPECT_TRUE(opt8() <= opt64(4));
  EXPECT_TRUE(opt8(4) <= opt64(8));
  EXPECT_TRUE(opt8() <= opt64());
  EXPECT_TRUE(opt8(4) <= opt64(4));
  EXPECT_FALSE(opt8(8) <= opt64(4));
  EXPECT_FALSE(opt8(4) <= opt64());

  EXPECT_FALSE(opt8() >= opt64(4));
  EXPECT_FALSE(opt8(4) >= opt64(8));
  EXPECT_TRUE(opt8() >= opt64());
  EXPECT_TRUE(opt8(4) >= opt64(4));
  EXPECT_TRUE(opt8(8) >= opt64(4));
  EXPECT_TRUE(opt8(4) >= opt64());
}

TEST(DeprecatedOptionalField, HeterogeneousComparisons) {
  heterogeneousComparisonsTest<DeprecatedOptionalField, folly::Optional>();
  heterogeneousComparisonsTest<folly::Optional, DeprecatedOptionalField>();
}

TEST(DeprecatedOptionalField, NoneComparisons) {
  using opt = DeprecatedOptionalField<int>;
  EXPECT_TRUE(opt() == none);
  EXPECT_TRUE(none == opt());
  EXPECT_FALSE(opt(1) == none);
  EXPECT_FALSE(none == opt(1));

  EXPECT_FALSE(opt() != none);
  EXPECT_FALSE(none != opt());
  EXPECT_TRUE(opt(1) != none);
  EXPECT_TRUE(none != opt(1));

  EXPECT_FALSE(opt() < none);
  EXPECT_FALSE(none < opt());
  EXPECT_FALSE(opt(1) < none);
  EXPECT_TRUE(none < opt(1));

  EXPECT_FALSE(opt() > none);
  EXPECT_FALSE(none > opt());
  EXPECT_FALSE(none > opt(1));
  EXPECT_TRUE(opt(1) > none);

  EXPECT_TRUE(opt() <= none);
  EXPECT_TRUE(none <= opt());
  EXPECT_FALSE(opt(1) <= none);
  EXPECT_TRUE(none <= opt(1));

  EXPECT_TRUE(opt() >= none);
  EXPECT_TRUE(none >= opt());
  EXPECT_TRUE(opt(1) >= none);
  EXPECT_FALSE(none >= opt(1));
}

TEST(DeprecatedOptionalField, Conversions) {
  DeprecatedOptionalField<bool> mbool;
  DeprecatedOptionalField<short> mshort;
  DeprecatedOptionalField<char*> mstr;
  DeprecatedOptionalField<int> mint;

  // These don't compile
  // bool b = mbool;
  // short s = mshort;
  // char* c = mstr;
  // int x = mint;
  // char* c(mstr);
  // short s(mshort);
  // int x(mint);

  // intended explicit operator bool, for if (opt).
  bool b(mbool);
  EXPECT_FALSE(b);

  // Truthy tests work and are not ambiguous
  if (mbool && mshort && mstr && mint) { // only checks not-empty
    if (*mbool && *mshort && *mstr && *mint) { // only checks value
      ;
    }
  }

  mbool = false;
  EXPECT_TRUE(bool(mbool));
  EXPECT_FALSE(*mbool);

  mbool = true;
  EXPECT_TRUE(bool(mbool));
  EXPECT_TRUE(*mbool);

  mbool = none;
  EXPECT_FALSE(bool(mbool));

  // No conversion allowed; does not compile
  // EXPECT_TRUE(mbool == false);
}

TEST(DeprecatedOptionalField, Pointee) {
  DeprecatedOptionalField<int> x;
  EXPECT_FALSE(get_pointer(x));
  x = 1;
  EXPECT_TRUE(get_pointer(x));
  *get_pointer(x) = 2;
  EXPECT_TRUE(*x == 2);
  x = none;
  EXPECT_FALSE(get_pointer(x));
}

TEST(DeprecatedOptionalField, SelfAssignment) {
  DeprecatedOptionalField<int> a{42};
  a = static_cast<decltype(a)&>(a); // suppress self-assign warning
  ASSERT_TRUE(a.has_value() && a.value() == 42);

  DeprecatedOptionalField<int> b{23333333};
  b = static_cast<decltype(b)&&>(b); // suppress self-move warning
  ASSERT_TRUE(b.has_value() && b.value() == 23333333);
}

namespace {

class ContainsOptional {
 public:
  ContainsOptional() {}
  explicit ContainsOptional(int x) : opt_(x) {}
  bool hasValue() const {
    return opt_.has_value();
  }
  int value() const {
    return opt_.value();
  }

  ContainsOptional(const ContainsOptional& other) = default;
  ContainsOptional& operator=(const ContainsOptional& other) = default;
  ContainsOptional(ContainsOptional&& other) = default;
  ContainsOptional& operator=(ContainsOptional&& other) = default;

 private:
  DeprecatedOptionalField<int> opt_;
};

} // namespace

/**
 * Test that a class containing an DeprecatedOptionalField can be copy and move
 * assigned.
 */
TEST(DeprecatedOptionalField, AssignmentContained) {
  {
    ContainsOptional source(5), target;
    target = source;
    EXPECT_TRUE(target.hasValue());
    EXPECT_EQ(5, target.value());
  }

  {
    ContainsOptional source(5), target;
    target = std::move(source);
    EXPECT_TRUE(target.hasValue());
    EXPECT_EQ(5, target.value());
    EXPECT_FALSE(source.hasValue());
  }

  {
    ContainsOptional opt_uninit, target(10);
    target = opt_uninit;
    EXPECT_FALSE(target.hasValue());
  }
}

TEST(DeprecatedOptionalField, Exceptions) {
  DeprecatedOptionalField<int> empty;
  EXPECT_THROW(empty.value(), folly::OptionalEmptyException);
}

TEST(DeprecatedOptionalField, NoThrowDefaultConstructible) {
  EXPECT_TRUE(std::is_nothrow_default_constructible<
              DeprecatedOptionalField<bool>>::value);
}

namespace {

struct NoDestructor {};

struct WithDestructor {
  ~WithDestructor();
};

} // namespace

TEST(DeprecatedOptionalField, TriviallyDestructible) {
  // These could all be static_asserts but EXPECT_* give much nicer output on
  // failure.
  EXPECT_TRUE(std::is_trivially_destructible<
              DeprecatedOptionalField<NoDestructor>>::value);
  EXPECT_TRUE(
      std::is_trivially_destructible<DeprecatedOptionalField<int>>::value);
  EXPECT_FALSE(std::is_trivially_destructible<
               DeprecatedOptionalField<WithDestructor>>::value);
}

TEST(DeprecatedOptionalField, Hash) {
  // Test it's usable in std::unordered map (compile time check)
  std::unordered_map<DeprecatedOptionalField<int>, DeprecatedOptionalField<int>>
      obj;
  // Also check the std::hash template can be instantiated by the compiler
  std::hash<DeprecatedOptionalField<int>>()(DeprecatedOptionalField<int>());
  std::hash<DeprecatedOptionalField<int>>()(DeprecatedOptionalField<int>(3));
}

namespace {

struct WithConstMember {
  /* implicit */ WithConstMember(int val) : x(val) {}
  const int x;
};

// Make this opaque to the optimizer by preventing inlining.
FOLLY_NOINLINE void replaceWith2(DeprecatedOptionalField<WithConstMember>& o) {
  o.emplace(2);
}

} // namespace

TEST(DeprecatedOptionalField, ConstMember) {
  // Verify that the compiler doesn't optimize out the second load of
  // o->x based on the assumption that the field is const.
  //
  // Current DeprecatedOptionalField implementation doesn't defend against that
  // assumption, thus replacing an optional where the object has const
  // members is technically UB and would require wrapping each access
  // to the storage with std::launder, but this prevents useful
  // optimizations.
  //
  // Implementations of std::optional in both libstdc++ and libc++ are
  // subject to the same UB. It is then reasonable to believe that
  // major compilers don't rely on the constness assumption.
  DeprecatedOptionalField<WithConstMember> o(1);
  int sum = 0;
  sum += o->x;
  replaceWith2(o);
  sum += o->x;
  EXPECT_EQ(sum, 3);
}

} // namespace thrift
} // namespace apache
