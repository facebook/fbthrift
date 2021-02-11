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
#include <vector>

#include <thrift/compiler/ast/t_node.h>

namespace apache {
namespace thrift {
namespace compiler {

class t_const;

/**
 * class t_type
 *
 * Generic representation of any parsed element that can support annotations
 */
class t_annotated : public t_node {
 public:
  // TODO(afuller): Make these private, and any shared state should be const.
  std::map<std::string, std::string> annotations_;
  std::map<std::string, std::shared_ptr<t_const>> annotation_objects_;
  int annotation_last_lineno_ = -1;

  ~t_annotated() override;

  // Returns true if there exists an annotation with the given name.
  bool has_annotation(const std::string& name) const {
    return annotations_.find(name) != annotations_.end();
  }
  // Returns true if there exists an annotation with any of the given names.
  bool has_annotation(std::initializer_list<std::string> names) const;

  // Returns the value of an annotation with the given name.
  // If not found returns the provided default or "".
  const std::string& get_annotation(
      const std::string& name,
      const std::string* default_value = nullptr) const;
  std::string get_annotation(const std::string& name, std::string default_value)
      const;

  // Returns the value of the first annotation found with a given name.
  // If not found returns the provided default or "".
  const std::string& get_annotation(
      std::initializer_list<std::string> names,
      const std::string* default_value = nullptr) const;
  std::string get_annotation(
      std::initializer_list<std::string> names,
      std::string default_value) const;

  // Returns the ptr to the value of the first annotation found with a given
  // name. If not found returns nullptr.
  const std::string* get_annotation_or_null(const std::string& name) const;
  const std::string* get_annotation_or_null(
      std::initializer_list<std::string> names) const;

  void add_structured_annotation(std::unique_ptr<t_const> annot);

  const std::vector<const t_const*>& structured_annotations() const {
    return structured_annotations_raw_;
  }

 private:
  std::vector<std::shared_ptr<const t_const>> structured_annotations_;
  std::vector<const t_const*> structured_annotations_raw_;
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
  // TODO (partisan): Try to use unique_ptr and rewrite the code relying on
  // copies.
  std::shared_ptr<t_const> object_val;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
