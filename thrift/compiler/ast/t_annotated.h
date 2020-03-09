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

#include <map>
#include <memory>
#include <string>

#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_doc.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * class t_type
 *
 * Generic representation of any parsed element that can support annotations
 */
class t_annotated : public t_doc {
 public:
  virtual ~t_annotated() {}

  std::map<std::string, std::string> annotations_;
  std::map<std::string, std::shared_ptr<t_const>> annotation_objects_;
};

/**
 * Placeholder struct to return key and value of an annotation during parsing.
 */
struct t_annotation {
  t_annotation() = default;
  t_annotation(const std::string& key_, const std::string& val_)
      : key(key_), val(val_) {}
  std::string key;
  std::string val;
  std::shared_ptr<t_const> object_val;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
