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

include "thrift/annotation/scope.thrift"

package "facebook.com/thrift/annotation/cpp"

namespace cpp2 facebook.thrift.annotation.cpp
namespace java com.facebook.thrift.annotation.cpp_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.cpp
namespace go thrift.annotation.cpp
namespace py thrift.annotation.cpp

enum RefType {
  Unique = 0,
  Shared = 1,
  SharedMutable = 2,
}

@scope.Field
struct Ref {
  1: RefType type;
} (thrift.uri = "facebook.com/thrift/annotation/cpp/Ref")

@scope.Field
struct Lazy {
  // Use std::unique_ptr<folly::IOBuf> instead of folly::IOBuf to store serialized data.
  1: bool ref = false;
} (thrift.uri = "facebook.com/thrift/annotation/cpp/Lazy")

@scope.Struct
struct DisableLazyChecksum {} (
  thrift.uri = "facebook.com/thrift/annotation/cpp/DisableLazyChecksum",
)

// An annotation that applies a C++ adapter to typedef or field. For example:
//
//   @cpp.Adapter{name = "IdAdapter"}
//   typedef i64 MyI64;
//
// Here the type 'MyI64' has the C++ adapter `IdAdapter`.
//
//   struct User {
//     @cpp.Adapter{name = "IdAdapter"}
//     1: i64 id;
//   }
//
// Here the field `id` has the C++ adapter `IdAdapter`.
@scope.Field
@scope.Typedef
struct Adapter {
  // The name of a C++ adapter type used to convert between Thrift and native
  // C++ representation. The Adapter either Type adapter of Field adapter that
  // provides following APIs
  //
  //  struct ThriftTypeAdapter {
  //    static AdaptedType fromThrift(ThriftType thrift);
  //    static {const ThriftType& | ThriftType} toThrift(const AdaptedType& native);
  //  };
  //
  //  struct ThriftFieldAdapter {
  //    // Context is an instantiation of apache::thrift::FieldAdapterContext
  //    template <class Context>
  //    static void construct(AdaptedType& field, Context ctx);
  //
  //    template <class Context>
  //    static AdaptedType fromThriftField(ThriftType value, Context ctx);
  //
  //    template <class Context>
  //    static {const ThriftType& | ThriftType} toThrift(const AdaptedType& adapted, Context ctx);
  //  };
  1: string name;
} (thrift.uri = "facebook.com/thrift/annotation/cpp/Adapter")

@scope.Struct
struct PackIsset {
  1: bool atomic = true;
} (thrift.uri = "facebook.com/thrift/annotation/cpp/PackIsset")

@scope.Struct
struct MinimizePadding {} (
  thrift.uri = "facebook.com/thrift/annotation/cpp/MinimizePadding",
)

@scope.Struct
struct TriviallyRelocatable {} (
  thrift.uri = "facebook.com/thrift/annotation/cpp/TriviallyRelocatable",
)
