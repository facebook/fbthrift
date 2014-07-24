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

#include "hs_test.h"

#include <map>
#include <string>
#include <vector>

#include <thrift/lib/cpp/transport/TTransport.h>
#include <thrift/lib/cpp/protocol/TSimpleJSONProtocol.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>

using namespace apache::thrift;
using namespace std;

/**
 * Mock Transport reads from and writes to a buffer
 */
class MockTransport : public apache::thrift::transport::TTransport {
 public:
  MockTransport() : buffer() {}

  vector<uint8_t> buffer;


  bool isOpen() { return true; }
  void open() {}
  void close() {}

  uint32_t read_virt(uint8_t* buf, uint32_t n) {
    int32_t len = buffer.size();
    if (len >= n) {
      memcpy(buf, buffer.data(), n);
      buffer.erase(buffer.begin(), buffer.begin() + n);
      return n;
    } else {
      memcpy(buf, buffer.data(), len);
      buffer.erase(buffer.begin(), buffer.end());
      return len;
    }
  }

  void write_virt(const uint8_t* buf, uint32_t len) {
    buffer.insert(buffer.end(), buf, buf+len);
  }
};

// Allocate a new TestStruct object from Haskell
TestStruct* getStructPtr() {
  TestStruct *testStruct = new TestStruct();
  return testStruct;
}

// Free TestStruct from Haskell
void freeTestStruct(TestStruct* ts) {
  delete ts;
}

// Free the string from Haskell
void deleteSResult(SerializedResult* ptr) {
  free((void*)ptr->str);
}

Foo *getFooPtr() {
  return new Foo();
}

int getFooBar(Foo* foo) {
  return foo->bar;
}

int getFooBaz(Foo* foo) {
  return foo->baz;
}

void fillFoo(Foo* foo, int bar, int baz) {
  foo->bar = bar;
  foo->baz = baz;
}

// Fill a TestStruct with values from Haskell
void fillStruct(TestStruct* dest, CTestStruct* source) {
  dest->f_bool = source->f_bool != 0;
  dest->f_byte = source->f_byte;
  dest->f_float = source->f_float;
  dest->f_i16 = source->f_i16;
  dest->f_i32 = source->f_i32;
  dest->f_i64 = source->f_i64;
  dest->f_double = source->f_double;
  dest->f_list = std::vector<int16_t>(source->f_list,
                                      source->f_list + source->f_list_len);
  dest->f_map = std::map<int16_t, int32_t>();
  for (int j = 0; j < source->f_map_len; j++)
    dest->f_map[source->f_map_keys[j]] = source->f_map_vals[j];
  dest->f_string = std::string(source->f_string);
  dest->f_set = std::set<int8_t>(source->f_set,
                                 source->f_set + source->f_set_len);
  dest->o_i32 = source->o_i32;
  dest->__isset.o_i32 = source->o_isset;
  dest->foo = Foo(*source->foo);
}

// Read fields of a TestStruct in to Haskell
void readStruct(CTestStruct* dest, TestStruct* source) {
  dest->f_bool = source->f_bool ? 1 : 0;
  dest->f_byte = source->f_byte;
  dest->f_float = source->f_float;
  dest->f_i16 = source->f_i16;
  dest->f_i32 = source->f_i32;
  dest->f_i64 = source->f_i64;
  dest->f_double = source->f_double;
  dest->f_list = source->f_list.data();
  dest->f_list_len = source->f_list.size();
  dest->f_map_len = source->f_map.size();
  dest->f_map_keys = (int16_t*)malloc(sizeof(int16_t) * dest->f_map_len);
  dest->f_map_vals = (int32_t*)malloc(sizeof(int32_t) * dest->f_map_len);
  int j = 0;
  for (auto p : source->f_map) {
    dest->f_map_keys[j] = p.first;
    dest->f_map_vals[j] = p.second;
    j++;
  }
  dest->f_string = source->f_string.c_str();
  dest->f_set_len = source->f_set.size();
  dest->f_set = (int8_t*)malloc(sizeof(int8_t) * dest->f_set_len);
  j = 0;
  for (auto e : source->f_set) {
    dest->f_set[j] = e;
    j++;
  }
  dest->o_i32 = source->o_i32;
  dest->o_isset = source->__isset.o_i32;
  dest->foo = &source->foo;
}

// Free the buffers that were allocated in `readStruct`
void freeBuffers(CTestStruct* obj) {
  free(obj->f_map_keys);
  free(obj->f_map_vals);
  free(obj->f_set);
}

// Serialize a TestStruct using the given Protocol
void serializeStruct(protocol::TProtocol &prot,
                     MockTransport &mt,
                     SerializedResult *sr,
                     TestStruct *obj) {
  obj->write(&prot);
  sr->str = (char*)malloc(mt.buffer.size());
  sr->len = mt.buffer.size();
  memcpy((void*)sr->str, (void*)mt.buffer.data(), mt.buffer.size());
}

// Deserialize a TestStruct using the given Protocol
TestStruct* deserializeStruct(char *data,
                              int len,
                              protocol::TProtocol &prot,
                              MockTransport &mt) {
  TestStruct *ts = new TestStruct();
  mt.buffer.insert(mt.buffer.end(), data, data + len);
  ts->read(&prot);
  return ts;
}

// Serialize a TestStruct using TBinaryProtocol
void serializeBinary(SerializedResult *sr, TestStruct *obj) {
  auto mockTrans = std::make_shared<MockTransport>();
  protocol::TBinaryProtocol oprot(mockTrans);

  serializeStruct(oprot, *mockTrans, sr, obj);
}

// Deserialize a TestStruct using TBinaryProtocol
TestStruct* deserializeBinary(char *data, int len) {
  auto mockTrans = std::make_shared<MockTransport>();
  protocol::TBinaryProtocol oprot(mockTrans);

  return deserializeStruct(data, len, oprot, *mockTrans);
}

// Serialize a TestStruct using TSimpleJSONProtocol
void serializeJSON(SerializedResult *sr, TestStruct *obj) {
  auto mockTrans = std::make_shared<MockTransport>();
  protocol::TSimpleJSONProtocol oprot(mockTrans);

  serializeStruct(oprot, *mockTrans, sr, obj);
}

// Deserialize a TestStruct using TSimpleJSONProtocol
TestStruct* deserializeJSON(char *data, int len) {
  MockTransport mt;
  auto mockTrans = std::make_shared<MockTransport>();
  protocol::TSimpleJSONProtocol oprot(mockTrans);

  return deserializeStruct(data, len, oprot, *mockTrans);
}
