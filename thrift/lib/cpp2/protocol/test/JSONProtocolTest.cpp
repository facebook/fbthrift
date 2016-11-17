/*
 * Copyright 2016 Facebook, Inc.
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
#include <thrift/lib/cpp/protocol/TJSONProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/protocol/JSONProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp/Module_types.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/Module_types_custom_protocol.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace {

class JSONProtocolTest : public testing::Test {};

using P1 = TJSONProtocol;
using W2 = JSONProtocolWriter;
using R2 = JSONProtocolReader;

template <typename T>
struct action_traits_impl;
template <typename C, typename A>
struct action_traits_impl<void(C::*)(A&) const> { using arg_type = A; };
template <typename C, typename A>
struct action_traits_impl<void(C::*)(A&)> { using arg_type = A; };
template <typename F>
using action_traits = action_traits_impl<decltype(&F::operator())>;
template <typename F>
using arg = typename action_traits<F>::arg_type;

string writing_cpp1(function<void(P1&)> f) {
  auto buffer = make_shared<TMemoryBuffer>();
  P1 proto(buffer);
  f(proto);
  return buffer->getBufferAsString();
}

string writing_cpp2(function<void(W2&)> f) {
  IOBufQueue queue;
  W2 writer;
  writer.setOutput(&queue);
  f(writer);
  string _return;
  queue.appendToString(_return);
  return _return;
}

template <typename F>
arg<F> returning(F&& f) {
  arg<F> ret;
  f(ret);
  return ret;
}

template <typename T>
T reading_cpp1(ByteRange input, function<T(P1&)> f) {
  auto buffer = make_shared<TMemoryBuffer>(
      const_cast<uint8_t*>(input.data()), input.size());
  P1 proto(buffer);
  return f(proto);
}

template <typename T>
T reading_cpp1(StringPiece input, function<T(P1&)>&& f) {
  using F = typename std::remove_reference<decltype(f)>::type;
  return reading_cpp1(ByteRange(input), std::forward<F>(f));
}

template <typename T>
T reading_cpp2(ByteRange input, function<T(R2&)> f) {
  IOBuf buf(IOBuf::WRAP_BUFFER, input);
  R2 reader;
  reader.setInput(&buf);
  return f(reader);
}

template <typename T>
T reading_cpp2(StringPiece input, function<T(R2&)>&& f) {
  using F = typename std::remove_reference<decltype(f)>::type;
  return reading_cpp2(ByteRange(input), std::forward<F>(f));
}

}

TEST_F(JSONProtocolTest, writeBool_false) {
  auto expected = "0";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeBool(false);
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeBool(false);
  }));
}

TEST_F(JSONProtocolTest, writeBool_true) {
  auto expected = "1";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeBool(true);
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeBool(true);
  }));
}

TEST_F(JSONProtocolTest, writeByte) {
  auto expected = "17";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeByte(17);
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeByte(17);
  }));
}

TEST_F(JSONProtocolTest, writeI16) {
  auto expected = "1017";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeI16(1017);
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeI16(1017);
  }));
}

TEST_F(JSONProtocolTest, writeI32) {
  auto expected = "100017";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeI32(100017);
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeI32(100017);
  }));
}

TEST_F(JSONProtocolTest, writeI64) {
  auto expected = "5000000017";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeI64(5000000017);
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeI64(5000000017);
  }));
}

TEST_F(JSONProtocolTest, writeDouble) {
  auto expected = "5.25";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeDouble(5.25);
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeDouble(5.25);
  }));
}

TEST_F(JSONProtocolTest, writeFloat) {
  auto expected = "5.25";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeFloat(5.25f);
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeFloat(5.25f);
  }));
}

TEST_F(JSONProtocolTest, writeString) {
  auto expected = R"("foobar")";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeString(string("foobar"));
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeString(string("foobar"));
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeString("foobar");
  }));
}

TEST_F(JSONProtocolTest, writeBinary) {
  auto expected = R"("Zm9vYmFy")";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeBinary(string("foobar"));
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeBinary(string("foobar"));
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeBinary(IOBuf::wrapBuffer(ByteRange(StringPiece("foobar"))));
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeBinary(*IOBuf::wrapBuffer(ByteRange(StringPiece("foobar"))));
  }));
}

TEST_F(JSONProtocolTest, writeSerializedData) {
  auto expected = "foobar";
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeSerializedData(
            IOBuf::wrapBuffer(ByteRange(StringPiece("foobar"))));
        }));
}

TEST_F(JSONProtocolTest, writeMessage) {
  auto expected = R"([1,"foobar",1,3])";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeMessageBegin("foobar", TMessageType::T_CALL, 3);
        p.writeMessageEnd();
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeMessageBegin("foobar", MessageType::T_CALL, 3);
        p.writeMessageEnd();
  }));
}

TEST_F(JSONProtocolTest, writeStruct) {
  auto expected = R"({"3":{"i64":17},"12":{"str":"some-data"}})";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeStructBegin("foobar");
        p.writeFieldBegin("i64-field", TType::T_I64, 3);
        p.writeI64(17);
        p.writeFieldEnd();
        p.writeFieldBegin("str-field", TType::T_STRING, 12);
        p.writeString(string("some-data"));
        p.writeFieldEnd();
        p.writeFieldStop();
        p.writeStructEnd();
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeStructBegin("foobar");
        p.writeFieldBegin("i64-field", TType::T_I64, 3);
        p.writeI64(17);
        p.writeFieldEnd();
        p.writeFieldBegin("str-field", TType::T_STRING, 12);
        p.writeString(string("some-data"));
        p.writeFieldEnd();
        p.writeFieldStop();
        p.writeStructEnd();
  }));
}

TEST_F(JSONProtocolTest, writeMap_string_i64) {
  auto expected = R"(["str","i64",3,{"foo":13,"bar":17,"baz":19}])";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeMapBegin(TType::T_STRING, TType::T_I64, 3);
        p.writeString(string("foo"));
        p.writeI64(13);
        p.writeString(string("bar"));
        p.writeI64(17);
        p.writeString(string("baz"));
        p.writeI64(19);
        p.writeMapEnd();
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeMapBegin(TType::T_STRING, TType::T_I64, 3);
        p.writeString(string("foo"));
        p.writeI64(13);
        p.writeString(string("bar"));
        p.writeI64(17);
        p.writeString(string("baz"));
        p.writeI64(19);
        p.writeMapEnd();
  }));
}

TEST_F(JSONProtocolTest, writeList_string) {
  auto expected = R"(["str",3,"foo","bar","baz"])";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeListBegin(TType::T_STRING, 3);
        p.writeString(string("foo"));
        p.writeString(string("bar"));
        p.writeString(string("baz"));
        p.writeListEnd();
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeListBegin(TType::T_STRING, 3);
        p.writeString(string("foo"));
        p.writeString(string("bar"));
        p.writeString(string("baz"));
        p.writeListEnd();
  }));
}

TEST_F(JSONProtocolTest, writeSet_string) {
  auto expected = R"(["str",3,"foo","bar","baz"])";
  EXPECT_EQ(expected, writing_cpp1([](P1& p) {
        p.writeSetBegin(TType::T_STRING, 3);
        p.writeString(string("foo"));
        p.writeString(string("bar"));
        p.writeString(string("baz"));
        p.writeSetEnd();
  }));
  EXPECT_EQ(expected, writing_cpp2([](W2& p) {
        p.writeSetBegin(TType::T_STRING, 3);
        p.writeString(string("foo"));
        p.writeString(string("bar"));
        p.writeString(string("baz"));
        p.writeSetEnd();
  }));
}

TEST_F(JSONProtocolTest, serializedSizeBool_false) {
  EXPECT_EQ(2, W2().serializedSizeBool(false));
}

TEST_F(JSONProtocolTest, serializedSizeBool_true) {
  EXPECT_EQ(2, W2().serializedSizeBool(true));
}

TEST_F(JSONProtocolTest, serializedSizeByte) {
  EXPECT_EQ(6, W2().serializedSizeByte(17));
}

TEST_F(JSONProtocolTest, serializedSizeI16) {
  EXPECT_EQ(8, W2().serializedSizeI16(1017));
}

TEST_F(JSONProtocolTest, serializedSizeI32) {
  EXPECT_EQ(13, W2().serializedSizeI32(100017));
}

TEST_F(JSONProtocolTest, serializedSizeI64) {
  EXPECT_EQ(25, W2().serializedSizeI64(5000000017));
}

TEST_F(JSONProtocolTest, serializedSizeDouble) {
  EXPECT_EQ(25, W2().serializedSizeDouble(5.25));
}

TEST_F(JSONProtocolTest, serializedSizeFloat) {
  EXPECT_EQ(25, W2().serializedSizeFloat(5.25f));
}

TEST_F(JSONProtocolTest, serializedSizeStop) {
  EXPECT_EQ(0, W2().serializedSizeStop());
}

TEST_F(JSONProtocolTest, readBool_false) {
  auto input = "0";
  auto expected = false;
  EXPECT_EQ(expected, reading_cpp1<bool>(input, [](P1& p) {
        return returning([&](bool& _) { p.readBool(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<bool>(input, [](R2& p) {
        return returning([&](bool& _) { p.readBool(_); });
  }));
}

TEST_F(JSONProtocolTest, readBool_true) {
  auto input = "1";
  auto expected = true;
  EXPECT_EQ(expected, reading_cpp1<bool>(input, [](P1& p) {
        return returning([&](bool& _) { p.readBool(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<bool>(input, [](R2& p) {
        return returning([&](bool& _) { p.readBool(_); });
  }));
}

TEST_F(JSONProtocolTest, readByte) {
  auto input = "17";
  auto expected = int8_t(17);
  EXPECT_EQ(expected, reading_cpp1<int8_t>(input, [](P1& p) {
        return returning([&](int8_t& _) { p.readByte(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<int8_t>(input, [](R2& p) {
        return returning([&](int8_t& _) { p.readByte(_); });
  }));
}

TEST_F(JSONProtocolTest, readI16) {
  auto input = "1017";
  auto expected = int16_t(1017);
  EXPECT_EQ(expected, reading_cpp1<int16_t>(input, [](P1& p) {
        return returning([&](int16_t& _) { p.readI16(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<int16_t>(input, [](R2& p) {
        return returning([&](int16_t& _) { p.readI16(_); });
  }));
}

TEST_F(JSONProtocolTest, readI32) {
  auto input = "100017";
  auto expected = int32_t(100017);
  EXPECT_EQ(expected, reading_cpp1<int32_t>(input, [](P1& p) {
        return returning([&](int32_t& _) { p.readI32(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<int32_t>(input, [](R2& p) {
        return returning([&](int32_t& _) { p.readI32(_); });
  }));
}

TEST_F(JSONProtocolTest, readI64) {
  auto input = "5000000017";
  auto expected = int64_t(5000000017);
  EXPECT_EQ(expected, reading_cpp1<int64_t>(input, [](P1& p) {
        return returning([&](int64_t& _) { p.readI64(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<int64_t>(input, [](R2& p) {
        return returning([&](int64_t& _) { p.readI64(_); });
  }));
}

TEST_F(JSONProtocolTest, readDouble) {
  auto input = "5.25";
  auto expected = 5.25;
  EXPECT_EQ(expected, reading_cpp1<double>(input, [](P1& p) {
        return returning([&](double& _) { p.readDouble(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<double>(input, [](R2& p) {
        return returning([&](double& _) { p.readDouble(_); });
  }));
}

TEST_F(JSONProtocolTest, readFloat) {
  auto input = "5.25";
  auto expected = 5.25f;
  EXPECT_EQ(expected, reading_cpp1<float>(input, [](P1& p) {
        return returning([&](float& _) { p.readFloat(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<float>(input, [](R2& p) {
        return returning([&](float& _) { p.readFloat(_); });
  }));
}

TEST_F(JSONProtocolTest, readString) {
  auto input = R"("foobar")";
  auto expected = "foobar";
  EXPECT_EQ(expected, reading_cpp1<string>(input, [](P1& p) {
        return returning([&](string& _) { p.readString(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<string>(input, [](R2& p) {
        return returning([&](string& _) { p.readString(_); });
  }));
}

TEST_F(JSONProtocolTest, readString_raw) {
  auto input = R"("\u0019\u0002\u0000\u0000")";
  auto expected = string("\x19\x02\x00\x00", 4);
  EXPECT_EQ(expected, reading_cpp1<string>(input, [](P1& p) {
        return returning([&](string& _) { p.readString(_); });
  }));
  EXPECT_EQ(expected, reading_cpp1<string>(input, [](P1& p) {
        p.allowDecodeUTF8(true);
        return returning([&](string& _) { p.readString(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<string>(input, [](R2& p) {
        p.setAllowDecodeUTF8(false);
        return returning([&](string& _) { p.readString(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<string>(input, [](R2& p) {
        return returning([&](string& _) { p.readString(_); });
  }));
}

TEST_F(JSONProtocolTest, readString_utf8) {
  auto input = R"("\u263A")";
  auto expected = string(u8"\u263A");
  EXPECT_EQ(expected, reading_cpp1<string>(input, [](P1& p) {
        p.allowDecodeUTF8(true);
        return returning([&](string& _) { p.readString(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<string>(input, [](R2& p) {
        return returning([&](string& _) { p.readString(_); });
  }));
}

TEST_F(JSONProtocolTest, readBinary) {
  auto input = R"("Zm9vYmFy")";
  auto expected = "foobar";
  EXPECT_EQ(expected, reading_cpp1<string>(input, [](P1& p) {
        return returning([&](string& _) { p.readBinary(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<string>(input, [](R2& p) {
        return returning([&](string& _) { p.readBinary(_); });
  }));
  EXPECT_EQ(expected, reading_cpp2<string>(input, [](R2& p) {
        auto buf = IOBuf::create(0);
        p.readBinary(buf);
        return StringPiece(buf->coalesce()).str();
  }));
  EXPECT_EQ(expected, reading_cpp2<string>(input, [](R2& p) {
        IOBuf buf(IOBuf::CREATE, 0);
        p.readBinary(buf);
        return StringPiece(buf.coalesce()).str();
  }));
}

TEST_F(JSONProtocolTest, readMap_string_i64) {
  using concrete_map_t = map<string, int64_t>;
  auto input = R"(["str","i64",3,{"foo":13,"bar":17,"baz":19}])";
  auto expected = concrete_map_t{{"foo", 13}, {"bar", 17}, {"baz", 19}};
  EXPECT_EQ(expected, reading_cpp1<concrete_map_t>(input, [](P1& p) {
        return returning([&](concrete_map_t& _) {
          TType keyType = TType::T_STOP;
          TType valType = TType::T_STOP;
          uint32_t size = 0;
          bool sizeUnknown = false;
          p.readMapBegin(keyType, valType, size, sizeUnknown);
          EXPECT_EQ(TType::T_STRING, keyType);
          EXPECT_EQ(TType::T_I64, valType);
          EXPECT_EQ(3, size);
          EXPECT_FALSE(sizeUnknown);
          while (size--) {
            string key;
            int64_t val;
            p.readString(key);
            p.readI64(val);
            _[key] = val;
          }
          p.readMapEnd();
        });
  }));
  EXPECT_EQ(expected, reading_cpp2<concrete_map_t>(input, [](R2& p) {
        return returning([&](concrete_map_t& _) {
          TType keyType = TType::T_STOP;
          TType valType = TType::T_STOP;
          uint32_t size = 0;
          p.readMapBegin(keyType, valType, size);
          EXPECT_EQ(TType::T_STRING, keyType);
          EXPECT_EQ(TType::T_I64, valType);
          EXPECT_EQ(3, size);
          while (size--) {
            string key;
            int64_t val;
            p.readString(key);
            p.readI64(val);
            _[key] = val;
          }
          p.readMapEnd();
        });
  }));
}

TEST_F(JSONProtocolTest, readList_string) {
  using concrete_list_t = vector<string>;
  auto input = R"(["str",3,"foo","bar","baz"])";
  auto expected = concrete_list_t{"foo", "bar", "baz"};
  EXPECT_EQ(expected, reading_cpp1<concrete_list_t>(input, [](P1& p) {
        return returning([&](concrete_list_t& _) {
          TType elemType = TType::T_STOP;
          uint32_t size = 0;
          bool sizeUnknown = false;
          p.readListBegin(elemType, size, sizeUnknown);
          EXPECT_EQ(TType::T_STRING, elemType);
          EXPECT_EQ(3, size);
          EXPECT_FALSE(sizeUnknown);
          while (size--) {
            string elem;
            p.readString(elem);
            _.push_back(elem);
          }
          p.readListEnd();
        });
  }));
  EXPECT_EQ(expected, reading_cpp2<concrete_list_t>(input, [](R2& p) {
        return returning([&](concrete_list_t& _) {
          TType elemType = TType::T_STOP;
          uint32_t size = 0;
          p.readListBegin(elemType, size);
          EXPECT_EQ(TType::T_STRING, elemType);
          EXPECT_EQ(3, size);
          while (size--) {
            string elem;
            p.readString(elem);
            _.push_back(elem);
          }
          p.readListEnd();
        });
  }));
}

TEST_F(JSONProtocolTest, readSet_string) {
  using concrete_set_t = set<string>;
  auto input = R"(["str",3,"foo","bar","baz"])";
  auto expected = concrete_set_t{"foo", "bar", "baz"};
  EXPECT_EQ(expected, reading_cpp1<concrete_set_t>(input, [](P1& p) {
        return returning([&](concrete_set_t& _) {
          TType elemType = TType::T_STOP;
          uint32_t size = 0;
          bool sizeUnknown = false;
          p.readListBegin(elemType, size, sizeUnknown);
          EXPECT_EQ(TType::T_STRING, elemType);
          EXPECT_EQ(3, size);
          EXPECT_FALSE(sizeUnknown);
          while (size--) {
            string elem;
            p.readString(elem);
            _.insert(elem);
          }
          p.readListEnd();
        });
  }));
  EXPECT_EQ(expected, reading_cpp2<concrete_set_t>(input, [](R2& p) {
        return returning([&](concrete_set_t& _) {
          TType elemType = TType::T_STOP;
          uint32_t size = 0;
          p.readListBegin(elemType, size);
          EXPECT_EQ(TType::T_STRING, elemType);
          EXPECT_EQ(3, size);
          while (size--) {
            string elem;
            p.readString(elem);
            _.insert(elem);
          }
          p.readListEnd();
        });
  }));
}

template <typename P, typename F>
static void readStruct(P& p, F f) {
  string name_;
  p.readStructBegin(name_);
  EXPECT_EQ("", name_);
  f();
  p.readStructEnd();
}

template <typename P, typename F>
static void readField(P& p, TType fieldType, int16_t fieldId, F f) {
  string name_;
  TType fieldType_ = TType::T_STOP;
  int16_t fieldId_ = numeric_limits<int16_t>::max();
  p.readFieldBegin(name_, fieldType_, fieldId_);
  EXPECT_EQ("", name_);
  EXPECT_EQ(fieldType, fieldType_);
  EXPECT_EQ(fieldId, fieldId_);
  f();
  p.readFieldEnd();
}

TEST_F(JSONProtocolTest, readStruct) {
  using concrete_struct_t = tuple<int64_t, string>;
  auto input = R"({"3":{"i64":17},"12":{"str":"some-data"}})";
  auto expected = concrete_struct_t{17, "some-data"};
  EXPECT_EQ(expected, reading_cpp1<concrete_struct_t>(input, [](P1& p) {
        return returning([&](concrete_struct_t& _) {
          readStruct(p, [&] {
            readField(p, TType::T_I64, 3, [&] { p.readI64(get<0>(_)); });
            readField(p, TType::T_STRING, 12, [&] { p.readString(get<1>(_)); });
          });
        });
  }));
  EXPECT_EQ(expected, reading_cpp2<concrete_struct_t>(input, [](R2& p) {
        return returning([&](concrete_struct_t& _) {
          readStruct(p, [&] {
            readField(p, TType::T_I64, 3, [&] { p.readI64(get<0>(_)); });
            readField(p, TType::T_STRING, 12, [&] { p.readString(get<1>(_)); });
          });
        });
  }));
}

TEST_F(JSONProtocolTest, readMessage) {
  auto input = R"([1,"foobar",1,3])";
  struct Unit {
    bool operator==(Unit) const { return true; }
  };
  auto expected = Unit{};
  EXPECT_EQ(expected, reading_cpp1<Unit>(input, [](P1& p) {
        string name;
        TMessageType messageType = TMessageType(-1);
        int32_t seqId = -1;
        p.readMessageBegin(name, messageType, seqId);
        EXPECT_EQ("foobar", name);
        EXPECT_EQ(TMessageType::T_CALL, messageType);
        EXPECT_EQ(3, seqId);
        p.readMessageEnd();
        return Unit{};
  }));
  EXPECT_EQ(expected, reading_cpp2<Unit>(input, [](R2& p) {
        string name;
        MessageType messageType = MessageType(-1);
        int32_t seqId = -1;
        p.readMessageBegin(name, messageType, seqId);
        EXPECT_EQ("foobar", name);
        EXPECT_EQ(MessageType::T_CALL, messageType);
        EXPECT_EQ(3, seqId);
        p.readMessageEnd();
        return Unit{};
  }));
}

TEST_F(JSONProtocolTest, roundtrip) {
  //  cpp2 -> str -> cpp2
  const auto orig = cpp2::OneOfEach{};
  const auto serialized = JSONSerializer::serialize<string>(orig);
  cpp2::OneOfEach deserialized;
  const auto size = JSONSerializer::deserialize(serialized, deserialized);
  EXPECT_EQ(serialized.size(), size);
  EXPECT_EQ(orig, deserialized);
}

TEST_F(JSONProtocolTest, super_roundtrip) {
  //  cpp2 -> str -> cp1 -> str -> cpp2
  util::ThriftSerializerJson<> cpp1_serializer;
  const auto orig = cpp2::OneOfEach{};
  const auto serialized_1 = JSONSerializer::serialize<string>(orig);
  const auto deserialized_size_1 =
    returning([&](tuple<cpp1::OneOfEach, uint32_t>& _) {
        get<1>(_) = cpp1_serializer.deserialize(serialized_1, &get<0>(_));
    });
  const auto deserialized_1 = get<0>(deserialized_size_1);
  const auto size_1 = get<1>(deserialized_size_1);
  EXPECT_EQ(serialized_1.size(), size_1);
  const auto serialized_2 = returning([&](string& _) {
      cpp1_serializer.serialize(deserialized_1, &_);
  });
  EXPECT_EQ(serialized_1, serialized_2);
  const auto deserialized_size_2 =
    returning([&](tuple<cpp2::OneOfEach, uint32_t>& _) {
        get<1>(_) = JSONSerializer::deserialize(serialized_2, get<0>(_));
    });
  const auto deserialized_2 = get<0>(deserialized_size_2);
  const auto size_2 = get<1>(deserialized_size_2);
  EXPECT_EQ(serialized_2.size(), size_2);
  EXPECT_EQ(orig, deserialized_2);
}
