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

cpp_include "<folly/io/IOBuf.h>"
cpp_include "<folly/FBString.h>"

namespace cpp2 apache.thrift.type
namespace py3 apache.thrift.type
namespace php apache_thrift_type
namespace java.swift com.facebook.thrift.type
namespace java com.facebook.thrift.type
namespace java2 com.facebook.thrift.type
namespace py.asyncio apache_thrift_asyncio.type_rep
namespace go thrift.lib.thrift.type_rep
namespace py thrift.lib.thrift.type_rep

// Typedef for binary data which can be represented as a string of 8-bit bytes
//
// Each language can map this type into a customized memory efficient object
@thrift.Experimental
typedef binary (cpp2.type = "folly::fbstring") ByteString

// Typedef for binary data
//
// Each language can map this type into a customized memory efficient object
// May be used for zero-copy slice of data
@thrift.Experimental
typedef binary (cpp2.type = "folly::IOBuf") ByteBuffer
