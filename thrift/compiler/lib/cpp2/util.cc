/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <algorithm>

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
  auto const components = get_gen_namespace_components(program);
  return "::" + boost::algorithm::join(components, "::");
}

bool is_orderable(
    std::unordered_set<t_type const*>& seen,
    std::unordered_map<t_type const*, bool>& memo,
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
  auto memo_it = memo.find(&type);
  if (memo_it != memo.end()) {
    return memo_it->second;
  }
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
  bool result = false;
  auto g2 = make_scope_guard([&] { memo[&type] = result; });
  if (type.is_typedef()) {
    auto const& real = [&]() -> auto&& {
      return *type.get_true_type();
    };
    auto const& next = *(dynamic_cast<t_typedef const&>(type).get_type());
    return result = is_orderable(seen, memo, next) &&
        (!(real().is_set() || real().is_map()) ||
         !has_disqualifying_annotation(type));
  }
  if (type.is_struct() || type.is_xception()) {
    auto& members = dynamic_cast<t_struct const&>(type).get_members();
    return result = std::all_of(members.begin(), members.end(), [&](auto f) {
             return is_orderable(seen, memo, *(f->get_type()));
           });
  }
  if (type.is_list()) {
    return result = is_orderable(
               seen,
               memo,
               *(dynamic_cast<t_list const&>(type).get_elem_type()));
  }
  if (type.is_set()) {
    return result = !has_disqualifying_annotation(type) &&
        is_orderable(
               seen, memo, *(dynamic_cast<t_set const&>(type).get_elem_type()));
  }
  if (type.is_map()) {
    return result = !has_disqualifying_annotation(type) &&
        is_orderable(
               seen,
               memo,
               *(dynamic_cast<t_map const&>(type).get_key_type())) &&
        is_orderable(
               seen, memo, *(dynamic_cast<t_map const&>(type).get_val_type()));
  }
  return false;
}

bool is_orderable(t_type const& type) {
  std::unordered_set<t_type const*> seen;
  std::unordered_map<t_type const*, bool> memo;
  return is_orderable(seen, memo, type);
}

namespace {

std::string const& map_find_first(
    std::map<std::string, std::string> const& m,
    std::initializer_list<char const*> keys) {
  for (auto const& key : keys) {
    auto const it = m.find(key);
    if (it != m.end()) {
      return it->second;
    }
  }
  static auto const& empty = *new std::string();
  return empty;
}

} // namespace

std::string const& get_cpp_type(const t_type* type) {
  return map_find_first(
      type->annotations_,
      {
          "cpp.type",
          "cpp2.type",
      });
}

bool is_implicit_ref(const t_type* type) {
  auto const* resolved_typedef = type->get_true_type();
  return resolved_typedef != nullptr && resolved_typedef->is_binary() &&
      get_cpp_type(resolved_typedef).find("std::unique_ptr") !=
      std::string::npos &&
      get_cpp_type(resolved_typedef).find("folly::IOBuf") != std::string::npos;
}

bool is_cpp_ref(const t_field* f) {
  return f->annotations_.count("cpp.ref") ||
      f->annotations_.count("cpp2.ref") ||
      f->annotations_.count("cpp.ref_type") ||
      f->annotations_.count("cpp2.ref_type") || is_implicit_ref(f->get_type());
}

bool is_stack_arguments(
    std::map<std::string, std::string> const& options,
    t_function const& function) {
  auto it = function.annotations_.find("cpp.stack_arguments");
  auto ptr = it == function.annotations_.end() ? nullptr : &it->second;
  return ptr ? *ptr != "0" : options.count("stack_arguments") != 0;
}

int32_t get_split_count(std::map<std::string, std::string> const& options) {
  auto iter = options.find("types_cpp_splits");
  if (iter == options.end()) {
    return 0;
  }
  return std::stoi(iter->second);
}

} // namespace cpp2
} // namespace compiler
} // namespace thrift
} // namespace apache
