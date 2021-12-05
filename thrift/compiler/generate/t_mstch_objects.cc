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

#include <thrift/compiler/generate/t_mstch_objects.h>

namespace apache {
namespace thrift {
namespace compiler {

bool mstch_base::has_option(const std::string& option) const {
  return cache_->parsed_options_.find(option) != cache_->parsed_options_.end();
}

std::string mstch_base::get_option(const std::string& option) const {
  auto itr = cache_->parsed_options_.find(option);
  if (itr != cache_->parsed_options_.end()) {
    return itr->second;
  }
  return {};
}

void mstch_base::register_has_option(std::string key, std::string option) {
  register_method(
      std::move(key), [this, option = std::move(option)]() -> mstch::node {
        return has_option(option);
      });
}

std::shared_ptr<mstch_base> enum_value_generator::generate(
    t_enum_value const* enum_value,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_enum_value>(enum_value, generators, cache, pos);
}

std::shared_ptr<mstch_base> enum_generator::generate(
    t_enum const* enm,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_enum>(enm, generators, cache, pos);
}

std::shared_ptr<mstch_base> const_value_generator::generate(
    t_const_value const* const_value,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t index,
    t_const const* current_const,
    t_type const* expected_type) const {
  return std::make_shared<mstch_const_value>(
      const_value, current_const, expected_type, generators, cache, pos, index);
}

std::shared_ptr<mstch_base> const_value_generator::generate(
    std::pair<t_const_value*, t_const_value*> const& value_pair,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t index,
    t_const const* current_const,
    std::pair<const t_type*, const t_type*> const& expected_types) const {
  return std::make_shared<mstch_const_value_key_mapped_pair>(
      value_pair, current_const, expected_types, generators, cache, pos, index);
}

std::shared_ptr<mstch_base> type_generator::generate(
    t_type const* type,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_type>(type, generators, cache, pos);
}

std::shared_ptr<mstch_base> field_generator::generate(
    t_field const* field,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t index,
    field_generator_context const* field_context) const {
  return std::make_shared<mstch_field>(
      field, generators, cache, pos, index, field_context);
}

std::shared_ptr<mstch_base> annotation_generator::generate(
    const t_annotation& annotation,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t index) const {
  return std::make_shared<mstch_annotation>(
      annotation.first, annotation.second, generators, cache, pos, index);
}

std::shared_ptr<mstch_base> structured_annotation_generator::generate(
    const t_const* annotValue,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t index) const {
  return std::make_shared<mstch_structured_annotation>(
      *annotValue, generators, cache, pos, index);
}

std::shared_ptr<mstch_base> struct_generator::generate(
    t_struct const* strct,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_struct>(strct, generators, cache, pos);
}

std::shared_ptr<mstch_base> function_generator::generate(
    t_function const* function,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_function>(function, generators, cache, pos);
}

std::shared_ptr<mstch_base> service_generator::generate(
    t_service const* service,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_service>(service, generators, cache, pos);
}

std::shared_ptr<mstch_base> typedef_generator::generate(
    t_typedef const* typedf,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_typedef>(typedf, generators, cache, pos);
}

std::shared_ptr<mstch_base> const_generator::generate(
    t_const const* cnst,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t index,
    t_const const* current_const,
    t_type const* expected_type,
    const std::string& field_name) const {
  return std::make_shared<mstch_const>(
      cnst,
      current_const,
      expected_type,
      generators,
      cache,
      pos,
      index,
      field_name);
}

std::shared_ptr<mstch_base> program_generator::generate(
    t_program const* program,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_program>(program, generators, cache, pos);
}

mstch::node mstch_enum::values() {
  return generate_enum_values(enm_->get_enum_values());
}

mstch::node mstch_type::get_struct() {
  if (type_->is_struct() || type_->is_xception()) {
    std::string id =
        type_->program()->name() + get_type_namespace(type_->program());
    return generate_elements_cached(
        std::vector<t_struct const*>{dynamic_cast<t_struct const*>(type_)},
        generators_->struct_generator_.get(),
        cache_->structs_,
        id);
  }
  return mstch::node();
}

mstch::node mstch_type::get_enum() {
  if (resolved_type_->is_enum()) {
    std::string id =
        type_->program()->name() + get_type_namespace(type_->program());
    return generate_elements_cached(
        std::vector<t_enum const*>{dynamic_cast<t_enum const*>(resolved_type_)},
        generators_->enum_generator_.get(),
        cache_->enums_,
        id);
  }
  return mstch::node();
}

mstch::node mstch_type::get_list_type() {
  if (resolved_type_->is_list()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_list*>(resolved_type_)->get_elem_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_set_type() {
  if (resolved_type_->is_set()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_set*>(resolved_type_)->get_elem_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_key_type() {
  if (resolved_type_->is_map()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_map*>(resolved_type_)->get_key_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_value_type() {
  if (resolved_type_->is_map()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_map*>(resolved_type_)->get_val_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_typedef_type() {
  if (type_->is_typedef()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_typedef*>(type_)->get_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_typedef() {
  if (type_->is_typedef()) {
    return generators_->typedef_generator_->generate(
        dynamic_cast<const t_typedef*>(type_), generators_, cache_, pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_sink_elem_type() {
  if (type_->is_sink()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_sink*>(type_)->get_sink_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_sink_final_reponse_type() {
  if (type_->is_sink()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_sink*>(type_)->get_final_response_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_sink_first_response_type() {
  if (type_->is_sink()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_sink*>(type_)->get_first_response_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_stream_elem_type() {
  if (type_->is_streamresponse()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_stream_response*>(type_)->get_elem_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_stream_first_response_type() {
  if (resolved_type_->is_streamresponse()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_stream_response*>(resolved_type_)
            ->get_first_response_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_field::value() {
  if (field_->get_value()) {
    return generators_->const_value_generator_->generate(
        field_->get_value(), generators_, cache_, pos_);
  }
  return mstch::node();
}

mstch::node mstch_const_value_key_mapped_pair::element_key() {
  return generators_->const_value_generator_->generate(
      pair_.first,
      generators_,
      cache_,
      pos_,
      index_,
      current_const_,
      expected_types_.first);
}

mstch::node mstch_const_value_key_mapped_pair::element_value() {
  return generators_->const_value_generator_->generate(
      pair_.second,
      generators_,
      cache_,
      pos_,
      index_,
      current_const_,
      expected_types_.second);
}

mstch::node mstch_const_value::value() {
  switch (type_) {
    case cv::CV_DOUBLE:
      return format_double_string(const_value_->get_double());
    case cv::CV_BOOL:
      return std::to_string(const_value_->get_bool());
    case cv::CV_INTEGER:
      return std::to_string(const_value_->get_integer());
    case cv::CV_STRING:
      return const_value_->get_string();
    default:
      return mstch::node();
  }
}

mstch::node mstch_const_value::integer_value() {
  if (type_ == cv::CV_INTEGER) {
    return std::to_string(const_value_->get_integer());
  }
  return mstch::node();
}

mstch::node mstch_const_value::double_value() {
  if (type_ == cv::CV_DOUBLE) {
    return format_double_string(const_value_->get_double());
  }
  return mstch::node();
}

mstch::node mstch_const_value::bool_value() {
  if (type_ == cv::CV_BOOL) {
    return const_value_->get_bool() == true;
  }
  return mstch::node();
}

mstch::node mstch_const_value::is_non_zero() {
  switch (type_) {
    case cv::CV_DOUBLE:
      return const_value_->get_double() != 0.0;
    case cv::CV_BOOL:
      return const_value_->get_bool() == true;
    case cv::CV_INTEGER:
      return const_value_->get_integer() != 0;
    default:
      return mstch::node();
  }
}

mstch::node mstch_const_value::enum_name() {
  if (type_ == cv::CV_INTEGER && const_value_->is_enum()) {
    return const_value_->get_enum()->get_name();
  }
  return mstch::node();
}

mstch::node mstch_const_value::enum_value_name() {
  if (type_ == cv::CV_INTEGER && const_value_->is_enum()) {
    return const_value_->get_enum_value()->get_name();
  }
  return mstch::node();
}

mstch::node mstch_const_value::string_value() {
  if (type_ == cv::CV_STRING) {
    std::string string_val = const_value_->get_string();
    for (auto itr = string_val.begin(); itr != string_val.end(); ++itr) {
      if (*itr == '"') {
        itr = string_val.insert(itr, '\\');
        ++itr;
      }
    }
    return string_val;
  }
  return mstch::node();
}

mstch::node mstch_const_value::list_elems() {
  if (type_ == cv::CV_LIST) {
    const t_type* expected_type = nullptr;
    if (expected_type_) {
      if (expected_type_->is_list()) {
        expected_type =
            dynamic_cast<const t_list*>(expected_type_)->get_elem_type();
      } else if (expected_type_->is_set()) {
        expected_type =
            dynamic_cast<const t_set*>(expected_type_)->get_elem_type();
      }
    }
    return generate_consts(
        const_value_->get_list(), current_const_, expected_type);
  }
  return mstch::node();
}

mstch::node mstch_const_value::map_elems() {
  if (type_ == cv::CV_MAP) {
    std::pair<const t_type*, const t_type*> expected_types;
    if (expected_type_ && expected_type_->is_map()) {
      const auto* m = dynamic_cast<const t_map*>(expected_type_);
      expected_types = {m->get_key_type(), m->get_val_type()};
    }
    return generate_consts(
        const_value_->get_map(), current_const_, expected_types);
  }
  return mstch::node();
}

mstch::node mstch_const_value::is_const_struct() {
  if (!const_value_->ttype()) {
    return false;
  }
  const auto* type = const_value_->ttype()->deref().get_true_type();
  return type->is_struct() || type->is_xception();
}

mstch::node mstch_const_value::const_struct_type() {
  if (!const_value_->ttype()) {
    return {};
  }

  const auto* type = const_value_->ttype()->deref().get_true_type();
  if (type->is_struct() || type->is_xception()) {
    return generators_->type_generator_->generate(type, generators_, cache_);
  }

  return {};
}

mstch::node mstch_const_value::const_struct() {
  std::vector<t_const*> constants;
  std::vector<int32_t> idx;
  std::vector<std::string> field_names;
  mstch::array a;

  const auto* type = const_value_->ttype()->deref().get_true_type();
  if (type->is_struct() || type->is_xception()) {
    auto const* strct = dynamic_cast<t_struct const*>(type);
    for (auto member : const_value_->get_map()) {
      auto const* field = strct->get_field_by_name(member.first->get_string());
      assert(field != nullptr);
      constants.push_back(new t_const(
          nullptr,
          field->get_type(),
          field->get_name(),
          member.second->clone()));
      idx.push_back(field->get_key());
      field_names.push_back(field->get_name());
    }
  }

  for (size_t i = 0; i < constants.size(); ++i) {
    auto pos = element_position(i, constants.size());
    a.push_back(generators_->const_generator_->generate(
        constants[i],
        generators_,
        cache_,
        pos,
        idx[i],
        current_const_,
        constants[i]->get_type(),
        field_names[i]));
  }
  return a;
}

mstch::node mstch_const_value::owning_const() {
  return generators_->const_generator_->generate(
      const_value_->get_owner(), generators_, cache_, pos_);
}

mstch::node mstch_field::type() {
  return generators_->type_generator_->generate(
      field_->get_type(), generators_, cache_, pos_);
}

mstch::node mstch_struct::fields() {
  return generate_fields(strct_->get_members());
}

mstch::node mstch_struct::thrift_uri() {
  return strct_->get_annotation("thrift.uri");
}

mstch::node mstch_struct::exception_safety() {
  if (!strct_->is_xception()) {
    return std::string("");
  }

  const auto* t_ex_ptr = dynamic_cast<const t_exception*>(strct_);
  auto s = t_ex_ptr->safety();

  switch (s) {
    case t_error_safety::safe:
      return std::string("SAFE");
    default:
      return std::string("UNSPECIFIED");
  }
}

mstch::node mstch_struct::exception_blame() {
  if (!strct_->is_xception()) {
    return std::string("");
  }

  const auto* t_ex_ptr = dynamic_cast<const t_exception*>(strct_);
  auto s = t_ex_ptr->blame();

  switch (s) {
    case t_error_blame::server:
      return std::string("SERVER");
    case t_error_blame::client:
      return std::string("CLIENT");
    default:
      return std::string("UNSPECIFIED");
  }
}

mstch::node mstch_struct::exception_kind() {
  if (!strct_->is_xception()) {
    return std::string("");
  }

  const auto* t_ex_ptr = dynamic_cast<const t_exception*>(strct_);
  auto s = t_ex_ptr->kind();

  switch (s) {
    case t_error_kind::transient:
      return std::string("TRANSIENT");
    case t_error_kind::stateful:
      return std::string("STATEFUL");
    case t_error_kind::permanent:
      return std::string("PERMANENT");
    default:
      return std::string("UNSPECIFIED");
  }
}

mstch::node mstch_function::return_type() {
  return generators_->type_generator_->generate(
      function_->get_returntype(), generators_, cache_, pos_);
}

mstch::node mstch_function::exceptions() {
  return generate_fields(function_->get_xceptions()->get_members());
}

mstch::node mstch_function::stream_exceptions() {
  return generate_fields(function_->get_stream_xceptions()->get_members());
}

mstch::node mstch_function::sink_exceptions() {
  return generate_fields(function_->get_sink_xceptions()->get_members());
}

mstch::node mstch_function::sink_final_response_exceptions() {
  return generate_fields(
      function_->get_sink_final_response_xceptions()->get_members());
}

mstch::node mstch_function::arg_list() {
  return generate_fields(function_->get_paramlist()->get_members());
}

mstch::node mstch_function::returns_stream() {
  return function_->returns_stream();
}

mstch::node mstch_service::functions() {
  return generate_functions(get_functions());
}

mstch::node mstch_service::extends() {
  auto const* extends = service_->get_extends();
  if (extends) {
    return generate_cached_extended_service(extends);
  }
  return mstch::node();
}

mstch::node mstch_service::generate_cached_extended_service(
    const t_service* service) {
  std::string id =
      service->program()->name() + get_service_namespace(service->program());
  size_t element_index = 0;
  size_t element_count = 1;
  return generate_element_cached(
      service,
      generators_->service_generator_.get(),
      cache_->services_,
      id,
      element_index,
      element_count);
}

mstch::node mstch_typedef::type() {
  return generators_->type_generator_->generate(
      typedf_->get_type(), generators_, cache_, pos_);
}

mstch::node mstch_const::type() {
  return generators_->type_generator_->generate(
      cnst_->get_type(), generators_, cache_, pos_);
}

mstch::node mstch_const::value() {
  return generators_->const_value_generator_->generate(
      cnst_->get_value(), generators_, cache_, pos_, 0, cnst_, expected_type_);
}

mstch::node mstch_const::program() {
  return generators_->program_generator_->generate(
      cnst_->get_program(), generators_, cache_, pos_);
}

mstch::node mstch_program::has_thrift_uris() {
  for (const auto& strct : program_->structs()) {
    if (strct->has_annotation("thrift.uri")) {
      return true;
    }
  }
  return false;
}

mstch::node mstch_program::structs() {
  std::string id = program_->name() + get_program_namespace(program_);
  return generate_elements_cached(
      get_program_objects(),
      generators_->struct_generator_.get(),
      cache_->structs_,
      id);
}

mstch::node mstch_program::enums() {
  std::string id = program_->name() + get_program_namespace(program_);
  return generate_elements_cached(
      get_program_enums(),
      generators_->enum_generator_.get(),
      cache_->enums_,
      id);
}

mstch::node mstch_program::services() {
  std::string id = program_->name() + get_program_namespace(program_);
  return generate_elements_cached(
      program_->services(),
      generators_->service_generator_.get(),
      cache_->services_,
      id);
}

mstch::node mstch_program::typedefs() {
  return generate_typedefs(program_->typedefs());
}

mstch::node mstch_program::constants() {
  mstch::array a;
  const auto& container = program_->consts();
  for (size_t i = 0; i < container.size(); ++i) {
    auto pos = element_position(i, container.size());
    a.push_back(generators_->const_generator_->generate(
        container[i],
        generators_,
        cache_,
        pos,
        i,
        container[i],
        container[i]->get_type()));
  }
  return a;
}

const std::vector<t_struct*>& mstch_program::get_program_objects() {
  return program_->objects();
}
const std::vector<t_enum*>& mstch_program::get_program_enums() {
  return program_->enums();
}

} // namespace compiler
} // namespace thrift
} // namespace apache
