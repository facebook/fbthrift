/*
 * Copyright 2009-present Facebook, Inc.
 *
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

#ifndef _THRIFT_TEST_GENERICPROTOCOLTEST_TCC_
#define _THRIFT_TEST_GENERICPROTOCOLTEST_TCC_ 1

#include <limits>

#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/Thrift.h>

#include <thrift/test/GenericHelpers.h>

#define ERR_LEN 512
extern char errorMessage[ERR_LEN];

namespace apache {
namespace thrift {
namespace test {

template <typename TProto, typename Val>
void testNaked(Val val) {
  auto protocol = std::make_shared<TProto>(
      std::make_shared<transport::TMemoryBuffer>());

  GenericIO::write(protocol, val);
  protocol->getTransport()->flush();
  Val out;
  GenericIO::read(protocol, out);
  if (out != val) {
    snprintf(errorMessage, ERR_LEN, "Invalid naked test (type: %s)", ClassNames::getName<Val>());
    throw TLibraryException(errorMessage);
  }
}

template <typename TProto, protocol::TType type, typename Val>
void testField(const Val val) {
  auto protocol = std::make_shared<TProto>(
      std::make_shared<transport::TMemoryBuffer>());

  protocol->writeStructBegin("test_struct");
  protocol->writeFieldBegin("test_field", type, (int16_t)15);

  GenericIO::write(protocol, val);

  protocol->writeFieldEnd();
  protocol->writeStructEnd();
  protocol->getTransport()->flush();

  std::string name;
  protocol::TType fieldType;
  int16_t fieldId;

  protocol->readStructBegin(name);
  protocol->readFieldBegin(name, fieldType, fieldId);

  if (fieldId != 15) {
    snprintf(errorMessage, ERR_LEN, "Invalid ID (type: %s)", typeid(val).name());
    throw TLibraryException(errorMessage);
  }
  if (fieldType != type) {
    snprintf(errorMessage, ERR_LEN, "Invalid Field Type (type: %s)", typeid(val).name());
    throw TLibraryException(errorMessage);
  }

  Val out;
  GenericIO::read(protocol, out);

  if (out != val) {
    snprintf(errorMessage, ERR_LEN, "Invalid value read (type: %s)", typeid(val).name());
    throw TLibraryException(errorMessage);
  }

  protocol->readFieldEnd();
  protocol->readStructEnd();
}

template <typename TProto>
void testMessage() {
  struct TMessage {
    const char* name;
    protocol::TMessageType type;
    int32_t seqid;
  } messages[4] = {
    {"short message name", protocol::T_CALL, 0},
    {"1", protocol::T_REPLY, 12345},
    {"loooooooooooooooooooooooooooooooooong", protocol::T_EXCEPTION, 1 << 16},
    {"Janky", protocol::T_CALL, 0}
  };

  for (int i = 0; i < 4; i++) {
    auto protocol = std::make_shared<TProto>(
        std::make_shared<transport::TMemoryBuffer>());

    protocol->writeMessageBegin(messages[i].name,
                                messages[i].type,
                                messages[i].seqid);
    protocol->writeMessageEnd();
    protocol->getTransport()->flush();

    std::string name;
    protocol::TMessageType type;
    int32_t seqid;

    protocol->readMessageBegin(name, type, seqid);
    if (name != messages[i].name ||
        type != messages[i].type ||
        seqid != messages[i].seqid) {
      throw TLibraryException("readMessageBegin failed.");
    }
  }
}

template <typename TProto>
void testProtocol(const char* protoname) {
  using namespace protocol;
  try {
    testNaked<TProto, int8_t>((int8_t)123);

    for (int32_t i = 0; i < 128; i++) {
      testField<TProto, T_BYTE, int8_t>((int8_t)i);
      testField<TProto, T_BYTE, int8_t>((int8_t)-i);
    }

    testNaked<TProto, int16_t>((int16_t)0);
    testNaked<TProto, int16_t>((int16_t)1);
    testNaked<TProto, int16_t>((int16_t)15000);
    testNaked<TProto, int16_t>((int16_t)0x7fff);
    testNaked<TProto, int16_t>((int16_t)-1);
    testNaked<TProto, int16_t>((int16_t)-15000);
    testNaked<TProto, int16_t>((int16_t)-0x7fff);
    testNaked<TProto, int16_t>(std::numeric_limits<int16_t>::min());
    testNaked<TProto, int16_t>(std::numeric_limits<int16_t>::max());

    testField<TProto, T_I16, int16_t>((int16_t)0);
    testField<TProto, T_I16, int16_t>((int16_t)1);
    testField<TProto, T_I16, int16_t>((int16_t)7);
    testField<TProto, T_I16, int16_t>((int16_t)150);
    testField<TProto, T_I16, int16_t>((int16_t)15000);
    testField<TProto, T_I16, int16_t>((int16_t)0x7fff);
    testField<TProto, T_I16, int16_t>((int16_t)-1);
    testField<TProto, T_I16, int16_t>((int16_t)-7);
    testField<TProto, T_I16, int16_t>((int16_t)-150);
    testField<TProto, T_I16, int16_t>((int16_t)-15000);
    testField<TProto, T_I16, int16_t>((int16_t)-0x7fff);

    testNaked<TProto, int32_t>(0);
    testNaked<TProto, int32_t>(1);
    testNaked<TProto, int32_t>(15000);
    testNaked<TProto, int32_t>(0xffff);
    testNaked<TProto, int32_t>(-1);
    testNaked<TProto, int32_t>(-15000);
    testNaked<TProto, int32_t>(-0xffff);
    testNaked<TProto, int32_t>(std::numeric_limits<int32_t>::min());
    testNaked<TProto, int32_t>(std::numeric_limits<int32_t>::max());

    testField<TProto, T_I32, int32_t>(0);
    testField<TProto, T_I32, int32_t>(1);
    testField<TProto, T_I32, int32_t>(7);
    testField<TProto, T_I32, int32_t>(150);
    testField<TProto, T_I32, int32_t>(15000);
    testField<TProto, T_I32, int32_t>(31337);
    testField<TProto, T_I32, int32_t>(0xffff);
    testField<TProto, T_I32, int32_t>(0xffffff);
    testField<TProto, T_I32, int32_t>(-1);
    testField<TProto, T_I32, int32_t>(-7);
    testField<TProto, T_I32, int32_t>(-150);
    testField<TProto, T_I32, int32_t>(-15000);
    testField<TProto, T_I32, int32_t>(-0xffff);
    testField<TProto, T_I32, int32_t>(-0xffffff);
    testNaked<TProto, int64_t>(std::numeric_limits<int32_t>::min());
    testNaked<TProto, int64_t>(std::numeric_limits<int32_t>::max());
    testNaked<TProto, int64_t>(std::numeric_limits<int32_t>::min() + 10);
    testNaked<TProto, int64_t>(std::numeric_limits<int32_t>::max() - 16);
    testNaked<TProto, int64_t>(std::numeric_limits<int64_t>::min());
    testNaked<TProto, int64_t>(std::numeric_limits<int64_t>::max());


    testNaked<TProto, int64_t>(0);
    for (int64_t i = 0; i < 62; i++) {
      testNaked<TProto, int64_t>(1L << i);
      testNaked<TProto, int64_t>(-(1L << i));
    }

    testField<TProto, T_I64, int64_t>(0);
    for (int i = 0; i < 62; i++) {
      testField<TProto, T_I64, int64_t>(1L << i);
      testField<TProto, T_I64, int64_t>(-(1L << i));
    }

    testNaked<TProto, double>(123.456);

    testNaked<TProto, std::string>("");
    testNaked<TProto, std::string>("short");
    testNaked<TProto, std::string>("borderlinetiny");
    testNaked<TProto, std::string>("a bit longer than the smallest possible");
    testNaked<TProto, std::string>("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA"); //kinda binary test

    testField<TProto, T_STRING, std::string>("");
    testField<TProto, T_STRING, std::string>("short");
    testField<TProto, T_STRING, std::string>("borderlinetiny");
    testField<TProto, T_STRING, std::string>("a bit longer than the smallest possible");

    testMessage<TProto>();

    printf("%s => OK\n", protoname);
} catch (const TException& e) {
    snprintf(errorMessage, ERR_LEN, "%s => Test FAILED: %s", protoname, e.what());
    throw TLibraryException(errorMessage);
  }
}
}
}
}

#endif
