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

namespace cpp2 apache.thrift.type
namespace py3 apache.thrift.type
namespace php apache_thrift_type
namespace java.swift com.facebook.thrift.type
namespace java com.facebook.thrift.type
namespace java2 com.facebook.thrift.type
namespace py.asyncio apache_thrift_asyncio.type_rep
namespace go thrift.lib.thrift.type_rep
namespace py thrift.lib.thrift.type_rep

// Standard protocols.
enum StandardProtocol {
  Custom = 0,

  // Standard protocols.
  Binary = 1,
  Compact = 2,

  // Deprecated protocols.
  Json = 3,
  SimpleJson = 4,
}

// A union representation of a protocol.
union ProtocolUnion {
  1: StandardProtocol standard;
  2: string custom;
} (thrift.uri = "facebook.com/thrift/type/Protocol")
