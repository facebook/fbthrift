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

#include <memory>
#include <string>
#include <utility>

#include <thrift/compiler/ast/t_throws.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

class t_stream_response : public t_type {
 public:
  explicit t_stream_response(
      t_type_ref elem_type, std::unique_ptr<t_throws> throws = nullptr)
      : elem_type_(std::move(elem_type)), throws_(std::move(throws)) {}

  const t_type_ref* elem_type() const { return &elem_type_; }
  const t_throws* throws() const { return t_throws::get_or_empty(throws_); }

  void set_first_response_type(
      std::unique_ptr<t_type_ref> first_response_type) {
    first_response_type_ = std::move(first_response_type);
  }

  bool has_first_response() const { return first_response_type_ != nullptr; }
  const t_type_ref* first_response_type() const {
    return first_response_type_.get();
  }

  bool is_streamresponse() const override { return true; }

  std::string get_full_name() const override {
    if (has_first_response()) {
      return first_response_type_->type()->get_full_name() + ", stream<" +
          elem_type_.type()->get_full_name() + ">";
    }
    return "stream<" + elem_type_.type()->get_full_name() + ">";
  }

  type get_type_value() const override { return type::t_stream; }

 private:
  t_type_ref elem_type_;
  std::unique_ptr<t_throws> throws_;
  std::unique_ptr<t_type_ref> first_response_type_;

 public:
  // TODO(afuller): Delete everything below here. It is only provided for
  // backwards compatibility.

  explicit t_stream_response(
      const t_type* elem_type, std::unique_ptr<t_throws> throws = nullptr)
      : t_stream_response(t_type_ref(elem_type), std::move(throws)) {}

  void set_first_response_type(const t_type* first_response_type) {
    set_first_response_type(std::make_unique<t_type_ref>(first_response_type));
  }
  const t_type* get_elem_type() const { return elem_type()->type(); }
  const t_type* get_first_response_type() const {
    return has_first_response() ? first_response_type()->type() : nullptr;
  }
  t_throws* get_throws_struct() const { return throws_.get(); }
  bool has_throws_struct() const { return throws_ == nullptr; }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
