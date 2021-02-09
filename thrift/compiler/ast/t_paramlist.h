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

#pragma once

#include <thrift/compiler/ast/t_struct.h>

namespace apache {
namespace thrift {
namespace compiler {

class t_paramlist : public t_struct {
 public:
  using t_struct::t_struct;

  bool is_paramlist() const override {
    return true;
  }

  t_field* get_stream_field() {
    return has_stream_field ? members_[0].get() : nullptr;
  }

  void set_stream_field(std::unique_ptr<t_field> stream_field) {
    assert(!has_stream_field);
    assert(stream_field->get_type()->is_streamresponse());

    // Add as the first member.
    members_raw_.insert(members_raw_.begin(), stream_field.get());
    members_in_id_order_.insert(
        members_in_id_order_.begin(), stream_field.get());
    members_.insert(members_.begin(), std::move(stream_field));
    has_stream_field = true;
  }

 private:
  // not stored as a normal member, as it's not serialized like a normal
  // field into the pargs struct
  bool has_stream_field = false;

  friend class t_struct;
  t_paramlist* clone_DO_NOT_USE() const override {
    auto clone = std::make_unique<t_paramlist>(program_, name_);
    cloneStruct(clone.get());
    clone->has_stream_field = has_stream_field;
    return clone.release();
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
