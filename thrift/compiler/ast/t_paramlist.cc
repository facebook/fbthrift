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

#include <stdexcept>

#include <thrift/compiler/ast/t_paramlist.h>

namespace apache {
namespace thrift {
namespace compiler {

void t_paramlist::set_stream_field(std::unique_ptr<t_field> stream_field) {
  assert(!has_stream_field_);
  assert(stream_field->get_type()->is_streamresponse());

  // Inject as the first member.
  // TODO(afuller): Don't modify struct's private data.
  fields_raw_.insert(fields_raw_.begin(), stream_field.get());
  fields_raw_id_order_.insert(fields_raw_id_order_.begin(), stream_field.get());

  if (!stream_field->get_name().empty()) {
    fields_by_name_.put(*stream_field);
  }
  fields_id_order_.insert(fields_id_order_.begin(), stream_field.get());
  fields_.insert(fields_.begin(), std::move(stream_field));
  has_stream_field_ = true;
}

t_paramlist* t_paramlist::clone_DO_NOT_USE() const {
  auto clone = std::make_unique<t_paramlist>(program_);
  auto itr = fields_.begin();
  std::unique_ptr<t_field> stream_field;
  if (has_stream_field_) {
    stream_field = (*itr++)->clone_DO_NOT_USE();
  }
  for (; itr != fields_.end(); ++itr) {
    clone->append((*itr)->clone_DO_NOT_USE());
  }
  if (has_stream_field_) {
    clone->set_stream_field(std::move(stream_field));
  };
  return clone.release();
}

} // namespace compiler
} // namespace thrift
} // namespace apache
