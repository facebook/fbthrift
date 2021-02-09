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

#include <thrift/compiler/ast/t_named.h>
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_paramlist.h>
#include <thrift/compiler/ast/t_sink.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

enum class t_function_qualifier {
  None = 0,
  Oneway,
  Idempotent,
  Readonly,
};

/**
 * class t_function
 *
 * Representation of a function. Key parts are return type, function name,
 * optional modifiers, and an argument list, which is implemented as a thrift
 * struct.
 *
 */
class t_function : public t_named {
 public:
  /**
   * Constructor for t_function
   *
   * @param returntype       - The type of the value that will be returned
   * @param name             - The symbolic name of the function
   * @param paramlist        - The parameters that are passed to the functions
   * @param xceptions        - Declare the exceptions that function might throw
   * @param stream_xceptions - Exceptions to be sent via the stream
   * @param qualifier        - The qualifier of the function, if any.
   */
  t_function(
      t_type* returntype,
      std::string name,
      std::unique_ptr<t_paramlist> paramlist,
      std::unique_ptr<t_struct> xceptions = nullptr,
      std::unique_ptr<t_struct> stream_xceptions = nullptr,
      t_function_qualifier qualifier = t_function_qualifier::None)
      : t_named(std::move(name)),
        returntype_(returntype),
        paramlist_(std::move(paramlist)),
        xceptions_(std::move(xceptions)),
        stream_xceptions_(std::move(stream_xceptions)),
        qualifier_(qualifier) {
    if (is_oneway()) {
      if (!xceptions_->get_members().empty()) {
        throw std::string("Oneway methods can't throw exceptions.");
      }

      if (returntype_ == nullptr || !returntype_->is_void()) {
        throw std::string("Oneway methods must have void return type.");
      }
    }

    if (!xceptions_) {
      xceptions_ = std::make_unique<t_struct>(nullptr);
    }

    if (!stream_xceptions_) {
      stream_xceptions_ = std::make_unique<t_struct>(nullptr);
    }

    sink_xceptions_ = std::make_unique<t_struct>(nullptr);
    sink_final_response_xceptions_ = std::make_unique<t_struct>(nullptr);

    if (!stream_xceptions_->get_members().empty()) {
      if (returntype == nullptr || !returntype->is_streamresponse()) {
        throw std::string("`stream throws` only valid on stream methods");
      }
    }
  }

  t_function(
      t_sink* returntype,
      std::string name,
      std::unique_ptr<t_paramlist> paramlist,
      std::unique_ptr<t_struct> xceptions)
      : t_named(std::move(name)),
        returntype_(returntype),
        paramlist_(std::move(paramlist)),
        xceptions_(std::move(xceptions)),
        sink_xceptions_(
            std::unique_ptr<t_struct>(returntype->get_sink_xceptions())),
        sink_final_response_xceptions_(std::unique_ptr<t_struct>(
            returntype->get_final_response_xceptions())),
        qualifier_(t_function_qualifier::None) {
    if (!xceptions_) {
      xceptions_ = std::make_unique<t_struct>(nullptr);
    }
    stream_xceptions_ = std::make_unique<t_struct>(nullptr);
    if (!sink_xceptions_) {
      sink_xceptions_ = std::make_unique<t_struct>(nullptr);
    }
    if (!sink_final_response_xceptions_) {
      sink_final_response_xceptions_ = std::make_unique<t_struct>(nullptr);
    }
  }

  /**
   * t_function getters
   */
  t_type* get_returntype() const {
    return returntype_;
  }

  t_paramlist* get_paramlist() const {
    return paramlist_.get();
  }

  t_struct* get_xceptions() const {
    return xceptions_.get();
  }

  t_struct* get_stream_xceptions() const {
    return stream_xceptions_.get();
  }

  t_struct* get_sink_xceptions() const {
    return sink_xceptions_.get();
  }

  t_struct* get_sink_final_response_xceptions() const {
    return sink_final_response_xceptions_.get();
  }

  bool is_oneway() const {
    return qualifier_ == t_function_qualifier::Oneway;
  }

  bool returns_stream() const {
    return returntype_->is_streamresponse();
  }

  bool returns_sink() const {
    return returntype_->is_sink();
  }

  bool is_interaction_constructor() const {
    return isInteractionConstructor_;
  }
  void set_is_interaction_constructor() {
    isInteractionConstructor_ = true;
  }
  bool is_interaction_member() const {
    return isInteractionMember_;
  }
  void set_is_interaction_member() {
    isInteractionMember_ = true;
  }

 private:
  t_type* returntype_;
  std::unique_ptr<t_paramlist> paramlist_;
  std::unique_ptr<t_struct> xceptions_;
  std::unique_ptr<t_struct> stream_xceptions_;
  std::unique_ptr<t_struct> sink_xceptions_;
  std::unique_ptr<t_struct> sink_final_response_xceptions_;
  t_function_qualifier qualifier_;
  bool isInteractionConstructor_{false};
  bool isInteractionMember_{false};
};

} // namespace compiler
} // namespace thrift
} // namespace apache
