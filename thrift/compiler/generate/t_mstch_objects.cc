/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

std::shared_ptr<mstch_base> generate_cached(
    const mstch_program_factory& factory,
    const t_program* program,
    std::shared_ptr<const mstch_factories> factories,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos) {
  const auto& id = program->path();
  auto itr = cache->programs_.find(id);
  if (itr == cache->programs_.end()) {
    itr = cache->programs_.emplace_hint(
        itr, id, factory.generate(program, factories, cache, pos));
  }
  return itr->second;
}

std::shared_ptr<mstch_base> generate_cached(
    const mstch_service_factory& factory,
    const t_program* program,
    const t_service* service,
    std::shared_ptr<const mstch_factories> factories,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos) {
  std::string service_id = program->path() + service->get_name();
  auto itr = cache->services_.find(service_id);
  if (itr == cache->services_.end()) {
    itr = cache->services_.emplace_hint(
        itr, service_id, factory.generate(service, factories, cache, pos));
  }
  return itr->second;
}

bool mstch_base::has_option(const std::string& option) const {
  return cache_->options_.find(option) != cache_->options_.end();
}

std::string mstch_base::get_option(const std::string& option) const {
  auto itr = cache_->options_.find(option);
  if (itr != cache_->options_.end()) {
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

mstch::node mstch_base::is_struct() {
  return dynamic_cast<mstch_struct*>(this) != nullptr;
}

mstch_factories::mstch_factories() {
  set_program_factory<mstch_program>();
  set_type_factory<mstch_type>();
  set_typedef_factory<mstch_typedef>();
  set_struct_factory<mstch_struct>();
  set_field_factory<mstch_field>();
  set_enum_factory<mstch_enum>();
  set_enum_value_factory<mstch_enum_value>();
  set_const_factory<mstch_const>();
  set_const_value_factory<mstch_const_value>();
  set_const_map_element_factory<mstch_const_map_element>();
  set_structured_annotation_factory<mstch_structured_annotation>();
  set_deprecated_annotation_factory<mstch_deprecated_annotation>();
}

mstch::node mstch_enum::values() {
  return generate_enum_values(enm_->get_enum_values());
}

mstch::node mstch_type::get_struct() {
  if (type_->is_struct() || type_->is_xception()) {
    std::string id =
        type_->program()->name() + get_type_namespace(type_->program());
    return generate_elements_cached(
        std::vector<const t_struct*>{dynamic_cast<const t_struct*>(type_)},
        factories_->struct_factory.get(),
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
        std::vector<const t_enum*>{dynamic_cast<const t_enum*>(resolved_type_)},
        factories_->enum_factory.get(),
        cache_->enums_,
        id);
  }
  return mstch::node();
}

mstch::node mstch_type::get_list_type() {
  if (resolved_type_->is_list()) {
    return factories_->type_factory->generate(
        dynamic_cast<const t_list*>(resolved_type_)->get_elem_type(),
        factories_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_set_type() {
  if (resolved_type_->is_set()) {
    return factories_->type_factory->generate(
        dynamic_cast<const t_set*>(resolved_type_)->get_elem_type(),
        factories_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_key_type() {
  if (resolved_type_->is_map()) {
    return factories_->type_factory->generate(
        dynamic_cast<const t_map*>(resolved_type_)->get_key_type(),
        factories_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_value_type() {
  if (resolved_type_->is_map()) {
    return factories_->type_factory->generate(
        dynamic_cast<const t_map*>(resolved_type_)->get_val_type(),
        factories_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_typedef_type() {
  if (type_->is_typedef()) {
    return factories_->type_factory->generate(
        dynamic_cast<const t_typedef*>(type_)->get_type(),
        factories_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_typedef() {
  if (type_->is_typedef()) {
    return factories_->typedef_factory->generate(
        dynamic_cast<const t_typedef*>(type_), factories_, cache_, pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_sink_elem_type() {
  if (type_->is_sink()) {
    return factories_->type_factory->generate(
        dynamic_cast<const t_sink*>(type_)->get_sink_type(),
        factories_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_sink_final_reponse_type() {
  if (type_->is_sink()) {
    return factories_->type_factory->generate(
        dynamic_cast<const t_sink*>(type_)->get_final_response_type(),
        factories_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_sink_first_response_type() {
  if (type_->is_sink()) {
    if (const auto sinkresponse =
            dynamic_cast<const t_sink*>(type_)->get_first_response_type()) {
      return factories_->type_factory->generate(
          sinkresponse, factories_, cache_, pos_);
    }
  }
  return mstch::node();
}

mstch::node mstch_type::get_stream_elem_type() {
  if (type_->is_streamresponse()) {
    return factories_->type_factory->generate(
        dynamic_cast<const t_stream_response*>(type_)->get_elem_type(),
        factories_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_stream_first_response_type() {
  if (resolved_type_->is_streamresponse()) {
    if (const auto streamresponse =
            dynamic_cast<const t_stream_response*>(resolved_type_)
                ->get_first_response_type()) {
      return factories_->type_factory->generate(
          streamresponse, factories_, cache_, pos_);
    }
  }
  return mstch::node();
}

mstch::node mstch_field::value() {
  if (!field_->get_value()) {
    return mstch::node();
  }
  return factories_->const_value_factory->generate(
      field_->get_value(), factories_, cache_, pos_, 0, nullptr, nullptr);
}

mstch::node mstch_const_map_element::element_key() {
  return factories_->const_value_factory->generate(
      element_.first,
      factories_,
      cache_,
      pos_,
      index_,
      current_const_,
      expected_types_.first);
}

mstch::node mstch_const_map_element::element_value() {
  return factories_->const_value_factory->generate(
      element_.second,
      factories_,
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
  if (type_ == cv::CV_INTEGER && const_value_->is_enum() &&
      const_value_->get_enum_value() != nullptr) {
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
  if (type_ != cv::CV_MAP) {
    return mstch::node();
  }
  std::pair<const t_type*, const t_type*> expected_types;
  if (expected_type_ && expected_type_->is_map()) {
    const auto* m = dynamic_cast<const t_map*>(expected_type_);
    expected_types = {m->get_key_type(), m->get_val_type()};
  }
  return generate_elements(
      const_value_->get_map(),
      factories_->const_map_element_factory.get(),
      current_const_,
      expected_types);
}

mstch::node mstch_const_value::is_const_struct() {
  if (!const_value_->ttype()) {
    return false;
  }
  const auto* type = const_value_->ttype()->get_true_type();
  return type->is_struct() || type->is_xception();
}

mstch::node mstch_const_value::const_struct_type() {
  if (!const_value_->ttype()) {
    return {};
  }

  const auto* type = const_value_->ttype()->get_true_type();
  if (type->is_struct() || type->is_xception()) {
    return factories_->type_factory->generate(type, factories_, cache_);
  }

  return {};
}

mstch::node mstch_const_value::const_struct() {
  std::vector<t_const*> constants;
  std::vector<const t_field*> fields;
  mstch::array a;

  const auto* type = const_value_->ttype()->get_true_type();
  if (type->is_struct() || type->is_xception()) {
    const auto* strct = dynamic_cast<const t_struct*>(type);
    for (auto member : const_value_->get_map()) {
      const auto* field = strct->get_field_by_name(member.first->get_string());
      assert(field != nullptr);
      constants.push_back(new t_const(
          nullptr,
          field->get_type(),
          field->get_name(),
          member.second->clone()));
      fields.push_back(field);
    }
  }

  for (size_t i = 0; i < constants.size(); ++i) {
    auto pos = element_position(i, constants.size());
    a.push_back(factories_->const_factory->generate(
        constants[i],
        factories_,
        cache_,
        pos,
        fields[i]->get_key(),
        current_const_,
        constants[i]->get_type(),
        fields[i]));
  }
  return a;
}

mstch::node mstch_const_value::owning_const() {
  return factories_->const_factory->generate(
      const_value_->get_owner(),
      factories_,
      cache_,
      pos_,
      0,
      nullptr,
      nullptr,
      nullptr);
}

mstch::node mstch_field::type() {
  return factories_->type_factory->generate(
      field_->get_type(), factories_, cache_, pos_);
}

mstch::node mstch_struct::fields() {
  return generate_fields(strct_->get_members());
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

const std::vector<const t_field*>& mstch_struct::get_members_in_key_order() {
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

mstch::node mstch_function::return_type() {
  return factories_->type_factory->generate(
      function_->get_returntype(), factories_, cache_, pos_);
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
  const auto* extends = service_->get_extends();
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
      factories_->service_factory.get(),
      cache_->services_,
      id,
      element_index,
      element_count);
}

mstch::node mstch_typedef::type() {
  return factories_->type_factory->generate(
      typedf_->get_type(), factories_, cache_, pos_);
}

mstch::node mstch_const::type() {
  return factories_->type_factory->generate(
      cnst_->get_type(), factories_, cache_, pos_);
}

mstch::node mstch_const::value() {
  return factories_->const_value_factory->generate(
      cnst_->get_value(), factories_, cache_, pos_, 0, cnst_, expected_type_);
}

mstch::node mstch_const::program() {
  return factories_->program_factory->generate(
      cnst_->get_program(), factories_, cache_, pos_);
}

mstch::node mstch_program::has_thrift_uris() {
  for (const auto& strct : program_->structs()) {
    if (!strct->uri().empty()) {
      return true;
    }
  }
  return false;
}

mstch::node mstch_program::structs() {
  std::string id = program_->name() + get_program_namespace(program_);
  return generate_elements_cached(
      get_program_objects(),
      factories_->struct_factory.get(),
      cache_->structs_,
      id);
}

mstch::node mstch_program::enums() {
  std::string id = program_->name() + get_program_namespace(program_);
  return generate_elements_cached(
      get_program_enums(), factories_->enum_factory.get(), cache_->enums_, id);
}

mstch::node mstch_program::services() {
  std::string id = program_->name() + get_program_namespace(program_);
  return generate_elements_cached(
      program_->services(),
      factories_->service_factory.get(),
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
    a.push_back(factories_->const_factory->generate(
        container[i],
        factories_,
        cache_,
        pos,
        i,
        container[i],
        container[i]->get_type(),
        nullptr));
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
