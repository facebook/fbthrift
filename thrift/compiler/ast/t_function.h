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

#include <thrift/compiler/ast/node_list.h>
#include <thrift/compiler/ast/t_named.h>
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_paramlist.h>
#include <thrift/compiler/ast/t_throws.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

enum class t_function_qualifier {
  unspecified = 0,
  one_way,
  idempotent,
  read_only,
};

/**
 * A thrift function declaration.
 */
class t_function final : public t_named {
 public:
  /**
   * Constructor for t_function
   *
   * @param return_type      - The type of the value that will be returned
   * @param name             - The symbolic name of the function
   * @param paramlist        - The parameters that are passed to the functions
   */
  t_function(
      t_type_ref return_type,
      std::string name,
      std::unique_ptr<t_paramlist> paramlist,
      t_function_qualifier qualifier = {});

  const t_type_ref& return_type() const { return return_type_; }
  const t_paramlist* params() const { return paramlist_.get(); }

  // The qualifier of the function, if any.
  t_function_qualifier qualifier() const { return qualifier_; }

  // The declared exceptions that function might throw.
  //
  // Returns nullptr when the throws clause is absent.
  t_throws* exceptions() { return exceptions_.get(); }
  const t_throws* exceptions() const { return exceptions_.get(); }
  // Use nullptr to indicate an absent throws clause.
  void set_exceptions(std::unique_ptr<t_throws> exceptions);

  bool is_interaction_constructor() const { return isInteractionConstructor_; }
  void set_is_interaction_constructor() { isInteractionConstructor_ = true; }
  bool is_interaction_member() const { return isInteractionMember_; }
  void set_is_interaction_member() { isInteractionMember_ = true; }

 private:
  t_type_ref return_type_;
  std::unique_ptr<t_paramlist> paramlist_;
  std::unique_ptr<t_throws> exceptions_;
  const t_function_qualifier qualifier_;
  bool isInteractionConstructor_{false};
  bool isInteractionMember_{false};

  // TODO(afuller): Delete everything below here. It is only provided for
  // backwards compatibility.
 public:
  t_function(
      const t_type* return_type,
      std::string name,
      std::unique_ptr<t_paramlist> paramlist,
      std::unique_ptr<t_throws> exceptions = nullptr,
      t_function_qualifier qualifier = {})
      : t_function(
            t_type_ref::from_req_ptr(return_type),
            std::move(name),
            std::move(paramlist),
            qualifier) {
    set_exceptions(std::move(exceptions));
  }

  t_paramlist* get_paramlist() const { return paramlist_.get(); }
  const t_type* get_return_type() const { return return_type().get_type(); }
  const t_type* get_returntype() const { return return_type().get_type(); }
  const t_throws* get_xceptions() const {
    return t_throws::or_empty(exceptions());
  }
  const t_throws* get_stream_xceptions() const {
    return t_throws::or_empty(stream_exceptions_);
  }
  const t_throws* get_sink_xceptions() const {
    return t_throws::or_empty(sink_exceptions_);
  }
  const t_throws* get_sink_final_response_xceptions() const {
    return t_throws::or_empty(sink_final_response_exceptions_);
  }
  bool is_oneway() const { return qualifier_ == t_function_qualifier::one_way; }
  bool returns_stream() const { return return_type_->is_streamresponse(); }
  bool returns_sink() const { return return_type_->is_sink(); }

 private:
  // Extracted from return type for easy access.
  const t_throws* stream_exceptions_ = nullptr;
  const t_throws* sink_exceptions_ = nullptr;
  const t_throws* sink_final_response_exceptions_ = nullptr;
};

using t_function_list = node_list<t_function>;

} // namespace compiler
} // namespace thrift
} // namespace apache
