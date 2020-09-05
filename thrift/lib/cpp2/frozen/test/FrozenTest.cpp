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

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/frozen/FrozenUtil.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_layouts.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_types_custom_protocol.h>
#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

using namespace apache::thrift;
using namespace apache::thrift::frozen;
using namespace apache::thrift::test;
using namespace apache::thrift::util;
using namespace testing;

template <class T>
std::string toString(const T& x) {
  return debugString(x);
}
template <class T>
std::string toString(const Layout<T>& x) {
  std::ostringstream xStr;
  xStr << x;
  return xStr.str();
}

#define EXPECT_PRINTED_EQ(a, b) EXPECT_EQ(toString(a), toString(b))

EveryLayout stressValue2 = [] {
  EveryLayout x;
  *x.aBool_ref() = true;
  *x.aInt_ref() = 2;
  *x.aList_ref() = {3, 5};
  *x.aSet_ref() = {7, 11};
  *x.aHashSet_ref() = {13, 17};
  *x.aMap_ref() = {{19, 23}, {29, 31}};
  *x.aHashMap_ref() = {{37, 41}, {43, 47}};
  x.optInt_ref() = 53;
  *x.aFloat_ref() = 59.61;
  x.optMap_ref() = {{2, 4}, {3, 9}};
  return x;
}();

template <class T>
Layout<T>&& layout(const T& x, Layout<T>&& layout = Layout<T>()) {
  size_t size = LayoutRoot::layout(x, layout);
  (void)size;
  return std::move(layout);
}

template <class T>
Layout<T> layout(const T& x, size_t& size) {
  Layout<T> layout;
  size = LayoutRoot::layout(x, layout);
  return std::move(layout);
}

auto tom1 = [] {
  Pet1 max;
  *max.name_ref() = "max";
  Pet1 ed;
  *ed.name_ref() = "ed";
  Person1 tom;
  *tom.name_ref() = "tom";
  *tom.height_ref() = 1.82f;
  tom.age_ref() = 30;
  tom.pets_ref()->push_back(max);
  tom.pets_ref()->push_back(ed);
  return tom;
}();
auto tom2 = [] {
  Pet2 max;
  *max.name_ref() = "max";
  Pet2 ed;
  *ed.name_ref() = "ed";
  Person2 tom;
  *tom.name_ref() = "tom";
  *tom.weight_ref() = 169;
  tom.age_ref() = 30;
  tom.pets_ref()->push_back(max);
  tom.pets_ref()->push_back(ed);
  return tom;
}();

TEST(Frozen, EndToEnd) {
  auto view = freeze(tom1);
  EXPECT_EQ(*tom1.name_ref(), view.name());
  ASSERT_TRUE(view.name_ref().has_value());
  EXPECT_EQ(*tom1.name_ref(), *view.name_ref());
  ASSERT_TRUE(view.age().has_value());
  EXPECT_EQ(*tom1.age_ref(), view.age().value());
  EXPECT_EQ(*tom1.height_ref(), view.height());
  EXPECT_EQ(view.pets()[0].name(), *tom1.pets_ref()[0].name_ref());
  auto& pets = *tom1.pets_ref();
  auto fpets = view.pets();
  ASSERT_EQ(pets.size(), fpets.size());
  for (size_t i = 0; i < tom1.pets_ref()->size(); ++i) {
    EXPECT_EQ(*pets[i].name_ref(), fpets[i].name());
  }
  Layout<Person1> layout;
  LayoutRoot::layout(tom1, layout);
  auto ttom = view.thaw();
  EXPECT_TRUE(ttom.__isset.name);
  EXPECT_PRINTED_EQ(tom1, ttom);
}

TEST(Frozen, Comparison) {
  EXPECT_EQ(frozenSize(tom1), frozenSize(tom2));
  auto view1 = freeze(tom1);
  auto view2 = freeze(tom2);
  // name is optional now, and was __isset=true, so we don't write it
  ASSERT_TRUE(view2.age().has_value());
  EXPECT_EQ(*tom2.age_ref(), view2.age().value());
  EXPECT_EQ(*tom2.name_ref(), view2.name());
  EXPECT_EQ(tom2.name_ref()->size(), view2.name().size());
  auto ttom2 = view2.thaw();
  EXPECT_EQ(tom2, ttom2) << debugString(tom2) << debugString(ttom2);
}

TEST(Frozen, Compatibility) {
  // Make sure Person1 works well with Person2
  schema::MemorySchema schema;
  Layout<Person1> person1;
  Layout<Person2> person2;

  size_t size = LayoutRoot::layout(tom1, person1);

  saveRoot(person1, schema);
  loadRoot(person2, schema);

  std::string storage(size, 'X');
  folly::MutableStringPiece charRange(&storage.front(), size);
  const folly::MutableByteRange bytes(charRange);
  folly::MutableByteRange freezeRange = bytes;

  ByteRangeFreezer::freeze(person1, tom1, freezeRange);
  auto view12 = person1.view({bytes.begin(), 0});
  auto view21 = person2.view({bytes.begin(), 0});
  EXPECT_EQ(view12.name(), view21.name());
  EXPECT_EQ(view12.age(), view21.age());
  EXPECT_TRUE(view12.height());
  EXPECT_FALSE(view21.weight());
  ASSERT_GE(view12.pets().size(), 2);
  EXPECT_EQ(view12.pets()[0].name(), view21.pets()[0].name());
  EXPECT_EQ(view12.pets()[1].name(), view21.pets()[1].name());
}

// It's important to make sure the hash function not change, other wise the
// existing indexed data will be messed up.
TEST(Frozen, HashCompatibility) {
  // int
  std::hash<int64_t> intHash;
  for (int64_t i = -10000; i < 10000; ++i) {
    EXPECT_EQ(Layout<int64_t>::hash(i), intHash(i));
  }

  // string
  using StrLayout = Layout<std::string>;
  using View = StrLayout::View;

  auto follyHash = [](const View& v) {
    return folly::hash::fnv64_buf(v.begin(), v.size());
  };

  std::vector<std::string> strs{
      "hello", "WOrld", "luckylook", "facebook", "Let it go!!"};
  for (auto&& s : strs) {
    View v(s);
    EXPECT_EQ(StrLayout::hash(v), follyHash(v));
  }
}

TEST(Frozen, EmbeddedSchema) {
  std::string storage;
  {
    schema::Schema schema;
    schema::MemorySchema memSchema;

    Layout<Person1> person1a;

    size_t size;
    size = LayoutRoot::layout(tom1, person1a);
    saveRoot(person1a, memSchema);

    schema::convert(memSchema, schema);

    CompactSerializer::serialize(schema, &storage);
    size_t start = storage.size();
    storage.resize(size + storage.size());

    folly::MutableStringPiece charRange(&storage[start], size);
    folly::MutableByteRange bytes(charRange);
    ByteRangeFreezer::freeze(person1a, tom1, bytes);
  }
  {
    schema::Schema schema;
    schema::MemorySchema memSchema;
    Layout<Person2> person2;

    size_t start = CompactSerializer::deserialize(storage, schema);

    schema::convert(std::move(schema), memSchema);

    loadRoot(person2, memSchema);

    folly::StringPiece charRange(&storage[start], storage.size() - start);
    folly::ByteRange bytes(charRange);
    auto view = person2.view({bytes.begin(), 0});
    EXPECT_EQ(*tom1.name_ref(), view.name());
    ASSERT_EQ(tom1.__isset.age, view.age().has_value());
    if (auto age = tom1.age_ref()) {
      EXPECT_EQ(*age, view.age().value());
    }
    EXPECT_EQ(*tom1.pets_ref()[0].name_ref(), view.pets()[0].name());
    EXPECT_EQ(*tom1.pets_ref()[1].name_ref(), view.pets()[1].name());
  }
}

TEST(Frozen, NoLayout) {
  ViewPosition null{nullptr, 0};

  EXPECT_FALSE(Layout<bool>().view(null));
  EXPECT_EQ(0, Layout<int>().view(null));
  EXPECT_EQ(0.0f, Layout<float>().view(null));
  EXPECT_EQ(
      apache::thrift::frozen::OptionalFieldView<int>(),
      Layout<folly::Optional<int>>().view(null));
  EXPECT_EQ(std::string(), Layout<std::string>().view(null));
  EXPECT_EQ(std::vector<int>(), Layout<std::vector<int>>().view(null).thaw());
  EXPECT_EQ(Person1(), Layout<Person1>().view(null).thaw());
  EXPECT_EQ(Pet1(), Layout<Pet1>().view(null).thaw());
  EXPECT_EQ(std::set<int>(), Layout<std::set<int>>().view(null).thaw());
  EXPECT_EQ(
      (std::map<int, int>()), (Layout<std::map<int, int>>().view(null).thaw()));

  Layout<Person1> emptyPersonLayout;
  std::array<uint8_t, 100> storage;
  folly::MutableByteRange bytes(storage.begin(), storage.end());
  EXPECT_THROW(
      ByteRangeFreezer::freeze(emptyPersonLayout, tom1, bytes),
      LayoutException);
}

template <class T>
void testMaxLayout(const T& value) {
  auto minLayout = Layout<T>();
  auto valLayout = minLayout;
  auto maxLayout = maximumLayout<T>();
  LayoutRoot::layout(value, valLayout);
  EXPECT_GT(valLayout.size, 0);
  ASSERT_GT(maxLayout.size, 0);
  std::array<uint8_t, 1000> storage;
  folly::MutableByteRange bytes(storage.begin(), storage.end());
  EXPECT_THROW(
      ByteRangeFreezer::freeze(minLayout, value, bytes), LayoutException);
  auto f = ByteRangeFreezer::freeze(maxLayout, value, bytes);
  auto check = f.thaw();
  EXPECT_EQ(value, check);
}

TEST(Frozen, MaxLayoutVector) {
  testMaxLayout(std::vector<int>{99, 24});
}

TEST(Frozen, MaxLayoutPairTree) {
  using std::make_pair;
  auto p1 = make_pair(5, 2.3);
  auto p2 = make_pair(4, p1);
  auto p3 = make_pair(3, p2);
  auto p4 = make_pair(2, p3);
  auto p5 = make_pair(1, p4);
  auto p6 = make_pair(0, p5);
  testMaxLayout(p6);
}

TEST(Frozen, MaxLayoutStress) {
  testMaxLayout(stressValue2);
}

TEST(Frozen, String) {
  std::string str = "Hello";
  auto fstr = freeze(str);
  EXPECT_EQ(str, folly::StringPiece(fstr));
  EXPECT_EQ(std::string(), folly::StringPiece(freeze(std::string())));
}

TEST(Frozen, VectorString) {
  std::vector<std::string> strs{"hello", "sara"};
  auto fstrs = freeze(strs);
  EXPECT_EQ(strs[0], fstrs[0]);
  EXPECT_EQ(strs[1], fstrs[1]);
  EXPECT_EQ(strs.size(), fstrs.size());
  std::vector<std::string> check;
}

TEST(Frozen, BigMap) {
  PlaceTest t;
  for (int i = 0; i < 1000; ++i) {
    auto& place = t.places_ref()[i * i * i % 757368944];
    *place.name_ref() = folly::to<std::string>(i);
    for (int j = 0; j < 200; ++j) {
      ++place.popularityByHour_ref()[rand() % (24 * 7)];
    }
  }
  folly::IOBufQueue bq(folly::IOBufQueue::cacheChainLength());
  CompactSerializer::serialize(t, &bq);
  auto compactSize = bq.chainLength();
  auto frozenSize = ::frozenSize(t);
  EXPECT_EQ(t, freeze(t).thaw());
  EXPECT_LT(frozenSize, compactSize * 0.7);
}
Tiny tiny1 = [] {
  Tiny obj;
  obj.a = "just a";
  return obj;
}();
Tiny tiny2 = [] {
  Tiny obj;
  obj.a = "two";
  *obj.b_ref() = "set";
  return obj;
}();
Tiny tiny4 = [] {
  Tiny obj;
  obj.a = "four";
  *obj.b_ref() = "itty";
  *obj.c_ref() = "bitty";
  *obj.d_ref() = "strings";
  return obj;
}();

TEST(Frozen, Tiny) {
  EXPECT_EQ(tiny4, freeze(tiny4).thaw());
  EXPECT_EQ(24, frozenSize(tiny4));
}

TEST(Frozen, SchemaSaving) {
  // calculate a layout
  Layout<EveryLayout> stressLayoutCalculated;
  CHECK(LayoutRoot::layout(stressValue2, stressLayoutCalculated));

  // save it
  schema::MemorySchema schemaSaved;
  saveRoot(stressLayoutCalculated, schemaSaved);

  // reload it
  Layout<EveryLayout> stressLayoutLoaded;
  loadRoot(stressLayoutLoaded, schemaSaved);

  // make sure the two layouts are identical (via printing)
  EXPECT_PRINTED_EQ(stressLayoutCalculated, stressLayoutLoaded);

  // make sure layouts round-trip
  schema::MemorySchema schemaLoaded;
  saveRoot(stressLayoutLoaded, schemaLoaded);
  EXPECT_EQ(schemaSaved, schemaLoaded);
}

TEST(Frozen, Enum) {
  Person1 he, she;
  *he.gender_ref() = Gender::Male;
  *she.gender_ref() = Gender::Female;
  EXPECT_EQ(he, freeze(he).thaw());
  EXPECT_EQ(she, freeze(she).thaw());
}

TEST(Frozen, EnumAsKey) {
  EnumAsKeyTest thriftObj;
  thriftObj.enumSet_ref()->insert(Gender::Male);
  thriftObj.enumMap_ref()->emplace(Gender::Female, 1219);
  thriftObj.outsideEnumSet_ref()->insert(Animal::DOG);
  thriftObj.outsideEnumMap_ref()->emplace(Animal::CAT, 7779);

  auto frozenObj = freeze(thriftObj);
  EXPECT_THAT(frozenObj.enumSet(), Contains(Gender::Male));
  EXPECT_THAT(frozenObj.outsideEnumSet(), Contains(Animal::DOG));
  EXPECT_EQ(frozenObj.enumMap().at(Gender::Female), 1219);
  EXPECT_EQ(frozenObj.outsideEnumMap().at(Animal::CAT), 7779);
}

template <class T>
size_t frozenBits(const T& value) {
  Layout<T> layout;
  LayoutRoot::layout(value, layout);
  return layout.bits;
}

TEST(Frozen, Bool) {
  Pet1 meat, vegan, dunno;
  meat.vegan_ref() = false;
  vegan.vegan_ref() = true;
  // Always-empty optionals take 0 bits.
  // Sometimes-full optionals take >=1 bits.
  // Always-false bools take 0 bits.
  // Sometimes-true bools take 1 bits.
  // dunno => Nothing => 0 bits.
  // meat => Just(False) => 1 bit.
  // vegan => Just(True) => 2 bits.
  EXPECT_LT(frozenBits(dunno), frozenBits(meat));
  EXPECT_LT(frozenBits(meat), frozenBits(vegan));
  EXPECT_FALSE(*freeze(meat).vegan());
  EXPECT_TRUE(*freeze(vegan).vegan());
  EXPECT_FALSE(freeze(dunno).vegan().has_value());
  EXPECT_EQ(meat, freeze(meat).thaw());
  EXPECT_EQ(vegan, freeze(vegan).thaw());
  EXPECT_EQ(dunno, freeze(dunno).thaw());
}

TEST(Frozen, ThawPart) {
  auto f = freeze(tom1);
  EXPECT_EQ(f.pets()[0].name(), "max");
  EXPECT_EQ(f.pets()[1].name(), "ed");

  auto max = f.pets()[0].thaw();
  auto ed = f.pets()[1].thaw();
  EXPECT_EQ(typeid(max), typeid(Pet1));
  EXPECT_EQ(typeid(ed), typeid(Pet1));
  EXPECT_EQ(*max.name_ref(), "max");
  EXPECT_EQ(*ed.name_ref(), "ed");
}

TEST(Frozen, SchemaConversion) {
  schema::MemorySchema memSchema;
  schema::Schema schema;

  Layout<EveryLayout> stressLayoutCalculated;
  CHECK(LayoutRoot::layout(stressValue2, stressLayoutCalculated));

  schema::MemorySchema schemaSaved;
  saveRoot(stressLayoutCalculated, schemaSaved);

  schema::convert(schemaSaved, schema);
  schema::convert(std::move(schema), memSchema);

  EXPECT_EQ(memSchema, schemaSaved);
}

TEST(Frozen, SparseSchema) {
  {
    auto l = layout(tiny1);
    schema::MemorySchema schema;
    saveRoot(l, schema);
    EXPECT_LE(schema.getLayouts().size(), 4);
  }
  {
    auto l = layout(tiny2);
    schema::MemorySchema schema;
    saveRoot(l, schema);
    EXPECT_LE(schema.getLayouts().size(), 7);
  }
}

TEST(Frozen, DedupedSchema) {
  {
    auto l = layout(tiny4);
    schema::MemorySchema schema;
    saveRoot(l, schema);
    EXPECT_LE(schema.getLayouts().size(), 7); // 13 layouts originally
  }
  {
    auto l = layout(stressValue2);
    schema::MemorySchema schema;
    saveRoot(l, schema);
    EXPECT_LE(schema.getLayouts().size(), 24); // 49 layouts originally
  }
}

TEST(Frozen, TypeHelpers) {
  auto f = freeze(tom1);
  View<Pet1> m = f.pets()[0];
  EXPECT_EQ(m.name(), "max");
}

TEST(Frozen, RangeTrivialRange) {
  auto data = std::vector<float>{3.0, 4.0, 5.0};
  auto view = freeze(data);
  auto r = folly::Range<const float*>(view.range());
  EXPECT_EQ(data, std::vector<float>(r.begin(), r.end()));
}

TEST(Frozen, PaddingLayout) {
  using std::pair;
  using std::vector;
  // The 'distance' field of the vector<double> is small and sensitive to
  // padding adjustments. If actual distances are returned in
  // layoutBytesDistance instead of worst-case distances, the below structure
  // will successfully freeze at offset zero but fail at later offsets.
  vector<vector<vector<double>>> test(10);
  test.push_back({{1.0}});
  size_t size;
  auto testLayout = layout(test, size);
  for (size_t offset = 0; offset < 8; ++offset) {
    std::unique_ptr<byte[]> store(new byte[size + offset + 16]);
    folly::MutableByteRange bytes(store.get() + offset, size + 16);

    auto view = ByteRangeFreezer::freeze(testLayout, test, bytes);
    auto range = view[10][0].range();
    EXPECT_EQ(range[0], 1.0);
    EXPECT_EQ(reinterpret_cast<intptr_t>(range.begin()) % alignof(double), 0);
  }
}

TEST(Frozen, Bundled) {
  using String = Bundled<std::string>;
  String s("Hello");

  EXPECT_EQ("Hello", s);
  EXPECT_FALSE(s.empty());
  EXPECT_EQ(nullptr, s.findFirstOfType<int>());

  s.hold(47);
  s.hold(11);

  EXPECT_EQ(47, *s.findFirstOfType<int>());
  EXPECT_EQ(nullptr, s.findFirstOfType<std::string>());
}
