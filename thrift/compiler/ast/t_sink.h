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

/**
 * A sink is a lightweight object type that just wraps another data type.
 *
 */

class t_sink : public t_type {
 public:
  explicit t_sink(
      t_type_ref sink_type,
      t_struct* sink_xceptions,
      t_type_ref final_response_type,
      t_struct* final_response_xceptions)
      : sink_type_(std::move(sink_type)),
        sink_xceptions_(sink_xceptions),
        final_response_type_(std::move(final_response_type)),
        final_response_xceptions_(final_response_xceptions) {}

  void set_first_response_type(std::unique_ptr<t_type_ref> first_response) {
    first_response_type_ = std::move(first_response);
  }

  bool has_first_response() const {
    return first_response_type_ != nullptr;
  }

  const t_type* get_first_response_type() const {
    return first_response_type_->get_type();
  }

  const t_type* get_sink_type() const {
    return sink_type_.get_type();
  }

  t_struct* get_sink_xceptions() const {
    return sink_xceptions_;
  }

  const t_type* get_final_response_type() const {
    return final_response_type_.get_type();
  }

  t_struct* get_final_response_xceptions() const {
    return final_response_xceptions_;
  }

  bool is_sink() const override {
    return true;
  }

  std::string get_full_name() const override {
    return "sink<" + sink_type_.get_type()->get_full_name() + ", " +
        final_response_type_.get_type()->get_full_name() + ">" +
        (sink_has_first_response()
             ? (", " + first_response_type_->get_type()->get_full_name())
             : "");
  }

  type get_type_value() const override {
    return type::t_sink;
  }

 private:
  t_type_ref sink_type_;
  t_struct* sink_xceptions_;
  t_type_ref final_response_type_;
  t_struct* final_response_xceptions_;
  std::unique_ptr<t_type_ref> first_response_type_;

 public:
  // TODO(afuller): Delete everything below here. It is only provided for
  // backwards compatibility.

  explicit t_sink(
      const t_type* sink_type,
      t_struct* sink_xceptions,
      const t_type* final_response_type,
      t_struct* final_response_xceptions)
      : t_sink(
            t_type_ref(sink_type),
            sink_xceptions,
            t_type_ref(final_response_type),
            final_response_xceptions) {}

  void set_first_response(const t_type* first_response) {
    set_first_response_type(std::make_unique<t_type_ref>(first_response));
  }

  bool sink_has_first_response() const {
    return has_first_response();
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
