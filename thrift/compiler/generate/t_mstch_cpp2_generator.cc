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
#include <array>
#include <memory>
#include <queue>
#include <vector>

#include <boost/algorithm/string/replace.hpp>
#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>
#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/util.h>
#include <thrift/compiler/validator/validator.h>

using namespace std;

namespace {
using namespace apache::thrift::compiler;
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

std::string const& get_cpp_template(const t_type* type) {
  return map_find_first(
      type->annotations_,
      {
          "cpp.template",
          "cpp2.template",
      });
}

bool is_cpp_ref_unique_either(const t_field* f) {
  return f->annotations_.count("cpp.ref") ||
      f->annotations_.count("cpp2.ref") ||
      (f->annotations_.count("cpp.ref_type") &&
       f->annotations_.at("cpp.ref_type") == "unique") ||
      (f->annotations_.count("cpp2.ref_type") &&
       f->annotations_.at("cpp2.ref_type") == "unique") ||
      cpp2::is_implicit_ref(f->get_type());
}

bool is_annotation_blacklisted_in_fatal(const std::string& key) {
  const static std::set<std::string> black_list{
      "cpp.methods",
      "cpp.name",
      "cpp.ref",
      "cpp.ref_type",
      "cpp.template",
      "cpp.type",
      "cpp2.methods",
      "cpp2.ref",
      "cpp2.ref_type",
      "cpp2.template",
      "cpp2.type",
  };
  return black_list.find(key) != black_list.end();
}

bool same_types(const t_type* a, const t_type* b) {
  if (!a || !b) {
    return false;
  }

  if (get_cpp_template(a) != get_cpp_template(b) ||
      cpp2::get_cpp_type(a) != cpp2::get_cpp_type(b)) {
    return false;
  }

  const auto* resolved_a = a->get_true_type();
  const auto* resolved_b = b->get_true_type();

  if (resolved_a->get_type_value() != resolved_b->get_type_value()) {
    return false;
  }

  switch (resolved_a->get_type_value()) {
    case t_types::TypeValue::TYPE_LIST: {
      const auto* list_a = dynamic_cast<const t_list*>(resolved_a);
      const auto* list_b = dynamic_cast<const t_list*>(resolved_b);
      return same_types(list_a->get_elem_type(), list_b->get_elem_type());
    }
    case t_types::TypeValue::TYPE_SET: {
      const auto* set_a = dynamic_cast<const t_set*>(resolved_a);
      const auto* set_b = dynamic_cast<const t_set*>(resolved_b);
      return same_types(set_a->get_elem_type(), set_b->get_elem_type());
    }
    case t_types::TypeValue::TYPE_MAP: {
      const auto* map_a = dynamic_cast<const t_map*>(resolved_a);
      const auto* map_b = dynamic_cast<const t_map*>(resolved_b);
      return same_types(map_a->get_key_type(), map_b->get_key_type()) &&
          same_types(map_a->get_val_type(), map_b->get_val_type());
    }
    default:;
  }
  return true;
}

template <typename Node>
const std::string& get_cpp_name(const Node* node) {
  auto name = node->annotations_.find("cpp.name");
  return name != node->annotations_.end() ? name->second : node->get_name();
}

std::vector<t_annotation> get_fatal_annotations(
    std::map<std::string, std::string> annotations) {
  std::vector<t_annotation> fatal_annotations;
  for (const auto& iter : annotations) {
    if (is_annotation_blacklisted_in_fatal(iter.first)) {
      continue;
    }
    fatal_annotations.emplace_back(iter.first, iter.second);
  }

  return fatal_annotations;
}

std::string get_fatal_string_short_id(const std::string& key) {
  return boost::algorithm::replace_all_copy(key, ".", "_");
}

std::string get_fatal_namesoace_name_short_id(
    const std::string& lang,
    const std::string& ns) {
  std::string replacement = lang == "cpp" || lang == "cpp2" ? "__" : "_";
  std::string result = boost::algorithm::replace_all_copy(ns, ".", replacement);
  if (lang == "php_path") {
    return boost::algorithm::replace_all_copy(ns, "/", "_");
  }
  return result;
}

std::string get_fatal_namesoace(
    const std::string& lang,
    const std::string& ns) {
  if (lang == "cpp" || lang == "cpp2") {
    return boost::algorithm::replace_all_copy(ns, ".", "::");
  } else if (lang == "php") {
    return boost::algorithm::replace_all_copy(ns, ".", "_");
  }
  return ns;
}

std::string render_fatal_string(const std::string& normal_string) {
  const static std::map<char, std::string> substition{
      {'\0', "\\0"},
      {'\n', "\\n"},
      {'\r', "\\r"},
      {'\t', "\\t"},
      {'\'', "\\\'"},
      {'\\', "\\\\"},
  };
  std::ostringstream res;
  res << "::fatal::sequence<char";
  for (const char& c : normal_string) {
    res << ", '";
    auto found = substition.find(c);
    if (found != substition.end()) {
      res << found->second;
    } else {
      res << c;
    }
    res << "'";
  }
  res << ">";
  return res.str();
}
} // namespace

namespace apache {
namespace thrift {
namespace compiler {

class cpp2_generator_context {
 public:
  static cpp2_generator_context create(t_program const* const program) {
    cpp2_generator_context ctx(program);
    return ctx;
  }

  cpp2_generator_context(cpp2_generator_context&&) = default;
  cpp2_generator_context& operator=(cpp2_generator_context&&) = default;

  bool is_orderable(t_type const& type) {
    std::unordered_set<t_type const*> seen;
    auto& memo = is_orderable_memo_;
    return cpp2::is_orderable(seen, memo, type);
  }

 private:
  explicit cpp2_generator_context(t_program const*) {}

  std::unordered_map<t_type const*, bool> is_orderable_memo_;
};

class t_mstch_cpp2_generator : public t_mstch_generator {
 public:
  t_mstch_cpp2_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/);

  void generate_program() override;
  void fill_validator_list(validator_list&) const override;
  static std::string get_cpp2_namespace(t_program const* program);
  static mstch::array get_namespace_array(t_program const* program);
  static mstch::node cpp_includes(t_program const* program);
  static mstch::node include_prefix(
      t_program const* program,
      std::string& include_prefix);

 private:
  void set_mstch_generators();
  void generate_reflection(t_program const* program);
  void generate_visitation(t_program const* program);
  void generate_constants(t_program const* program);
  void generate_metadata(t_program const* program);
  void generate_structs(t_program const* program);
  void generate_service(t_service const* service);

  std::shared_ptr<cpp2_generator_context> context_;
};

class mstch_cpp2_enum : public mstch_enum {
 public:
  mstch_cpp2_enum(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum(enm, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enum:empty?", &mstch_cpp2_enum::is_empty},
            {"enum:size", &mstch_cpp2_enum::size},
            {"enum:min", &mstch_cpp2_enum::min},
            {"enum:max", &mstch_cpp2_enum::max},
            {"enum:cpp_is_unscoped", &mstch_cpp2_enum::cpp_is_unscoped},
            {"enum:cpp_enum_type", &mstch_cpp2_enum::cpp_enum_type},
            {"enum:cpp_declare_bitwise_ops",
             &mstch_cpp2_enum::cpp_declare_bitwise_ops},
            {"enum:has_zero", &mstch_cpp2_enum::has_zero},
            {"enum:fatal_annotations?",
             &mstch_cpp2_enum::has_fatal_annotations},
            {"enum:fatal_annotations", &mstch_cpp2_enum::fatal_annotations},
            {"enum:legacy_type_id", &mstch_cpp2_enum::get_legacy_type_id},
        });
  }
  mstch::node is_empty() {
    return enm_->get_enum_values().empty();
  }
  mstch::node size() {
    return std::to_string(enm_->get_enum_values().size());
  }
  mstch::node min() {
    if (!enm_->get_enum_values().empty()) {
      auto e_min = std::min_element(
          enm_->get_enum_values().begin(),
          enm_->get_enum_values().end(),
          [](t_enum_value* a, t_enum_value* b) {
            return a->get_value() < b->get_value();
          });
      return get_cpp_name(*e_min);
    }
    return mstch::node();
  }
  mstch::node max() {
    if (!enm_->get_enum_values().empty()) {
      auto e_max = std::max_element(
          enm_->get_enum_values().begin(),
          enm_->get_enum_values().end(),
          [](t_enum_value* a, t_enum_value* b) {
            return a->get_value() < b->get_value();
          });
      return get_cpp_name(*e_max);
    }
    return mstch::node();
  }
  std::string const& cpp_is_unscoped_() {
    return map_find_first(
        enm_->annotations_,
        {
            "cpp2.deprecated_enum_unscoped",
            "cpp.deprecated_enum_unscoped",
        });
  }
  mstch::node cpp_is_unscoped() {
    return cpp_is_unscoped_();
  }
  mstch::node cpp_enum_type() {
    if (enm_->annotations_.count("cpp.enum_type")) {
      return enm_->annotations_.at("cpp.enum_type");
    } else if (enm_->annotations_.count("cpp2.enum_type")) {
      return enm_->annotations_.at("cpp2.enum_type");
    }
    if (!cpp_is_unscoped_().empty()) {
      return std::string("int");
    }
    return std::string();
  }
  mstch::node cpp_declare_bitwise_ops() {
    if (enm_->annotations_.count("cpp.declare_bitwise_ops")) {
      return enm_->annotations_.at("cpp.declare_bitwise_ops");
    } else if (enm_->annotations_.count("cpp2.declare_bitwise_ops")) {
      return enm_->annotations_.at("cpp2.declare_bitwise_ops");
    }
    return std::string();
  }
  mstch::node has_zero() {
    auto* enm_value = enm_->find_value(0);
    if (enm_value != nullptr) {
      return generators_->enum_value_generator_->generate(
          enm_value, generators_, cache_, pos_);
    }
    return mstch::node();
  }
  mstch::node has_fatal_annotations() {
    return get_fatal_annotations(enm_->annotations_).size() > 0;
  }
  mstch::node fatal_annotations() {
    return generate_elements(
        get_fatal_annotations(enm_->annotations_),
        generators_->annotation_generator_.get(),
        generators_,
        cache_);
  }
  mstch::node get_legacy_type_id() {
    return std::to_string(enm_->get_type_id());
  }
};

class mstch_cpp2_enum_value : public mstch_enum_value {
 public:
  mstch_cpp2_enum_value(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum_value(enm_value, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enumValue:cpp_name", &mstch_cpp2_enum_value::cpp_name},
            {"enumValue:fatal_annotations?",
             &mstch_cpp2_enum_value::has_fatal_annotations},
            {"enumValue:fatal_annotations",
             &mstch_cpp2_enum_value::fatal_annotations},
        });
  }
  mstch::node cpp_name() {
    return get_cpp_name(enm_value_);
  }
  mstch::node has_fatal_annotations() {
    return get_fatal_annotations(enm_value_->annotations_).size() > 0;
  }
  mstch::node fatal_annotations() {
    return generate_elements(
        get_fatal_annotations(enm_value_->annotations_),
        generators_->annotation_generator_.get(),
        generators_,
        cache_);
  }
};

class mstch_cpp2_const_value : public mstch_const_value {
 public:
  mstch_cpp2_const_value(
      t_const_value const* const_value,
      t_const const* current_const,
      t_type const* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_const_value(
            const_value,
            current_const,
            expected_type,
            generators,
            cache,
            pos,
            index) {}

 private:
  bool same_type_as_expected() const override {
    return const_value_->get_owner() &&
        same_types(expected_type_, const_value_->get_owner()->get_type());
  }
};

class mstch_cpp2_type : public mstch_type {
 public:
  mstch_cpp2_type(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_type(type, generators, cache, pos) {
    register_methods(
        this,
        {
            {"type:resolves_to_base?", &mstch_cpp2_type::resolves_to_base},
            {"type:resolves_to_integral?",
             &mstch_cpp2_type::resolves_to_integral},
            {"type:resolves_to_base_or_enum?",
             &mstch_cpp2_type::resolves_to_base_or_enum},
            {"type:resolves_to_container?",
             &mstch_cpp2_type::resolves_to_container},
            {"type:resolves_to_container_or_struct?",
             &mstch_cpp2_type::resolves_to_container_or_struct},
            {"type:resolves_to_container_or_enum?",
             &mstch_cpp2_type::resolves_to_container_or_enum},
            {"type:resolves_to_complex_return?",
             &mstch_cpp2_type::resolves_to_complex_return},
            {"type:resolves_to_fixed_size?",
             &mstch_cpp2_type::resolves_to_fixed_size},
            {"type:resolves_to_enum?", &mstch_cpp2_type::resolves_to_enum},
            {"type:transitively_refers_to_struct?",
             &mstch_cpp2_type::transitively_refers_to_struct},
            {"type:cpp_type", &mstch_cpp2_type::cpp_type},
            {"type:resolved_cpp_type", &mstch_cpp2_type::resolved_cpp_type},
            {"type:string_or_binary?", &mstch_cpp2_type::is_string_or_binary},
            {"type:cpp_template", &mstch_cpp2_type::cpp_template},
            {"type:cpp_indirection", &mstch_cpp2_type::cpp_indirection},
            {"type:non_empty_struct?", &mstch_cpp2_type::is_non_empty_struct},
            {"type:namespace_cpp2", &mstch_cpp2_type::namespace_cpp2},
            {"type:sync_methods_return_try?",
             &mstch_cpp2_type::sync_methods_return_try},
            {"type:cpp_declare_hash", &mstch_cpp2_type::cpp_declare_hash},
            {"type:cpp_declare_equal_to",
             &mstch_cpp2_type::cpp_declare_equal_to},
            {"type:no_getters_setters?", &mstch_cpp2_type::no_getters_setters},
            {"type:fatal_type_class", &mstch_cpp2_type::fatal_type_class},
            {"type:program_name", &mstch_cpp2_type::program_name},
            {"type:cpp_use_allocator?", &mstch_cpp2_type::cpp_use_allocator},
        });
  }
  std::string get_type_namespace(t_program const* program) override {
    return cpp2::get_gen_namespace(*program);
  }
  mstch::node resolves_to_base() {
    return resolved_type_->is_base_type();
  }
  mstch::node resolves_to_integral() {
    return resolved_type_->is_byte() || resolved_type_->is_any_int();
  }
  mstch::node resolves_to_base_or_enum() {
    return resolved_type_->is_base_type() || resolved_type_->is_enum();
  }
  mstch::node resolves_to_container() {
    return resolved_type_->is_container();
  }
  mstch::node resolves_to_container_or_struct() {
    return resolved_type_->is_container() || resolved_type_->is_struct() ||
        resolved_type_->is_xception();
  }
  mstch::node resolves_to_container_or_enum() {
    return resolved_type_->is_container() || resolved_type_->is_enum();
  }
  mstch::node resolves_to_complex_return() {
    return resolved_type_->is_container() ||
        resolved_type_->is_string_or_binary() || resolved_type_->is_struct() ||
        resolved_type_->is_xception();
  }
  mstch::node resolves_to_fixed_size() {
    return resolved_type_->is_bool() || resolved_type_->is_byte() ||
        resolved_type_->is_any_int() || resolved_type_->is_enum() ||
        resolved_type_->is_floating_point();
  }
  mstch::node resolves_to_enum() {
    return resolved_type_->is_enum();
  }
  mstch::node transitively_refers_to_struct() {
    // fast path is unnecessary but may avoid allocations
    if (resolved_type_->is_struct()) {
      return true;
    }
    if (!resolved_type_->is_container()) {
      return false;
    }
    // type is a container: traverse (breadthwise, but could be depthwise)
    std::queue<t_type const*> queue;
    queue.push(resolved_type_);
    while (!queue.empty()) {
      auto next = queue.front();
      queue.pop();
      if (next->is_struct()) {
        return true;
      }
      if (!next->is_container()) {
        continue;
      }
      if (false) {
      } else if (next->is_list()) {
        queue.push(static_cast<t_list const*>(next)->get_elem_type());
      } else if (next->is_set()) {
        queue.push(static_cast<t_set const*>(next)->get_elem_type());
      } else if (next->is_map()) {
        queue.push(static_cast<t_map const*>(next)->get_key_type());
        queue.push(static_cast<t_map const*>(next)->get_val_type());
      } else {
        assert(false);
      }
    }
    return false;
  }
  mstch::node cpp_type() {
    return cpp2::get_cpp_type(type_);
  }
  mstch::node resolved_cpp_type() {
    return cpp2::get_cpp_type(resolved_type_);
  }
  mstch::node is_string_or_binary() {
    return resolved_type_->is_string_or_binary();
  }
  mstch::node cpp_template() {
    return get_cpp_template(type_);
  }
  mstch::node cpp_indirection() {
    if (resolved_type_->annotations_.count("cpp.indirection")) {
      return resolved_type_->annotations_.at("cpp.indirection");
    }
    return std::string();
  }
  mstch::node cpp_declare_hash() {
    return resolved_type_->annotations_.count("cpp.declare_hash") ||
        resolved_type_->annotations_.count("cpp2.declare_hash");
  }
  mstch::node cpp_declare_equal_to() {
    return resolved_type_->annotations_.count("cpp.declare_equal_to") ||
        resolved_type_->annotations_.count("cpp2.declare_equal_to");
  }
  mstch::node cpp_use_allocator() {
    return resolved_type_->annotations_.count("cpp.use_allocator") ||
        type_->annotations_.count("cpp.use_allocator");
  }
  mstch::node is_non_empty_struct() {
    if (resolved_type_->is_struct() || resolved_type_->is_xception()) {
      auto as_struct = dynamic_cast<t_struct const*>(resolved_type_);
      return !as_struct->get_members().empty();
    }
    return false;
  }
  mstch::node namespace_cpp2() {
    return t_mstch_cpp2_generator::get_namespace_array(type_->get_program());
  }
  mstch::node sync_methods_return_try() {
    return cache_->parsed_options_.count("sync_methods_return_try") != 0;
  }
  mstch::node no_getters_setters() {
    return cache_->parsed_options_.count("no_getters_setters") != 0;
  }
  mstch::node fatal_type_class() {
    return get_fatal_type_class(resolved_type_);
  }
  std::string get_fatal_type_class(const t_type* ttype) {
    if (ttype->is_typedef()) {
      return get_fatal_type_class(
          dynamic_cast<t_typedef const*>(ttype)->get_type());
    }

    if (ttype->is_void()) {
      return "::apache::thrift::type_class::nothing";
    } else if (ttype->is_binary()) {
      return "::apache::thrift::type_class::binary";
    } else if (ttype->is_string()) {
      return "::apache::thrift::type_class::string";
    } else if (ttype->is_floating_point()) {
      return "::apache::thrift::type_class::floating_point";
    } else if (ttype->is_base_type()) {
      return "::apache::thrift::type_class::integral";
    } else if (ttype->is_enum()) {
      return "::apache::thrift::type_class::enumeration";
    } else if (ttype->is_list()) {
      return "::apache::thrift::type_class::list<" +
          get_fatal_type_class(
                 dynamic_cast<const t_list*>(ttype)->get_elem_type()) +
          ">";
    } else if (ttype->is_map()) {
      return "::apache::thrift::type_class::map<" +
          get_fatal_type_class(
                 dynamic_cast<const t_map*>(ttype)->get_key_type()) +
          ", " +
          get_fatal_type_class(
                 dynamic_cast<const t_map*>(ttype)->get_val_type()) +
          ">";
    } else if (ttype->is_set()) {
      return "::apache::thrift::type_class::set<" +
          get_fatal_type_class(
                 dynamic_cast<const t_set*>(ttype)->get_elem_type()) +
          ">";
    } else if (ttype->is_struct()) {
      if (dynamic_cast<const t_struct*>(ttype)->is_union()) {
        return "::apache::thrift::type_class::variant";
      } else {
        return "::apache::thrift::type_class::structure";
      }
    } else {
      // TODO: stream is not supported
      return "::apache::thrift::type_class::unknown";
    }
  }
  mstch::node program_name() {
    std::string name;
    if (auto prog = type_->get_program()) {
      name = prog->get_name();
    }
    return name;
  }
};

class mstch_cpp2_field : public mstch_field {
 public:
  mstch_cpp2_field(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index)
      : mstch_field(field, generators, cache, pos, index) {
    register_methods(
        this,
        {
            {"field:index_plus_one", &mstch_cpp2_field::index_plus_one},
            {"field:cpp_name", &mstch_cpp2_field::cpp_name},
            {"field:next_field_key", &mstch_cpp2_field::next_field_key},
            {"field:next_field_type", &mstch_cpp2_field::next_field_type},
            {"field:cpp_ref?", &mstch_cpp2_field::cpp_ref},
            {"field:cpp_ref_unique?", &mstch_cpp2_field::cpp_ref_unique},
            {"field:cpp_ref_unique_either?",
             &mstch_cpp2_field::cpp_ref_unique_either},
            {"field:cpp_ref_shared?", &mstch_cpp2_field::cpp_ref_shared},
            {"field:cpp_ref_shared_const?",
             &mstch_cpp2_field::cpp_ref_shared_const},
            {"field:cpp_noncopyable?", &mstch_cpp2_field::cpp_noncopyable},
            {"field:enum_has_value", &mstch_cpp2_field::enum_has_value},
            {"field:terse_writes?", &mstch_cpp2_field::terse_writes},
            {"field:fatal_annotations?",
             &mstch_cpp2_field::has_fatal_annotations},
            {"field:fatal_annotations", &mstch_cpp2_field::fatal_annotations},
            {"field:fatal_required_qualifier",
             &mstch_cpp2_field::fatal_required_qualifier},
            {"field:visibility", &mstch_cpp2_field::visibility},
            {"field:metadata_name", &mstch_cpp2_field::metadata_name},
        });
  }
  mstch::node index_plus_one() {
    return std::to_string(index_ + 1);
  }
  mstch::node cpp_name() {
    return get_cpp_name(field_);
  }
  mstch::node cpp_ref() {
    return has_annotation("cpp.ref") || has_annotation("cpp2.ref") ||
        has_annotation("cpp.ref_type") || has_annotation("cpp2.ref_type");
  }
  mstch::node cpp_ref_unique() {
    return has_annotation("cpp.ref") || has_annotation("cpp2.ref") ||
        get_annotation("cpp.ref_type") == "unique" ||
        get_annotation("cpp2.ref_type") == "unique";
  }
  mstch::node cpp_ref_unique_either() {
    return boost::get<bool>(cpp_ref_unique()) ||
        cpp2::is_implicit_ref(field_->get_type());
  }
  mstch::node cpp_ref_shared() {
    return get_annotation("cpp.ref_type") == "shared" ||
        get_annotation("cpp2.ref_type") == "shared";
  }
  mstch::node cpp_ref_shared_const() {
    return get_annotation("cpp.ref_type") == "shared_const" ||
        get_annotation("cpp2.ref_type") == "shared_const";
  }
  mstch::node cpp_noncopyable() {
    auto type = field_->get_type();
    return type->is_struct() &&
        static_cast<const t_struct*>(type)->annotations_.count(
            "cpp2.noncopyable") != 0;
  }
  mstch::node enum_has_value() {
    if (field_->get_type()->is_enum()) {
      auto const* enm = dynamic_cast<t_enum const*>(field_->get_type());
      auto const* const_value = field_->get_value();
      using cv = t_const_value::t_const_value_type;
      if (const_value->get_type() == cv::CV_INTEGER) {
        auto* enm_value = enm->find_value(const_value->get_integer());
        if (enm_value != nullptr) {
          return generators_->enum_value_generator_->generate(
              enm_value, generators_, cache_, pos_);
        }
      }
    }
    return mstch::node();
  }
  mstch::node next_field_key() {
    return std::to_string(field_->get_next()->get_key());
  }
  mstch::node next_field_type() {
    return field_->get_next()
        ? generators_->type_generator_->generate(
              field_->get_next()->get_type(), generators_, cache_, pos_)
        : mstch::node("");
  }
  mstch::node terse_writes() {
    // Add terse writes for unqualified fields when comparison is cheap:
    // (e.g. i32/i64, empty strings/list/map)
    auto t = field_->get_type()->get_true_type();
    return cache_->parsed_options_.count("terse_writes") != 0 &&
        field_->get_req() != t_field::e_req::T_OPTIONAL &&
        field_->get_req() != t_field::e_req::T_REQUIRED &&
        (is_cpp_ref_unique_either(field_) ||
         (!t->is_struct() && !t->is_xception()));
  }
  mstch::node has_fatal_annotations() {
    return get_fatal_annotations(field_->annotations_).size() > 0;
  }
  mstch::node fatal_annotations() {
    return generate_elements(
        get_fatal_annotations(field_->annotations_),
        generators_->annotation_generator_.get(),
        generators_,
        cache_);
  }
  mstch::node fatal_required_qualifier() {
    switch (field_->get_req()) {
      case t_field::e_req::T_REQUIRED:
        return std::string("required");
      case t_field::e_req::T_OPTIONAL:
        return std::string("optional");
      case t_field::e_req::T_OPT_IN_REQ_OUT:
        return std::string("required_of_writer");
      default:
        throw runtime_error("unknown required qualifier");
    }
  }
  mstch::node visibility() {
    bool isPrivate = field_->get_req() == t_field::e_req::T_OPTIONAL &&
        !cpp2::is_cpp_ref(field_);
    return std::string(isPrivate ? "private" : "public");
  }
  mstch::node metadata_name() {
    auto key = field_->get_key();
    auto suffix = key >= 0 ? std::to_string(key) : "_" + std::to_string(-key);
    return field_->get_name() + "_" + suffix;
  }
};

class mstch_cpp2_struct : public mstch_struct {
 public:
  mstch_cpp2_struct(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      std::shared_ptr<cpp2_generator_context> context)
      : mstch_struct(strct, generators, cache, pos),
        context_(std::move(context)) {
    register_methods(
        this,
        {
            {"struct:fields_size", &mstch_cpp2_struct::fields_size},
            {"struct:filtered_fields", &mstch_cpp2_struct::filtered_fields},
            {"struct:fields_in_layout_order",
             &mstch_cpp2_struct::fields_in_layout_order},
            {"struct:is_struct_orderable?",
             &mstch_cpp2_struct::is_struct_orderable},
            {"struct:fields_contain_cpp_ref_unique_either?",
             &mstch_cpp2_struct::has_cpp_ref_unique_either},
            {"struct:cpp_methods", &mstch_cpp2_struct::cpp_methods},
            {"struct:cpp_declare_hash", &mstch_cpp2_struct::cpp_declare_hash},
            {"struct:cpp_declare_equal_to",
             &mstch_cpp2_struct::cpp_declare_equal_to},
            {"struct:cpp_noncopyable", &mstch_cpp2_struct::cpp_noncopyable},
            {"struct:cpp_noncomparable", &mstch_cpp2_struct::cpp_noncomparable},
            {"struct:cpp_noexcept_move", &mstch_cpp2_struct::cpp_noexcept_move},
            {"struct:cpp_noexcept_move_ctor",
             &mstch_cpp2_struct::cpp_noexcept_move_ctor},
            {"struct:virtual", &mstch_cpp2_struct::cpp_virtual},
            {"struct:message", &mstch_cpp2_struct::message},
            {"struct:isset_fields?", &mstch_cpp2_struct::has_isset_fields},
            {"struct:isset_fields", &mstch_cpp2_struct::isset_fields},
            {"struct:is_large?", &mstch_cpp2_struct::is_large},
            {"struct:no_getters_setters?",
             &mstch_cpp2_struct::no_getters_setters},
            {"struct:fatal_annotations?",
             &mstch_cpp2_struct::has_fatal_annotations},
            {"struct:fatal_annotations", &mstch_cpp2_struct::fatal_annotations},
            {"struct:legacy_type_id", &mstch_cpp2_struct::get_legacy_type_id},
            {"struct:metadata_name", &mstch_cpp2_struct::metadata_name},
            {"struct:mixin_fields", &mstch_cpp2_struct::mixin_fields},
            {"struct:num_union_members",
             &mstch_cpp2_struct::get_num_union_members},
            {"struct:cpp_allocator", &mstch_cpp2_struct::cpp_allocator},
            {"struct:cpp_allocator_via", &mstch_cpp2_struct::cpp_allocator_via},
        });
  }
  mstch::node fields_size() {
    return std::to_string(strct_->get_members().size());
  }
  mstch::node filtered_fields() {
    auto has_annotation = [](t_field const* field, std::string const& name) {
      return field->annotations_.count(name);
    };
    // Filter fields according to the following criteria:
    // Get all base_types but strings (empty and non-empty)
    // Get all non empty strings
    // Get all non empty containers
    // Get all enums
    // Get all containers with references
    std::vector<t_field const*> filtered_fields;
    for (auto const* field : get_members_in_layout_order()) {
      const t_type* type = field->get_type()->get_true_type();
      if ((type->is_base_type() && !type->is_string_or_binary()) ||
          (type->is_string_or_binary() && field->get_value() != nullptr) ||
          (type->is_container() && field->get_value() != nullptr &&
           !field->get_value()->is_empty()) ||
          (type->is_struct() &&
           (strct_ != dynamic_cast<t_struct const*>(type)) &&
           ((field->get_value() && !field->get_value()->is_empty()) ||
            ((has_annotation(field, "cpp.ref") ||
              has_annotation(field, "cpp2.ref") ||
              has_annotation(field, "cpp.ref_type") ||
              has_annotation(field, "cpp2.ref_type")) &&
             (field->get_req() != t_field::e_req::T_OPTIONAL)))) ||
          type->is_enum() ||
          (type->is_container() &&
           (has_annotation(field, "cpp.ref") ||
            has_annotation(field, "cpp2.ref") ||
            has_annotation(field, "cpp.ref_type") ||
            has_annotation(field, "cpp2.ref_type")) &&
           (field->get_req() != t_field::e_req::T_OPTIONAL))) {
        filtered_fields.push_back(field);
      }
    }
    return generate_elements(
        filtered_fields,
        generators_->field_generator_.get(),
        generators_,
        cache_);
  }

  mstch::node mixin_fields() {
    mstch::array fields;
    for (auto i : strct_->get_mixins_and_members()) {
      fields.push_back(mstch::map{{"mixin:name", i.mixin->get_name()},
                                  {"mixin:field_name", i.member->get_name()}});
    }
    return fields;
  }

  mstch::node is_struct_orderable() {
    return context_->is_orderable(*strct_) &&
        !strct_->annotations_.count("no_default_comparators");
  }
  mstch::node has_cpp_ref_unique_either() {
    for (auto const* f : strct_->get_members()) {
      if (is_cpp_ref_unique_either(f)) {
        return true;
      }
    }
    return false;
  }
  mstch::node cpp_methods() {
    if (strct_->annotations_.count("cpp.methods")) {
      return strct_->annotations_.at("cpp.methods");
    } else if (strct_->annotations_.count("cpp2.methods")) {
      return strct_->annotations_.at("cpp2.methods");
    }
    return std::string();
  }
  mstch::node cpp_declare_hash() {
    return strct_->annotations_.count("cpp.declare_hash") ||
        strct_->annotations_.count("cpp2.declare_hash");
  }
  mstch::node cpp_declare_equal_to() {
    return strct_->annotations_.count("cpp.declare_equal_to") ||
        strct_->annotations_.count("cpp2.declare_equal_to");
  }
  mstch::node cpp_noncopyable() {
    return bool(strct_->annotations_.count("cpp2.noncopyable"));
  }
  mstch::node cpp_noncomparable() {
    return bool(strct_->annotations_.count("cpp2.noncomparable"));
  }
  mstch::node cpp_noexcept_move() {
    return bool(strct_->annotations_.count("cpp.noexcept_move"));
  }
  mstch::node cpp_noexcept_move_ctor() {
    return strct_->annotations_.count("cpp.noexcept_move") ||
        strct_->annotations_.count("cpp.noexcept_move_ctor") ||
        strct_->annotations_.count("cpp2.noexcept_move_ctor");
  }
  mstch::node cpp_virtual() {
    return strct_->annotations_.count("cpp.virtual") > 0 ||
        strct_->annotations_.count("cpp2.virtual") > 0;
  }
  mstch::node message() {
    if (strct_->annotations_.count("message")) {
      return strct_->annotations_.at("message");
    }
    return std::string();
  }
  mstch::node cpp_allocator() {
    if (strct_->annotations_.count("cpp.allocator")) {
      return strct_->annotations_.at("cpp.allocator");
    }
    return std::string();
  }
  mstch::node cpp_allocator_via() {
    if (strct_->annotations_.count("cpp.allocator_via")) {
      auto name = strct_->annotations_.at("cpp.allocator_via");
      for (const auto* field : strct_->get_members()) {
        if (get_cpp_name(field) == name) {
          return name;
        }
      }
      throw runtime_error("No cpp.allocator_via field \"" + name + "\"");
    }
    return std::string();
  }
  mstch::node has_isset_fields() {
    for (const auto* field : strct_->get_members()) {
      if (field->get_req() != t_field::e_req::T_REQUIRED &&
          !field->annotations_.count("cpp.ref") &&
          !field->annotations_.count("cpp2.ref") &&
          !field->annotations_.count("cpp.ref_type") &&
          !field->annotations_.count("cpp2.ref_type")) {
        return true;
      }
    }
    return false;
  }
  mstch::node isset_fields() {
    std::vector<t_field const*> fields;
    for (const auto* field : strct_->get_members()) {
      if (field->get_req() != t_field::e_req::T_REQUIRED &&
          !field->annotations_.count("cpp.ref") &&
          !field->annotations_.count("cpp2.ref") &&
          !field->annotations_.count("cpp.ref_type") &&
          !field->annotations_.count("cpp2.ref_type")) {
        fields.push_back(field);
      }
    }
    if (fields.empty()) {
      return mstch::node();
    }
    return generate_elements(
        fields, generators_->field_generator_.get(), generators_, cache_);
  }
  mstch::node is_large() {
    // Outline constructors and destructors if the struct has
    // enough members and at least one has a non-trivial destructor
    // (involving at least a branch and a likely deallocation).
    // TODO(ott): Support unions.
    constexpr size_t kLargeStructThreshold = 4;
    if (strct_->get_members().size() <= kLargeStructThreshold) {
      return false;
    }
    for (auto const* field : strct_->get_members()) {
      auto const* resolved_typedef = field->get_type()->get_true_type();
      if (cpp2::is_cpp_ref(field) || resolved_typedef->is_string_or_binary() ||
          resolved_typedef->is_container()) {
        return true;
      }
    }
    return false;
  }
  mstch::node no_getters_setters() {
    return cache_->parsed_options_.count("no_getters_setters") != 0;
  }
  mstch::node has_fatal_annotations() {
    return get_fatal_annotations(strct_->annotations_).size() > 0;
  }
  mstch::node fatal_annotations() {
    return generate_elements(
        get_fatal_annotations(strct_->annotations_),
        generators_->annotation_generator_.get(),
        generators_,
        cache_);
  }
  mstch::node get_legacy_type_id() {
    return std::to_string(strct_->get_type_id());
  }
  mstch::node metadata_name() {
    return strct_->get_program()->get_name() + "_" + strct_->get_name();
  }

  mstch::node get_num_union_members() {
    if (!strct_->is_union()) {
      throw runtime_error("not a union struct");
    }
    return std::to_string(strct_->get_members().size());
  }

 protected:
  // Computes the alignment of field on the target platform.
  // Returns 0 if cannot compute the alignment.
  static size_t compute_alignment(t_field const* field) {
    auto const& annotations = field->annotations_;
    if (annotations.find("cpp.ref_type") != annotations.end() ||
        annotations.find("cpp2.ref_type") != annotations.end()) {
      return 8;
    }
    t_type const* type = field->get_type();
    switch (type->get_type_value()) {
      case t_types::TypeValue::TYPE_BOOL:
      case t_types::TypeValue::TYPE_BYTE:
        return 1;
      case t_types::TypeValue::TYPE_I16:
        return 2;
      case t_types::TypeValue::TYPE_I32:
      case t_types::TypeValue::TYPE_FLOAT:
      case t_types::TypeValue::TYPE_ENUM:
        return 4;
      case t_types::TypeValue::TYPE_I64:
      case t_types::TypeValue::TYPE_DOUBLE:
      case t_types::TypeValue::TYPE_STRING:
      case t_types::TypeValue::TYPE_BINARY:
      case t_types::TypeValue::TYPE_LIST:
      case t_types::TypeValue::TYPE_SET:
      case t_types::TypeValue::TYPE_MAP:
        return 8;
      case t_types::TypeValue::TYPE_STRUCT: {
        size_t align = 1;
        const size_t kMaxAlign = 8;
        t_struct const* strct = static_cast<t_struct const*>(type);
        for (auto const* member : strct->get_members()) {
          size_t member_align = compute_alignment(member);
          if (member_align == 0) {
            // Unknown alignment, bail out.
            return 0;
          }
          align = std::max(align, member_align);
          if (align == kMaxAlign) {
            // No need to continue because the struct already has the maximum
            // alignment.
            return align;
          }
        }
        // The __isset member that is generated in the presence of non-required
        // fields doesn't affect the alignment, because, having only bool
        // fields, it has the alignments of 1.
        return align;
      }
      default:
        return 0;
    }
  }

  // Returns the struct members reordered to minimize padding if the
  // cpp.minimize_padding annotation is specified.
  const std::vector<t_field*>& get_members_in_layout_order() {
    auto const& members = strct_->get_members();
    if (strct_->annotations_.find("cpp.minimize_padding") ==
        strct_->annotations_.end()) {
      return members;
    }

    if (members.size() == fields_in_layout_order_.size()) {
      // Already reordered.
      return fields_in_layout_order_;
    }

    // Compute field alignments.
    struct FieldAlign {
      t_field* field = nullptr;
      size_t align = 0;
    };
    std::vector<FieldAlign> field_alignments;
    field_alignments.reserve(members.size());
    for (t_field* member : members) {
      auto align = compute_alignment(member);
      if (align == 0) {
        // Unknown alignment, don't reorder anything.
        return members;
      }
      field_alignments.push_back(FieldAlign{member, align});
    }

    // Sort by decreasing alignment using stable sort to avoid unnecessary
    // reordering.
    std::stable_sort(
        field_alignments.begin(),
        field_alignments.end(),
        [](auto const& lhs, auto const& rhs) { return lhs.align > rhs.align; });

    // Construct the reordered field vector.
    fields_in_layout_order_.reserve(members.size());
    std::transform(
        field_alignments.begin(),
        field_alignments.end(),
        std::back_inserter(fields_in_layout_order_),
        [](FieldAlign const& fa) { return fa.field; });
    return fields_in_layout_order_;
  }

  mstch::node fields_in_layout_order() {
    return generate_elements(
        get_members_in_layout_order(),
        generators_->field_generator_.get(),
        generators_,
        cache_);
  }

  std::shared_ptr<cpp2_generator_context> context_;

  std::vector<t_field*> fields_in_layout_order_;
};

class mstch_cpp2_function : public mstch_function {
 public:
  mstch_cpp2_function(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_function(function, generators, cache, pos) {
    register_methods(
        this,
        {
            {"function:coroutine?", &mstch_cpp2_function::coroutine},
            {"function:eb", &mstch_cpp2_function::event_based},
            {"function:cpp_name", &mstch_cpp2_function::cpp_name},
            {"function:stack_arguments?",
             &mstch_cpp2_function::stack_arguments},
        });
  }
  mstch::node coroutine() {
    return bool(function_->annotations_.count("cpp.coroutine"));
  }
  mstch::node event_based() {
    return function_->annotations_.count("thread") &&
        function_->annotations_.at("thread") == "eb";
  }
  mstch::node cpp_name() {
    return get_cpp_name(function_);
  }
  mstch::node stack_arguments() {
    return cpp2::is_stack_arguments(cache_->parsed_options_, *function_);
  }
};

class mstch_cpp2_service : public mstch_service {
 public:
  mstch_cpp2_service(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_service(service, generators, cache, pos) {
    register_methods(
        this,
        {
            {"service:program_name", &mstch_cpp2_service::program_name},
            {"service:include_prefix", &mstch_cpp2_service::include_prefix},
            {"service:thrift_includes", &mstch_cpp2_service::thrift_includes},
            {"service:namespace_cpp2", &mstch_cpp2_service::namespace_cpp2},
            {"service:oneway_functions", &mstch_cpp2_service::oneway_functions},
            {"service:oneways?", &mstch_cpp2_service::has_oneway},
            {"service:cpp_includes", &mstch_cpp2_service::cpp_includes},
            {"service:metadata_name", &mstch_cpp2_service::metadata_name},
        });
  }
  std::string get_service_namespace(t_program const* program) override {
    return t_mstch_cpp2_generator::get_cpp2_namespace(program);
  }
  mstch::node program_name() {
    return service_->get_program()->get_name();
  }
  mstch::node cpp_includes() {
    return t_mstch_cpp2_generator::cpp_includes(service_->get_program());
  }
  mstch::node include_prefix() {
    return t_mstch_cpp2_generator::include_prefix(
        service_->get_program(), cache_->parsed_options_["include_prefix"]);
  }
  mstch::node thrift_includes() {
    mstch::array a{};
    for (auto const* program :
         service_->get_program()->get_included_programs()) {
      const auto& program_id = program->get_path();
      if (!cache_->programs_.count(program_id)) {
        cache_->programs_[program_id] =
            generators_->program_generator_->generate(
                program, generators_, cache_);
      }
      a.push_back(cache_->programs_[program_id]);
    }
    return a;
  }
  mstch::node namespace_cpp2() {
    return t_mstch_cpp2_generator::get_namespace_array(service_->get_program());
  }
  mstch::node oneway_functions() {
    std::vector<t_function const*> oneway_functions;
    for (auto const* function : service_->get_functions()) {
      if (function->is_oneway()) {
        oneway_functions.push_back(function);
      }
    }
    return generate_elements(
        oneway_functions,
        generators_->function_generator_.get(),
        generators_,
        cache_);
  }
  mstch::node has_oneway() {
    for (auto const* function : service_->get_functions()) {
      if (function->is_oneway()) {
        return true;
      }
    }
    return false;
  }
  mstch::node metadata_name() {
    return service_->get_program()->get_name() + "_" + service_->get_name();
  }
};

class mstch_cpp2_annotation : public mstch_annotation {
 public:
  mstch_cpp2_annotation(
      const std::string& key,
      const std::string& val,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_annotation(key, val, generators, cache, pos, index) {
    register_methods(
        this,
        {
            {"annotation:safe_key", &mstch_cpp2_annotation::safe_key},
            {"annotation:fatal_string", &mstch_cpp2_annotation::fatal_string},
        });
  }
  mstch::node safe_key() {
    return get_fatal_string_short_id(key_);
  }
  mstch::node fatal_string() {
    return render_fatal_string(val_);
  }
};

class mstch_cpp2_const : public mstch_const {
 public:
  mstch_cpp2_const(
      t_const const* cnst,
      t_const const* current_const,
      t_type const* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index,
      const std::string& field_name)
      : mstch_const(
            cnst,
            current_const,
            expected_type,
            generators,
            cache,
            pos,
            index,
            field_name) {
    register_methods(
        this,
        {
            {"constant:enum_value", &mstch_cpp2_const::enum_value},
        });
  }
  mstch::node enum_value() {
    if (cnst_->get_type()->is_enum()) {
      auto const* enm = static_cast<t_enum const*>(cnst_->get_type());
      auto const* enm_val = enm->find_value(cnst_->get_value()->get_integer());
      if (enm_val) {
        return enm_val->get_name();
      } else {
        return std::to_string(cnst_->get_value()->get_integer());
      }
    }
    return mstch::node();
  }
};

class mstch_cpp2_program : public mstch_program {
 public:
  mstch_cpp2_program(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      boost::optional<int32_t> split_id = boost::none)
      : mstch_program(program, generators, cache, pos), split_id_(split_id) {
    register_methods(
        this,
        {
            {"program:cpp_includes", &mstch_cpp2_program::cpp_includes},
            {"program:namespace_cpp2", &mstch_cpp2_program::namespace_cpp2},
            {"program:include_prefix", &mstch_cpp2_program::include_prefix},
            {"program:cpp_declare_hash?",
             &mstch_cpp2_program::cpp_declare_hash},
            {"program:thrift_includes", &mstch_cpp2_program::thrift_includes},
            {"program:frozen?", &mstch_cpp2_program::frozen},
            {"program:frozen_packed?", &mstch_cpp2_program::frozen_packed},
            {"program:indirection?", &mstch_cpp2_program::has_indirection},
            {"program:json?", &mstch_cpp2_program::json},
            {"program:nimble?", &mstch_cpp2_program::nimble},
            {"program:fatal_languages", &mstch_cpp2_program::fatal_languages},
            {"program:fatal_enums", &mstch_cpp2_program::fatal_enums},
            {"program:fatal_unions", &mstch_cpp2_program::fatal_unions},
            {"program:fatal_structs", &mstch_cpp2_program::fatal_structs},
            {"program:fatal_constants", &mstch_cpp2_program::fatal_constants},
            {"program:fatal_services", &mstch_cpp2_program::fatal_services},
            {"program:fatal_identifiers",
             &mstch_cpp2_program::fatal_identifiers},
            {"program:fatal_data_member",
             &mstch_cpp2_program::fatal_data_member},
            {"program:indirection_recursive?",
             &mstch_cpp2_program::has_indirection_recursive},
            {"program:indirection", &mstch_cpp2_program::indirection},
            {"program:enforce_required?",
             &mstch_cpp2_program::enforce_required},
            {"program:gen_metadata?", &mstch_cpp2_program::gen_metadata},
        });
  }
  std::string get_program_namespace(t_program const* program) override {
    return t_mstch_cpp2_generator::get_cpp2_namespace(program);
  }

  std::vector<const t_typedef*> alias_to_struct() {
    std::vector<const t_typedef*> result;
    for (const t_typedef* i : program_->get_typedefs()) {
      const t_type* alias = i->get_type();
      if (alias->is_typedef() && alias->annotations_.count("cpp.type")) {
        const t_type* ttype = i->get_type()->get_true_type();
        if (ttype->is_struct() || ttype->is_xception()) {
          result.push_back(i);
        }
      }
    }
    return result;
  }
  template <typename Node>
  void collect_fatal_string_annotated(
      std::map<std::string, std::string>& fatal_strings,
      const Node* node) {
    // TODO: extra copy
    auto cpp_name = get_cpp_name(node);
    fatal_strings.emplace(get_fatal_string_short_id(cpp_name), cpp_name);
    for (const auto& a : node->annotations_) {
      if (!is_annotation_blacklisted_in_fatal(a.first)) {
        fatal_strings.emplace(get_fatal_string_short_id(a.first), a.first);
      }
    }
  }
  std::vector<std::string> get_fatal_enum_names() {
    std::vector<std::string> result;
    for (const auto* enm : program_->get_enums()) {
      result.push_back(get_fatal_string_short_id(enm->get_name()));
    }
    return result;
  }
  std::vector<std::string> get_fatal_union_names() {
    std::vector<std::string> result;
    for (const auto* obj : program_->get_objects()) {
      if (obj->is_union()) {
        result.push_back(get_fatal_string_short_id(obj->get_name()));
      }
    }
    return result;
  }
  std::vector<std::string> get_fatal_struct_names() {
    std::vector<std::string> result;
    for (const auto* obj : program_->get_objects()) {
      if (!obj->is_union()) {
        result.push_back(get_fatal_string_short_id(obj->get_name()));
      }
    }
    // typedefs resolve to struct
    for (const t_typedef* i : alias_to_struct()) {
      result.push_back(get_fatal_string_short_id(i->get_name()));
    }
    return result;
  }
  std::vector<std::string> get_fatal_constant_names() {
    std::vector<std::string> result;
    for (const auto* cnst : program_->get_consts()) {
      result.push_back(get_fatal_string_short_id(cnst->get_name()));
    }
    return result;
  }
  std::vector<std::string> get_fatal_service_names() {
    std::vector<std::string> result;
    for (const auto* service : program_->get_services()) {
      result.push_back(get_fatal_string_short_id(service->get_name()));
    }
    return result;
  }
  mstch::node to_fatal_string_array(const std::vector<std::string>&& vec) {
    mstch::array a;
    for (size_t i = 0; i < vec.size(); i++) {
      a.push_back(mstch::map{
          {"fatal_string:name", vec.at(i)},
          {"last?", i == vec.size() - 1},
      });
    }
    return mstch::map{{"fatal_strings:items", a}};
  }

  mstch::node namespace_cpp2() {
    return t_mstch_cpp2_generator::get_namespace_array(program_);
  }
  mstch::node cpp_includes() {
    return t_mstch_cpp2_generator::cpp_includes(program_);
  }
  mstch::node include_prefix() {
    return t_mstch_cpp2_generator::include_prefix(
        program_, cache_->parsed_options_["include_prefix"]);
  }
  mstch::node cpp_declare_hash() {
    bool cpp_declare_in_structs = std::any_of(
        program_->get_structs().begin(),
        program_->get_structs().end(),
        [](const auto* strct) {
          return strct->annotations_.count("cpp.declare_hash") ||
              strct->annotations_.count("cpp2.declare_hash");
        });
    bool cpp_declare_in_typedefs = std::any_of(
        program_->get_typedefs().begin(),
        program_->get_typedefs().end(),
        [](const auto* typedf) {
          return typedf->get_type()->annotations_.count("cpp.declare_hash") ||
              typedf->get_type()->annotations_.count("cpp2.declare_hash");
        });
    return cpp_declare_in_structs || cpp_declare_in_typedefs;
  }
  mstch::node thrift_includes() {
    mstch::array a{};
    for (auto const* program : program_->get_included_programs()) {
      const auto& program_id = program->get_path();
      if (!cache_->programs_.count(program_id)) {
        cache_->programs_[program_id] =
            generators_->program_generator_->generate(
                program, generators_, cache_);
      }
      a.push_back(cache_->programs_[program_id]);
    }
    return a;
  }
  mstch::node frozen() {
    return cache_->parsed_options_.count("frozen") != 0;
  }
  mstch::node frozen_packed() {
    auto iter = cache_->parsed_options_.find("frozen");
    return iter != cache_->parsed_options_.end() && iter->second == "packed";
  }
  mstch::node has_indirection() {
    // NOTE: this can be problematic, since typedef can be from an imported
    // thrift file.
    for (auto const* typedf : program_->get_typedefs()) {
      if (typedf->get_type()->annotations_.count("cpp.indirection")) {
        return true;
      }
    }
    return false;
  }
  mstch::node json() {
    return cache_->parsed_options_.count("json") != 0;
  }
  mstch::node nimble() {
    return cache_->parsed_options_.count("nimble") != 0;
  }
  mstch::node fatal_languages() {
    mstch::array a;
    size_t size = program_->get_namespaces().size();
    size_t idx = 0;
    for (const auto& pair : program_->get_namespaces()) {
      a.push_back(mstch::map{
          {"language:safe_name", get_fatal_string_short_id(pair.first)},
          {"language:safe_namespace",
           get_fatal_namesoace_name_short_id(pair.first, pair.second)},
          {"last?", idx == size - 1},
      });
      ++idx;
    }
    return mstch::map{{"fatal_languages:items", a}};
  }
  mstch::node fatal_enums() {
    return to_fatal_string_array(get_fatal_enum_names());
  }
  mstch::node fatal_unions() {
    return to_fatal_string_array(get_fatal_union_names());
  }
  mstch::node fatal_structs() {
    return to_fatal_string_array(get_fatal_struct_names());
  }
  mstch::node fatal_constants() {
    return to_fatal_string_array(get_fatal_constant_names());
  }
  mstch::node fatal_services() {
    return to_fatal_string_array(get_fatal_service_names());
  }
  mstch::node fatal_identifiers() {
    std::map<std::string, std::string> unique_names;
    unique_names.emplace(
        get_fatal_string_short_id(program_->get_name()), program_->get_name());
    // languages and namespaces
    for (const auto& pair : program_->get_namespaces()) {
      unique_names.emplace(get_fatal_string_short_id(pair.first), pair.first);
      unique_names.emplace(
          get_fatal_namesoace_name_short_id(pair.first, pair.second),
          get_fatal_namesoace(pair.first, pair.second));
    }
    // enums
    for (const auto* enm : program_->get_enums()) {
      collect_fatal_string_annotated(unique_names, enm);
      unique_names.emplace(
          get_fatal_string_short_id(enm->get_name()), enm->get_name());
      for (const auto& i : enm->get_enum_values()) {
        collect_fatal_string_annotated(unique_names, i);
      }
    }
    // structs, unions and exceptions
    for (const auto* obj : program_->get_objects()) {
      if (obj->is_union()) {
        // When generating <program_name>_fatal_union.h, we will generate
        // <union_name>_Type_enum_traits
        unique_names.emplace("Type", "Type");
      }
      collect_fatal_string_annotated(unique_names, obj);
      for (const auto& m : obj->get_members()) {
        collect_fatal_string_annotated(unique_names, m);
      }
    }
    // consts
    for (const auto* cnst : program_->get_consts()) {
      unique_names.emplace(
          get_fatal_string_short_id(cnst->get_name()), cnst->get_name());
    }
    // services
    for (const auto* service : program_->get_services()) {
      // function annotations are not currently included.
      unique_names.emplace(
          get_fatal_string_short_id(service->get_name()), service->get_name());
      for (const auto* f : service->get_functions()) {
        unique_names.emplace(
            get_fatal_string_short_id(f->get_name()), f->get_name());
        for (const auto* p : f->get_arglist()->get_members()) {
          unique_names.emplace(
              get_fatal_string_short_id(p->get_name()), p->get_name());
        }
      }
    }
    // typedefs resolve to struct
    for (const t_typedef* i : alias_to_struct()) {
      unique_names.emplace(
          get_fatal_string_short_id(i->get_name()), i->get_name());
    }

    mstch::array a;
    for (const auto& name : unique_names) {
      a.push_back(mstch::map{
          {"identifier:name", name.first},
          {"identifier:fatal_string", render_fatal_string(name.second)},
      });
    }
    return a;
  }
  mstch::node fatal_data_member() {
    std::unordered_set<std::string> fields;
    std::vector<const std::string*> ordered_fields;
    for (const t_struct* s : program_->get_objects()) {
      if (!s->is_union()) {
        for (const t_field* f : s->get_members()) {
          auto result = fields.insert(get_cpp_name(f));
          if (result.second) {
            ordered_fields.push_back(&*result.first);
          }
        }
      }
    }
    mstch::array a;
    for (const auto& f : ordered_fields) {
      a.push_back(*f);
    }
    return a;
  }
  mstch::node has_indirection_recursive() {
    for (const t_struct* s : program_->get_objects()) {
      if (s->is_union()) {
        continue;
      }
      for (const t_field* f : s->get_members()) {
        if (!f->get_type()->is_typedef()) {
          continue;
        }
        const auto* true_type = f->get_type()->get_true_type();
        if (true_type->annotations_.count("cpp.indirection")) {
          return true;
        }
      }
    }
    return false;
  }
  mstch::node indirection() {
    std::map<std::string, std::map<std::string, std::string>> indirections;
    for (const t_struct* s : program_->get_objects()) {
      if (s->is_union()) {
        continue;
      }
      for (const t_field* f : s->get_members()) {
        if (!f->get_type()->is_typedef()) {
          continue;
        }
        const auto* true_type = f->get_type()->get_true_type();
        if (true_type->annotations_.count("cpp.indirection")) {
          if (indirections.count(s->get_name()) > 0) {
            indirections.at(s->get_name())
                .emplace(
                    get_cpp_name(f),
                    true_type->annotations_.at("cpp.indirection"));
          } else {
            indirections.emplace(
                s->get_name(),
                std::map<std::string, std::string>{
                    {get_cpp_name(f),
                     true_type->annotations_.at("cpp.indirection")}});
          }
        }
      }
    }
    mstch::array result;
    for (const auto& indirection : indirections) {
      mstch::array per_struct;
      for (const auto& itr : indirection.second) {
        per_struct.push_back(
            mstch::map{{"indirection:field_name", itr.first},
                       {"indirection:indirection_name", itr.second}});
      }
      result.push_back(
          mstch::map{{"indirection:struct_name", indirection.first},
                     {"indirection:indirection_fields", per_struct}});
    }
    return result;
  }
  mstch::node enforce_required() {
    return cache_->parsed_options_.count("deprecated_enforce_required") != 0;
  }
  mstch::node gen_metadata() {
    return cache_->parsed_options_.count("no_metadata") == 0;
  }

 private:
  boost::optional<std::vector<t_struct*>> objects_;
  boost::optional<std::vector<t_enum*>> enums_;
  const boost::optional<int32_t> split_id_;

  const std::vector<t_enum*>& get_program_enums() override {
    if (!enums_) {
      init_objects_enums();
    }

    return *enums_;
  }

  const std::vector<t_struct*>& get_program_objects() override {
    if (!objects_) {
      init_objects_enums();
    }

    return *objects_;
  }

  void init_objects_enums() {
    const auto& prog_objects = program_->get_objects();
    const auto& prog_enums = program_->get_enums();

    if (!split_id_) {
      objects_ = gen_sorted_objects(program_, prog_objects);
      enums_ = prog_enums;
      return;
    }

    int32_t split_count =
        std::max(cpp2::get_split_count(cache_->parsed_options_), 1);

    objects_.emplace();
    enums_.emplace();

    const size_t cnt = prog_objects.size() + prog_enums.size();
    for (size_t i = split_id_.value_or(0); i < cnt; i += split_count) {
      if (i < prog_objects.size()) {
        objects_->push_back(prog_objects[i]);
      } else {
        enums_->push_back(prog_enums[i - prog_objects.size()]);
      }
    }
  }

  static std::vector<t_struct*> gen_sorted_objects(
      const t_program* program,
      const std::vector<t_struct*>& objects) {
    auto edges = [program](t_struct* obj) {
      std::vector<t_struct*> deps;
      for (auto* f : obj->get_members()) {
        // Ignore ref fields.
        if (f->annotations_.count("cpp.ref") ||
            f->annotations_.count("cpp2.ref") ||
            f->annotations_.count("cpp.ref_type") ||
            f->annotations_.count("cpp2.ref_type")) {
          continue;
        }

        auto add_dependency = [&](t_type* type) {
          if (type->is_struct()) {
            auto* strct = dynamic_cast<t_struct*>(type);
            // We're only interested in types defined in the current program.
            if (strct->get_program() == program) {
              deps.emplace_back(strct);
            }
          }
        };

        auto t = f->get_type()->get_true_type();
        if (t->is_map()) {
          auto* map = dynamic_cast<t_map*>(t);
          add_dependency(map->get_key_type());
          add_dependency(map->get_val_type());
        } else {
          add_dependency(t);
        }
      }

      // Order all deps in the order they are defined in.
      std::sort(
          deps.begin(), deps.end(), [](const t_struct* a, const t_struct* b) {
            return a->get_lineno() < b->get_lineno();
          });

      return deps;
    };
    return topological_sort<t_struct*>(objects.begin(), objects.end(), edges);
  }
};

class enum_cpp2_generator : public enum_generator {
 public:
  enum_cpp2_generator() = default;
  ~enum_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_enum>(enm, generators, cache, pos);
  }
};

class enum_value_cpp2_generator : public enum_value_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_enum_value>(
        enm_value, generators, cache, pos);
  }
};

class type_cpp2_generator : public type_generator {
 public:
  type_cpp2_generator() = default;
  ~type_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_type>(type, generators, cache, pos);
  }
};

class field_cpp2_generator : public field_generator {
 public:
  field_cpp2_generator() = default;
  ~field_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const override {
    return std::make_shared<mstch_cpp2_field>(
        field, generators, cache, pos, index);
  }
};

class function_cpp2_generator : public function_generator {
 public:
  function_cpp2_generator() = default;
  ~function_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_function>(
        function, generators, cache, pos);
  }
};

class struct_cpp2_generator : public struct_generator {
 public:
  explicit struct_cpp2_generator(
      std::shared_ptr<cpp2_generator_context> context)
      : context_(std::move(context)) {}
  ~struct_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_struct>(
        strct, generators, cache, pos, context_);
  }

 private:
  std::shared_ptr<cpp2_generator_context> context_;
};

class service_cpp2_generator : public service_generator {
 public:
  service_cpp2_generator() = default;
  ~service_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_service>(
        service, generators, cache, pos);
  }
};

class annotation_cpp2_generator : public annotation_generator {
 public:
  annotation_cpp2_generator() = default;
  ~annotation_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_annotation const& keyval,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const override {
    return std::make_shared<mstch_cpp2_annotation>(
        keyval.key, keyval.val, generators, cache, pos, index);
  }
};

class const_cpp2_generator : public const_generator {
 public:
  const_cpp2_generator() = default;
  ~const_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_const const* cnst,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      t_type const* expected_type = nullptr,
      const std::string& field_name = std::string()) const override {
    return std::make_shared<mstch_cpp2_const>(
        cnst,
        current_const,
        expected_type,
        generators,
        cache,
        pos,
        index,
        field_name);
  }
};

class const_value_cpp2_generator : public const_value_generator {
 public:
  const_value_cpp2_generator() = default;
  ~const_value_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_const_value const* const_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      t_type const* expected_type = nullptr) const override {
    return std::make_shared<mstch_cpp2_const_value>(
        const_value,
        current_const,
        expected_type,
        generators,
        cache,
        pos,
        index);
  }
};

class program_cpp2_generator : public program_generator {
 public:
  program_cpp2_generator() = default;
  ~program_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_program>(
        program, generators, cache, pos);
  }
  std::shared_ptr<mstch_base> generate_with_split_id(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      int32_t split_id) const {
    return std::make_shared<mstch_cpp2_program>(
        program, generators, cache, ELEMENT_POSITION::NONE, split_id);
  }
};

t_mstch_cpp2_generator::t_mstch_cpp2_generator(
    t_program* program,
    t_generation_context context,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& /*option_string*/)
    : t_mstch_generator(
          program,
          std::move(context),
          "cpp2",
          parsed_options,
          true),
      context_(std::make_shared<cpp2_generator_context>(
          cpp2_generator_context::create(program))) {
  out_dir_base_ = "gen-cpp2";
}

void t_mstch_cpp2_generator::generate_program() {
  auto const* program = get_program();
  set_mstch_generators();

  if (cache_->parsed_options_.count("reflection") ||
      cache_->parsed_options_.count("visitation")) {
    generate_reflection(program);
    if (cache_->parsed_options_.count("visitation")) {
      generate_visitation(program);
    }
  }

  generate_structs(program);
  generate_constants(program);
  for (const auto* service : program->get_services()) {
    generate_service(service);
  }
  generate_metadata(program);
}

void t_mstch_cpp2_generator::set_mstch_generators() {
  generators_->set_enum_generator(std::make_unique<enum_cpp2_generator>());
  generators_->set_enum_value_generator(
      std::make_unique<enum_value_cpp2_generator>());
  generators_->set_type_generator(std::make_unique<type_cpp2_generator>());
  generators_->set_field_generator(std::make_unique<field_cpp2_generator>());
  generators_->set_function_generator(
      std::make_unique<function_cpp2_generator>());
  generators_->set_struct_generator(
      std::make_unique<struct_cpp2_generator>(context_));
  generators_->set_service_generator(
      std::make_unique<service_cpp2_generator>());
  generators_->set_const_generator(std::make_unique<const_cpp2_generator>());
  generators_->set_const_value_generator(
      std::make_unique<const_value_cpp2_generator>());
  generators_->set_annotation_generator(
      std::make_unique<annotation_cpp2_generator>());
  generators_->set_program_generator(
      std::make_unique<program_cpp2_generator>());
}

void t_mstch_cpp2_generator::generate_constants(t_program const* program) {
  std::string name = program->get_name();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }
  render_to_file(
      cache_->programs_[id], "module_constants.h", name + "_constants.h");
  render_to_file(
      cache_->programs_[id], "module_constants.cpp", name + "_constants.cpp");
}

void t_mstch_cpp2_generator::generate_metadata(const t_program* program) {
  const auto& name = program->get_name();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }

  render_to_file(
      cache_->programs_[id], "module_metadata.h", name + "_metadata.h");
  if (cache_->parsed_options_.count("no_metadata") == 0) {
    render_to_file(
        cache_->programs_[id], "module_metadata.cpp", name + "_metadata.cpp");
  }
}

void t_mstch_cpp2_generator::generate_reflection(t_program const* program) {
  const auto& name = program->get_name();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }

  // Combo include: all
  render_to_file(
      cache_->programs_[id], "module_fatal_all.h", name + "_fatal_all.h");
  // Combo include: types
  render_to_file(
      cache_->programs_[id], "module_fatal_types.h", name + "_fatal_types.h");
  // Unique Compile-time Strings, Metadata tags and Metadata registration
  render_to_file(cache_->programs_[id], "module_fatal.h", name + "_fatal.h");

  render_to_file(
      cache_->programs_[id], "module_fatal_enum.h", name + "_fatal_enum.h");
  render_to_file(
      cache_->programs_[id], "module_fatal_union.h", name + "_fatal_union.h");
  render_to_file(
      cache_->programs_[id], "module_fatal_struct.h", name + "_fatal_struct.h");
  render_to_file(
      cache_->programs_[id],
      "module_fatal_constant.h",
      name + "_fatal_constant.h");
  render_to_file(
      cache_->programs_[id],
      "module_fatal_service.h",
      name + "_fatal_service.h");
}

void t_mstch_cpp2_generator::generate_visitation(const t_program* program) {
  const auto& name = program->get_name();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }

  render_to_file(
      cache_->programs_[id], "module_visitation.h", name + "_visitation.h");

  render_to_file(
      cache_->programs_[id],
      "module_for_each_field.h",
      name + "_for_each_field.h");

  render_to_file(
      cache_->programs_[id], "module_visit_union.h", name + "_visit_union.h");
}

void t_mstch_cpp2_generator::generate_structs(t_program const* program) {
  const auto& name = program->get_name();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }
  render_to_file(cache_->programs_[id], "module_data.h", name + "_data.h");
  render_to_file(cache_->programs_[id], "module_data.cpp", name + "_data.cpp");
  render_to_file(cache_->programs_[id], "module_types.h", name + "_types.h");
  render_to_file(
      cache_->programs_[id], "module_types.tcc", name + "_types.tcc");

  if (auto split_count = cpp2::get_split_count(parsed_options_)) {
    auto digit = to_string(split_count - 1).size();
    for (int split_id = 0; split_id < split_count; ++split_id) {
      auto s = to_string(split_id);
      s = string(digit - s.size(), '0') + s;
      render_to_file(
          program_cpp2_generator{}.generate_with_split_id(
              program, generators_, cache_, split_id),
          "module_types.cpp",
          name + "_types." + s + ".split.cpp");
    }
  } else {
    render_to_file(
        cache_->programs_[id], "module_types.cpp", name + "_types.cpp");
  }

  render_to_file(
      cache_->programs_[id],
      "module_types_custom_protocol.h",
      name + "_types_custom_protocol.h");
  if (cache_->parsed_options_.count("frozen2")) {
    render_to_file(
        cache_->programs_[id], "module_layouts.h", name + "_layouts.h");
    render_to_file(
        cache_->programs_[id], "module_layouts.cpp", name + "_layouts.cpp");
  }
}

void t_mstch_cpp2_generator::generate_service(t_service const* service) {
  const auto& id = get_program()->get_path();
  std::string name = service->get_name();
  std::string service_id = id + name;
  if (!cache_->services_.count(service_id)) {
    cache_->services_[service_id] =
        generators_->service_generator_->generate(service, generators_, cache_);
  }
  render_to_file(
      cache_->services_[service_id],
      "ServiceAsyncClient.h",
      name + "AsyncClient.h");
  render_to_file(
      cache_->services_[service_id],
      "ServiceAsyncClient.cpp",
      name + "AsyncClient.cpp");
  render_to_file(cache_->services_[service_id], "service.cpp", name + ".cpp");
  render_to_file(cache_->services_[service_id], "service.h", name + ".h");
  render_to_file(cache_->services_[service_id], "service.tcc", name + ".tcc");
  render_to_file(
      cache_->services_[service_id],
      "types_custom_protocol.h",
      name + "_custom_protocol.h");

  std::vector<std::array<std::string, 3>> protocols = {
      {{"binary", "BinaryProtocol", "T_BINARY_PROTOCOL"}},
      {{"compact", "CompactProtocol", "T_COMPACT_PROTOCOL"}},
  };
  for (const auto& protocol : protocols) {
    render_to_file(
        cache_->services_[service_id],
        "service_processmap_protocol.cpp",
        name + "_processmap_" + protocol.at(0) + ".cpp");
  }
}

std::string t_mstch_cpp2_generator::get_cpp2_namespace(
    t_program const* program) {
  return cpp2::get_gen_namespace(*program);
}

mstch::array t_mstch_cpp2_generator::get_namespace_array(
    t_program const* program) {
  auto const v = cpp2::get_gen_namespace_components(*program);
  mstch::array a;
  for (auto it = v.begin(); it != v.end(); ++it) {
    mstch::map m;
    m.emplace("namespace:name", *it);
    a.push_back(m);
  }
  for (auto itr = a.begin(); itr != a.end(); ++itr) {
    boost::get<mstch::map>(*itr).emplace("first?", itr == a.begin());
    boost::get<mstch::map>(*itr).emplace("last?", std::next(itr) == a.end());
  }
  return a;
}

mstch::node t_mstch_cpp2_generator::cpp_includes(t_program const* program) {
  mstch::array a{};
  for (auto include : program->get_cpp_includes()) {
    mstch::map cpp_include;
    if (include.at(0) != '<') {
      include = "\"" + include + "\"";
    }
    cpp_include.emplace("cpp_include", std::string(include));
    a.push_back(cpp_include);
  }
  return a;
}

mstch::node t_mstch_cpp2_generator::include_prefix(
    t_program const* program,
    std::string& include_prefix) {
  auto prefix = program->get_include_prefix();
  if (prefix.empty()) {
    if (include_prefix.empty()) {
      return prefix;
    } else {
      return include_prefix + "/gen-cpp2/";
    }
  }
  if (boost::filesystem::path(prefix).has_root_directory()) {
    return include_prefix + "/gen-cpp2/";
  }
  return prefix + "gen-cpp2/";
}

namespace {
class annotation_validator : public validator {
 public:
  using validator::visit;

  /**
   * Make sure there is no incompatible annotation.
   */
  bool visit(t_struct* s) override;
};

bool annotation_validator::visit(t_struct* s) {
  for (auto* member : s->get_members()) {
    if (!member->is_mixin()) {
      continue;
    }
    for (const auto& i : member->annotations_) {
      if ((i.first == "cpp.ref" || i.first == "cpp2.ref") &&
          i.second == "true") {
        add_error(
            member->get_lineno(),
            "Mixin field `" + member->get_name() +
                "` can not have annotation (" + i.first + " = " + i.second +
                ")");
      }
    }
  }
  return true;
}
class service_method_validator : public validator {
 public:
  explicit service_method_validator(
      std::map<std::string, std::string> const& options)
      : options_(options) {}

  using validator::visit;
  /**
   * Make sure there is no 'cpp.coroutine' annotation set when
   * 'stack_arguments' is turned on.
   */
  bool visit(t_service* service) override;

 private:
  std::map<std::string, std::string> options_;
};

bool service_method_validator::visit(t_service* service) {
  for (const auto func : service->get_functions()) {
    auto suppress_key = "cpp.coroutine_stack_arguments_broken_suppress_error";
    auto const& annots = func->annotations_;
    bool suppressed = annots.count(suppress_key) != 0;
    if (suppressed) {
      continue;
    }
    bool is_coro = annots.count("cpp.coroutine") != 0;
    if (!is_coro) {
      continue;
    }
    bool is_sa = cpp2::is_stack_arguments(options_, *func);
    if (!is_sa) {
      continue;
    }
    // when cpp.coroutine and stack_arguments are both on, return failure if
    // this function has complex types (including string and binary).
    auto args = func->get_arglist()->get_members();
    bool ok = std::all_of(args.begin(), args.end(), [](auto arg) {
      auto type = arg->get_type()->get_true_type();
      return type->is_base_type() && !type->is_string_or_binary();
    });

    if (!ok) {
      add_error(
          func->get_lineno(),
          service->get_name() + ":" + func->get_name() +
              " use of cpp.coroutine and stack_arguments together is "
              "disallowed");
    }
  }
  return true;
}
class splits_validator : public validator {
 public:
  explicit splits_validator(int split_count) : split_count_(split_count) {}

  using validator::visit;
  bool visit(t_program* program) override {
    set_program(program);
    const int32_t object_count =
        program->get_objects().size() + program->get_enums().size();
    if (split_count_ != 0 && split_count_ > object_count) {
      add_error(
          boost::none,
          "types_cpp_splits=" + to_string(split_count_) +
              " is misconfigured: it can not be greater than number of object, which is " +
              to_string(object_count) + ".");
    }
    return true;
  }

 private:
  int32_t split_count_;
};
} // namespace

void t_mstch_cpp2_generator::fill_validator_list(validator_list& l) const {
  l.add<annotation_validator>();
  l.add<service_method_validator>(this->parsed_options_);
  l.add<splits_validator>(cpp2::get_split_count(parsed_options_));
}

THRIFT_REGISTER_GENERATOR(mstch_cpp2, "cpp2", "");

} // namespace compiler
} // namespace thrift
} // namespace apache
