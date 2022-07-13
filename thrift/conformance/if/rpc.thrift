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
namespace java2 org.apache.thrift.conformance

struct RequestResponseTestDescription {
  1: optional Request request; // optional so we can also test no argument RPCs
  2: optional Response response; // optional so we can also test void return type RPCs
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
  1: RequestResponseServerTestResult requestResponse;
}

struct RequestResponseServerTestResult {
  1: optional Request request;
}
