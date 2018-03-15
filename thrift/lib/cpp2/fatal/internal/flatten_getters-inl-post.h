/*
 * Copyright 2016 Facebook, Inc.
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
#ifndef THRIFT_FATAL_FLATTEN_GETTERS_INL_POST_H_
#define THRIFT_FATAL_FLATTEN_GETTERS_INL_POST_H_ 1

#include <fatal/type/apply.h>
#include <fatal/type/cat.h>

namespace apache {
namespace thrift {
namespace detail {
namespace flatten_getters_impl {

// member handler: non-terminal non-struct
template <
  typename, typename TerminalFilter, typename Member,
  typename = typename Member::type_class,
  bool = TerminalFilter::template apply<Member>::value
>
struct m {
  using type = fatal::list<>;
};

// member handler: terminal non-struct
template <
  typename Path, typename TerminalFilter, typename Member, typename TypeClass
>
struct m<Path, TerminalFilter, Member, TypeClass, true> {
  using type = fatal::list<flat_getter<Path, Member>>;
};

// member handler: terminal struct
template <typename Path, typename TerminalFilter, typename Member>
struct m<Path, TerminalFilter, Member, type_class::structure, true> {
  using type = fatal::list<flat_getter<Path, Member>>;
};

// member handler: non-terminal struct
template <typename Path, typename TerminalFilter, typename Member>
struct m<Path, TerminalFilter, Member, type_class::structure, false>:
  s<
    fatal::push_back<Path, Member>,
    TerminalFilter,
    reflect_struct<typename Member::type>
  >
{};

// currying of the member handler
template <typename Path, typename TerminalFilter>
struct c {
  template<typename Member>
  using apply = typename m<Path, TerminalFilter, Member>::type;
};

// structure handler
template <typename Path, typename TerminalFilter, typename Info>
struct s {
  using type = fatal::apply_to<
    fatal::transform<typename Info::members, c<Path, TerminalFilter>>,
    fatal::cat
  >;
};

} // namespace flatten_getters_impl
} // namespace detail
} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_FLATTEN_GETTERS_INL_POST_H_
