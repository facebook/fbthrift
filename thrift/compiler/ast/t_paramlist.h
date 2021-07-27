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

#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_struct.h>

namespace apache {
namespace thrift {
namespace compiler {

class t_program;

// TODO(afuller): Inherit from t_structured instead.
class t_paramlist : public t_struct {
 public:
  // Param lists are unnamed.
  // TODO(afuller): The program should always be null, because this is an
  // 'unnamed' t_type (or more accurately, not a type at all).
  explicit t_paramlist(t_program* program) : t_struct(program, "") {}

  t_field* get_stream_field() {
    return has_stream_field_ ? fields_[0].get() : nullptr;
  }

  void set_stream_field(std::unique_ptr<t_field> stream_field);

 private:
  // not stored as a normal member, as it's not serialized like a normal
  // field into the pargs struct
  bool has_stream_field_ = false;

  friend class t_structured;
  t_paramlist* clone_DO_NOT_USE() const override;

  // TODO(afuller): Remove everything below this comment. It is only provided
  // for backwards compatibility.
 public:
  bool is_paramlist() const override { return true; }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
