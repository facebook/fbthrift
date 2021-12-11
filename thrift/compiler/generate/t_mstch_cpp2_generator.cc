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
#include <cassert>
#include <memory>
#include <queue>
#include <set>
#include <vector>

#include <boost/algorithm/string/replace.hpp>

#include <thrift/compiler/gen/cpp/type_resolver.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>
#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/util.h>
#include <thrift/compiler/validator/validator.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

std::string const& get_cpp_template(const t_type* type) {
  return type->get_annotation({"cpp.template", "cpp2.template"});
}

bool is_cpp_ref_unique_either(const t_field* f) {
  return cpp2::is_unique_ref(f) || cpp2::is_implicit_ref(f->get_type());
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
      "cpp.internal.deprecated._data.method",
  };
  return black_list.find(key) != black_list.end();
}

bool same_types(const t_type* a, const t_type* b) {
  if (!a || !b) {
    return false;
  }

  if (get_cpp_template(a) != get_cpp_template(b) ||
      cpp2::get_type(a) != cpp2::get_type(b)) {
    return false;
  }

  const auto* resolved_a = a->get_true_type();
  const auto* resolved_b = b->get_true_type();

  if (resolved_a->get_type_value() != resolved_b->get_type_value()) {
    return false;
  }

  switch (resolved_a->get_type_value()) {
    case t_type::type::t_list: {
      const auto* list_a = static_cast<const t_list*>(resolved_a);
      const auto* list_b = static_cast<const t_list*>(resolved_b);
      return same_types(list_a->get_elem_type(), list_b->get_elem_type());
    }
    case t_type::type::t_set: {
      const auto* set_a = static_cast<const t_set*>(resolved_a);
      const auto* set_b = static_cast<const t_set*>(resolved_b);
      return same_types(set_a->get_elem_type(), set_b->get_elem_type());
    }
    case t_type::type::t_map: {
      const auto* map_a = static_cast<const t_map*>(resolved_a);
      const auto* map_b = static_cast<const t_map*>(resolved_b);
      return same_types(map_a->get_key_type(), map_b->get_key_type()) &&
          same_types(map_a->get_val_type(), map_b->get_val_type());
    }
    default:;
  }
  return true;
}

std::vector<t_annotation> get_fatal_annotations(
    std::map<std::string, annotation_value> annotations) {
  std::vector<t_annotation> fatal_annotations;
  for (const auto& iter : annotations) {
    if (is_annotation_blacklisted_in_fatal(iter.first)) {
      continue;
    }
    fatal_annotations.push_back({iter.first, iter.second});
  }

  return fatal_annotations;
}

std::string get_fatal_string_short_id(const std::string& key) {
  return boost::algorithm::replace_all_copy(key, ".", "_");
}

std::string get_fatal_namesoace_name_short_id(
    const std::string& lang, const std::string& ns) {
  std::string replacement = lang == "cpp" || lang == "cpp2" ? "__" : "_";
  std::string result = boost::algorithm::replace_all_copy(ns, ".", replacement);
  if (lang == "php_path") {
    return boost::algorithm::replace_all_copy(ns, "/", "_");
  }
  return result;
}

std::string get_fatal_namesoace(
    const std::string& lang, const std::string& ns) {
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

std::string get_out_dir_base(
    const std::map<std::string, std::string>& options) {
  return options.find("py3cpp") != options.end() ? "gen-py3cpp" : "gen-cpp2";
}

std::string mangle_field_name(const std::string& name) {
  return "__fbthrift_field_" + name;
}

} // namespace

class cpp2_generator_context {
 public:
  static cpp2_generator_context create() { return cpp2_generator_context(); }

  cpp2_generator_context(cpp2_generator_context&&) = default;
  cpp2_generator_context& operator=(cpp2_generator_context&&) = default;

  bool is_orderable(t_type const& type) {
    std::unordered_set<t_type const*> seen;
    auto& memo = is_orderable_memo_;
    return cpp2::is_orderable(seen, memo, type);
  }

  gen::cpp::type_resolver& resolver() { return resolver_; }

 private:
  cpp2_generator_context() = default;

  std::unordered_map<t_type const*, bool> is_orderable_memo_;
  gen::cpp::type_resolver resolver_;
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
      t_program const* program, std::map<std::string, std::string>& options);

 private:
  void set_mstch_generators();
  void generate_sinit(t_program const* program);
  void generate_reflection(t_program const* program);
  void generate_visitation(t_program const* program);
  void generate_constants(t_program const* program);
  void generate_metadata(t_program const* program);
  void generate_structs(t_program const* program);
  void generate_service(t_service const* service);

  std::shared_ptr<cpp2_generator_context> context_;
  std::unordered_map<std::string, int32_t> client_name_to_split_count_;
};

class mstch_cpp2_enum : public mstch_enum {
 public:
  mstch_cpp2_enum(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum(enm, std::move(generators), std::move(cache), pos) {
    register_methods(
        this,
        {
            {"enum:empty?", &mstch_cpp2_enum::is_empty},
            {"enum:size", &mstch_cpp2_enum::size},
            {"enum:min", &mstch_cpp2_enum::min},
            {"enum:max", &mstch_cpp2_enum::max},
            {"enum:cpp_is_unscoped", &mstch_cpp2_enum::cpp_is_unscoped},
            {"enum:cpp_name", &mstch_cpp2_enum::cpp_name},
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
  mstch::node is_empty() { return enm_->get_enum_values().empty(); }
  mstch::node size() { return std::to_string(enm_->get_enum_values().size()); }
  mstch::node min() {
    if (!enm_->get_enum_values().empty()) {
      auto e_min = std::min_element(
          enm_->get_enum_values().begin(),
          enm_->get_enum_values().end(),
          [](t_enum_value* a, t_enum_value* b) {
            return a->get_value() < b->get_value();
          });
      return cpp2::get_name(*e_min);
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
      return cpp2::get_name(*e_max);
    }
    return mstch::node();
  }
  std::string const& cpp_is_unscoped_() {
    return enm_->get_annotation(
        {"cpp2.deprecated_enum_unscoped", "cpp.deprecated_enum_unscoped"});
  }
  mstch::node cpp_is_unscoped() { return cpp_is_unscoped_(); }
  mstch::node cpp_name() { return cpp2::get_name(enm_); }
  mstch::node cpp_enum_type() {
    static std::string kInt = "int";
    return enm_->get_annotation(
        {"cpp.enum_type", "cpp2.enum_type"},
        cpp_is_unscoped_().empty() ? nullptr : &kInt);
  }
  mstch::node cpp_declare_bitwise_ops() {
    return enm_->get_annotation(
        {"cpp.declare_bitwise_ops", "cpp2.declare_bitwise_ops"});
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
    return get_fatal_annotations(enm_->annotations()).size() > 0;
  }
  mstch::node fatal_annotations() {
    return generate_annotations(get_fatal_annotations(enm_->annotations()));
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
      : mstch_enum_value(
            enm_value, std::move(generators), std::move(cache), pos) {
    register_methods(
        this,
        {
            {"enum_value:name_hash", &mstch_cpp2_enum_value::name_hash},
            {"enum_value:cpp_name", &mstch_cpp2_enum_value::cpp_name},
            {"enum_value:fatal_annotations?",
             &mstch_cpp2_enum_value::has_fatal_annotations},
            {"enum_value:fatal_annotations",
             &mstch_cpp2_enum_value::fatal_annotations},
        });
  }
  mstch::node name_hash() {
    return "__fbthrift_hash_" + cpp2::sha256_hex(enm_value_->get_name());
  }
  mstch::node cpp_name() { return cpp2::get_name(enm_value_); }
  mstch::node has_fatal_annotations() {
    return get_fatal_annotations(enm_value_->annotations()).size() > 0;
  }
  mstch::node fatal_annotations() {
    return generate_annotations(
        get_fatal_annotations(enm_value_->annotations()));
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
            std::move(generators),
            std::move(cache),
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
      ELEMENT_POSITION const pos,
      std::shared_ptr<cpp2_generator_context> context)
      : mstch_type(type, std::move(generators), std::move(cache), pos),
        context_(std::move(context)) {
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
            {"type:cpp_name", &mstch_cpp2_type::cpp_name},
            {"type:cpp_type", &mstch_cpp2_type::cpp_type},
            {"type:cpp_standard_type", &mstch_cpp2_type::cpp_standard_type},
            {"type:cpp_adapter", &mstch_cpp2_type::cpp_adapter},
            {"type:raw_binary?", &mstch_cpp2_type::raw_binary},
            {"type:raw_string_or_binary?",
             &mstch_cpp2_type::raw_string_or_binary},
            {"type:string_or_binary?", &mstch_cpp2_type::is_string_or_binary},
            {"type:resolved_cpp_type", &mstch_cpp2_type::resolved_cpp_type},
            {"type:cpp_template", &mstch_cpp2_type::cpp_template},
            {"type:cpp_indirection?", &mstch_cpp2_type::cpp_indirection},
            {"type:non_empty_struct?", &mstch_cpp2_type::is_non_empty_struct},
            {"type:namespace_cpp2", &mstch_cpp2_type::namespace_cpp2},
            {"type:cpp_declare_hash", &mstch_cpp2_type::cpp_declare_hash},
            {"type:cpp_declare_equal_to",
             &mstch_cpp2_type::cpp_declare_equal_to},
            {"type:type_class", &mstch_cpp2_type::type_class},
            {"type:type_class_with_indirection",
             &mstch_cpp2_type::type_class_with_indirection},
            {"type:program_name", &mstch_cpp2_type::program_name},
            {"type:cpp_use_allocator?", &mstch_cpp2_type::cpp_use_allocator},
        });
    register_has_option(
        "type:sync_methods_return_try?", "sync_methods_return_try");
  }
  std::string get_type_namespace(t_program const* program) override {
    return cpp2::get_gen_namespace(*program);
  }
  mstch::node resolves_to_base() { return resolved_type_->is_base_type(); }
  mstch::node resolves_to_integral() {
    return resolved_type_->is_byte() || resolved_type_->is_any_int();
  }
  mstch::node resolves_to_base_or_enum() {
    return resolved_type_->is_base_type() || resolved_type_->is_enum();
  }
  mstch::node resolves_to_container() { return resolved_type_->is_container(); }
  mstch::node resolves_to_container_or_struct() {
    return resolved_type_->is_container() || resolved_type_->is_struct() ||
        resolved_type_->is_xception();
  }
  mstch::node resolves_to_container_or_enum() {
    return resolved_type_->is_container() || resolved_type_->is_enum();
  }
  mstch::node resolves_to_complex_return() {
    if (resolved_type_->is_service()) {
      return false;
    }
    return resolved_type_->is_container() ||
        resolved_type_->is_string_or_binary() || resolved_type_->is_struct() ||
        resolved_type_->is_xception();
  }
  mstch::node resolves_to_fixed_size() {
    return resolved_type_->is_bool() || resolved_type_->is_byte() ||
        resolved_type_->is_any_int() || resolved_type_->is_enum() ||
        resolved_type_->is_floating_point();
  }
  mstch::node resolves_to_enum() { return resolved_type_->is_enum(); }
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
  mstch::node cpp_name() { return cpp2::get_name(type_); }
  mstch::node cpp_type() { return context_->resolver().get_type_name(type_); }
  mstch::node cpp_standard_type() {
    return context_->resolver().get_standard_type_name(type_);
  }
  mstch::node cpp_adapter() {
    if (const auto* adapter =
            gen::cpp::type_resolver::find_first_adapter(type_)) {
      return *adapter;
    }
    return {};
  }
  mstch::node raw_binary() {
    return resolved_type_->is_binary() && !is_adapted();
  }
  mstch::node raw_string_or_binary() {
    return resolved_type_->is_string_or_binary() && !is_adapted();
  }
  mstch::node resolved_cpp_type() { return cpp2::get_type(resolved_type_); }
  mstch::node is_string_or_binary() {
    return resolved_type_->is_string_or_binary();
  }
  mstch::node cpp_template() { return get_cpp_template(type_); }
  mstch::node cpp_indirection() {
    return resolved_type_->has_annotation("cpp.indirection");
  }
  mstch::node cpp_declare_hash() {
    return resolved_type_->has_annotation(
        {"cpp.declare_hash", "cpp2.declare_hash"});
  }
  mstch::node cpp_declare_equal_to() {
    return resolved_type_->has_annotation(
        {"cpp.declare_equal_to", "cpp2.declare_equal_to"});
  }
  mstch::node cpp_use_allocator() {
    return resolved_type_->has_annotation("cpp.use_allocator") ||
        type_->has_annotation("cpp.use_allocator");
  }
  mstch::node is_non_empty_struct() {
    auto as_struct = dynamic_cast<t_struct const*>(resolved_type_);
    return as_struct && as_struct->has_fields();
  }
  mstch::node namespace_cpp2() {
    return t_mstch_cpp2_generator::get_namespace_array(type_->program());
  }
  mstch::node type_class() { return cpp2::get_gen_type_class(*resolved_type_); }
  mstch::node type_class_with_indirection() {
    return cpp2::get_gen_type_class_with_indirection(*resolved_type_);
  }
  mstch::node program_name() {
    std::string name;
    if (auto prog = type_->program()) {
      name = prog->name();
    }
    return name;
  }

 private:
  std::shared_ptr<cpp2_generator_context> context_;

  bool is_adapted() const {
    return gen::cpp::type_resolver::find_first_adapter(type_) != nullptr;
  }
};

class mstch_cpp2_field : public mstch_field {
 public:
  mstch_cpp2_field(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index,
      field_generator_context const* field_context,
      std::shared_ptr<cpp2_generator_context> context)
      : mstch_field(
            field,
            std::move(generators),
            std::move(cache),
            pos,
            index,
            field_context),
        context_(std::move(context)) {
    register_methods(
        this,
        {
            {"field:name_hash", &mstch_cpp2_field::name_hash},
            {"field:index_plus_one", &mstch_cpp2_field::index_plus_one},
            {"field:has_isset?", &mstch_cpp2_field::has_isset},
            {"field:isset_index", &mstch_cpp2_field::isset_index},
            {"field:cpp_name", &mstch_cpp2_field::cpp_name},
            {"field:cpp_storage_name", &mstch_cpp2_field::cpp_storage_name},
            {"field:cpp_storage_type", &mstch_cpp2_field::cpp_storage_type},
            {"field:cpp_deprecated_accessor_type",
             &mstch_cpp2_field::cpp_deprecated_accessor_type},
            {"field:has_deprecated_accessors?",
             &mstch_cpp2_field::has_deprecated_accessors},
            {"field:next_field_key", &mstch_cpp2_field::next_field_key},
            {"field:prev_field_key", &mstch_cpp2_field::prev_field_key},
            {"field:next_field_type", &mstch_cpp2_field::next_field_type},
            {"field:non_opt_cpp_ref?", &mstch_cpp2_field::non_opt_cpp_ref},
            {"field:cpp_ref?", &mstch_cpp2_field::cpp_ref},
            {"field:cpp_ref_unique?", &mstch_cpp2_field::cpp_ref_unique},
            {"field:cpp_ref_shared?", &mstch_cpp2_field::cpp_ref_shared},
            {"field:cpp_ref_shared_const?",
             &mstch_cpp2_field::cpp_ref_shared_const},
            {"field:cpp_adapter", &mstch_cpp2_field::cpp_adapter},
            {"field:zero_copy_arg", &mstch_cpp2_field::zero_copy_arg},
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
            {"field:lazy?", &mstch_cpp2_field::lazy},
            {"field:lazy_ref?", &mstch_cpp2_field::lazy_ref},
            {"field:boxed_ref?", &mstch_cpp2_field::boxed_ref},
            {"field:transitively_refers_to_unique?",
             &mstch_cpp2_field::transitively_refers_to_unique},
            {"field:eligible_for_storage_name_mangling?",
             &mstch_cpp2_field::eligible_for_storage_name_mangling},
        });
  }
  mstch::node name_hash() {
    return "__fbthrift_hash_" + cpp2::sha256_hex(field_->get_name());
  }
  mstch::node index_plus_one() { return std::to_string(index_ + 1); }
  mstch::node isset_index() {
    assert(field_context_);
    return field_context_->isset_index;
  }
  mstch::node cpp_name() { return cpp2::get_name(field_); }
  mstch::node cpp_storage_name() {
    if (!is_eligible_for_storage_name_mangling()) {
      return cpp2::get_name(field_);
    }

    return mangle_field_name(cpp2::get_name(field_));
  }
  mstch::node cpp_storage_type() {
    return context_->resolver().get_storage_type_name(field_);
  }
  mstch::node eligible_for_storage_name_mangling() {
    return is_eligible_for_storage_name_mangling();
  }
  mstch::node cpp_deprecated_accessor_type() {
    // The type to use for pre-field_ref backwards compatiblity functions.
    // These leaked the internal storage type directly.
    //
    // TODO(afuller): Remove this once all non-field_ref based accessors have
    // been removed.
    return context_->resolver().get_storage_type_name(field_);
  }
  mstch::node has_deprecated_accessors() {
    return !cpp2::is_explicit_ref(field_) && !cpp2::is_lazy(field_) &&
        !gen::cpp::type_resolver::find_first_adapter(field_) &&
        !has_option("no_getters_setters");
  }
  mstch::node cpp_ref() { return cpp2::is_explicit_ref(field_); }
  mstch::node non_opt_cpp_ref() {
    return cpp2::is_explicit_ref(field_) &&
        field_->get_req() != t_field::e_req::optional;
  }
  mstch::node lazy() { return cpp2::is_lazy(field_); }
  mstch::node lazy_ref() { return cpp2::is_lazy_ref(field_); }
  mstch::node boxed_ref() {
    return gen::cpp::find_ref_type(*field_) == gen::cpp::reference_type::boxed;
  }
  mstch::node transitively_refers_to_unique() {
    return cpp2::field_transitively_refers_to_unique(field_);
  }
  mstch::node cpp_ref_unique() { return cpp2::is_unique_ref(field_); }
  mstch::node cpp_ref_shared() {
    return gen::cpp::find_ref_type(*field_) ==
        gen::cpp::reference_type::shared_mutable;
  }
  mstch::node cpp_ref_shared_const() {
    return gen::cpp::find_ref_type(*field_) ==
        gen::cpp::reference_type::shared_const;
  }
  mstch::node cpp_adapter() {
    if (const std::string* adapter =
            gen::cpp::type_resolver::find_first_adapter(field_)) {
      return *adapter;
    }
    return {};
  }
  mstch::node cpp_noncopyable() {
    return field_->get_type()->has_annotation(
        {"cpp.noncopyable", "cpp2.noncopyable"});
  }
  mstch::node enum_has_value() {
    if (auto enm = dynamic_cast<t_enum const*>(field_->get_type())) {
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
  mstch::node prev_field_key() {
    assert(field_context_ && field_context_->prev);
    return field_context_->prev->get_key();
  }
  mstch::node next_field_key() {
    assert(field_context_ && field_context_->next);
    return field_context_->next->get_key();
  }
  mstch::node next_field_type() {
    assert(field_context_ && field_context_->next);
    return field_context_->next
        ? generators_->type_generator_->generate(
              field_context_->next->get_type(), generators_, cache_, pos_)
        : mstch::node("");
  }
  mstch::node terse_writes() {
    // Add terse writes for unqualified fields when comparison is cheap:
    // (e.g. i32/i64, empty strings/list/map)
    auto t = field_->get_type()->get_true_type();
    return has_option("terse_writes") &&
        field_->get_req() != t_field::e_req::optional &&
        field_->get_req() != t_field::e_req::required &&
        (is_cpp_ref_unique_either(field_) ||
         (!t->is_struct() && !t->is_xception()));
  }
  mstch::node zero_copy_arg() {
    switch (field_->get_type()->get_type_value()) {
      case t_type::type::t_binary:
      case t_type::type::t_struct:
        return std::string("true");
      default:
        return std::string("false");
    }
  }
  mstch::node has_fatal_annotations() {
    return get_fatal_annotations(field_->annotations()).size() > 0;
  }
  mstch::node has_isset() { return cpp2::field_has_isset(field_); }
  mstch::node fatal_annotations() {
    return generate_annotations(get_fatal_annotations(field_->annotations()));
  }
  mstch::node fatal_required_qualifier() {
    switch (field_->get_req()) {
      case t_field::e_req::required:
        return std::string("required");
      case t_field::e_req::optional:
        return std::string("optional");
      case t_field::e_req::opt_in_req_out:
        return std::string("required_of_writer");
      default:
        throw std::runtime_error("unknown required qualifier");
    }
  }

  mstch::node visibility() {
    return std::string(is_private() ? "private" : "public");
  }

  mstch::node metadata_name() {
    auto key = field_->get_key();
    auto suffix = key >= 0 ? std::to_string(key) : "_" + std::to_string(-key);
    return field_->get_name() + "_" + suffix;
  }

 private:
  bool is_private() const {
    auto req = field_->get_req();
    bool isPrivate = true;
    if (cpp2::is_lazy(field_)) {
      // Lazy field has to be private.
    } else if (cpp2::is_ref(field_)) {
      if (gen::cpp::find_ref_type(*field_) == gen::cpp::reference_type::boxed) {
        isPrivate = false;
      } else {
        isPrivate = has_option("deprecated_private_fields_for_cpp_ref");
      }
    } else if (req == t_field::e_req::required) {
      isPrivate = false;
    } else if (req == t_field::e_req::optional) {
      // Optional fields are always private.
    } else if (req == t_field::e_req::opt_in_req_out) {
      isPrivate = !has_option("deprecated_public_fields");
    }
    return isPrivate;
  }

  bool is_eligible_for_storage_name_mangling() const {
    const auto* strct = field_context_->strct;

    if (strct->is_union() || strct->is_exception()) {
      return false;
    }

    if (strct->has_annotation({"cpp.methods", "cpp2.methods"})) {
      return false;
    }

    return is_private();
  }

  std::shared_ptr<cpp2_generator_context> context_;
};

class mstch_cpp2_struct : public mstch_struct {
 public:
  mstch_cpp2_struct(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      std::shared_ptr<cpp2_generator_context> context)
      : mstch_struct(strct, std::move(generators), std::move(cache), pos),
        context_(std::move(context)) {
    register_methods(
        this,
        {
            {"struct:fields_size", &mstch_cpp2_struct::fields_size},
            {"struct:explicitly_constructed_fields",
             &mstch_cpp2_struct::explicitly_constructed_fields},
            {"struct:fields_in_key_order",
             &mstch_cpp2_struct::fields_in_key_order},
            {"struct:fields_in_layout_order",
             &mstch_cpp2_struct::fields_in_layout_order},
            {"struct:is_struct_orderable?",
             &mstch_cpp2_struct::is_struct_orderable},
            {"struct:nondefault_copy_ctor_and_assignment?",
             &mstch_cpp2_struct::nondefault_copy_ctor_and_assignment},
            {"struct:cpp_methods", &mstch_cpp2_struct::cpp_methods},
            {"struct:cpp_declare_hash", &mstch_cpp2_struct::cpp_declare_hash},
            {"struct:cpp_declare_equal_to",
             &mstch_cpp2_struct::cpp_declare_equal_to},
            {"struct:cpp_noncopyable", &mstch_cpp2_struct::cpp_noncopyable},
            {"struct:cpp_noncomparable", &mstch_cpp2_struct::cpp_noncomparable},
            {"struct:is_eligible_for_constexpr?",
             &mstch_cpp2_struct::is_eligible_for_constexpr},
            {"struct:virtual", &mstch_cpp2_struct::cpp_virtual},
            {"struct:message", &mstch_cpp2_struct::message},
            {"struct:isset_fields?", &mstch_cpp2_struct::has_isset_fields},
            {"struct:isset_fields", &mstch_cpp2_struct::isset_fields},
            {"struct:isset_fields_size", &mstch_cpp2_struct::isset_fields_size},
            {"struct:packed_isset", &mstch_cpp2_struct::packed_isset},
            {"struct:lazy_fields?", &mstch_cpp2_struct::has_lazy_fields},
            {"struct:indexing?", &mstch_cpp2_struct::indexing},
            {"struct:write_lazy_field_checksum",
             &mstch_cpp2_struct::write_lazy_field_checksum},
            {"struct:is_large?", &mstch_cpp2_struct::is_large},
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
            {"struct:cpp_data_method?", &mstch_cpp2_struct::cpp_data_method},
            {"struct:cpp_frozen2_exclude?",
             &mstch_cpp2_struct::cpp_frozen2_exclude},
        });
  }
  mstch::node fields_size() { return std::to_string(strct_->fields().size()); }
  mstch::node explicitly_constructed_fields() {
    // Filter fields according to the following criteria:
    // Get all enums
    // Get all base_types but empty strings
    // Get all non-empty structs and containers
    // Get all non-optional references with basetypes, enums,
    // non-empty structs, and containers
    std::vector<t_field const*> filtered_fields;
    for (auto const* field : get_members_in_layout_order()) {
      const t_type* type = field->get_type()->get_true_type();
      // Filter out all optional references.
      if (cpp2::is_explicit_ref(field) &&
          field->get_req() == t_field::e_req::optional) {
        continue;
      }
      if (type->is_enum() ||
          (type->is_base_type() && !type->is_string_or_binary()) ||
          (type->is_string_or_binary() && field->get_value() != nullptr) ||
          (type->is_container() && field->get_value() != nullptr &&
           !field->get_value()->is_empty()) ||
          (type->is_struct() &&
           (strct_ != dynamic_cast<t_struct const*>(type)) &&
           ((field->get_value() && !field->get_value()->is_empty()) ||
            (cpp2::is_explicit_ref(field) &&
             field->get_req() != t_field::e_req::optional))) ||
          (type->is_container() && cpp2::is_explicit_ref(field) &&
           field->get_req() != t_field::e_req::optional) ||
          (type->is_base_type() && cpp2::is_explicit_ref(field) &&
           field->get_req() != t_field::e_req::optional)) {
        filtered_fields.push_back(field);
      }
    }
    return generate_fields(filtered_fields);
  }

  mstch::node mixin_fields() {
    mstch::array fields;
    for (auto i : cpp2::get_mixins_and_members(*strct_)) {
      fields.push_back(mstch::map{
          {"mixin:name", i.mixin->get_name()},
          {"mixin:field_name", i.member->get_name()}});
    }
    return fields;
  }

  mstch::node is_struct_orderable() {
    return context_->is_orderable(*strct_) &&
        !strct_->has_annotation("no_default_comparators");
  }
  mstch::node nondefault_copy_ctor_and_assignment() {
    for (auto const& f : strct_->fields()) {
      if (cpp2::field_transitively_refers_to_unique(&f) || cpp2::is_lazy(&f) ||
          gen::cpp::type_resolver::find_first_adapter(&f)) {
        return true;
      }
    }
    return false;
  }
  mstch::node cpp_methods() {
    return strct_->get_annotation({"cpp.methods", "cpp2.methods"});
  }
  mstch::node cpp_declare_hash() {
    return strct_->has_annotation({"cpp.declare_hash", "cpp2.declare_hash"});
  }
  mstch::node cpp_declare_equal_to() {
    return strct_->has_annotation(
        {"cpp.declare_equal_to", "cpp2.declare_equal_to"});
  }
  mstch::node cpp_noncopyable() {
    if (strct_->has_annotation({"cpp.noncopyable", "cpp2.noncopyable"})) {
      return true;
    }
    bool result = false;
    cpp2::for_each_transitive_field(strct_, [&result](const t_field* field) {
      if (!field->get_type()->has_annotation(
              {"cpp.noncopyable", "cpp2.noncopyable"})) {
        return true;
      }
      switch (gen::cpp::find_ref_type(*field)) {
        case gen::cpp::reference_type::shared_const:
        case gen::cpp::reference_type::shared_mutable: {
          return true;
        }
        case gen::cpp::reference_type::boxed:
        case gen::cpp::reference_type::none:
        case gen::cpp::reference_type::unique:
        case gen::cpp::reference_type::unrecognized: {
          break;
        }
      }
      result = true;
      return false;
    });
    return result;
  }
  mstch::node cpp_noncomparable() {
    return strct_->has_annotation({"cpp.noncomparable", "cpp2.noncomparable"});
  }
  mstch::node is_eligible_for_constexpr() {
    return is_eligible_for_constexpr_(strct_) ||
        strct_->has_annotation({"cpp.methods", "cpp2.methods"});
  }
  mstch::node cpp_virtual() {
    return strct_->has_annotation({"cpp.virtual", "cpp2.virtual"});
  }
  mstch::node message() {
    return strct_->is_exception() ? strct_->get_annotation("message")
                                  : mstch::node();
  }
  mstch::node cpp_allocator() {
    return strct_->get_annotation("cpp.allocator");
  }
  mstch::node cpp_data_method() {
    return strct_->has_annotation("cpp.internal.deprecated._data.method");
  }
  mstch::node cpp_frozen2_exclude() {
    return strct_->has_annotation("cpp.frozen2_exclude");
  }
  mstch::node cpp_allocator_via() {
    if (const auto* name =
            strct_->find_annotation_or_null("cpp.allocator_via")) {
      for (const auto& field : strct_->fields()) {
        if (cpp2::get_name(&field) == *name) {
          return mangle_field_name(*name);
        }
      }
      throw std::runtime_error("No cpp.allocator_via field \"" + *name + "\"");
    }
    return std::string();
  }
  mstch::node has_lazy_fields() {
    for (const auto& field : strct_->get_members()) {
      if (cpp2::is_lazy(field)) {
        return true;
      }
    }
    return false;
  }
  mstch::node indexing() { return has_lazy_fields(); }
  mstch::node write_lazy_field_checksum() {
    if (strct_->find_structured_annotation_or_null(
            "facebook.com/thrift/annotation/cpp/DisableLazyChecksum")) {
      return std::string("false");
    }

    return std::string("true");
  }
  mstch::node has_isset_fields() {
    for (const auto& field : strct_->fields()) {
      if (cpp2::field_has_isset(&field)) {
        return true;
      }
    }
    return false;
  }
  mstch::node isset_fields() {
    std::vector<t_field const*> fields;
    for (const auto& field : strct_->fields()) {
      if (cpp2::field_has_isset(&field)) {
        fields.push_back(&field);
      }
    }
    if (fields.empty()) {
      return mstch::node();
    }
    return generate_fields(fields);
  }
  mstch::node isset_fields_size() {
    std::size_t size = 0;
    for (const auto& field : strct_->fields()) {
      if (cpp2::field_has_isset(&field)) {
        size++;
      }
    }
    return std::to_string(size);
  }
  mstch::node packed_isset() { return cpp2::packed_isset(*strct_) != nullptr; }

  mstch::node is_large() {
    // Outline constructors and destructors if the struct has
    // enough members and at least one has a non-trivial destructor
    // (involving at least a branch and a likely deallocation).
    // TODO(ott): Support unions.
    if (strct_->is_exception()) {
      return true;
    }
    constexpr size_t kLargeStructThreshold = 4;
    if (strct_->fields().size() <= kLargeStructThreshold) {
      return false;
    }
    for (auto const& field : strct_->fields()) {
      auto const* resolved_typedef = field.type()->get_true_type();
      if (cpp2::is_ref(&field) || resolved_typedef->is_string_or_binary() ||
          resolved_typedef->is_container()) {
        return true;
      }
    }
    return false;
  }
  mstch::node has_fatal_annotations() {
    return get_fatal_annotations(strct_->annotations()).size() > 0;
  }
  mstch::node fatal_annotations() {
    return generate_annotations(get_fatal_annotations(strct_->annotations()));
  }
  mstch::node get_legacy_type_id() {
    return std::to_string(strct_->get_type_id());
  }
  mstch::node metadata_name() {
    return strct_->program()->name() + "_" + strct_->get_name();
  }

  mstch::node get_num_union_members() {
    if (!strct_->is_union()) {
      throw std::runtime_error("not a union struct");
    }
    return std::to_string(strct_->fields().size());
  }

 protected:
  // Computes the alignment of field on the target platform.
  // Returns 0 if cannot compute the alignment.
  static size_t compute_alignment(t_field const* field) {
    if (cpp2::is_ref(field)) {
      return 8;
    }
    t_type const* type = field->get_type();
    switch (type->get_type_value()) {
      case t_type::type::t_bool:
      case t_type::type::t_byte:
        return 1;
      case t_type::type::t_i16:
        return 2;
      case t_type::type::t_i32:
      case t_type::type::t_float:
      case t_type::type::t_enum:
        return 4;
      case t_type::type::t_i64:
      case t_type::type::t_double:
      case t_type::type::t_string:
      case t_type::type::t_binary:
      case t_type::type::t_list:
      case t_type::type::t_set:
      case t_type::type::t_map:
        return 8;
      case t_type::type::t_struct: {
        size_t align = 1;
        const size_t kMaxAlign = 8;
        t_struct const* strct = dynamic_cast<t_struct const*>(type);
        if (!strct) {
          throw std::runtime_error(
              "cpp.minimize_padding requires struct definitions to be "
              "topologically sorted. Move definition of `" +
              type->get_name() + "` before its use in field `" +
              field->get_name() + "`.");
        }
        for (auto const& field : strct->fields()) {
          size_t field_align = compute_alignment(&field);
          if (field_align == 0) {
            // Unknown alignment, bail out.
            return 0;
          }
          align = std::max(align, field_align);
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
  const std::vector<const t_field*>& get_members_in_layout_order() {
    if (strct_->fields().size() == fields_in_layout_order_.size()) {
      // Already reordered.
      return fields_in_layout_order_;
    }

    if (!strct_->has_annotation("cpp.minimize_padding")) {
      return fields_in_layout_order_ = strct_->fields().copy();
    }

    // Compute field alignments.
    struct FieldAlign {
      const t_field* field = nullptr;
      size_t align = 0;
    };
    std::vector<FieldAlign> field_alignments;
    field_alignments.reserve(strct_->fields().size());
    for (const auto& field : strct_->fields()) {
      auto align = compute_alignment(&field);
      if (align == 0) {
        // Unknown alignment, don't reorder anything.
        return fields_in_layout_order_ = strct_->fields().copy();
      }
      field_alignments.push_back(FieldAlign{&field, align});
    }

    // Sort by decreasing alignment using stable sort to avoid unnecessary
    // reordering.
    std::stable_sort(
        field_alignments.begin(),
        field_alignments.end(),
        [](auto const& lhs, auto const& rhs) { return lhs.align > rhs.align; });

    // Construct the reordered field vector.
    fields_in_layout_order_.reserve(strct_->fields().size());
    std::transform(
        field_alignments.begin(),
        field_alignments.end(),
        std::back_inserter(fields_in_layout_order_),
        [](FieldAlign const& fa) { return fa.field; });
    return fields_in_layout_order_;
  }

  mstch::node fields_in_layout_order() {
    return generate_fields(get_members_in_layout_order());
  }

  // Returns the struct members ordered by the key.
  const std::vector<const t_field*>& get_members_in_key_order() {
    if (strct_->fields().size() == fields_in_key_order_.size()) {
      // Already reordered.
      return fields_in_key_order_;
    }

    fields_in_key_order_ = strct_->fields().copy();
    // Sort by increasing key.
    std::sort(
        fields_in_key_order_.begin(),
        fields_in_key_order_.end(),
        [](const auto* lhs, const auto* rhs) {
          return lhs->get_key() < rhs->get_key();
        });

    return fields_in_key_order_;
  }

  mstch::node fields_in_key_order() {
    return generate_fields(get_members_in_key_order());
  }

  std::shared_ptr<cpp2_generator_context> context_;

  std::vector<const t_field*> fields_in_key_order_;
  std::vector<const t_field*> fields_in_layout_order_;
  cpp2::is_eligible_for_constexpr is_eligible_for_constexpr_;
};

class mstch_cpp2_function : public mstch_function {
 public:
  mstch_cpp2_function(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_function(function, std::move(generators), std::move(cache), pos) {
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
    return function_->has_annotation("cpp.coroutine") ||
        function_->is_interaction_member();
  }
  mstch::node event_based() {
    return function_->get_annotation("thread") == "eb";
  }
  mstch::node cpp_name() { return cpp2::get_name(function_); }
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
      ELEMENT_POSITION const pos,
      int32_t split_id = 0,
      int32_t split_count = 1)
      : mstch_service(service, std::move(generators), std::move(cache), pos) {
    register_methods(
        this,
        {
            {"service:program_name", &mstch_cpp2_service::program_name},
            {"service:program_path", &mstch_cpp2_service::program_path},
            {"service:include_prefix", &mstch_cpp2_service::include_prefix},
            {"service:thrift_includes", &mstch_cpp2_service::thrift_includes},
            {"service:namespace_cpp2", &mstch_cpp2_service::namespace_cpp2},
            {"service:oneway_functions", &mstch_cpp2_service::oneway_functions},
            {"service:oneways?", &mstch_cpp2_service::has_oneway},
            {"service:cpp_includes", &mstch_cpp2_service::cpp_includes},
            {"service:metadata_name", &mstch_cpp2_service::metadata_name},
            {"service:parent_service_name",
             &mstch_cpp2_service::parent_service_name},
            {"service:reduced_client?", &mstch_service::is_interaction},
        });

    const auto all_functions = mstch_service::get_functions();
    for (size_t id = split_id; id < all_functions.size(); id += split_count) {
      functions_.push_back(all_functions[id]);
    }
  }
  std::string get_service_namespace(t_program const* program) override {
    return t_mstch_cpp2_generator::get_cpp2_namespace(program);
  }
  mstch::node program_name() { return service_->program()->name(); }
  mstch::node program_path() { return service_->program()->path(); }
  mstch::node cpp_includes() {
    return t_mstch_cpp2_generator::cpp_includes(service_->program());
  }
  mstch::node include_prefix() {
    return t_mstch_cpp2_generator::include_prefix(
        service_->program(), cache_->parsed_options_);
  }
  mstch::node thrift_includes() {
    mstch::array a{};
    for (auto const* program : service_->program()->get_included_programs()) {
      a.push_back(generators_->program_generator_->generate_cached(
          program, generators_, cache_));
    }
    return a;
  }
  mstch::node namespace_cpp2() {
    return t_mstch_cpp2_generator::get_namespace_array(service_->program());
  }
  mstch::node oneway_functions() {
    std::vector<t_function const*> oneway_functions;
    for (auto const* function : get_functions()) {
      if (function->qualifier() == t_function_qualifier::one_way) {
        oneway_functions.push_back(function);
      }
    }
    return generate_functions(oneway_functions);
  }
  mstch::node has_oneway() {
    for (auto const* function : get_functions()) {
      if (function->qualifier() == t_function_qualifier::one_way) {
        return true;
      }
    }
    return false;
  }
  mstch::node metadata_name() {
    return service_->program()->name() + "_" + service_->get_name();
  }
  mstch::node parent_service_name() {
    return cache_->parsed_options_.at("parent_service_name");
  }

 private:
  const std::vector<t_function*>& get_functions() const override {
    return functions_;
  }

  std::vector<t_function*> functions_;
};

class mstch_cpp2_annotation : public mstch_annotation {
 public:
  mstch_cpp2_annotation(
      const std::string& key,
      annotation_value val,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_annotation(
            key, val, std::move(generators), std::move(cache), pos, index) {
    register_methods(
        this,
        {
            {"annotation:safe_key", &mstch_cpp2_annotation::safe_key},
            {"annotation:fatal_string", &mstch_cpp2_annotation::fatal_string},
        });
  }
  mstch::node safe_key() { return get_fatal_string_short_id(key_); }
  mstch::node fatal_string() { return render_fatal_string(val_.value); }
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
            std::move(generators),
            std::move(cache),
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
      : mstch_program(program, std::move(generators), std::move(cache), pos),
        split_id_(split_id) {
    register_methods(
        this,
        {
            {"program:cpp_includes", &mstch_cpp2_program::cpp_includes},
            {"program:namespace_cpp2", &mstch_cpp2_program::namespace_cpp2},
            {"program:include_prefix", &mstch_cpp2_program::include_prefix},
            {"program:cpp_declare_hash?",
             &mstch_cpp2_program::cpp_declare_hash},
            {"program:thrift_includes", &mstch_cpp2_program::thrift_includes},
            {"program:frozen_packed?", &mstch_cpp2_program::frozen_packed},
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
        });
    register_has_option("program:tablebased?", "tablebased");
    register_has_option("program:no_metadata?", "no_metadata");
    register_has_option(
        "program:enforce_required?", "deprecated_enforce_required");
  }
  std::string get_program_namespace(t_program const* program) override {
    return t_mstch_cpp2_generator::get_cpp2_namespace(program);
  }

  std::vector<const t_typedef*> alias_to_struct() {
    std::vector<const t_typedef*> result;
    for (const t_typedef* i : program_->typedefs()) {
      const t_type* alias = i->get_type();
      if (alias->is_typedef() && alias->has_annotation("cpp.type")) {
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
      std::map<std::string, std::string>& fatal_strings, const Node* node) {
    // TODO: extra copy
    auto cpp_name = cpp2::get_name(node);
    fatal_strings.emplace(get_fatal_string_short_id(cpp_name), cpp_name);
    auto hash = cpp2::sha256_hex(node->get_name());
    fatal_strings.emplace("__fbthrift_hash_" + hash, node->get_name());
    for (const auto& a : node->annotations()) {
      if (!is_annotation_blacklisted_in_fatal(a.first)) {
        fatal_strings.emplace(get_fatal_string_short_id(a.first), a.first);
      }
    }
  }
  std::vector<std::string> get_fatal_enum_names() {
    std::vector<std::string> result;
    for (const auto* enm : program_->enums()) {
      result.push_back(get_fatal_string_short_id(enm->get_name()));
    }
    return result;
  }
  std::vector<std::string> get_fatal_union_names() {
    std::vector<std::string> result;
    for (const auto* obj : program_->objects()) {
      if (obj->is_union()) {
        result.push_back(get_fatal_string_short_id(obj->get_name()));
      }
    }
    return result;
  }
  std::vector<std::string> get_fatal_struct_names() {
    std::vector<std::string> result;
    for (const auto* obj : program_->objects()) {
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
    for (const auto* cnst : program_->consts()) {
      result.push_back(get_fatal_string_short_id(cnst->get_name()));
    }
    return result;
  }
  std::vector<std::string> get_fatal_service_names() {
    std::vector<std::string> result;
    for (const auto* service : program_->services()) {
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
        program_, cache_->parsed_options_);
  }
  mstch::node cpp_declare_hash() {
    bool cpp_declare_in_structs = std::any_of(
        program_->structs().begin(),
        program_->structs().end(),
        [](const auto* strct) {
          return strct->has_annotation(
              {"cpp.declare_hash", "cpp2.declare_hash"});
        });
    bool cpp_declare_in_typedefs = std::any_of(
        program_->typedefs().begin(),
        program_->typedefs().end(),
        [](const auto* typedf) {
          return typedf->get_type()->has_annotation(
              {"cpp.declare_hash", "cpp2.declare_hash"});
        });
    return cpp_declare_in_structs || cpp_declare_in_typedefs;
  }
  mstch::node thrift_includes() {
    mstch::array a{};
    for (auto const* program : program_->get_included_programs()) {
      a.push_back(generators_->program_generator_->generate_cached(
          program, generators_, cache_));
    }
    return a;
  }
  mstch::node frozen_packed() { return get_option("frozen") == "packed"; }
  mstch::node fatal_languages() {
    mstch::array a;
    size_t size = program_->namespaces().size();
    size_t idx = 0;
    for (const auto& pair : program_->namespaces()) {
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
        get_fatal_string_short_id(program_->name()), program_->name());
    // languages and namespaces
    for (const auto& pair : program_->namespaces()) {
      unique_names.emplace(get_fatal_string_short_id(pair.first), pair.first);
      unique_names.emplace(
          get_fatal_namesoace_name_short_id(pair.first, pair.second),
          get_fatal_namesoace(pair.first, pair.second));
    }
    // enums
    for (const auto* enm : program_->enums()) {
      collect_fatal_string_annotated(unique_names, enm);
      unique_names.emplace(
          get_fatal_string_short_id(enm->get_name()), enm->get_name());
      for (const auto& i : enm->get_enum_values()) {
        collect_fatal_string_annotated(unique_names, i);
      }
    }
    // structs, unions and exceptions
    for (const auto* obj : program_->objects()) {
      if (obj->is_union()) {
        // When generating <program_name>_fatal_union.h, we will generate
        // <union_name>_Type_enum_traits
        unique_names.emplace("Type", "Type");
      }
      collect_fatal_string_annotated(unique_names, obj);
      for (const auto& m : obj->fields()) {
        collect_fatal_string_annotated(unique_names, &m);
      }
    }
    // consts
    for (const auto* cnst : program_->consts()) {
      unique_names.emplace(
          get_fatal_string_short_id(cnst->get_name()), cnst->get_name());
    }
    // services
    for (const auto* service : program_->services()) {
      // function annotations are not currently included.
      unique_names.emplace(
          get_fatal_string_short_id(service->get_name()), service->get_name());
      for (const auto* f : service->get_functions()) {
        unique_names.emplace(
            get_fatal_string_short_id(f->get_name()), f->get_name());
        for (const auto& p : f->get_paramlist()->fields()) {
          unique_names.emplace(get_fatal_string_short_id(p.name()), p.name());
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
    for (const t_struct* s : program_->objects()) {
      if (!s->is_union()) {
        for (const t_field& f : s->fields()) {
          auto result = fields.insert(cpp2::get_name(&f));
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
    const auto& prog_objects = program_->objects();
    const auto& prog_enums = program_->enums();

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
      const t_program* program, const std::vector<t_struct*>& objects) {
    auto edges = [program](t_struct* obj) {
      std::vector<t_struct*> deps;
      for (auto& f : obj->fields()) {
        // Ignore ref fields.
        if (cpp2::is_explicit_ref(&f)) {
          continue;
        }

        auto add_dependency = [&](const t_type* type) {
          if (auto strct = dynamic_cast<const t_struct*>(type)) {
            // We're only interested in types defined in the current program.
            if (!strct->is_exception() && strct->program() == program) {
              // TODO(afuller): Remove const cast, once the return type also has
              // const elements.
              deps.emplace_back(const_cast<t_struct*>(strct));
            }
          }
        };

        auto t = f.type()->get_true_type();
        if (auto map = dynamic_cast<t_map const*>(t)) {
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
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_cpp2_enum>(
        enm, std::move(generators), std::move(cache), pos);
  }
};

class enum_value_cpp2_generator : public enum_value_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_cpp2_enum_value>(
        enm_value, std::move(generators), std::move(cache), pos);
  }
};

class type_cpp2_generator : public type_generator {
 public:
  explicit type_cpp2_generator(
      std::shared_ptr<cpp2_generator_context> context) noexcept
      : context_(std::move(context)) {}

  std::shared_ptr<mstch_base> generate(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_cpp2_type>(
        type, std::move(generators), std::move(cache), pos, context_);
  }

 private:
  std::shared_ptr<cpp2_generator_context> context_;
};

class field_cpp2_generator : public field_generator {
 public:
  explicit field_cpp2_generator(
      std::shared_ptr<cpp2_generator_context> context) noexcept
      : context_(std::move(context)) {}

  std::shared_ptr<mstch_base> generate(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      field_generator_context const* field_context) const override {
    return std::make_shared<mstch_cpp2_field>(
        field,
        std::move(generators),
        std::move(cache),
        pos,
        index,
        field_context,
        context_);
  }

 private:
  std::shared_ptr<cpp2_generator_context> context_;
};

class function_cpp2_generator : public function_generator {
 public:
  function_cpp2_generator() = default;
  ~function_cpp2_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_cpp2_function>(
        function, std::move(generators), std::move(cache), pos);
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
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_cpp2_struct>(
        strct, std::move(generators), std::move(cache), pos, context_);
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
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_cpp2_service>(
        service, std::move(generators), std::move(cache), pos);
  }
  std::shared_ptr<mstch_base> generate_with_split_id(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      int32_t split_id,
      int32_t split_count) const {
    return std::make_shared<mstch_cpp2_service>(
        service,
        generators,
        cache,
        ELEMENT_POSITION::NONE,
        split_id,
        split_count);
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
      ELEMENT_POSITION pos,
      int32_t index) const override {
    return std::make_shared<mstch_cpp2_annotation>(
        keyval.first,
        keyval.second,
        std::move(generators),
        std::move(cache),
        pos,
        index);
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
      ELEMENT_POSITION pos,
      int32_t index,
      t_const const* current_const,
      t_type const* expected_type,
      const std::string& field_name) const override {
    return std::make_shared<mstch_cpp2_const>(
        cnst,
        current_const,
        expected_type,
        std::move(generators),
        std::move(cache),
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
      ELEMENT_POSITION pos,
      int32_t index,
      t_const const* current_const,
      t_type const* expected_type) const override {
    return std::make_shared<mstch_cpp2_const_value>(
        const_value,
        current_const,
        expected_type,
        std::move(generators),
        std::move(cache),
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
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_cpp2_program>(
        program, std::move(generators), std::move(cache), pos);
  }
  std::shared_ptr<mstch_base> generate_with_split_id(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      int32_t split_id) const {
    return std::make_shared<mstch_cpp2_program>(
        program,
        std::move(generators),
        std::move(cache),
        ELEMENT_POSITION::NONE,
        split_id);
  }
};

t_mstch_cpp2_generator::t_mstch_cpp2_generator(
    t_program* program,
    t_generation_context context,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& /*option_string*/)
    : t_mstch_generator(
          program, std::move(context), "cpp2", parsed_options, true),
      context_(std::make_shared<cpp2_generator_context>(
          cpp2_generator_context::create())),
      client_name_to_split_count_(
          cpp2::get_client_name_to_split_count(parsed_options)) {
  out_dir_base_ = get_out_dir_base(parsed_options);
}

void t_mstch_cpp2_generator::generate_program() {
  auto const* program = get_program();
  set_mstch_generators();

  if (has_option("any")) {
    generate_sinit(program);
  }
  if (has_option("reflection")) {
    generate_reflection(program);
  }
  generate_structs(program);
  generate_constants(program);
  for (const auto* service : program->services()) {
    generate_service(service);
  }
  generate_metadata(program);
  generate_visitation(program);
}

void t_mstch_cpp2_generator::set_mstch_generators() {
  generators_->set_enum_generator(std::make_unique<enum_cpp2_generator>());
  generators_->set_enum_value_generator(
      std::make_unique<enum_value_cpp2_generator>());
  generators_->set_type_generator(
      std::make_unique<type_cpp2_generator>(context_));
  generators_->set_field_generator(
      std::make_unique<field_cpp2_generator>(context_));
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
  const auto& name = program->name();
  const auto& prog = cached_program(program);

  render_to_file(prog, "module_constants.h", name + "_constants.h");
  render_to_file(prog, "module_constants.cpp", name + "_constants.cpp");
}

void t_mstch_cpp2_generator::generate_metadata(const t_program* program) {
  const auto& name = program->name();
  const auto& prog = cached_program(program);

  render_to_file(prog, "module_metadata.h", name + "_metadata.h");
  if (!has_option("no_metadata")) {
    render_to_file(prog, "module_metadata.cpp", name + "_metadata.cpp");
  }
}

void t_mstch_cpp2_generator::generate_sinit(t_program const* program) {
  const auto& name = program->name();
  const auto& prog = cached_program(program);

  render_to_file(prog, "module_sinit.cpp", name + "_sinit.cpp");
}

void t_mstch_cpp2_generator::generate_reflection(t_program const* program) {
  const auto& name = program->name();
  const auto& prog = cached_program(program);

  // Combo include: all
  render_to_file(prog, "module_fatal_all.h", name + "_fatal_all.h");
  // Combo include: types
  render_to_file(prog, "module_fatal_types.h", name + "_fatal_types.h");
  // Unique Compile-time Strings, Metadata tags and Metadata registration
  render_to_file(prog, "module_fatal.h", name + "_fatal.h");

  render_to_file(prog, "module_fatal_enum.h", name + "_fatal_enum.h");
  render_to_file(prog, "module_fatal_union.h", name + "_fatal_union.h");
  render_to_file(prog, "module_fatal_struct.h", name + "_fatal_struct.h");
  render_to_file(prog, "module_fatal_constant.h", name + "_fatal_constant.h");
  render_to_file(prog, "module_fatal_service.h", name + "_fatal_service.h");
}

void t_mstch_cpp2_generator::generate_visitation(const t_program* program) {
  const auto& name = program->name();
  const auto& prog = cached_program(program);

  render_to_file(prog, "module_visitation.h", name + "_visitation.h");
  render_to_file(prog, "module_for_each_field.h", name + "_for_each_field.h");
  render_to_file(prog, "module_visit_union.h", name + "_visit_union.h");
  render_to_file(
      prog,
      "module_visit_by_thrift_field_metadata.h",
      name + "_visit_by_thrift_field_metadata.h");
}

void t_mstch_cpp2_generator::generate_structs(t_program const* program) {
  const auto& name = program->name();
  const auto& prog = cached_program(program);

  render_to_file(prog, "module_data.h", name + "_data.h");
  render_to_file(prog, "module_data.cpp", name + "_data.cpp");
  render_to_file(prog, "module_types.h", name + "_types.h");
  render_to_file(prog, "module_types.tcc", name + "_types.tcc");

  if (auto split_count = cpp2::get_split_count(parsed_options_)) {
    auto digit = std::to_string(split_count - 1).size();
    for (int split_id = 0; split_id < split_count; ++split_id) {
      auto s = std::to_string(split_id);
      s = std::string(digit - s.size(), '0') + s;
      render_to_file(
          program_cpp2_generator{}.generate_with_split_id(
              program, generators_, cache_, split_id),
          "module_types.cpp",
          name + "_types." + s + ".split.cpp");
    }
  } else {
    render_to_file(prog, "module_types.cpp", name + "_types.cpp");
  }

  render_to_file(
      prog,
      "module_types_custom_protocol.h",
      name + "_types_custom_protocol.h");
  if (has_option("frozen2")) {
    render_to_file(prog, "module_layouts.h", name + "_layouts.h");
    render_to_file(prog, "module_layouts.cpp", name + "_layouts.cpp");
  }
}

void t_mstch_cpp2_generator::generate_service(t_service const* service) {
  const auto& name = service->get_name();
  cache_->parsed_options_["parent_service_name"] = name; // for interactions
  auto serv = generators_->service_generator_->generate_cached(
      get_program(), service, generators_, cache_);

  render_to_file(serv, "ServiceAsyncClient.h", name + "AsyncClient.h");
  render_to_file(serv, "service.cpp", name + ".cpp");
  render_to_file(serv, "service.h", name + ".h");
  render_to_file(serv, "service.tcc", name + ".tcc");
  render_to_file(serv, "types_custom_protocol.h", name + "_custom_protocol.h");

  auto iter = client_name_to_split_count_.find(name);
  if (iter != client_name_to_split_count_.end()) {
    auto split_count = iter->second;
    auto digit = std::to_string(split_count - 1).size();
    for (int split_id = 0; split_id < split_count; ++split_id) {
      auto s = std::to_string(split_id);
      s = std::string(digit - s.size(), '0') + s;
      auto split_service = service_cpp2_generator{}.generate_with_split_id(
          service, generators_, cache_, split_id, split_count);
      render_to_file(
          split_service,
          "ServiceAsyncClient.cpp",
          name + "." + s + ".async_client_split.cpp");
    }
  } else {
    render_to_file(serv, "ServiceAsyncClient.cpp", name + "AsyncClient.cpp");
  }

  std::vector<std::array<std::string, 3>> protocols = {
      {{"binary", "BinaryProtocol", "T_BINARY_PROTOCOL"}},
      {{"compact", "CompactProtocol", "T_COMPACT_PROTOCOL"}},
  };
  for (const auto& protocol : protocols) {
    render_to_file(
        serv,
        "service_processmap_protocol.cpp",
        name + "_processmap_" + protocol.at(0) + ".cpp");
  }
  cache_->parsed_options_.erase("parent_service_name");
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
  for (auto include : program->cpp_includes()) {
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
    t_program const* program, std::map<std::string, std::string>& options) {
  auto prefix = program->include_prefix();
  auto include_prefix = options["include_prefix"];
  auto out_dir_base = get_out_dir_base(options);
  if (prefix.empty()) {
    if (include_prefix.empty()) {
      return prefix;
    } else {
      return include_prefix + "/" + out_dir_base + "/";
    }
  }
  if (boost::filesystem::path(prefix).has_root_directory()) {
    return include_prefix + "/" + out_dir_base + "/";
  }
  return prefix + out_dir_base + "/";
}

namespace {
class annotation_validator : public validator {
 public:
  explicit annotation_validator(
      std::map<std::string, std::string> const& options)
      : options_(options) {}
  using validator::visit;

  /**
   * Make sure there is no incompatible annotation.
   */
  bool visit(t_struct* s) override;

 private:
  const std::map<std::string, std::string>& options_;
};

bool annotation_validator::visit(t_struct* s) {
  if (cpp2::packed_isset(*s)) {
    if (options_.count("tablebased") != 0) {
      add_error(
          s->lineno(),
          "Tablebased serialization is incompatible with isset bitpacking for struct `" +
              s->get_name() + "`");
    }
  }

  for (const auto& field : s->fields()) {
    if (cpp2::is_mixin(field)) {
      // Mixins cannot be refs
      if (cpp2::is_explicit_ref(&field)) {
        add_error(
            field.lineno(),
            "Mixin field `" + field.name() + "` can not be a ref in cpp.");
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
  const std::map<std::string, std::string>& options_;
};

bool service_method_validator::visit(t_service* service) {
  auto suppress_key = "cpp.coroutine_stack_arguments_broken_suppress_error";
  for (const auto& func : service->functions()) {
    if (!func.has_annotation(suppress_key) &&
        func.has_annotation("cpp.coroutine") &&
        cpp2::is_stack_arguments(options_, func)) {
      // when cpp.coroutine and stack_arguments are both on, return failure if
      // this function has complex types (including string and binary).
      const auto& params = func.get_paramlist()->fields();
      bool ok =
          std::all_of(params.begin(), params.end(), [](const auto& param) {
            auto type = param.type()->get_true_type();
            return type->is_base_type() && !type->is_string_or_binary();
          });

      if (!ok) {
        add_error(
            func.lineno(),
            "`" + service->name() + "." + func.name() +
                "` use of cpp.coroutine and stack_arguments together is "
                "disallowed.");
      }
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
        program->objects().size() + program->enums().size();
    if (split_count_ != 0 && split_count_ > object_count) {
      add_error(
          boost::none,
          "`types_cpp_splits=" + std::to_string(split_count_) +
              "` is misconfigured: it can not be greater than number of object, which is " +
              std::to_string(object_count) + ".");
    }
    return true;
  }

 private:
  int32_t split_count_;
};

class lazy_field_validator : public validator {
 public:
  using validator::visit;
  bool visit(t_field* field) override {
    if (cpp2::is_lazy(field)) {
      auto t = field->get_type()->get_true_type();
      boost::optional<std::string> field_type;
      if (t->is_any_int() || t->is_bool() || t->is_byte()) {
        field_type = "Integral field";
      }
      if (t->is_floating_point()) {
        field_type = "Floating point field";
      }
      if (field_type) {
        add_error(
            field->get_lineno(),
            *field_type + " `" + field->get_name() +
                "` can not be marked as lazy, "
                "since doing so won't bring any benefit.");
      }
    }
    return true;
  }
};
} // namespace

void t_mstch_cpp2_generator::fill_validator_list(validator_list& l) const {
  l.add<annotation_validator>(this->parsed_options_);
  l.add<service_method_validator>(this->parsed_options_);
  l.add<splits_validator>(cpp2::get_split_count(parsed_options_));
  l.add<lazy_field_validator>();
}

THRIFT_REGISTER_GENERATOR(mstch_cpp2, "cpp2", "");

} // namespace compiler
} // namespace thrift
} // namespace apache
