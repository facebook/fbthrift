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
  testCase.name() = "RequestResponse/Success";

  auto& requestResponseBasicTestCase =
      testCase.test()->requestResponse_ref().emplace().basic_ref().emplace();
  requestResponseBasicTestCase.request().emplace().data() = "hello";
  requestResponseBasicTestCase.response().emplace().data() = "world";

  return ret;
}
} // namespace

TestSuite createRPCTestSuite() {
  TestSuite suite;
  suite.name() = "ThriftRPCTest";
  suite.tests()->push_back(createRequestResponseBasicTest());
  return suite;
}

} // namespace apache::thrift::conformance::data
