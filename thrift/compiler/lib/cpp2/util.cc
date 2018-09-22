/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <algorithm>
#include <unordered_set>

#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/util.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <thrift/compiler/ast/t_list.h>
#include <thrift/compiler/ast/t_map.h>
#include <thrift/compiler/ast/t_set.h>
#include <thrift/compiler/ast/t_struct.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace cpp2 {

static bool is_dot(char const c) {
  return c == '.';
}

std::vector<std::string> get_gen_namespace_components(
    t_program const& program) {
  auto const& cpp2 = program.get_namespace("cpp2");
  auto const& cpp = program.get_namespace("cpp");

  std::vector<std::string> components;

  if (!cpp2.empty()) {
    boost::algorithm::split(components, cpp2, is_dot);
  } else if (!cpp.empty()) {
    boost::algorithm::split(components, cpp, is_dot);
    components.push_back("cpp2");
  } else {
    components.push_back("cpp2");
  }

  return components;
}

std::string get_gen_namespace(t_program const& program) {
  return boost::algorithm::join(get_gen_namespace_components(program), "::");
}

static bool is_orderable(
    std::unordered_set<t_type const*>& seen,
    t_type const& type) {
  auto has_disqualifying_annotation = [](auto& t) {
    static auto const& keys = *new std::vector<std::string>{
        "cpp.template",
        "cpp2.template",
        "cpp.type",
        "cpp2.type",
    };
    return std::any_of(keys.begin(), keys.end(), [&](auto key) {
      return t.annotations_.count(key);
    });
  };
  if (!seen.insert(&type).second) {
    return true;
  }
  auto g = make_scope_guard([&] { seen.erase(&type); });
  // TODO: Consider why typedef is not resolved in this method
  if (type.is_base_type()) {
    return true;
  }
  if (type.is_enum()) {
    return true;
  }
  if (type.is_typedef()) {
    auto const& real = *type.get_true_type();
    auto const& next = *(dynamic_cast<t_typedef const&>(type).get_type());
    return is_orderable(seen, next) &&
        (!(real.is_set() || real.is_map()) ||
         !has_disqualifying_annotation(type));
  }
  if (type.is_struct() || type.is_xception()) {
    auto& members = dynamic_cast<t_struct const&>(type).get_members();
    return std::all_of(members.begin(), members.end(), [&](auto f) {
      return is_orderable(seen, *(f->get_type()));
    });
  }
  if (type.is_list()) {
    return is_orderable(
        seen, *(dynamic_cast<t_list const&>(type).get_elem_type()));
  }
  if (type.is_set()) {
    return !has_disqualifying_annotation(type) &&
        is_orderable(seen, *(dynamic_cast<t_set const&>(type).get_elem_type()));
  }
  if (type.is_map()) {
    return !has_disqualifying_annotation(type) &&
        is_orderable(
            seen, *(dynamic_cast<t_map const&>(type).get_key_type())) &&
        is_orderable(seen, *(dynamic_cast<t_map const&>(type).get_val_type()));
  }
  return false;
}

bool is_orderable(t_type const& type) {
  std::unordered_set<t_type const*> seen;
  return is_orderable(seen, type);
}

} // namespace cpp2
} // namespace compiler
} // namespace thrift
} // namespace apache
