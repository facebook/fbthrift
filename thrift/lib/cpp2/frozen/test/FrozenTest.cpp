/*
 * Copyright 2014-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/frozen/FrozenUtil.h>
#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_layouts.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_types_custom_protocol.h>

using namespace apache::thrift;
using namespace apache::thrift::frozen;
using namespace apache::thrift::test;
using namespace apache::thrift::util;

EveryLayout stressValue2 = [] {
  EveryLayout x;
  x.aBool = true;
  x.aInt = 2;
  x.aList = {3, 5};
  x.aSet = {7, 11};
  x.aHashSet = {13, 17};
  x.aMap = {{19, 23}, {29, 31}};
  x.aHashMap = {{37, 41}, {43, 47}};
  x.optInt = 53;
  x.__isset.optInt = true;
  x.aFloat = 59.61;
  x.optMap = {{2, 4}, {3, 9}};
  x.__isset.optMap = true;
  return x;
}();

template <class T>
Layout<T>&& layout(const T& x, Layout<T>&& layout = Layout<T>()) {
  size_t size = LayoutRoot::layout(x, layout);
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
  max.name = "max";
  Pet1 ed;
  ed.name = "ed";
  Person1 tom;
  tom.name = "tom";
  tom.height = 1.82f;
  tom.age = 30;
  tom.__isset.age = true;
  tom.pets.push_back(max);
  tom.pets.push_back(ed);
  return tom;
}();
auto tom2 = [] {
  Pet2 max;
  max.name = "max";
  Pet2 ed;
  ed.name = "ed";
  Person2 tom;
  tom.name = "tom";
  tom.weight = 169;
  tom.age = 30;
  tom.__isset.age = true;
  tom.pets.push_back(max);
  tom.pets.push_back(ed);
  return tom;
}();

TEST(Frozen, EndToEnd) {
  auto view = freeze(tom1);
  EXPECT_EQ(tom1.name, view.name());
  ASSERT_TRUE(view.age().hasValue());
  EXPECT_EQ(tom1.age, view.age().value());
  EXPECT_EQ(tom1.height, view.height());
  EXPECT_EQ(view.pets()[0].name(), tom1.pets[0].name);
  auto& pets = tom1.pets;
  auto fpets = view.pets();
  ASSERT_EQ(pets.size(), fpets.size());
  for (size_t i = 0; i < tom1.pets.size(); ++i) {
    EXPECT_EQ(pets[i].name, fpets[i].name());
  }
  Layout<Person1> layout;
  LayoutRoot::layout(tom1, layout);
  LOG(INFO) << layout;
  LOG(INFO) << apache::thrift::debugString(tom1);
  auto ttom = view.thaw();
  LOG(INFO) << apache::thrift::debugString(ttom);
  EXPECT_TRUE(ttom.__isset.name);
  EXPECT_EQ(tom1, ttom);
}

TEST(Frozen, Comparison) {
  EXPECT_EQ(frozenSize(tom1), frozenSize(tom2));
  auto view1 = freeze(tom1);
  auto view2 = freeze(tom2);
  // name is optional now, and was __isset=true, so we don't write it
  ASSERT_TRUE(view2.age().hasValue());
  EXPECT_EQ(tom2.age, view2.age().value());
  EXPECT_EQ(tom2.name, view2.name());
  EXPECT_EQ(tom2.name.size(), view2.name().size());
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
    EXPECT_EQ(tom1.name, view.name());
    ASSERT_EQ(tom1.__isset.age, view.age().hasValue());
    EXPECT_EQ(tom1.age, view.age().value());
    EXPECT_EQ(tom1.pets[0].name, view.pets()[0].name());
    EXPECT_EQ(tom1.pets[1].name, view.pets()[1].name());
  }
}

TEST(Frozen, NoLayout) {
  ViewPosition null{nullptr, 0};

  EXPECT_FALSE(Layout<bool>().view(null));
  EXPECT_EQ(0, Layout<int>().view(null));
  EXPECT_EQ(0.0f, Layout<float>().view(null));
  EXPECT_EQ(folly::Optional<int>(), Layout<folly::Optional<int>>().view(null));
  EXPECT_EQ(std::string(), Layout<std::string>().view(null));
  EXPECT_EQ(std::vector<int>(), Layout<std::vector<int>>().view(null).thaw());
  EXPECT_EQ(Person1(), Layout<Person1>().view(null).thaw());
  EXPECT_EQ(Pet1(), Layout<Pet1>().view(null).thaw());
  EXPECT_EQ(std::set<int>(), Layout<std::set<int>>().view(null).thaw());
  EXPECT_EQ((std::map<int, int>()),
            (Layout<std::map<int, int>>().view(null).thaw()));

  Layout<Person1> emptyPersonLayout;
  std::array<uint8_t, 100> storage;
  folly::MutableByteRange bytes(storage.begin(), storage.end());
  EXPECT_THROW(
      ByteRangeFreezer::freeze(emptyPersonLayout, tom1, bytes),
      LayoutException);
}

template<class T>
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
      ByteRangeFreezer::freeze(minLayout, value, bytes),
      LayoutException);
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
    auto& place = t.places[i * i * i % 757368944];
    place.name = folly::to<std::string>(i);
    for (int j = 0; j < 200; ++j) {
      ++place.popularityByHour[rand() % (24 * 7)];
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
  obj.b = "set";
  return obj;
}();
Tiny tiny4 = [] {
  Tiny obj;
  obj.a = "four";
  obj.b = "itty";
  obj.c = "bitty";
  obj.d = "strings";
  return obj;
}();

TEST(Frozen, Tiny) {
  EXPECT_EQ(tiny4, freeze(tiny4).thaw());
  EXPECT_EQ(24, frozenSize(tiny4));
}

template <class T>
std::string toString(const T& x) {
  std::ostringstream xStr;
  xStr << x;
  return xStr.str();
}

#define EXPECT_PRINTED_EQ(a, b) EXPECT_EQ(toString(a), toString(b))

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
  he.gender = Gender::Male;
  she.gender = Gender::Female;
  EXPECT_EQ(he, freeze(he).thaw());
  EXPECT_EQ(she, freeze(she).thaw());
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
  using std::vector;
  using std::pair;
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
