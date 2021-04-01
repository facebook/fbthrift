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

#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

class t_stream_response : public t_type {
 public:
  explicit t_stream_response(t_type_ref elem_type, t_struct* throws = nullptr)
      : elem_type_(std::move(elem_type)), throws_(throws) {}

  const t_type* get_elem_type() const { return elem_type_.type(); }

  void set_first_response_type(
      std::unique_ptr<t_type_ref> first_response_type) {
    first_response_type_ = std::move(first_response_type);
  }

  bool has_first_response() const { return first_response_type_ != nullptr; }

  const t_type* get_first_response_type() const {
    // TODO(afuller): Fix call sites that don't check has_first_response().
    // assert(first_response_type_ != nullptr);
    return has_first_response() ? first_response_type_->type() : nullptr;
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

  t_struct* get_throws_struct() const { return throws_; }
  bool has_throws_struct() const { return (bool)throws_; }

 private:
  t_type_ref elem_type_;
  t_struct* throws_;
  std::unique_ptr<t_type_ref> first_response_type_;

 public:
  // TODO(afuller): Delete everything below here. It is only provided for
  // backwards compatibility.

  explicit t_stream_response(
      const t_type* elem_type, t_struct* throws = nullptr)
      : t_stream_response(t_type_ref(elem_type), throws) {}

  void set_first_response_type(const t_type* first_response_type) {
    first_response_type_ = std::make_unique<t_type_ref>(first_response_type);
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
