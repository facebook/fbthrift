/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/conformance/data/RPCGenerator.h>

namespace apache::thrift::conformance::data {

namespace {
Test createRequestResponseBasicTest() {
  Test ret;
  ret.name() = "RequestResponseBasicTest";

  auto& testCase = ret.testCases()->emplace_back();
  testCase.name() = "RequestResponseBasic/Success";

  auto& rpcTest = testCase.rpc_ref().emplace();
  rpcTest.clientInstruction_ref()
      .emplace()
      .requestResponseBasic_ref()
      .emplace()
      .request()
      .emplace()
      .data() = "hello";
  rpcTest.clientTestResult_ref()
      .emplace()
      .requestResponseBasic_ref()
      .emplace()
      .response()
      .emplace()
      .data() = "world";

  rpcTest.serverInstruction_ref()
      .emplace()
      .requestResponseBasic_ref()
      .emplace()
      .response()
      .emplace()
      .data() = "world";
  rpcTest.serverTestResult_ref()
      .emplace()
      .requestResponseBasic_ref()
      .emplace()
      .request()
      .emplace()
      .data() = "hello";

  return ret;
}

Test createRequestResponseDeclaredExceptionTest() {
  Test ret;
  ret.name() = "RequestResponseDeclaredExceptionTest";

  auto& testCase = ret.testCases()->emplace_back();
  testCase.name() = "RequestResponseDeclaredException/Success";

  UserException userException;
  userException.msg() = "world";

  auto& rpcTest = testCase.rpc_ref().emplace();
  rpcTest.clientInstruction_ref()
      .emplace()
      .requestResponseDeclaredException_ref()
      .emplace()
      .request()
      .emplace()
      .data() = "hello";
  rpcTest.clientTestResult_ref()
      .emplace()
      .requestResponseDeclaredException_ref()
      .emplace()
      .userException() = userException;

  rpcTest.serverInstruction_ref()
      .emplace()
      .requestResponseDeclaredException_ref()
      .emplace()
      .userException() = userException;
  rpcTest.serverTestResult_ref()
      .emplace()
      .requestResponseDeclaredException_ref()
      .emplace()
      .request()
      .emplace()
      .data() = "hello";

  return ret;
}

Test createRequestResponseUndeclaredExceptionTest() {
  Test ret;
  ret.name() = "RequestResponseUndeclaredExceptionTest";

  auto& testCase = ret.testCases()->emplace_back();
  testCase.name() = "RequestResponseUndeclaredException/Success";

  auto& rpcTest = testCase.rpc_ref().emplace();
  rpcTest.clientInstruction_ref()
      .emplace()
      .requestResponseUndeclaredException_ref()
      .emplace()
      .request()
      .emplace()
      .data() = "hello";
  rpcTest.clientTestResult_ref()
      .emplace()
      .requestResponseUndeclaredException_ref()
      .emplace()
      .exceptionMessage() = "my undeclared exception";

  rpcTest.serverInstruction_ref()
      .emplace()
      .requestResponseUndeclaredException_ref()
      .emplace()
      .exceptionMessage() = "my undeclared exception";
  rpcTest.serverTestResult_ref()
      .emplace()
      .requestResponseUndeclaredException_ref()
      .emplace()
      .request()
      .emplace()
      .data() = "hello";

  return ret;
}

Test createRequestResponseNoArgVoidResponse() {
  Test ret;
  ret.name() = "RequestResponseNoArgVoidResponseTest";

  auto& testCase = ret.testCases()->emplace_back();
  testCase.name() = "RequestResponseNoArgVoidResponse/Success";

  auto& rpcTest = testCase.rpc_ref().emplace();
  rpcTest.clientInstruction_ref()
      .emplace()
      .requestResponseNoArgVoidResponse_ref()
      .emplace();
  rpcTest.clientTestResult_ref()
      .emplace()
      .requestResponseNoArgVoidResponse_ref()
      .emplace();

  rpcTest.serverInstruction_ref()
      .emplace()
      .requestResponseNoArgVoidResponse_ref()
      .emplace();
  rpcTest.serverTestResult_ref()
      .emplace()
      .requestResponseNoArgVoidResponse_ref()
      .emplace();

  return ret;
}

} // namespace

TestSuite createRPCTestSuite() {
  TestSuite suite;
  suite.name() = "ThriftRPCTest";
  suite.tests()->push_back(createRequestResponseBasicTest());
  suite.tests()->push_back(createRequestResponseDeclaredExceptionTest());
  suite.tests()->push_back(createRequestResponseUndeclaredExceptionTest());
  suite.tests()->push_back(createRequestResponseNoArgVoidResponse());
  return suite;
}

} // namespace apache::thrift::conformance::data
