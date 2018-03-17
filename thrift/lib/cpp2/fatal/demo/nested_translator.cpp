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

#include <thrift/lib/cpp2/fatal/flatten_getters.h>
#include <thrift/lib/cpp2/fatal/reflection.h>

#include <thrift/lib/cpp2/fatal/demo/json_print.h>

#include <thrift/lib/cpp2/fatal/demo/gen-cpp2/flat_config_constants.h>
#include <thrift/lib/cpp2/fatal/demo/gen-cpp2/flat_config_fatal_types.h>
#include <thrift/lib/cpp2/fatal/demo/gen-cpp2/nested_config_constants.h>
#include <thrift/lib/cpp2/fatal/demo/gen-cpp2/nested_config_fatal_types.h>

#include <iostream>

using namespace apache::thrift;
using namespace static_reflection::demo;

struct flat_to_nested_translator {
  template <typename Nested, std::size_t Index>
  void operator ()(
    fatal::indexed<Nested, Index>,
    flat_config const &from,
    nested_config &to
  ) const {
    using from_getter = fatal::get<
      reflect_struct<flat_config>::members,
      typename Nested::member::annotations::values::from_flat,
      fatal::get_type::name
    >;

    auto &to_member = Nested::getter::ref(to);
    auto const &from_member = from_getter::getter::ref(from);
    to_member = from_member;
  }
};

void translate(flat_config const &from, nested_config &to) {
  using nested_getters = flatten_getters<nested_config>;

  fatal::foreach<nested_getters>(flat_to_nested_translator(), from, to);
}

struct nested_to_flat_translator {
  template <typename Nested, std::size_t Index>
  void operator ()(
    fatal::indexed<Nested, Index>,
    nested_config const &from,
    flat_config &to
  ) const {
    using to_getter = fatal::get<
      reflect_struct<flat_config>::members,
      typename Nested::member::annotations::values::from_flat,
      fatal::get_type::name
    >;

    auto &to_member = to_getter::getter::ref(to);
    auto const &from_member = Nested::getter::ref(from);
    to_member = from_member;
  }
};

void translate(nested_config const &from, flat_config &to) {
  using nested_getters = flatten_getters<nested_config>;

  fatal::foreach<nested_getters>(nested_to_flat_translator(), from, to);
}

template <typename To, typename From>
void test(From const &from) {
  To to;
  translate(from, to);
  print(from);
  print(to);
}

int main() {
  std::cerr << "nested -> flat: ";
  test<static_reflection::demo::flat_config>(
    static_reflection::demo::nested_config_constants::example()
  );

  std::cerr << "flat -> nested: ";
  test<static_reflection::demo::nested_config>(
    static_reflection::demo::flat_config_constants::example()
  );

  return 0;
}
