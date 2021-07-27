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

#include <thrift/lib/hs/tests/cpp/hs_test.h>

#include <map>
#include <string>
#include <vector>

#include <thrift/lib/cpp/transport/TTransport.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/hs/tests/if/gen-cpp2/hs_test_types_custom_protocol.h>

using namespace apache::thrift;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;
using namespace std;

extern "C" {

TMemoryBuffer* newMB() {
  return new TMemoryBuffer();
}

uint32_t readMB(TMemoryBuffer* mb, uint8_t* buf, uint32_t n) {
  return mb->read(buf, n);
}

void writeMB(TMemoryBuffer* mb, const uint8_t* buf, uint32_t len) {
  mb->write(buf, len);
}

void deleteMB(TMemoryBuffer* mb) {
  delete mb;
}

// Allocate a new TestStruct object from Haskell
TestStruct* getStructPtr() {
  TestStruct* testStruct = new TestStruct();
  return testStruct;
}

// Free TestStruct from Haskell
void freeTestStruct(TestStruct* ts) {
  delete ts;
}

Foo* getFooPtr() {
  return new Foo();
}

int getFooBar(Foo* foo) {
  return *foo->bar_ref();
}

int getFooBaz(Foo* foo) {
  return *foo->baz_ref();
}

void fillFoo(Foo* foo, int bar, int baz) {
  *foo->bar_ref() = bar;
  *foo->baz_ref() = baz;
}

// Fill a TestStruct with values from Haskell
void fillStruct(TestStruct* dest, CTestStruct* source) {
  dest->f_bool = source->f_bool != 0;
  *dest->f_byte_ref() = source->f_byte;
  *dest->f_float_ref() = source->f_float;
  *dest->f_i16_ref() = source->f_i16;
  *dest->f_i32_ref() = source->f_i32;
  *dest->f_i64_ref() = source->f_i64;
  *dest->f_double_ref() = source->f_double;
  *dest->f_list_ref() =
      std::vector<int16_t>(source->f_list, source->f_list + source->f_list_len);
  *dest->f_map_ref() = std::map<int16_t, int32_t>();
  for (int j = 0; j < source->f_map_len; j++)
    dest->f_map_ref()[source->f_map_keys[j]] = source->f_map_vals[j];
  *dest->f_string_ref() = std::string(source->f_string);
  *dest->f_set_ref() =
      std::set<int8_t>(source->f_set, source->f_set + source->f_set_len);
  if (source->o_isset) {
    dest->o_i32_ref() = source->o_i32;
  } else {
    dest->o_i32_ref().reset();
  }
  *dest->foo_ref() = Foo(*source->foo);
}

// Read fields of a TestStruct in to Haskell
void readStruct(CTestStruct* dest, TestStruct* source) {
  dest->f_bool = source->f_bool ? 1 : 0;
  dest->f_byte = *source->f_byte_ref();
  dest->f_float = *source->f_float_ref();
  dest->f_i16 = *source->f_i16_ref();
  dest->f_i32 = *source->f_i32_ref();
  dest->f_i64 = *source->f_i64_ref();
  dest->f_double = *source->f_double_ref();
  dest->f_list = source->f_list_ref()->data();
  dest->f_list_len = source->f_list_ref()->size();
  dest->f_map_len = source->f_map_ref()->size();
  dest->f_map_keys = (int16_t*)malloc(sizeof(int16_t) * dest->f_map_len);
  dest->f_map_vals = (int32_t*)malloc(sizeof(int32_t) * dest->f_map_len);
  int j = 0;
  for (auto p : *source->f_map_ref()) {
    dest->f_map_keys[j] = p.first;
    dest->f_map_vals[j] = p.second;
    j++;
  }
  dest->f_string = source->f_string_ref()->c_str();
  dest->f_set_len = source->f_set_ref()->size();
  dest->f_set = (int8_t*)malloc(sizeof(int8_t) * dest->f_set_len);
  j = 0;
  for (auto e : *source->f_set_ref()) {
    dest->f_set[j] = e;
    j++;
  }
  dest->o_i32 = *source->o_i32_ref();
  dest->o_isset = source->o_i32_ref().has_value();
  dest->foo = &(*source->foo_ref());
}

// Free the buffers that were allocated in `readStruct`
void freeBuffers(CTestStruct* obj) {
  free(obj->f_map_keys);
  free(obj->f_map_vals);
  free(obj->f_set);
}

// Serialize a TestStruct using TBinaryProtocol
void serializeBinary(TMemoryBuffer* mt, TestStruct* obj) {
  auto buf = BinarySerializer::serialize<IOBufQueue>(*obj).move();
  auto range = buf->coalesce();
  mt->write(range.data(), range.size());
}

// Deserialize a TestStruct using TBinaryProtocol
TestStruct* deserializeBinary(TMemoryBuffer* mt) {
  auto buf = mt->wrapBufferAsIOBuf();
  auto res = std::make_unique<TestStruct>();
  BinarySerializer::deserialize(buf.get(), *res);
  return res.release();
}

// Serialize a TestStruct using TCompactProtocol
void serializeCompact(TMemoryBuffer* mt, TestStruct* obj) {
  auto buf = CompactSerializer::serialize<IOBufQueue>(*obj).move();
  auto range = buf->coalesce();
  mt->write(range.data(), range.size());
}

// Deserialize a TestStruct using TCompactProtocol
TestStruct* deserializeCompact(TMemoryBuffer* mt) {
  auto buf = mt->wrapBufferAsIOBuf();
  auto res = std::make_unique<TestStruct>();
  CompactSerializer::deserialize(buf.get(), *res);
  return res.release();
}

// Serialize a TestStruct using TJSONProtocol
void serializeJSON(TMemoryBuffer* mb, TestStruct* obj) {
  auto buf = JSONSerializer::serialize<IOBufQueue>(*obj).move();
  auto range = buf->coalesce();
  mb->write(range.data(), range.size());
}

// Deserialize a TestStruct using TJSONProtocol
TestStruct* deserializeJSON(TMemoryBuffer* mb) {
  auto buf = mb->wrapBufferAsIOBuf();
  auto res = std::make_unique<TestStruct>();
  JSONSerializer::deserialize(buf.get(), *res);
  return res.release();
}

// Serialize a TestStruct using TSimpleJSONProtocol
void serializeSimpleJSON(TMemoryBuffer* mb, TestStruct* obj) {
  auto buf = SimpleJSONSerializer::serialize<IOBufQueue>(*obj).move();
  auto range = buf->coalesce();
  mb->write(range.data(), range.size());
}

// Deserialize a TestStruct using TSimpleJSONProtocol
TestStruct* deserializeSimpleJSON(TMemoryBuffer* mb) {
  auto buf = mb->wrapBufferAsIOBuf();
  auto res = std::make_unique<TestStruct>();
  SimpleJSONSerializer::deserialize(buf.get(), *res);
  return res.release();
}

} // extern "C"
