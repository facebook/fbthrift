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

#pragma once

#include <thrift/lib/cpp2/Thrift.h>

namespace apache::thrift::protocol::detail {
// Teach cpp.indirection how to convert custom struct to thrift struct
template <class From, class To>
struct converter {
  To& operator()(From& v) const { return v; }
  const To& operator()(const From& v) const { return v; }
};

class ObjectStruct;
class ValueUnion;

template <class Base = ObjectStruct>
class ObjectWrapper;
template <class Base = ValueUnion>
class ValueWrapper;

using Object = ObjectWrapper<ObjectStruct>;
using Value = ValueWrapper<ValueUnion>;

template <class Base>
class ObjectWrapper : public Base {
 private:
  static_assert(std::is_same_v<Base, ObjectStruct>);
  friend struct ::apache::thrift::detail::st::struct_private_access;
  static const char* __fbthrift_thrift_uri();

 public:
  using Base::Base;
  using __fbthrift_cpp2_indirection_fn = detail::converter<ObjectWrapper, Base>;

  // TODO(ytj): Provide boost.json.value like APIs
  // www.boost.org/doc/libs/release/libs/json/doc/html/json/ref/boost__json__object.html

  size_t size() const { return Base::members()->size(); }
};

template <class Base>
class ValueWrapper : public Base {
 private:
  static_assert(std::is_same_v<Base, ValueUnion>);
  friend struct ::apache::thrift::detail::st::struct_private_access;
  static const char* __fbthrift_thrift_uri();

 public:
  using Base::Base;
  using __fbthrift_cpp2_indirection_fn = detail::converter<ValueWrapper, Base>;

  // TODO(ytj): Provide boost.json.value like APIs
  // www.boost.org/doc/libs/release/libs/json/doc/html/json/ref/boost__json__value.html

  std::string& emplace_string() { return Base::stringValue_ref().ensure(); }
};

} // namespace apache::thrift::protocol::detail
