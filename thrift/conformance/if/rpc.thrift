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

namespace cpp2 apache.thrift.conformance
namespace php apache_thrift
namespace py thrift.conformance.rpc
namespace py.asyncio thrift_asyncio.conformance.rpc
namespace py3 thrift.conformance
namespace java.swift org.apache.thrift.conformance

struct RpcTestCase {
  1: ClientInstruction clientInstruction;
  2: ClientTestResult clientTestResult;
  3: ServerInstruction serverInstruction;
  4: ServerTestResult serverTestResult;
}

struct Request {
  1: string data;
  2: optional i32 num;
}

struct Response {
  1: string data;
  2: optional i32 num;
}

union ServerTestResult {
  1: RequestResponseBasicServerTestResult requestResponseBasic;
}

union ClientTestResult {
  1: RequestResponseBasicClientTestResult requestResponseBasic;
}

struct RequestResponseBasicServerTestResult {
  1: Request request;
}

struct RequestResponseBasicClientTestResult {
  1: Response response;
}

union ClientInstruction {
  1: RequestResponseBasicClientInstruction requestResponseBasic;
}

union ServerInstruction {
  1: RequestResponseBasicServerInstruction requestResponseBasic;
}

struct RequestResponseBasicClientInstruction {
  1: Request request;
}

struct RequestResponseBasicServerInstruction {
  1: Response response;
}
