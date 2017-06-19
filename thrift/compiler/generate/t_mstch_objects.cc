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
    /*
    return generate_elements_cached(
        std::vector<t_struct const*>{dynamic_cast<t_struct const*>(type_)},
        generators_->struct_generator_.get(),
        cache_->structs_,
        id,
        generators_,
        cache_);
    */
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
    /*
    return generators_->const_value_generator_->generate(
        field_->get_value(), generators_, cache_, pos_);
    */
  }
  return mstch::node();
}

mstch::node mstch_field::type() {
  return generators_->type_generator_->generate(
      field_->get_type(), generators_, cache_, pos_);
}
