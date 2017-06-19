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

mstch::node mstch_enum::values() {
  return generate_elements(
      enm_->get_constants(),
      generators_->enum_value_generator_.get(),
      generators_,
      cache_);
}
