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
#include <thrift/lib/cpp2/frozen/FrozenUtil.h>
#include <thrift/lib/cpp2/frozen/FrozenTestUtil.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>

using namespace apache::thrift;
using namespace frozen;
using namespace util;

TEST(FrozenUtil, FreezeAndUse) {
  auto file = freezeToTempFile(std::string("hello"));
  auto view = mapFrozen<std::string>(folly::File(file.fd()));
  EXPECT_EQ(folly::StringPiece(view), "hello");
}

TEST(FrozenUtil, FreezeAndMap) {
  auto original = std::vector<std::string>{"hello", "world"};
  folly::test::TemporaryFile tmp;

  freezeToFile(original, folly::File(tmp.fd()));
  auto mapped = mapFrozen<std::vector<std::string>>(folly::File(tmp.fd()));

  auto thawed = mapped.thaw();
  EXPECT_EQ(original, thawed);
  original.push_back("different");
  EXPECT_NE(original, thawed);
}

TEST(FrozenUtil, FutureVersion) {
  folly::test::TemporaryFile tmp;

  {
    schema::Schema schema;
    schema.fileVersion = 1000;
    schema.__isset.fileVersion = true;

    std::string schemaStr;
    util::ThriftSerializerCompact<>().serialize(schema, &schemaStr);
    write(tmp.fd(), schemaStr.data(), schemaStr.size());
  }

  EXPECT_THROW(mapFrozen<std::string>(folly::File(tmp.fd())),
               FrozenFileForwardIncompatible);
}

TEST(FrozenUtil, FileSize) {
  auto original = std::vector<std::string>{"hello", "world"};
  folly::test::TemporaryFile tmp;
  freezeToFile(original, folly::File(tmp.fd()));
  struct stat stats;
  fstat(tmp.fd(), &stats);
  EXPECT_LT(stats.st_size, 500); // most of this is the schema
}

TEST(FrozenUtil, FreezeToString) {
  // multiplication tables for first three primes
  std::map<int, std::map<int, int>> m{
      {2, {{2, 4}, {3, 6}, {5, 10}}},
      {3, {{2, 6}, {3, 9}, {5, 15}}},
      {5, {{2, 10}, {3, 15}, {5, 25}}},
  };
  // In this example, the schema is 101 bytes and the data is only 17 bytes!
  std::string store;
  freezeToString(m, store);
  auto frozen = mapFrozen<std::map<int, std::map<int, int>>>(store);
  EXPECT_EQ(frozen.at(3).at(5), 15);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
