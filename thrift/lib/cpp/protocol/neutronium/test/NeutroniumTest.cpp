/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "thrift/lib/cpp/protocol/neutronium/test/gen-cpp/neutronium_test_types.h"
#include "thrift/lib/cpp/protocol/neutronium/Schema.h"
#include "thrift/lib/cpp/protocol/neutronium/Encoder.h"
#include "thrift/lib/cpp/protocol/neutronium/Decoder.h"

#include <limits>
#include <random>
#include <gtest/gtest.h>
#include "external/gflags/gflags.h"
#include "folly/Random.h"
#include "common/fbunit/OldFollyBenchmark.h"

#include "thrift/lib/cpp/protocol/TNeutroniumProtocol.h"
#include "thrift/lib/cpp/util/ThriftSerializer.h"

DECLARE_bool(benchmark);

using namespace apache;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::protocol::neutronium;
using namespace apache::thrift::protocol::neutronium::test;
using namespace apache::thrift::util;

namespace {
reflection::Schema reflectionSchema;
Schema schema;
}  // namespace

TEST(Neutronium, SchemaUtils) {
  EXPECT_EQ(T_BOOL, toTType(reflection::TYPE_BOOL));
  EXPECT_EQ(T_I08, toTType(reflection::TYPE_BYTE));
  EXPECT_EQ(T_I16, toTType(reflection::TYPE_I16));
  EXPECT_EQ(T_I32, toTType(reflection::TYPE_I32));
  EXPECT_EQ(T_I64, toTType(reflection::TYPE_I64));
  EXPECT_EQ(T_DOUBLE, toTType(reflection::TYPE_DOUBLE));
  EXPECT_EQ(T_STRING, toTType(reflection::TYPE_STRING));
  EXPECT_EQ(T_STRUCT, toTType(reflection::TYPE_STRUCT));
  EXPECT_EQ(T_MAP, toTType(reflection::TYPE_MAP));
  EXPECT_EQ(T_SET, toTType(reflection::TYPE_SET));
  EXPECT_EQ(T_LIST, toTType(reflection::TYPE_LIST));
}

TEST(Neutronium, BitUtils) {
  EXPECT_EQ(0, byteCount(0));
  EXPECT_EQ(1, byteCount(1));
  EXPECT_EQ(1, byteCount(8));
  EXPECT_EQ(2, byteCount(9));
  EXPECT_EQ(256, byteCount(2048));
  EXPECT_EQ(257, byteCount(2049));

  EXPECT_EQ(4, byteIndex(39));
  EXPECT_EQ(7, bitOffset(39));
  EXPECT_EQ(5, byteIndex(40));
  EXPECT_EQ(0, bitOffset(40));

  uint8_t buf[256];
  memset(buf, 0, 256);

  setBit(buf, 36);
  setBit(buf, 39);
  EXPECT_EQ((1 << 7) | (1 << 4), buf[4]);
  EXPECT_EQ(0, buf[5]);
  clearBit(buf, 39);
  EXPECT_EQ(1 << 4, buf[4]);
  EXPECT_EQ(0, buf[5]);
  setBit(buf, 40);
  EXPECT_EQ(1 << 4, buf[4]);
  EXPECT_EQ(1, buf[5]);

  memset(buf, 0, 256);

  setBits(buf, 36, 27, 0x2bcdef1);
  EXPECT_EQ(0, buf[3]);
  EXPECT_EQ(0x10, buf[4]);
  EXPECT_EQ(0xef, buf[5]);
  EXPECT_EQ(0xcd, buf[6]);
  EXPECT_EQ(0x2b, buf[7]);
  EXPECT_EQ(0, buf[8]);
  EXPECT_EQ(0x2bcdef1, getBits(buf, 36, 28));
}


TEST(Neutronium, SchemaFixedSize) {
  {
    uint64_t t = TestFixedSizeStruct1::_reflection_id;
    auto& dt = schema.map().at(t);
    EXPECT_EQ(2, dt.fixedSize);
    EXPECT_TRUE(dt.optionalFields.empty());
  }

  {
    uint64_t t = TestFixedSizeStruct2::_reflection_id;
    auto& dt = schema.map().at(t);
    EXPECT_EQ(12, dt.fixedSize);
    EXPECT_TRUE(dt.optionalFields.empty());
  }

  {
    uint64_t t = TestNotFixedSizeStruct2::_reflection_id;
    auto& dt = schema.map().at(t);
    EXPECT_EQ(-1, dt.fixedSize);
    EXPECT_TRUE(dt.optionalFields.empty());
  }
}

TEST(Neutronium, SchemaStruct1) {
  uint64_t t1 = TestStruct1::_reflection_id;
  auto& dt = schema.map().at(t1);
  EXPECT_EQ(7, dt.fields.size());
  EXPECT_EQ(reflection::TYPE_BOOL, dt.fields.at(1).type);
  EXPECT_TRUE(dt.fields.at(1).isRequired);
  EXPECT_EQ(reflection::TYPE_BOOL, dt.fields.at(2).type);
  EXPECT_FALSE(dt.fields.at(2).isRequired);
  EXPECT_EQ(reflection::TYPE_I32, dt.fields.at(3).type);
  EXPECT_TRUE(dt.fields.at(3).isRequired);
  EXPECT_EQ(reflection::TYPE_I32, dt.fields.at(4).type);
  EXPECT_FALSE(dt.fields.at(4).isRequired);
  EXPECT_EQ(reflection::TYPE_I64, dt.fields.at(5).type);
  EXPECT_TRUE(dt.fields.at(5).isRequired);
  EXPECT_EQ(reflection::TYPE_I64, dt.fields.at(6).type);
  EXPECT_FALSE(dt.fields.at(6).isRequired);
  EXPECT_EQ(reflection::TYPE_STRING, dt.fields.at(7).type);
  EXPECT_TRUE(dt.fields.at(7).isRequired);
}

TEST(Neutronium, SchemaStruct2) {
  uint64_t t1 = TestStruct1::_reflection_id;
  uint64_t t2 = TestStruct2::_reflection_id;
  auto& dt = schema.map().at(t2);
  EXPECT_EQ(6, dt.fields.size());
  EXPECT_EQ(t1, dt.fields.at(2).type);
}

TEST(Neutronium, EncodingDecoding1) {
  uint64_t t = TestStruct1::_reflection_id;

  std::unique_ptr<folly::IOBuf> buf = folly::IOBuf::create(0);

  {
    // a, c, e, g are required
    Encoder enc(&schema, nullptr, buf.get());
    enc.setRootType(t);
    enc.writeStructBegin("TestStruct1");

    // 3: i32 c
    enc.writeFieldBegin("c", T_I32, 3);
    enc.writeI32(43);
    enc.writeFieldEnd();

    // 5: i64 e
    enc.writeFieldBegin("e", T_I64, 5);
    enc.writeI64((static_cast<int64_t>(10) << 32) + 12345);
    enc.writeFieldEnd();

    // 2: optional bool b
    enc.writeFieldBegin("b", T_BOOL, 2);
    enc.writeBool(false);
    enc.writeFieldEnd();

    // 7: string g
    enc.writeFieldBegin("g", T_STRING, 7);
    enc.writeString("hello world");
    enc.writeFieldEnd();

    // 1: bool a
    enc.writeFieldBegin("a", T_BOOL, 1);
    enc.writeBool(true);
    enc.writeFieldEnd();

    enc.writeFieldStop();
    enc.writeStructEnd();

    EXPECT_EQ(buf->computeChainDataLength(), enc.bytesWritten());
  }

  buf->coalesce();

  {
    Decoder dec(&schema, nullptr, buf.get());
    dec.setRootType(t);

    dec.readStructBegin();

    TType type;
    int16_t tag;
    int32_t i32;
    int64_t i64;
    bool b;
    std::string s;

    // ints in tag order (variable size before fixed size):
    // 3: i32 c
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_I32, type);
    EXPECT_EQ(3, tag);
    dec.readI32(i32);
    EXPECT_EQ(43, i32);
    dec.readFieldEnd();

    // int64s in tag order:
    // 5: i64 e
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_I64, type);
    EXPECT_EQ(5, tag);
    dec.readI64(i64);
    EXPECT_EQ((static_cast<int64_t>(10) << 32) + 12345, i64);
    dec.readFieldEnd();

    // bytes in tag order
    // <none>

    // bools in tag order
    // 1: bool a
    // 2: optional bool b
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_BOOL, type);
    EXPECT_EQ(1, tag);
    dec.readBool(b);
    EXPECT_TRUE(b);
    dec.readFieldEnd();

    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_BOOL, type);
    EXPECT_EQ(2, tag);
    dec.readBool(b);
    EXPECT_FALSE(b);
    dec.readFieldEnd();

    // strings in tag order
    // 7: string g
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_STRING, type);
    EXPECT_EQ(7, tag);
    dec.readString(s);
    EXPECT_EQ(11, s.size());
    EXPECT_EQ("hello world", s);
    dec.readFieldEnd();

    // End
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_STOP, type);

    dec.readStructEnd();

    EXPECT_EQ(buf->computeChainDataLength(), dec.bytesRead());
  }
}

TEST(Neutronium, EncodingDecoding2) {
  uint64_t t = TestStruct2::_reflection_id;

  std::unique_ptr<folly::IOBuf> buf = folly::IOBuf::create(0);

  {
    // all fields (a, b, c, d, e) are required
    Encoder enc(&schema, nullptr, buf.get());
    enc.setRootType(t);
    enc.writeStructBegin("TestStruct2");

    // 3: string c
    enc.writeFieldBegin("c", T_STRING, 3);
    enc.writeString("hello");
    enc.writeFieldEnd();

    // 1: i32 a
    enc.writeFieldBegin("a", T_I32, 1);
    enc.writeI32(42);
    enc.writeFieldEnd();

    // 4: list<i32> d
    enc.writeFieldBegin("d", T_LIST, 4);
    enc.writeListBegin(T_I32, 6);
    enc.writeI32(10);
    enc.writeI32(1000);
    enc.writeI32(100);
    enc.writeI32(-1);
    enc.writeI32(std::numeric_limits<int32_t>::min());
    enc.writeI32(std::numeric_limits<int32_t>::max());
    enc.writeListEnd();
    enc.writeFieldEnd();

    // 2: TestStruct1 b
    enc.writeFieldBegin("b", T_STRUCT, 2);
    {
      // a, c, e, g are required
      enc.writeStructBegin("TestStruct1");

      // 1. bool a
      enc.writeFieldBegin("a", T_BOOL, 1);
      enc.writeBool(true);
      enc.writeFieldEnd();

      // 3: i32 c
      enc.writeFieldBegin("c", T_I32, 3);
      enc.writeI32(23);
      enc.writeFieldEnd();

      // 5: i64 e
      enc.writeFieldBegin("e", T_I64, 5);
      enc.writeI64(0);
      enc.writeFieldEnd();

      // 7: string g
      enc.writeFieldBegin("g", T_STRING, 7);
      enc.writeString("world");
      enc.writeFieldEnd();

      enc.writeFieldStop();
      enc.writeStructEnd();
    }
    enc.writeFieldEnd();

    // 5: list<string> e
    enc.writeFieldBegin("e", T_LIST, 5);
    enc.writeListBegin(T_STRING, 3);
    enc.writeString("first");
    enc.writeString("second");
    enc.writeString("third");
    enc.writeListEnd();
    enc.writeFieldEnd();

    // 6: map<i32, string> f
    enc.writeFieldBegin("f", T_MAP, 6);
    enc.writeMapBegin(T_I32, T_STRING, 2);
    enc.writeI32(23);
    enc.writeString("hello");
    enc.writeI32(42);
    enc.writeString("world");
    enc.writeMapEnd();
    enc.writeFieldEnd();

    enc.writeFieldStop();
    enc.writeStructEnd();

    EXPECT_EQ(buf->computeChainDataLength(), enc.bytesWritten());
  }

  buf->coalesce();

  {
    Decoder dec(&schema, nullptr, buf.get());
    dec.setRootType(t);

    dec.readStructBegin();

    TType type;
    TType type2;
    int16_t tag;
    int32_t i32;
    int64_t i64;
    uint32_t count;
    bool countUnknown;
    bool b;
    std::string s;

    // ints in tag order:
    // 1: i32 a
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_I32, type);
    EXPECT_EQ(1, tag);
    dec.readI32(i32);
    EXPECT_EQ(42, i32);
    dec.readFieldEnd();

    // int64s in tag order:
    // <none>

    // bytes in tag order:
    // <none>

    // bools in tag order:
    // <none>

    // strings and children in tag order:
    // 2: TestStruct1 b
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_STRUCT, type);
    EXPECT_EQ(2, tag);
    {
      dec.readStructBegin();

      // ints in tag order:
      // 3: i32 c
      dec.readFieldBegin(type, tag);
      EXPECT_EQ(T_I32, type);
      EXPECT_EQ(3, tag);
      dec.readI32(i32);
      EXPECT_EQ(23, i32);
      dec.readFieldEnd();

      // int64s in tag order:
      // 5: i64 e
      dec.readFieldBegin(type, tag);
      EXPECT_EQ(T_I64, type);
      EXPECT_EQ(5, tag);
      dec.readI64(i64);
      EXPECT_EQ(0, i64);
      dec.readFieldEnd();

      // bytes in tag order:
      // <none>

      // bools in tag order:
      // 1: bool a
      dec.readFieldBegin(type, tag);
      EXPECT_EQ(T_BOOL, type);
      EXPECT_EQ(1, tag);
      dec.readBool(b);
      EXPECT_TRUE(b);
      dec.readFieldEnd();

      // strings and children in tag order:
      // 7: string g
      dec.readFieldBegin(type, tag);
      EXPECT_EQ(T_STRING, type);
      EXPECT_EQ(7, tag);
      dec.readString(s);
      EXPECT_EQ(5, s.size());
      EXPECT_EQ("world", s);
      dec.readFieldEnd();

      // End
      dec.readFieldBegin(type, tag);
      EXPECT_EQ(T_STOP, type);

      dec.readStructEnd();
    }
    dec.readFieldEnd();

    // 3: string c
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_STRING, type);
    EXPECT_EQ(3, tag);
    dec.readString(s);
    EXPECT_EQ(5, s.size());
    EXPECT_EQ("hello", s);
    dec.readFieldEnd();

    // 4: list<i32> d
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_LIST, type);
    EXPECT_EQ(4, tag);
    {
      dec.readListBegin(type, count, countUnknown);
      EXPECT_EQ(T_I32, type);
      EXPECT_EQ(6, count);
      EXPECT_EQ(countUnknown, false);

      dec.readI32(i32);
      EXPECT_EQ(10, i32);
      dec.readI32(i32);
      EXPECT_EQ(1000, i32);
      dec.readI32(i32);
      EXPECT_EQ(100, i32);
      dec.readI32(i32);
      EXPECT_EQ(-1, i32);
      dec.readI32(i32);
      EXPECT_EQ(std::numeric_limits<int32_t>::min(), i32);
      dec.readI32(i32);
      EXPECT_EQ(std::numeric_limits<int32_t>::max(), i32);

      dec.readListEnd();
    }
    dec.readFieldEnd();

    // 5: list<string> e
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_LIST, type);
    EXPECT_EQ(5, tag);
    {
      dec.readListBegin(type, count, countUnknown);
      EXPECT_EQ(T_STRING, type);
      EXPECT_EQ(3, count);
      EXPECT_EQ(countUnknown, false);

      dec.readString(s);
      EXPECT_EQ("first", s);
      dec.readString(s);
      EXPECT_EQ("second", s);
      dec.readString(s);
      EXPECT_EQ("third", s);

      dec.readListEnd();
    }
    dec.readFieldEnd();

    // 6. map<i32, string> f
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_MAP, type);
    EXPECT_EQ(6, tag);
    {
      dec.readMapBegin(type, type2, count, countUnknown);
      EXPECT_EQ(T_I32, type);
      EXPECT_EQ(T_STRING, type2);
      EXPECT_EQ(2, count);
      EXPECT_EQ(countUnknown, false);

      dec.readI32(i32);
      EXPECT_EQ(23, i32);
      dec.readString(s);
      EXPECT_EQ("hello", s);
      dec.readI32(i32);
      EXPECT_EQ(42, i32);
      dec.readString(s);
      EXPECT_EQ("world", s);

      dec.readMapEnd();
    }
    dec.readFieldEnd();

    // End
    dec.readFieldBegin(type, tag);
    EXPECT_EQ(T_STOP, type);

    dec.readStructEnd();

    EXPECT_EQ(buf->computeChainDataLength(), dec.bytesRead());
  }
}

TEST(Neutronium, StringEncoding) {
  InternTable itable;

  std::unique_ptr<folly::IOBuf> buf = folly::IOBuf::create(0);
  Neutronium neutronium(&schema, &itable, buf.get());

  {
    TestStringEncoding1 a;
    a.a = 42;
    a.b = "hello";
    a.c = "world";
    a.d1 = "abcde";
    a.d2 = "abcdefghijklmno";

    a.e = "meow_X_woof";  // contains terminator
    {
      // TODO(tudorb): Make Neutronium exception-safe
      Neutronium n(&schema, &itable, buf.get());
      EXPECT_THROW({n.serialize(a);}, TException);
    }

    a.e = "meow_Y_woof";
    neutronium.serialize(a);
  }

  buf->coalesce();
  folly::StringPiece sp(reinterpret_cast<const char*>(buf->data()),
                        buf->length());
  EXPECT_EQ("world", itable.get(0));  // interned

  // Check that it doesn't exist in the serialized form
  EXPECT_NE(folly::StringPiece::npos, sp.find("hello"));
  EXPECT_EQ(folly::StringPiece::npos, sp.find("world"));

  {
    TestStringEncoding1 a;
    neutronium.deserialize(a);
    EXPECT_EQ(42, a.a);
    EXPECT_EQ("hello", a.b);
    EXPECT_EQ("world", a.c);
    EXPECT_EQ("abcdeXXXXX", a.d1);  // padded with 'X'
    EXPECT_EQ("abcdefghij", a.d2);  // truncated to fixed length
    EXPECT_EQ("meow_Y_woof", a.e);
  }
}

TEST(Neutronium, EnumEncoding1) {
  std::unique_ptr<folly::IOBuf> buf = folly::IOBuf::create(0);
  Neutronium neutronium(&schema, nullptr, buf.get());

  {
    TestEnumEncoding1 a;
    a.a = WORLD;
    a.c = true;
    a.d = 23;

    a.b = static_cast<Foo>(999);  // invalid
    {
      // TODO(tudorb): Make Neutronium exception-safe
      Neutronium n(&schema, nullptr, buf.get());
      EXPECT_THROW({n.serialize(a);}, std::exception);
    }
    a.b = GOODBYE;
    neutronium.serialize(a);
  }

  {
    TestEnumEncoding1 a;
    neutronium.deserialize(a);
    EXPECT_EQ(WORLD, a.a);
    EXPECT_EQ(GOODBYE, a.b);
    EXPECT_TRUE(a.c);
    EXPECT_EQ(23, a.d);
  }
}

TEST(Neutronium, EnumEncoding2) {
  std::unique_ptr<folly::IOBuf> buf = folly::IOBuf::create(0);
  Neutronium neutronium(&schema, nullptr, buf.get());

  {
    TestEnumEncoding1 a;
    // Invalid, but ensure that non-strict enums are encoded as integers and
    // don't actually cause exceptions during serialization / deserialization
    a.a = static_cast<Foo>(999);
    a.b = GOODBYE;
    a.c = true;
    a.d = 23;

    neutronium.serialize(a);
  }

  {
    TestEnumEncoding1 a;
    neutronium.deserialize(a);
    EXPECT_EQ(999, static_cast<uint32_t>(a.a));
    EXPECT_EQ(GOODBYE, a.b);
    EXPECT_TRUE(a.c);
    EXPECT_EQ(23, a.d);
  }
}

TEST(Neutronium, EnumEncoding3) {
  std::unique_ptr<folly::IOBuf> buf = folly::IOBuf::create(0);
  Neutronium neutronium(&schema, nullptr, buf.get());

  {
    TestEnumEncoding2 a;
    a.a = false;
    a.b = WORLD;
    a.__isset.d = true;
    a.d = GOODBYE;
    a.e = HELLO;
    neutronium.serialize(a);
  }

  buf->coalesce();

  // All of this, encoded in only one byte!
  EXPECT_EQ(1, buf->length());
  EXPECT_EQ(0x25, *buf->data());

  {
    TestEnumEncoding2 a;
    neutronium.deserialize(a);
    EXPECT_FALSE(a.a);
    EXPECT_EQ(WORLD, a.b);
    EXPECT_TRUE(a.__isset.d);
    EXPECT_EQ(GOODBYE, a.d);
    EXPECT_EQ(HELLO, a.e);
  }
}

namespace {

enum class Protocol {
  BINARY,
  COMPACT,
  NEUTRONIUM
};

struct BenchProtoBase {
  size_t size;
};

template <class T, class Serializer>
struct BenchProtoThriftBase : public BenchProtoBase {
  std::string str;

  void encode(const std::vector<T>& data) {
    Serializer serializer;
    for (auto& s : data) {
      std::string tmp;
      serializer.serialize(s, &tmp);
      str += tmp;
    }
    size = str.size();
  }

  void decode(int iters, size_t count, std::vector<T>& out) {
    Serializer serializer;
    while (iters--) {
      const uint8_t* p = reinterpret_cast<const uint8_t*>(str.data());
      size_t len = str.size();
      out.clear();
      for (size_t i = 0; i < count; i++) {
        T s;
        uint32_t n = serializer.deserialize(p, len, &s);
        p += n;
        len -= n;
        out.push_back(std::move(s));
      }
    }
  }
};

template <class T, Protocol Proto> struct BenchProto;

template <class T>
struct BenchProto<T, Protocol::BINARY>
  : public BenchProtoThriftBase<T, ThriftSerializerBinary<T>> {
};

template <class T>
struct BenchProto<T, Protocol::COMPACT>
  : public BenchProtoThriftBase<T, ThriftSerializerCompact<T>> {
};

template <class T>
struct BenchProto<T, Protocol::NEUTRONIUM> : public BenchProtoBase {
  std::unique_ptr<folly::IOBuf> buf;

  void encode(const std::vector<T>& data) {
    buf = folly::IOBuf::create(0);
    Neutronium neutronium(&schema, nullptr, buf.get());
    for (auto& s : data) {
      neutronium.serialize(s);
    }
    buf->coalesce();
    size = buf->length();
  }

  void decode(int iters, size_t count, std::vector<T>& out) {
    while (iters--) {
      Neutronium neutronium(&schema, nullptr, buf.get());
      out.clear();
      for (size_t i = 0; i < count; i++) {
        T s;
        neutronium.deserialize(s);
        out.push_back(std::move(s));
      }
    }
  }
};

template <class T>
struct Bench {
  explicit Bench(const char* n) : name(n) { }

  void init();

  const char* name;
  std::vector<T> data;
  std::vector<T> out;

  BenchProto<T, Protocol::BINARY> binary;
  BenchProto<T, Protocol::COMPACT> compact;
  BenchProto<T, Protocol::NEUTRONIUM> neutronium;
};

template <class T>
inline void Bench<T>::init() {
  out.reserve(data.size());

  binary.encode(data);
  LOG(INFO) << name << ": binary protocol size: " << binary.size;

  compact.encode(data);
  LOG(INFO) << name << ": compact protocol size: " << compact.size;

  neutronium.encode(data);
  LOG(INFO) << name << ": neutronium protocol size: " << neutronium.size;
}

Bench<BenchStruct1> bench1("BenchStruct1");
Bench<BenchStruct2> bench2("BenchStruct2");

template <class RND>
uint32_t genInt(RND& rnd) {
  uint32_t val = rnd();
  uint32_t nbytes = rnd() % 4;
  if (nbytes) {
    val &= (1 << (nbytes * 8)) - 1;
  }
  return val;
}


void initBenchmarks() {
  uint32_t seed = testing::FLAGS_gtest_random_seed;
  if (seed == 0) {
    seed = folly::randomNumberSeed();
  }

  LOG(INFO) << "Random seed is " << seed;
  std::mt19937 rnd(seed);

  static const size_t count1 = 1000;
  bench1.data.reserve(count1);
  for (size_t i = 0; i < count1; i++) {
    BenchStruct1 s;
    s.a = genInt(rnd);
    bench1.data.push_back(std::move(s));
  }
  bench1.init();

  static const size_t count2 = 1000;
  // in percent
  static const uint32_t aProb = 20;
  static const uint32_t bProb = 20;
  static const uint32_t cProb = 20;
  bench2.data.reserve(count2);
  for (size_t i = 0; i < count2; i++) {
    BenchStruct2 s;
    if (rnd() % 100 < aProb) {
      s.__isset.a = true;
      s.a = rnd();
    }
    if (rnd() % 100 < bProb) {
      s.__isset.b = true;
      size_t len = rnd() % 100;
      s.b.assign(len, 'x');
    }
    if (rnd() % 100 < cProb) {
      s.__isset.c = true;
      size_t len = rnd() % 100;
      s.c.reserve(len);
      for (size_t j = 0; j < len; j++) {
        s.c.push_back(genInt(rnd));
      }
    }
    bench2.data.push_back(std::move(s));
  }
  bench2.init();
}

}  // namespace

#define B(bench, proto) \
  namespace { \
  void bench##proto(int iters) { \
    bench.proto.decode(iters, bench.data.size(), bench.out); \
  } \
  } \
  BM_REGISTER(bench##proto);

B(bench1, binary);
B(bench1, compact);
B(bench1, neutronium);
BM_REGISTER_SUMMARY();

BM_REGISTER_LINEBREAK();

B(bench2, binary);
B(bench2, compact);
B(bench2, neutronium);
BM_REGISTER_SUMMARY();

#undef B

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  GFLAGS_INIT(argc, argv);

#define R(s) s::_reflection_register(reflectionSchema)
  R(TestFixedSizeStruct1);
  R(TestFixedSizeStruct2);
  R(TestNotFixedSizeStruct2);
  R(TestStruct1);
  R(TestStruct2);
  R(TestStringEncoding1);
  R(TestEnumEncoding1);
  R(TestEnumEncoding2);
  R(BenchStruct1);
  R(BenchStruct2);
#undef R
  schema = Schema(reflectionSchema);

  auto r = RUN_ALL_TESTS();
  if (FLAGS_benchmark && r == 0) {
    initBenchmarks();
    folly::RunAllBenchmarks();
  }

  return r;
}
