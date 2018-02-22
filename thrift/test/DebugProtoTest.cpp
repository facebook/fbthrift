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

#include <iostream>
#include <cmath>
#include <thrift/test/gen-cpp/DebugProtoTest_types.h>
#include <thrift/test/gen-cpp/DebugProtoTest_constants.h>
#include <thrift/test/gen-cpp2/DebugProtoTest_types_custom_protocol.h>
#include <thrift/test/gen-cpp2/DebugProtoTest_constants.h>
#include <thrift/lib/cpp/protocol/TDebugProtocol.h>
#include <thrift/lib/cpp2/protocol/DebugProtocol.h>


int main() {
  using std::cout;
  using std::endl;
  using namespace thrift::test::debug;

  cout << "CPP1\n";

  // CPP1
#define X_PRINT apache::thrift::ThriftDebugString
#define X_NS
#include "thrift/test/DebugProtoTest.tcc"
#undef X_NS
#undef X_PRINT

  cout << "CPP2\n";

  // CPP2
#define X_PRINT apache::thrift::debugString
#define X_NS cpp2::
#include "thrift/test/DebugProtoTest.tcc"
#undef X_NS
#undef X_PRINT

  return 0;
}
