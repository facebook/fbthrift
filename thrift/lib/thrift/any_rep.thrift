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

include "thrift/annotation/thrift.thrift"
include "thrift/lib/thrift/type.thrift"
include "thrift/lib/thrift/type_rep.thrift"

namespace cpp2 apache.thrift.type
namespace py3 apache.thrift.type
namespace php apache_thrift_type
namespace java com.facebook.thrift.type
namespace java.swift com.facebook.thrift.type_swift
namespace java2 com.facebook.thrift.type
namespace py.asyncio apache_thrift_asyncio.any_rep
namespace go thrift.lib.thrift.any_rep
namespace py thrift.lib.thrift.any_rep

// A struct that can hold any thrift supported value, encoded in any format.
@thrift.Experimental
struct AnyStruct {
  // The type stored in `data`.
  //
  // Must not be empty.
  1: type.Type type;

  // The protocol used to encode `data`.
  //
  // Must not be empty.
  2: type.Protocol protocol;

  // The encoded data.
  3: type_rep.ByteBuffer data;
} (thrift.uri = "facebook.com/thrift/type/Any")

// Like Any, except type and protocol can be empty.
//
// Can be upgraded to an Any after all the field are populated.
@thrift.Experimental
struct SemiAnyStruct {
  // The type stored in `data`, if known.
  1: type.Type type;

  // The protocol used to encode `data`, if known.
  2: type.Protocol protocol;

  // The encoded data.
  3: type_rep.ByteBuffer data;
} (thrift.uri = "facebook.com/thrift/type/SemiAny")
