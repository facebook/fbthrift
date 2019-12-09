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

#include <string>

#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

class t_stream_response : public t_type {
 public:
  explicit t_stream_response(t_type* elem_type, t_struct* throws = nullptr)
      : elem_type_(elem_type), throws_(throws) {}

  t_type* get_elem_type() const {
    return elem_type_;
  }

  t_type* get_first_response_type() const {
    return first_response_type_;
  }

  void set_first_response_type(t_type* first_response_type) {
    first_response_type_ = first_response_type;
  }

  bool is_streamresponse() const override {
    return true;
  }

  bool has_first_response() const {
    return (bool)(first_response_type_);
  }

  std::string get_full_name() const override {
    if (has_first_response()) {
      return first_response_type_->get_full_name() + ", stream<" +
          elem_type_->get_full_name() + ">";
    }
    return "stream<" + elem_type_->get_full_name() + ">";
  }

  std::string get_impl_full_name() const override {
    if (has_first_response()) {
      return first_response_type_->get_impl_full_name() + ", stream<" +
          elem_type_->get_impl_full_name() + ">";
    }
    return "stream<" + elem_type_->get_impl_full_name() + ">";
  }

  TypeValue get_type_value() const override {
    return TypeValue::TYPE_STREAM;
  }

  t_struct* get_throws_struct() const {
    return throws_;
  }
  bool has_throws_struct() const {
    return (bool)throws_;
  }

 private:
  t_type* first_response_type_{nullptr};
  t_type* elem_type_;
  t_struct* throws_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
