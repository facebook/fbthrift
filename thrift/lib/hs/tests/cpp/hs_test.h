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
#ifndef HS_TEST
#define HS_TEST

#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/hs/tests/if/gen-cpp2/hs_test_types.h>

extern "C" {

struct CTestStruct {
  bool        f_bool;
  int8_t      f_byte;
  float       f_float;
  int16_t     f_i16;
  int32_t     f_i32;
  int64_t     f_i64;
  double      f_double;
  int16_t*    f_list;
  int         f_list_len;
  int16_t*    f_map_keys;
  int32_t*    f_map_vals;
  int         f_map_len;
  const char* f_string;
  int8_t*     f_set;
  int         f_set_len;
  int         o_i32;
  bool        o_isset;
  apache::thrift::test::Foo* foo;
};

apache::thrift::transport::TMemoryBuffer* newMB();
uint32_t readMB(apache::thrift::transport::TMemoryBuffer*, uint8_t*, uint32_t);
void writeMB(apache::thrift::transport::TMemoryBuffer*,
               const uint8_t*, uint32_t);
void deleteMB(apache::thrift::transport::TMemoryBuffer*);

apache::thrift::test::TestStruct* getTestStruct();
void freeTestStruct(apache::thrift::test::TestStruct*);

apache::thrift::test::Foo* getFooPtr();
int getFooBar(apache::thrift::test::Foo*);
int getFooBaz(apache::thrift::test::Foo*);
void fillFoo(apache::thrift::test::Foo*, int, int);

void fillStruct(apache::thrift::test::TestStruct*, CTestStruct*);
void freeBuffers(CTestStruct*);
void readStruct(CTestStruct*, apache::thrift::test::TestStruct*);

void serializeBinary(
    apache::thrift::transport::TMemoryBuffer*,
    apache::thrift::test::TestStruct*);
apache::thrift::test::TestStruct* deserializeBinary(
    apache::thrift::transport::TMemoryBuffer*);

void serializeCompact(
    apache::thrift::transport::TMemoryBuffer*,
    apache::thrift::test::TestStruct*);
apache::thrift::test::TestStruct* deserializeCompact(
    apache::thrift::transport::TMemoryBuffer*);

void serializeJSON(
    apache::thrift::transport::TMemoryBuffer*,
    apache::thrift::test::TestStruct*);
apache::thrift::test::TestStruct* deserializeJSON(
    apache::thrift::transport::TMemoryBuffer*);

void serializeSimpleJSON(
    apache::thrift::transport::TMemoryBuffer*,
    apache::thrift::test::TestStruct*);
apache::thrift::test::TestStruct* deserializeSimpleJSON(
    apache::thrift::transport::TMemoryBuffer*);
}

#endif
