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

#include <thrift/compiler/lib/cpp2/util.h>

#include <algorithm>
#include <queue>
#include <stdexcept>

#include <openssl/sha.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <thrift/compiler/ast/t_list.h>
#include <thrift/compiler/ast/t_map.h>
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_set.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/gen/cpp/type_resolver.h>
#include <thrift/compiler/util.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace cpp2 {
namespace {

const std::string& value_or_empty(const std::string* value) {
  return value ? *value : empty_instance<std::string>();
}

} // namespace

bool is_orderable(
    std::unordered_set<t_type const*>& seen,
    std::unordered_map<t_type const*, bool>& memo,
    t_type const& type) {
  bool has_disqualifying_annotation = type.has_annotation({
      "cpp.template",
      "cpp2.template",
      "cpp.type",
      "cpp2.type",
  });
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
    auto const& real = [&]() -> auto&& { return *type.get_true_type(); };
    auto const& next = *(dynamic_cast<t_typedef const&>(type).get_type());
    return result = is_orderable(seen, memo, next) &&
        (!(real().is_set() || real().is_map()) ||
         !has_disqualifying_annotation);
  }
  if (type.is_struct() || type.is_xception()) {
    const auto& as_sturct = static_cast<t_struct const&>(type);
    return result = std::all_of(
               as_sturct.fields().begin(),
               as_sturct.fields().end(),
               [&](const auto& f) {
                 return is_orderable(seen, memo, f.type().deref());
               });
  }
  if (type.is_list()) {
    return result = is_orderable(
               seen,
               memo,
               *(dynamic_cast<t_list const&>(type).get_elem_type()));
  }
  if (type.is_set()) {
    return result = !has_disqualifying_annotation &&
        is_orderable(
               seen, memo, *(dynamic_cast<t_set const&>(type).get_elem_type()));
  }
  if (type.is_map()) {
    return result = !has_disqualifying_annotation &&
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

std::string const& get_type(const t_type* type) {
  return value_or_empty(gen::cpp::type_resolver::find_type(type));
}

bool is_implicit_ref(const t_type* type) {
  auto const* resolved_typedef = type->get_true_type();
  return resolved_typedef != nullptr && resolved_typedef->is_binary() &&
      get_type(resolved_typedef).find("std::unique_ptr") != std::string::npos &&
      get_type(resolved_typedef).find("folly::IOBuf") != std::string::npos;
}

bool field_transitively_refers_to_unique(const t_field* field) {
  switch (gen::cpp::find_ref_type(*field)) {
    case gen::cpp::reference_type::none:
    case gen::cpp::reference_type::unrecognized: {
      break;
    }
    case gen::cpp::reference_type::unique: {
      return true;
    }
    case gen::cpp::reference_type::boxed:
    case gen::cpp::reference_type::shared_const:
    case gen::cpp::reference_type::shared_mutable: {
      return false;
    }
  }
  std::queue<const t_type*> queue;
  queue.push(field->get_type());
  while (!queue.empty()) {
    auto type = queue.front()->get_true_type();
    queue.pop();
    if (cpp2::is_implicit_ref(type)) {
      return true;
    }
    switch (type->get_type_value()) {
      case t_type::type::t_list: {
        queue.push(static_cast<const t_list*>(type)->get_elem_type());
        break;
      }
      case t_type::type::t_set: {
        queue.push(static_cast<const t_set*>(type)->get_elem_type());
        break;
      }
      case t_type::type::t_map: {
        queue.push(static_cast<const t_map*>(type)->get_key_type());
        queue.push(static_cast<const t_map*>(type)->get_val_type());
        break;
      }
      default: {
        break;
      }
    }
  }
  return false;
}

bool is_eligible_for_constexpr::operator()(const t_type* type) {
  enum class eligible { unknown, yes, no };
  auto check = [this](const t_type* t) {
    auto it = cache_.find(t);
    if (it != cache_.end()) {
      return it->second ? eligible::yes : eligible::no;
    }
    bool result = false;
    if (t->has_annotation("cpp.indirection")) {
      // Custom types may not have constexpr constructors.
      result = false;
    } else if (
        t->is_any_int() || t->is_floating_point() || t->is_bool() ||
        t->is_enum()) {
      result = true;
    } else if (t->is_union() || t->is_exception()) {
      // Union and exception constructors are not defaulted.
      result = false;
    } else if (t->has_annotation(
                   {"cpp.virtual", "cpp2.virtual", "cpp.allocator"})) {
      result = false;
    } else {
      return eligible::unknown;
    }
    cache_[t] = result;
    return result ? eligible::yes : eligible::no;
  };
  auto result = check(type);
  if (result != eligible::unknown) {
    return result == eligible::yes;
  }
  if (const auto* s = dynamic_cast<const t_struct*>(type)) {
    result = eligible::yes;
    for_each_transitive_field(s, [&](const t_field* field) {
      result = check(field->get_type());
      if (result == eligible::no) {
        return false;
      } else if (is_explicit_ref(field) || is_lazy(field)) {
        result = eligible::no;
        return false;
      } else if (result == eligible::unknown) {
        if (!field->get_type()->is_struct()) {
          return false;
        }
        // Structs are eligible if all their fields are.
        result = eligible::yes;
      }
      return true;
    });
    return result == eligible::yes;
  }
  return false;
}

bool is_stack_arguments(
    std::map<std::string, std::string> const& options,
    t_function const& function) {
  if (function.has_annotation("cpp.stack_arguments")) {
    return function.get_annotation("cpp.stack_arguments") != "0";
  }
  return options.count("stack_arguments");
}

int32_t get_split_count(std::map<std::string, std::string> const& options) {
  auto iter = options.find("types_cpp_splits");
  if (iter == options.end()) {
    return 0;
  }
  return std::stoi(iter->second);
}

static auto split(const std::string& s, char delimiter) {
  std::vector<std::string> ret;
  boost::algorithm::split(ret, s, [&](char c) { return c == delimiter; });
  return ret;
}

std::unordered_map<std::string, int32_t> get_client_name_to_split_count(
    std::map<std::string, std::string> const& options) {
  auto iter = options.find("client_cpp_splits");
  if (iter == options.end()) {
    return {};
  }

  auto map = iter->second;
  if (map.size() < 2 || map[0] != '{' || *map.rbegin() != '}') {
    throw std::runtime_error("Invalid client_cpp_splits value: `" + map + "`");
  }
  map = map.substr(1, map.size() - 2);
  if (map.empty()) {
    return {};
  }
  std::unordered_map<std::string, int32_t> ret;
  for (auto kv : split(map, ',')) {
    auto a = split(kv, ':');
    if (a.size() != 2) {
      throw std::runtime_error(
          "Invalid pair `" + kv + "` in client_cpp_splits value: `" + map +
          "`");
    }
    ret[a[0]] = std::stoi(a[1]);
  }
  return ret;
}

bool is_mixin(const t_field& field) {
  return field.has_annotation("cpp.mixin");
}

bool has_ref_annotation(const t_field& field) {
  switch (gen::cpp::find_ref_type(field)) {
    case gen::cpp::reference_type::unique:
    case gen::cpp::reference_type::shared_const:
    case gen::cpp::reference_type::shared_mutable:
    case gen::cpp::reference_type::unrecognized:
      return true;
    case gen::cpp::reference_type::none:
    case gen::cpp::reference_type::boxed:
      return false;
  }
  throw std::logic_error("Unhandled ref_type");
}

static void get_mixins_and_members_impl(
    const t_struct& strct,
    const t_field* top_level_mixin,
    std::vector<mixin_member>& out) {
  for (const auto& member : strct.fields()) {
    if (is_mixin(member)) {
      assert(member.type()->get_true_type()->is_struct());
      auto mixin_struct =
          static_cast<const t_struct*>(member.type()->get_true_type());
      const auto& mixin =
          top_level_mixin != nullptr ? *top_level_mixin : member;

      // import members from mixin field
      for (const auto& member_from_mixin : mixin_struct->fields()) {
        out.push_back({&mixin, &member_from_mixin});
      }

      // import members from nested mixin field
      get_mixins_and_members_impl(*mixin_struct, &mixin, out);
    }
  }
}

std::vector<mixin_member> get_mixins_and_members(const t_struct& strct) {
  std::vector<mixin_member> ret;
  get_mixins_and_members_impl(strct, nullptr, ret);
  return ret;
}

namespace {
struct get_gen_type_class_options {
  bool gen_indirection = false;
  bool gen_indirection_inner_ = false;
};

std::string get_gen_type_class_(
    t_type const& type_, get_gen_type_class_options opts) {
  std::string const ns = "::apache::thrift::";
  std::string const tc = ns + "type_class::";

  auto const& type = *type_.get_true_type();

  bool const ind = type.has_annotation("cpp.indirection");
  if (ind && opts.gen_indirection && !opts.gen_indirection_inner_) {
    opts.gen_indirection_inner_ = true;
    auto const inner = get_gen_type_class_(type_, opts);
    auto const tag = ns + "detail::indirection_tag";
    auto const fun = ns + "detail::apply_indirection_fn";
    return tag + "<" + inner + ", " + fun + ">";
  }
  opts.gen_indirection_inner_ = false;

  if (type.is_void()) {
    return tc + "nothing";
  } else if (type.is_bool() || type.is_byte() || type.is_any_int()) {
    return tc + "integral";
  } else if (type.is_floating_point()) {
    return tc + "floating_point";
  } else if (type.is_enum()) {
    return tc + "enumeration";
  } else if (type.is_string()) {
    return tc + "string";
  } else if (type.is_binary()) {
    return tc + "binary";
  } else if (type.is_list()) {
    auto& list = dynamic_cast<t_list const&>(type);
    auto& elem = *list.get_elem_type();
    auto elem_tc = get_gen_type_class_(elem, opts);
    return tc + "list<" + elem_tc + ">";
  } else if (type.is_set()) {
    auto& set = dynamic_cast<t_set const&>(type);
    auto& elem = *set.get_elem_type();
    auto elem_tc = get_gen_type_class_(elem, opts);
    return tc + "set<" + elem_tc + ">";
  } else if (type.is_map()) {
    auto& map = dynamic_cast<t_map const&>(type);
    auto& key = *map.get_key_type();
    auto& val = *map.get_val_type();
    auto key_tc = get_gen_type_class_(key, opts);
    auto val_tc = get_gen_type_class_(val, opts);
    return tc + "map<" + key_tc + ", " + val_tc + ">";
  } else if (type.is_union()) {
    return tc + "variant";
  } else if (type.is_struct() || type.is_exception()) {
    return tc + "structure";
  } else {
    throw std::runtime_error("unknown type class for: " + type.get_full_name());
  }
}

} // namespace

std::string get_gen_type_class(t_type const& type) {
  get_gen_type_class_options opts;
  return get_gen_type_class_(type, opts);
}

std::string get_gen_type_class_with_indirection(t_type const& type) {
  get_gen_type_class_options opts;
  opts.gen_indirection = true;
  return get_gen_type_class_(type, opts);
}

std::string sha256_hex(std::string const& in) {
  std::uint8_t mid[SHA256_DIGEST_LENGTH];
  SHA256_CTX hasher;
  SHA256_Init(&hasher);
  SHA256_Update(&hasher, in.data(), in.size());
  SHA256_Final(mid, &hasher);

  constexpr auto alpha = "0123456789abcdef";

  std::string out;
  for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    constexpr auto mask = std::uint8_t(std::uint8_t(~std::uint8_t(0)) >> 4);
    auto hi = (mid[i] >> 4) & mask;
    auto lo = (mid[i] >> 0) & mask;
    out.push_back(alpha[hi]);
    out.push_back(alpha[lo]);
  }
  return out;
}

} // namespace cpp2
} // namespace compiler
} // namespace thrift
} // namespace apache
