/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef THRIFT_CPP2_H_
#define THRIFT_CPP2_H_

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/protocol/TType.h>

namespace apache { namespace thrift {

enum FragileConstructor {
  FRAGILE,
};

namespace detail {

template <typename T>
struct enum_hash {
  size_t operator()(T t) const {
    using underlying_t = typename std::underlying_type<T>::type;
    return std::hash<underlying_t>()(underlying_t(t));
  }
};

template <typename T>
struct enum_equal_to {
  bool operator()(T t0, T t1) const { return t0 == t1; }
};

}

/**
 * Class template (specialized for each type in generated code) that allows
 * access to write / read / serializedSize / serializedSizeZC functions in
 * a generic way.
 *
 * For native Cpp2 structs, one could call the corresponding methods
 * directly, but structs generated in compatibility mode (ie. typedef'ed
 * to the Thrift1 version) don't have them; they are defined as free
 * functions named <type>_read, <type>_write, etc, so they can't be accessed
 * generically (because the type name is part of the function name).
 *
 * Cpp2Ops bridges to either struct methods (for native Cpp2 structs)
 * or the corresponding free functions (for structs in compatibility mode).
 */
template <class T, class = void>
class Cpp2Ops {
 public:
  static void clear(T* );

  template <class P>
  static uint32_t write(P*, const T*);

  template <class P>
  static uint32_t read(P*, T*);

  template <class P>
  static uint32_t serializedSize(P*, const T*);

  template <class P>
  static uint32_t serializedSizeZC(P*, const T*);

  static constexpr apache::thrift::protocol::TType thriftType();
};

}} // apache::thrift

#endif // #ifndef THRIFT_CPP2_H_
