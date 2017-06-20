/*
 * Copyright 2004-present Facebook, Inc.
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

#include <thrift/compiler/generate/t_mstch_objects.h>

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
    int32_t /*index*/) const {
  return std::make_shared<mstch_const_value>(
      const_value, generators, cache, pos);
}

std::shared_ptr<mstch_base> const_value_generator::generate(
    std::pair<t_const_value*, t_const_value*> const& value_pair,
    std::shared_ptr<mstch_generators const> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  return std::make_shared<mstch_const_value>(
      value_pair, generators, cache, pos);
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
    int32_t index) const {
  return std::make_shared<mstch_field>(field, generators, cache, pos, index);
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

mstch::node mstch_enum::values() {
  return generate_elements(
      enm_->get_constants(),
      generators_->enum_value_generator_.get(),
      generators_,
      cache_);
}

mstch::node mstch_type::get_struct() {
  if (type_->is_struct() || type_->is_xception()) {
    std::string id = type_->get_program()->get_name() +
        get_type_namespace(type_->get_program());
    return generate_elements_cached(
        std::vector<t_struct const*>{dynamic_cast<t_struct const*>(type_)},
        generators_->struct_generator_.get(),
        cache_->structs_,
        id,
        generators_,
        cache_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_enum() {
  if (type_->is_enum()) {
    std::string id = type_->get_program()->get_name() +
        get_type_namespace(type_->get_program());
    return generate_elements_cached(
        std::vector<t_enum const*>{dynamic_cast<t_enum const*>(type_)},
        generators_->enum_generator_.get(),
        cache_->enums_,
        id,
        generators_,
        cache_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_list_type() {
  if (type_->is_list()) {
    return mstch::node();
  }
  return mstch::node();
}

mstch::node mstch_type::get_set_type() {
  if (type_->is_set()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_set*>(type_)->get_elem_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_key_type() {
  if (type_->is_map()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_map*>(type_)->get_key_type(),
        generators_,
        cache_,
        pos_);
  }
  return mstch::node();
}

mstch::node mstch_type::get_value_type() {
  if (type_->is_map()) {
    return generators_->type_generator_->generate(
        dynamic_cast<const t_map*>(type_)->get_val_type(),
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

mstch::node mstch_field::value() {
  if (field_->get_value()) {
    return generators_->const_value_generator_->generate(
        field_->get_value(), generators_, cache_, pos_);
  }
  return mstch::node();
}

mstch::node mstch_const_value::element_key() {
  return generators_->const_value_generator_->generate(
      pair_.first, generators_, cache_, pos_);
}

mstch::node mstch_const_value::element_value() {
  return generators_->const_value_generator_->generate(
      pair_.second, generators_, cache_, pos_);
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
    return const_value_->get_string();
  }
  return mstch::node();
}

mstch::node mstch_const_value::list_elems() {
  if (type_ == cv::CV_LIST) {
    return generate_elements(
        const_value_->get_list(),
        generators_->const_value_generator_.get(),
        generators_,
        cache_);
  }
  return mstch::node();
}

mstch::node mstch_const_value::map_elems() {
  if (type_ == cv::CV_MAP) {
    return generate_elements(
        const_value_->get_map(),
        generators_->const_value_generator_.get(),
        generators_,
        cache_);
  }
  return mstch::node();
}

mstch::node mstch_field::type() {
  return generators_->type_generator_->generate(
      field_->get_type(), generators_, cache_, pos_);
}

mstch::node mstch_struct::fields() {
  return generate_elements(
      strct_->get_members(),
      generators_->field_generator_.get(),
      generators_,
      cache_);
}

mstch::node mstch_function::return_type() {
  return generators_->type_generator_->generate(
      function_->get_returntype(), generators_, cache_, pos_);
}

mstch::node mstch_function::exceptions() {
  return generate_elements(
      function_->get_xceptions()->get_members(),
      generators_->field_generator_.get(),
      generators_,
      cache_);
}

mstch::node mstch_function::arg_list() {
  return generate_elements(
      function_->get_arglist()->get_members(),
      generators_->field_generator_.get(),
      generators_,
      cache_);
}

mstch::node mstch_service::functions() {
  return generate_elements(
      service_->get_functions(),
      generators_->function_generator_.get(),
      generators_,
      cache_);
}

mstch::node mstch_service::extends() {
  auto const* extends = service_->get_extends();
  if (extends) {
    std::string id = extends->get_program()->get_name() +
        get_service_namespace(extends->get_program());
    return generate_elements_cached(
        std::vector<t_service const*>{extends},
        generators_->service_generator_.get(),
        cache_->services_,
        id,
        generators_,
        cache_);
  }
  return mstch::node();
}
