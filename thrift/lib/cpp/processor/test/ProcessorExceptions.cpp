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


// This test checks for an exception when a server receives a struct
// with a required member missing.
// We test it by using a fake client (SampleService2) whose arguments
// definition define an optional field and send it to a server (SampleService)
// that expects a required argument

#include <gtest/gtest.h>
#include <memory>
#include "thrift/lib/cpp/processor/test/gen-cpp/SampleService.h"
#include "thrift/lib/cpp/processor/test/gen-cpp/SampleService2.h"
#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/lib/cpp/TApplicationException.h"

#include <iostream>

using namespace apache::thrift::test;

using std::make_shared;
using std::shared_ptr;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::protocol::TBinaryProtocol;
using apache::thrift::TApplicationException;

class SampleServiceHandler : public SampleServiceIf {
public:
  int32_t return42(const MyArgs& unused, int32_t i) {
    return 42;
  }
};

bool Inner::operator <(const Inner& r) const {
  return i < r.i;
}

bool Inner2::operator <(const Inner2& r) const {
  return i < r.i;
}

int32_t call_return42(std::function<void(MyArgs2&)> isset_cb) {
  SampleServiceProcessor proc(make_shared<SampleServiceHandler>());
  TMemoryBuffer inbuf, outbuf;
  // in and out are from the server's perspective
  shared_ptr<TBinaryProtocol> inproto = make_shared<TBinaryProtocol>(&inbuf);
  shared_ptr<TBinaryProtocol> outproto = make_shared<TBinaryProtocol>(&outbuf);
  // client's in and out are swapped
  SampleService2Client client(outproto, inproto);
  Inner2 inner;
  inner.i = 7;
  inner.__isset.i = true;
  MyArgs2 args;
  args.s = "qwerty";
  args.l = {1,2,3};
  args.m = {{"a", 1}, {"b", 2}};
  args.li = {inner};
  args.mi = {{11, inner}};
  args.complex_key = {{inner, 11}};
  args.__isset.s = args.__isset.i = args.i.__isset.i = args.__isset.l =
    args.__isset.m = args.__isset.li = args.li[0].__isset.i =
    args.__isset.mi = args.mi[11].__isset.i =
    args.__isset.complex_key = true;
  isset_cb(args);
  client.send_return42(args, 5);
  proc.process(inproto, outproto, nullptr);
  EXPECT_EQ(0, inbuf.getBufferAsString().size());
  try {
    int32_t ret = client.recv_return42();
    EXPECT_EQ(0, outbuf.getBufferAsString().size());
    return ret;
  } catch(...) {
    EXPECT_EQ(0, outbuf.getBufferAsString().size());
    throw;
  }
}

TEST(ProcessorExceptionTest, ok_if_required_set) {
  EXPECT_EQ(42, call_return42([] (MyArgs2& a) {}));
}

TEST(ProcessorExceptionTest, throw_if_scalar_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.__isset.s = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_inner_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.__isset.i = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_inner_field_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.i.__isset.i = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_list_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.__isset.l = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_map_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.__isset.m = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_list_of_struct_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.__isset.li = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_list_inner_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.li[0].__isset.i = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_map_of_struct_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.__isset.mi = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_map_inner_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.mi[11].__isset.i = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_map_key_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {a.__isset.complex_key = false;}),
    TApplicationException);
}

TEST(ProcessorExceptionTest, throw_if_map_key_inner_required_missing) {
  EXPECT_THROW(call_return42([] (MyArgs2& a) {
      std::pair<Inner2,int> elem = *a.complex_key.cbegin();
      elem.first.__isset.i = false;
      a.complex_key.clear();
      a.complex_key.insert(elem);
    }),
    TApplicationException);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
