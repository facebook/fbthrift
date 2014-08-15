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

#ifndef HS_TEST
#define HS_TEST

#include <thrift/lib/hs/tests/if/gen-cpp/hs_test_types.h>

class MockTransport;

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
  Foo*        foo;
};

MockTransport* newMT();
uint32_t readMT(MockTransport*, uint8_t*, uint32_t);
void writeMT(MockTransport*, const uint8_t*, uint32_t);
void deleteMT(MockTransport*);

TestStruct *getTestStruct();
void freeTestStruct(TestStruct*);

Foo *getFooPtr();
int  getFooBar(Foo*);
int  getFooBaz(Foo*);
void fillFoo(Foo*, int, int);

void fillStruct(TestStruct*, CTestStruct*);
void freeBuffers(CTestStruct*);
void readStruct(CTestStruct*, TestStruct*);

void serializeBinary(MockTransport*, TestStruct*);
TestStruct* deserializeBinary(MockTransport*);

void serializeCompact(MockTransport*, TestStruct*);
TestStruct* deserializeCompact(MockTransport*);

void serializeJSON(MockTransport*, TestStruct*);
TestStruct* deserializeJSON(MockTransport*);

void serializeSimpleJSON(MockTransport*, TestStruct*);
TestStruct* deserializeSimpleJSON(MockTransport*);


#endif
