/*
 * Copyright 2014 Facebook, Inc.
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
#include <thrift/lib/cpp2/frozen/test/gen-cpp/Example_types.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp/Example_layouts.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_types.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_layouts.h>
#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp/protocol/TDebugProtocol.h>
#include <thrift/lib/cpp/protocol/TCompactProtocol.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>

using namespace apache::thrift;
using namespace frozen;
using namespace util;

example2::EveryLayout stressValue = [] {
  example2::EveryLayout x;
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
size_t frozenSize(const T& v) {
  Layout<T> layout;
  return LayoutRoot::layout(v, layout);
}

template <class T>
Layout<T>&& layout(const T& x, Layout<T>&& layout = Layout<T>()) {
  size_t size = LayoutRoot::layout(x, layout);
  return std::move(layout);
}

auto tom1 = [] {
  example2::Pet1 max;
  max.name = "max";
  example2::Pet1 ed;
  ed.name = "ed";
  example2::Person1 tom;
  tom.name = "tom";
  tom.height = 1.82f;
  tom.age = 30;
  tom.__isset.age = true;
  tom.pets.push_back(max);
  tom.pets.push_back(ed);
  return tom;
}();
auto tom2 = [] {
  example2::Pet2 max;
  max.name = "max";
  example2::Pet2 ed;
  ed.name = "ed";
  example2::Person2 tom;
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
  for (int i = 0; i < tom1.pets.size(); ++i) {
    EXPECT_EQ(pets[i].name, fpets[i].name());
  }
  Layout<example2::Person1> layout;
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

TEST(Frozen, Versioning) {
  schema::Schema schema;
  Layout<example2::Person1> person1a;
  Layout<example2::Person1> person1b;
  Layout<example2::Person2> person2;

  size_t size = LayoutRoot::layout(tom1, person1a);
  person1a.saveRoot(schema);
  person1b.loadRoot(schema);
  EXPECT_THROW(person2.loadRoot(schema), LayoutTypeMismatchException);
  schema.relaxTypeChecks = true;
  person2.loadRoot(schema);

  std::string storage(size, 'X');
  folly::MutableStringPiece charRange(&storage.front(), size);
  folly::MutableByteRange bytes(charRange);

  ByteRangeFreezer::freeze(person1a, tom1, bytes);
  auto view1a = person1a.view({bytes.begin(), 0});
  auto view1b = person1b.view({bytes.begin(), 0});
  auto view2 = person2.view({bytes.begin(), 0});
  EXPECT_EQ(view1a.thaw(), view1b.thaw());
  EXPECT_EQ(view1a.name(), view2.name());
  EXPECT_EQ(view1a.age(), view2.age());
  EXPECT_EQ(view1a.pets()[0].name(), view2.pets()[0].name());
  EXPECT_EQ(view1a.pets()[1].name(), view2.pets()[1].name());
}

TEST(Frozen, Compatibility) {
  // Make sure Thrft2 Person1 works well with Thrift1 Person2
  schema::Schema schema;
  Layout<example2::Person1> person1cpp2;
  Layout<example1::Person2> person2cpp1;

  size_t size = LayoutRoot::layout(tom1, person1cpp2);

  person1cpp2.saveRoot(schema);
  EXPECT_THROW(person2cpp1.loadRoot(schema), LayoutTypeMismatchException);
  schema.relaxTypeChecks = true;
  person2cpp1.loadRoot(schema);

  std::string storage(size, 'X');
  folly::MutableStringPiece charRange(&storage.front(), size);
  folly::MutableByteRange bytes(charRange);

  ByteRangeFreezer::freeze(person1cpp2, tom1, bytes);
  auto view12 = person1cpp2.view({bytes.begin(), 0});
  auto view21 = person2cpp1.view({bytes.begin(), 0});
  EXPECT_EQ(view12.name(), view21.name());
  EXPECT_EQ(view12.age(), view21.age());
  EXPECT_EQ(view12.pets()[0].name(), view21.pets()[0].name());
  EXPECT_EQ(view12.pets()[1].name(), view21.pets()[1].name());
}

TEST(Frozen, EmbeddedSchema) {
  std::string storage;
  ThriftSerializerCompact<> serializer;
  {
    schema::Schema schema;

    Layout<example2::Person1> person1a;

    size_t size;
    size = LayoutRoot::layout(tom1, person1a);
    person1a.saveRoot(schema);

    serializer.serialize(schema, &storage);
    size_t start = storage.size();
    storage.resize(size + storage.size());

    folly::MutableStringPiece charRange(&storage[start], size);
    folly::MutableByteRange bytes(charRange);
    ByteRangeFreezer::freeze(person1a, tom1, bytes);
  }
  {
    schema::Schema schema;
    Layout<example2::Person2> person2;

    size_t start = serializer.deserialize(storage, &schema);
    EXPECT_THROW(person2.loadRoot(schema), LayoutTypeMismatchException);
    schema.relaxTypeChecks = true;
    person2.loadRoot(schema);

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
  EXPECT_EQ(example2::Person1(),
            Layout<example2::Person1>().view(null).thaw());
  EXPECT_EQ(example2::Pet1(), Layout<example2::Pet1>().view(null).thaw());
  EXPECT_EQ(std::set<int>(), Layout<std::set<int>>().view(null).thaw());
  EXPECT_EQ((std::map<int, int>()),
            (Layout<std::map<int, int>>().view(null).thaw()));
}

TEST(Frozen, String) {
  std::string str = "Hello";
  auto fstr = freeze(str);
  EXPECT_EQ(str, folly::StringPiece(fstr));
  EXPECT_EQ(std::string(), folly::StringPiece(freeze(std::string())));
}

TEST(Frozen, VectorString) {
  std::vector<std::string> strs{"hello", "sara"};
  LOG(INFO) << layout(strs);
  auto fstrs = freeze(strs);
  EXPECT_EQ(strs[0], fstrs[0]);
  EXPECT_EQ(strs[1], fstrs[1]);
  EXPECT_EQ(strs.size(), fstrs.size());
  std::vector<std::string> check;
}

TEST(Frozen, VectorVectorInt) {
  std::vector<std::vector<int>> vvi{{2, 3, 5, 7}, {11, 13, 17, 19}};
  auto fvvi = freeze(vvi);
  auto tvvi = fvvi.thaw();
  EXPECT_EQ(tvvi, vvi);
}

TEST(Frozen, BigMap) {
  example2::PlaceTest t;
  for (int i = 0; i < 1000; ++i) {
    auto& place = t.places[i * i * i % 757368944];
    place.name = folly::to<std::string>(i);
    for (int i = 0; i < 200; ++i) {
      ++place.popularityByHour[rand() % (24 * 7)];
    }
  }
  folly::IOBufQueue bq(folly::IOBufQueue::cacheChainLength());
  CompactSerializer::serialize(t, &bq);
  auto compactSize = bq.chainLength();
  auto frozenSize = ::frozenSize(t);
  EXPECT_EQ(t, freeze(t)->thaw());
  EXPECT_LT(frozenSize, compactSize * 0.7);
}

TEST(Frozen, Tiny) {
  example2::Tiny obj;
  obj.a = "four";
  obj.b = "itty";
  obj.c = "bitty";
  obj.d = "strings";
  EXPECT_EQ(obj, freeze(obj).thaw());
  EXPECT_EQ(24, frozenSize(obj));
}

template <class T>
std::string toString(const T& x) {
  std::ostringstream xStr;
  xStr << x;
  return xStr.str();
}

#define EXPECT_PRINTED_EQ(a, b)                                           \
  {                                                                       \
    auto aStr = toString(a), bStr = toString(b);                          \
    EXPECT_TRUE(aStr == bStr) << "\n" << #a << ": " << aStr << "\n" << #b \
                              << ": " << bStr;                            \
  }

TEST(Frozen, SchemaSaving) {
  // calculate a layout
  Layout<example2::EveryLayout> stressLayoutCalculated;
  CHECK(LayoutRoot::layout(stressValue, stressLayoutCalculated));

  // save it
  schema::Schema schemaSaved;
  stressLayoutCalculated.saveRoot(schemaSaved);

  // reload it
  Layout<example2::EveryLayout> stressLayoutLoaded;
  stressLayoutLoaded.loadRoot(schemaSaved);

  // make sure the two layouts are identical (via printing)
  EXPECT_PRINTED_EQ(stressLayoutCalculated, stressLayoutLoaded);

  // make sure layouts round-trip
  schema::Schema schemaLoaded;
  stressLayoutLoaded.saveRoot(schemaLoaded);
  EXPECT_EQ(schemaSaved, schemaLoaded);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
