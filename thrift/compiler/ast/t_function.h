/*
 * Copyright 2016-present Facebook, Inc.
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
#pragma once

#include <string>

#include <thrift/compiler/ast/t_doc.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * class t_function
 *
 * Representation of a function. Key parts are return type, function name,
 * optional modifiers, and an argument list, which is implemented as a thrift
 * struct.
 *
 */
class t_function : public t_doc {
 public:
  /**
   * Constructor for t_function
   *
   * @param returntype  - The type of the value that will be returned
   * @param name        - The symbolic name of the function
   * @param arglist     - The parameters that are passed to the functions
   * @param xceptions   - Declare the exceptions that function might throw
   * @param stream_xceptions - Exceptions to be sent via the stream
   * @param annotations - Optional args that add more functionality
   * @param oneway      - Determines if it is a one way function
   */
  t_function(
      t_type* returntype,
      std::string name,
      std::unique_ptr<t_struct> arglist,
      std::unique_ptr<t_struct> xceptions = nullptr,
      std::unique_ptr<t_struct> stream_xceptions = nullptr,
      std::unique_ptr<t_type> annotations = nullptr,
      bool oneway = false)
      : returntype_(returntype),
        name_(name),
        arglist_(std::move(arglist)),
        xceptions_(std::move(xceptions)),
        stream_xceptions_(std::move(stream_xceptions)),
        annotations_(std::move(annotations)),
        oneway_(oneway) {
    if (oneway_) {
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

    if (!stream_xceptions_->get_members().empty()) {
      if (returntype == nullptr || !returntype->is_streamresponse()) {
        throw std::string("`stream throws` only valid on stream methods");
      }
    }
  }

  ~t_function() {}

  /**
   * t_function getters
   */
  t_type* get_returntype() const {
    return returntype_;
  }

  const std::string& get_name() const {
    return name_;
  }

  t_struct* get_arglist() const {
    return arglist_.get();
  }

  t_struct* get_xceptions() const {
    return xceptions_.get();
  }

  t_struct* get_stream_xceptions() const {
    return stream_xceptions_.get();
  }

  t_type* get_annotations() const {
    return annotations_.get();
  }

  bool is_oneway() const {
    return oneway_;
  }

  // are any of the {return type/argument types} a pubsub stream?
  bool any_streams() const {
    if (returntype_->is_pubsub_stream()) {
      return true;
    }
    return any_stream_params();
  }

  bool coroutine() const {
    return annotations_ != nullptr &&
        annotations_->annotations_.count("coroutine") > 0;
  }

  bool any_stream_params() const {
    auto& members = arglist_->get_members();
    return std::any_of(members.cbegin(), members.cend(), [](auto const& arg) {
      return arg->get_type()->is_pubsub_stream();
    });
  }

 private:
  t_type* returntype_;
  std::string name_;
  std::unique_ptr<t_struct> arglist_;
  std::unique_ptr<t_struct> xceptions_;
  std::unique_ptr<t_struct> stream_xceptions_;
  std::unique_ptr<t_type> annotations_;
  bool oneway_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
