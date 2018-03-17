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

#include <thrift/lib/cpp2/fatal/container_traits.h>
#include <thrift/lib/cpp2/fatal/reflection.h>

#include <thrift/lib/cpp2/fatal/demo/json_print.h>

#include <thrift/lib/cpp2/fatal/demo/gen-cpp2/flat_config_constants.h>
#include <thrift/lib/cpp2/fatal/demo/gen-cpp2/flat_config_fatal_types.h>
#include <thrift/lib/cpp2/fatal/demo/gen-cpp2/legacy_config_constants.h>

#include <folly/Conv.h>
#include <folly/init/Init.h>

#include <iostream>
#include <string>

using namespace apache::thrift;
using namespace static_reflection::demo;

struct get_property {
  template <typename Member>
  using apply = typename Member::annotations::values::property;
};

struct legacy_to_flat_translator {
  template <typename Member>
  void operator()(fatal::tag<Member>, std::string const& from, flat_config& to)
      const {
    auto& value = Member::getter::ref(to);
    value = folly::to<typename Member::type>(from);
  }
};

void translate(legacy_config const& from, flat_config& to) {
  for (auto const& i : from) {
    fatal::trie_find<reflect_struct<flat_config>::members, get_property>(
        i.first.begin(),
        i.first.end(),
        legacy_to_flat_translator(),
        i.second,
        to);
  }
}

struct flat_to_legacy_translator {
  template <typename Member, std::size_t Index>
  void operator()(
      fatal::indexed<Member, Index>,
      flat_config const& from,
      legacy_config& to) {
    using property = typename Member::annotations::values::property;
    auto const key = fatal::z_data<property>();
    auto const& value = Member::getter::ref(from);
    to[key] = folly::to<std::string>(value);
  }
};

void translate(flat_config const& from, legacy_config& to) {
  using members = reflect_struct<flat_config>::members;

  fatal::foreach<members>(flat_to_legacy_translator(), from, to);
}

template <typename To, typename From>
void test(From const& from) {
  To to;
  translate(from, to);
  print(from);
  print(to);
}

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  std::cerr << "legacy -> flat: ";
  test<static_reflection::demo::flat_config>(
      static_reflection::demo::legacy_config_constants::example());

  std::cerr << "flat -> legacy: ";
  test<static_reflection::demo::legacy_config>(
      static_reflection::demo::flat_config_constants::example());

  return 0;
}
