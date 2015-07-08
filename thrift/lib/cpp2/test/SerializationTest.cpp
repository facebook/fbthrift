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

#include <memory>
#include <folly/Format.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;

TestStruct makeTestStruct() {
  TestStruct s;
  s.s = "test";
  s.i = 48;
  return s;
}

TEST(SerializationTest, CompactSerializerRoundtripPasses) {
  auto s = makeTestStruct();

  folly::IOBufQueue q;
  CompactSerializer::serialize(s, &q);

  TestStruct out;
  CompactSerializer::deserialize(q.front(), out);

  EXPECT_EQ(out, s);
}

TEST(SerializationTest, BinarySerializerRoundtripPasses) {
  auto s = makeTestStruct();

  folly::IOBufQueue q;
  BinarySerializer::serialize(s, &q);

  TestStruct out;
  BinarySerializer::deserialize(q.front(), out);

  EXPECT_EQ(out, s);
}

TEST(SerializationTest, SimpleJSONSerializerRoundtripPasses) {
  auto s = makeTestStruct();

  folly::IOBufQueue q;
  SimpleJSONSerializer::serialize(s, &q);

  TestStruct out;
  SimpleJSONSerializer::deserialize(q.front(), out);

  EXPECT_EQ(out, s);
}

TEST(SerializationTest, MixedRoundtripFails) {
  auto s = makeTestStruct();

  folly::IOBufQueue q;
  CompactSerializer::serialize(s, &q);

  try {
    TestStruct out;
    BinarySerializer::deserialize(q.front(), out);
    FAIL();
  } catch (...) {
    // Should underflow
  }
}

TestStructRecursive makeTestStructRecursive(size_t levels) {
  unique_ptr<TestStructRecursive> s;
  for (size_t i = levels; i > 0; --i) {
    auto t = make_unique<TestStructRecursive>();
    t->tag = sformat("level-{}", i);
    t->cdr = std::move(s);
    s = std::move(t);
  }
  TestStructRecursive ret;
  ret.tag = "level-0";
  ret.cdr = std::move(s);
  return ret;
}

size_t getRecDepth(const TestStructRecursive& s) {
  auto p = &s;
  size_t depth = 0;
  while ((p = p->cdr.get())) {
    ++depth;
  }
  return depth;
}

TEST(SerializationTest, RecursiveNoDepthCompactSerializerRoundtripPasses) {
  auto s = makeTestStructRecursive(0);

  folly::IOBufQueue q;
  CompactSerializer::serialize(s, &q);

  TestStructRecursive out;
  CompactSerializer::deserialize(q.front(), out);

  EXPECT_EQ(s, out);
}

TEST(SerializationTest, RecursiveDeepCompactSerializerRoundtripPasses) {
  auto s = makeTestStructRecursive(6);
  EXPECT_EQ(6, getRecDepth(s));

  folly::IOBufQueue q;
  CompactSerializer::serialize(s, &q);

  TestStructRecursive out;
  CompactSerializer::deserialize(q.front(), out);

  EXPECT_EQ(s, out);
}

TEST(SerializationTest, RecursiveNoDepthBinarySerializerRoundtripPasses) {
  auto s = makeTestStructRecursive(0);

  folly::IOBufQueue q;
  BinarySerializer::serialize(s, &q);

  TestStructRecursive out;
  BinarySerializer::deserialize(q.front(), out);

  EXPECT_EQ(s, out);
}

TEST(SerializationTest, RecursiveDeepBinarySerializerRoundtripPasses) {
  auto s = makeTestStructRecursive(6);
  EXPECT_EQ(6, getRecDepth(s));

  folly::IOBufQueue q;
  BinarySerializer::serialize(s, &q);

  TestStructRecursive out;
  BinarySerializer::deserialize(q.front(), out);

  EXPECT_EQ(s, out);
}

TEST(SerializationTest, RecursiveNoDepthSimpleJSONSerializerRoundtripPasses) {
  auto s = makeTestStructRecursive(0);

  folly::IOBufQueue q;
  SimpleJSONSerializer::serialize(s, &q);

  TestStructRecursive out;
  SimpleJSONSerializer::deserialize(q.front(), out);

  EXPECT_EQ(s, out);
}

TEST(SerializationTest, RecursiveDeepSimpleJSONSerializerRoundtripPasses) {
  auto s = makeTestStructRecursive(6);
  EXPECT_EQ(6, getRecDepth(s));

  folly::IOBufQueue q;
  SimpleJSONSerializer::serialize(s, &q);

  TestStructRecursive out;
  SimpleJSONSerializer::deserialize(q.front(), out);

  EXPECT_EQ(s, out);
}

TEST(SerializationTest, StringOverloads) {
  auto s = makeTestStruct();

  std::string str;
  CompactSerializer::serialize(s, &str);

  {
    TestStruct out;
    CompactSerializer::deserialize(str, out);
    EXPECT_EQ(out, s);
  }
}

namespace {
// Large enough to ensure externally allocated buffer and to make sure
// IOBufQueue::insert doesn't copy instead of linking
constexpr size_t kBufSize = 8192;

template <class Serializer>
void testIOBufSharingUnmanagedBuffer() {
  char tmp[kBufSize];
  memcpy(tmp, "hello", 5);
  TestStructIOBuf s;
  s.buf = folly::IOBuf(folly::IOBuf::WRAP_BUFFER, tmp, sizeof(tmp));

  for (unsigned int i = 0; i < 4; ++i) {
    ExternalBufferSharing serializationSharing =
      i & 1 ? SHARE_EXTERNAL_BUFFER : COPY_EXTERNAL_BUFFER;
    ExternalBufferSharing deserializationSharing =
      i & 2 ? SHARE_EXTERNAL_BUFFER : COPY_EXTERNAL_BUFFER;

    folly::IOBufQueue q;
    Serializer::serialize(s, &q, serializationSharing);

    TestStructIOBuf s2;
    Serializer::deserialize(q.front(), s2, deserializationSharing);

    size_t size = 0;
    for (auto& br : s2.buf) {
      if (br.empty()) {
        continue;
      }
      if (i == 3) {
        // Expect only one non-empty buffer, which must be ours
        EXPECT_EQ(size, 0);
        EXPECT_EQ(s.buf.data(), br.data());
        EXPECT_EQ(s.buf.length(), br.size());
      } else {
        EXPECT_NE(s.buf.data(), br.data());
      }
      size += br.size();
    }
    EXPECT_EQ(s.buf.length(), size);
    s2.buf.coalesce();
    EXPECT_EQ(0, memcmp(s2.buf.data(), s.buf.data(), s.buf.length()));
  }
}

template <class Serializer>
void testIOBufSharingManagedBuffer() {
  TestStructIOBuf s;
  s.buf = folly::IOBuf(folly::IOBuf::CREATE, kBufSize);
  memcpy(s.buf.writableTail(), "hello", 5);
  s.buf.append(kBufSize);

  for (unsigned int i = 0; i < 4; ++i) {
    ExternalBufferSharing serializationSharing =
      i & 1 ? SHARE_EXTERNAL_BUFFER : COPY_EXTERNAL_BUFFER;
    ExternalBufferSharing deserializationSharing =
      i & 2 ? SHARE_EXTERNAL_BUFFER : COPY_EXTERNAL_BUFFER;

    folly::IOBufQueue q;
    Serializer::serialize(s, &q, serializationSharing);

    TestStructIOBuf s2;
    Serializer::deserialize(q.front(), s2, deserializationSharing);

    size_t size = 0;
    for (auto& br : s2.buf) {
      if (br.empty()) {
        continue;
      }
      // Expect only one non-empty buffer, which must be ours
      EXPECT_EQ(size, 0);
      EXPECT_EQ(s.buf.data(), br.data());
      EXPECT_EQ(s.buf.length(), br.size());
      size += br.size();
    }
  }
}

}  // namespace

TEST(SerializationTest, CompactSerializerIOBufSharingUnmanagedBuffer) {
  testIOBufSharingUnmanagedBuffer<CompactSerializer>();
}

TEST(SerializationTest, BinarySerializerIOBufSharingUnmanagedBuffer) {
  testIOBufSharingUnmanagedBuffer<BinarySerializer>();
}

TEST(SerializationTest, CompactSerializerIOBufSharingManagedBuffer) {
  testIOBufSharingManagedBuffer<CompactSerializer>();
}

TEST(SerializationTest, BinarySerializerIOBufSharingManagedBuffer) {
  testIOBufSharingManagedBuffer<BinarySerializer>();
}
