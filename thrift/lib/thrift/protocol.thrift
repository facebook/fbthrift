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

// TODO(ytj): merge this file into thrift/lib/thrift/type.thrift

include "thrift/lib/thrift/protocol_detail.thrift"
include "thrift/lib/thrift/id.thrift"
cpp_include "folly/container/F14Map.h"

package "facebook.com/thrift/protocol"

namespace cpp2 apache.thrift.protocol
namespace py3 apache.thrift.protocol
namespace php apache_thrift_protocol
namespace java com.facebook.thrift.protocol
namespace java.swift com.facebook.thrift.protocol_swift
namespace py.asyncio apache_thrift_asyncio.protocol
namespace go thrift.lib.thrift.protocol
namespace py thrift.lib.thrift.protocol

typedef protocol_detail.Object Object (thrift.uri = "")
typedef protocol_detail.Value Value (thrift.uri = "")
typedef map<i16, Mask> (cpp.template = "folly::F14VectorMap") FieldIdToMask

typedef id.ExternId PathSegmentId // TODO(ytj): add adapter

struct Path {
  1: list<PathSegmentId> path;
}

// Inclusive fields should always be an even number.
union Mask {
  1: FieldIdToMask exclusive; // Fields that will be excluded.
  2: FieldIdToMask inclusive; // Fields that will be included.
}
