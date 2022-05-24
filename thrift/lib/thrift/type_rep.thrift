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
include "thrift/annotation/java.thrift"
include "thrift/lib/thrift/id.thrift"

cpp_include "<folly/io/IOBuf.h>"
cpp_include "<folly/FBString.h>"

// Canonical underlying representations for well-known Thrift types.
package "facebook.com/thrift/type"

namespace cpp2 apache.thrift.type
namespace py3 apache.thrift.type
namespace php apache_thrift_type
namespace java com.facebook.thrift.type
namespace java.swift com.facebook.thrift.type_swift
namespace java2 com.facebook.thrift.type
namespace py.asyncio apache_thrift_asyncio.type_rep
namespace go thrift.lib.thrift.type_rep
namespace py thrift.lib.thrift.type_rep

// The minimum and default number of bytes that can be used to identify
// a type.
//
// The expected number of types that can be hashed before a
// collision is 2^(8*{numBytes}/2).
// Which is ~4.3 billion types for the min, and ~18.45 quintillion
// types for the default.
@thrift.Experimental
const byte minTypeHashBytes = 8;
@thrift.Experimental
const byte defaultTypeHashBytes = 16;

// Typedef for binary data which can be represented as a string of 8-bit bytes
//
// Each language can map this type into a customized memory efficient object
@thrift.Experimental
@java.Adapter{
  adapterClassName = "com.facebook.thrift.adapter.common.UnpooledByteBufTypeAdapter",
  typeClassName = "io.netty.buffer.ByteBuf",
}
typedef binary (cpp2.type = "folly::fbstring") ByteString

// Typedef for binary data
//
// Each language can map this type into a customized memory efficient object
// May be used for zero-copy slice of data
@thrift.Experimental
@java.Adapter{
  adapterClassName = "com.facebook.thrift.adapter.common.UnpooledByteBufTypeAdapter",
  typeClassName = "io.netty.buffer.ByteBuf",
}
typedef binary (cpp2.type = "folly::IOBuf") ByteBuffer

// A fixed-length span of time, represented as a signed count of seconds and
// nanoseconds (nanos).
//
// Considered 'normalized', when `nanos` is in the range 0 to 999'999'999
// inclusive, or `seconds` is 0 and `nanos` is in the range -999'999'999 to
// 999'999'999 inclusive.
//
// TODO(afuller): Adapt to appropriate native types.
@thrift.Experimental
struct Duration {
  // The count of seconds.
  1: i64 seconds;
  // The count of nanoseconds.
  2: i32 nanos;
}

// An instant in time encoded as a count of seconds and nanoseconds (nanos)
// since midnight on January 1, 1970 UTC (i.e. Unix epoch).
//
// Considered 'normalized', when `nanos` is in the range 0 to 999'999'999
// inclusive.
//
// TODO(afuller): Adapt to appropriate native types.
// TODO(afuller): Consider making this a 'strong' typedef of `Duration`, which
// would ensure both a separate URI and native type in all languages.
@thrift.Experimental
struct Time {
  // The count of seconds.
  1: i64 seconds;
  // The count of nanoseconds.
  2: i32 nanos;
}

// An 'internet timestamp' as described in [RFC 3339](https://www.ietf.org/rfc/rfc3339.txt)
//
// Similar to `Time`, but can only represent values in the range
// 0001-01-01T00:00:00Z to 9999-12-31T23:59:59Z inclusive, for compatibility
// with the 'date string' format. Thus `seconds` must be in the range
// -62'135'769'600 to 253'402'300'799 inclusive, when normalized.
//
// TODO(afuller): Add extra validation when adapting to/from native types.
@thrift.Experimental
typedef Time Timestamp

// Standard protocols.
@thrift.Experimental
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
@thrift.Experimental
union ProtocolUnion {
  1: StandardProtocol standard;
  2: string custom;
} (thrift.uri = "facebook.com/thrift/type/Protocol")

// TODO(afuller): Allow 'void' type for union fields.
@thrift.Experimental
enum Void {
  NoValue = 0,
}

// A integer fraction of the form {numerator} / {denominator}
//
// Useful for representing ratios, rates, and metric accumulators.
//
// Considered 'normalized' when the denominator is positive.
//
// TODO(afuller): Add a wrapper that ensures the in memory form is always
// normalized.
@thrift.Experimental
struct Fraction {
  // The numerator/dividend/upper number of the fraction.
  1: i64 numerator;
  // The denominator/divisor/lower number of the fraction.
  2: i64 denominator;
}

// A (scheme-less) URI.
//
// Identical to RFC 3986, but with every component optional.
@thrift.Experimental // TODO(afuller): Adapt.
typedef string Uri

// The uri of an IDL defined type.
@thrift.Experimental
union TypeUri {
  // The unique Thrift URI for this type.
  1: Uri uri;
  // A prefix of the SHA2-256 hash of the URI.
  2: ByteString typeHashPrefixSha2_256;
  // An externally stored URI value.
  3: id.ValueId id;
}

// Uniquely identifies a Thrift type.
@thrift.Experimental
union TypeName {
  1: Void boolType;
  2: Void byteType;
  3: Void i16Type;
  4: Void i32Type;
  5: Void i64Type;
  6: Void floatType;
  7: Void doubleType;
  8: Void stringType;
  9: Void binaryType;
  10: TypeUri enumType;
  11: TypeUri structType;
  12: TypeUri unionType;
  13: TypeUri exceptionType;
  14: Void listType;
  15: Void setType;
  16: Void mapType;
}

// A concrete Thrift type.
struct TypeStruct {
  // The type name.
  1: TypeName name;
  // The type params, if appropriate.
  2: list<TypeStruct> params;
} (thrift.uri = "facebook.com/thrift/type/Type")
