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

#include <thrift/compiler/ast/t_function.h>

#include <stdexcept>

#include <thrift/compiler/ast/t_sink.h>
#include <thrift/compiler/ast/t_stream.h>

namespace apache {
namespace thrift {
namespace compiler {

t_function::t_function(
    t_type_ref return_type,
    std::string name,
    std::unique_ptr<t_paramlist> paramlist,
    t_function_qualifier qualifier)
    : t_named(std::move(name)),
      return_type_(std::move(return_type)),
      paramlist_(std::move(paramlist)),
      qualifier_(qualifier) {
  // Grab stream related exceptions from return type, for easy access.
  // TODO(afuller): Remove these if possible.
  if (const auto* tstream_resp =
          dynamic_cast<const t_stream_response*>(return_type_.get_type())) {
    stream_exceptions_ = tstream_resp->exceptions();
  } else if (
      const auto* tsink =
          dynamic_cast<const t_sink*>(return_type_.get_type())) {
    sink_exceptions_ = tsink->sink_exceptions();
    sink_final_response_exceptions_ = tsink->final_response_exceptions();
  }

  // Validate function
  // TODO(afuller): Move this to a post parse step.
  if (is_oneway() &&
      (return_type_.get_type() == nullptr ||
       !return_type_.get_type()->is_void())) {
    throw std::runtime_error(
        "Oneway methods must have void return type: " + name_);
  }

  // TODO(afuller): Move this to a post parse step.
  if (!t_throws::is_null_or_empty(stream_exceptions_) &&
      (return_type_.get_type() == nullptr ||
       !return_type_.get_type()->is_streamresponse())) {
    throw std::runtime_error(
        "`stream throws` only valid on stream methods: " + name_);
  }
}

void t_function::set_exceptions(std::unique_ptr<t_throws> exceptions) {
  exceptions_ = std::move(exceptions);
  // TODO(afuller): Move this to a post parse step.
  if (is_oneway() && !t_throws::is_null_or_empty(exceptions_.get())) {
    throw std::runtime_error(
        "Oneway methods can't throw exceptions: " + name());
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
