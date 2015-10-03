/*
 * Copyright 2015 Facebook, Inc.
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
#ifndef THRIFT_FATAL_FOLLY_DYNAMIC_H_
#define THRIFT_FATAL_FOLLY_DYNAMIC_H_ 1

#include <thrift/lib/cpp2/fatal/reflect_category.h>
#include <thrift/lib/cpp2/fatal/reflection.h>

#include <folly/dynamic.h>

#include <type_traits>
#include <utility>

namespace apache { namespace thrift {
namespace detail {
template <thrift_category> struct dynamic_converter_impl;
} // namespace detail {

/**
 * Converts an object to its `folly::dynamic` representation using Thrift's
 * reflection support.
 *
 * All Thrift types are required to be generated using the 'fatal' cpp2 flag,
 * otherwise compile-time reflection metadata won't be available.
 *
 * The root object is output to the given `folly::dynamic` output parameter.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
void to_dynamic(folly::dynamic &out, T &&what) {
  using impl = detail::dynamic_converter_impl<
    reflect_category<typename std::decay<T>::type>::value
  >;

  impl::to(out, std::forward<T>(what));
}

/**
 * Converts an object to its `folly::dynamic` representation using Thrift's
 * reflection support.
 *
 * All Thrift types are required to be generated using the 'fatal' cpp2 flag,
 * otherwise compile-time reflection metadata won't be available.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
folly::dynamic to_dynamic(T &&what) {
  folly::dynamic result(folly::dynamic::object);

  to_dynamic(result, std::forward<T>(what));

  return result;
}

}} // apache::thrift

#include <thrift/lib/cpp2/fatal/folly_dynamic-inl.h>

#endif // THRIFT_FATAL_FOLLY_DYNAMIC_H_
