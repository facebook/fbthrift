/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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
namespace php thrift
namespace py thrift.conformance.protocol
namespace py.asyncio thrift_asyncio.conformance.protocol
namespace py3 thrift.conformance
namespace java.swift org.apache.thrift.conformance
namespace go thrift.conformance.protocol

// Standard protocols.
enum StandardProtocol {
    Binary = 1,
    Compact = 2,
    Json = 3,
    SimpleJson = 4;
}

// An encoding protocol.
union Protocol {
  // A standard protocol.
  1: StandardProtocol standard;
  // The unique name for a custom protocol.
  2: string custom;
}
