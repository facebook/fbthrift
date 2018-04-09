/*
 * Copyright 2016-present Facebook, Inc.
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
#ifndef THRIFT_FATAL_FLATTEN_GETTERS_H_
#define THRIFT_FATAL_FLATTEN_GETTERS_H_ 1

#include <fatal/type/data_member_getter.h>
#include <fatal/type/get_type.h>
#include <fatal/type/push.h>
#include <fatal/type/transform.h>
#include <thrift/lib/cpp2/fatal/reflection.h>

#include <thrift/lib/cpp2/fatal/internal/flatten_getters-inl-pre.h>

namespace apache {
namespace thrift {

template <typename Path, typename Member>
struct flat_getter {
  using path = Path;
  using member = Member;
  using full_path = fatal::push_back<Path, Member>;

  using getter = fatal::apply_to<
      fatal::transform<full_path, fatal::get_type::getter>,
      fatal::chained_data_member_getter>;
};

template <typename T, typename TerminalFilter = detail::flatten_getters_impl::f>
using flatten_getters = typename detail::flatten_getters_impl::
    s<fatal::list<>, TerminalFilter, reflect_struct<T>>::type;

} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/fatal/internal/flatten_getters-inl-post.h>

#endif // THRIFT_FATAL_FLATTEN_GETTERS_H_
