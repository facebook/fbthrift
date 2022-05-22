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
include "thrift/lib/thrift/any_rep.thrift"

namespace cpp2 apache.thrift.type
namespace py3 apache.thrift.type
namespace php apache_thrift_type
namespace java.swift com.facebook.thrift.type
namespace java com.facebook.thrift.type
namespace java2 com.facebook.thrift.type
namespace py.asyncio apache_thrift_asyncio.any
namespace go thrift.lib.thrift.any
namespace py thrift.lib.thrift.any

// A list of Any values that can be indexed by its generated ValueId.
//
// For example, the index of a value can be use as a ValueId in any
// language where `noId` maps to ~`std::stirng::npos`.
@thrift.Experimental
typedef list<any_rep.AnyStruct> AnyValueList
