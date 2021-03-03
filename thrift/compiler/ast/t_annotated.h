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

// An ordered list of the aliases for an annotation.
//
// If two aliases are set on a node, the value for the first alias in the span
// will be used.
//
// TODO(afuller): Make this non-owning like std::span and std::string_view.
class aliases {
 public:
  using value_type = std::string;
  using const_reference = const std::string&;
  using reference = const_reference;
  using size_type = std::size_t;
  using const_iterator = std::vector<std::string>::const_iterator;
  using iterator = const_iterator;

  /* implicit */ aliases(std::initializer_list<std::string> list)
      : aliases_(list) {}
  /* implicit */ aliases(std::string name) {
    aliases_.emplace_back(std::move(name));
  }
  /* implicit */ aliases(const char* name) {
    aliases_.emplace_back(std::move(name));
  }

  const_iterator begin() const {
    return aliases_.begin();
  }
  const_iterator end() const {
    return aliases_.end();
  }
  size_type size() const {
    return aliases_.size();
  }

 private:
  std::vector<std::string> aliases_;
};

/**
 * class t_annotated
 *
 * Generic representation of any parsed element that can support annotations
 */
class t_annotated : public t_node {
 public:
  ~t_annotated() override;

  // The annotaions declared directly on this node.
  const std::map<std::string, std::string>& annotations() const {
    return annotations_;
  }

  // Returns true if there exists an annotation with the given name.
  bool has_annotation(const aliases& name) const {
    return get_annotation_or_null(name) != nullptr;
  }

  // Returns the pointer to the value of the first annotation found with the
  // given name.
  //
  // If not found returns nullptr.
  const std::string* get_annotation_or_null(const aliases& name) const;

  // Returns the value of an annotation with the given name.
  //
  // If not found returns the provided default or "".
  template <typename D = const std::string*>
  decltype(auto) get_annotation(
      const aliases& name,
      D&& default_value = nullptr) const {
    return annotation_or(
        get_annotation_or_null(name), std::forward<D>(default_value));
  }

  void reset_annotations(
      std::map<std::string, std::string> annotations,
      int last_lineno) {
    annotations_ = std::move(annotations);
    last_annotation_lineno_ = last_lineno;
  }

  void set_annotation(const std::string& key, std::string value) {
    annotations_[key] = std::move(value);
  }

  const std::vector<const t_const*>& structured_annotations() const {
    return structured_annotations_raw_;
  }
  void add_structured_annotation(std::unique_ptr<t_const> annot);

  int last_annotation_lineno() const {
    return last_annotation_lineno_;
  }

 protected:
  // t_annotated is abstract.
  t_annotated() = default;

  template <typename D>
  static std::string annotation_or(const std::string* val, D&& def) {
    if (val != nullptr) {
      return *val;
    }
    return std::forward<D>(def);
  }

  static const std::string& annotation_or(
      const std::string* val,
      const std::string* def) {
    return val ? *val : (def ? *def : kEmptyString);
  }
  static const std::string& annotation_or(
      const std::string* val,
      std::string* def) {
    return val ? *val : (def ? *def : kEmptyString);
  }

 private:
  std::map<std::string, std::string> annotations_;
  // TODO(afuller): Looks like only this is only used by t_json_generator.
  // Consider removing.
  int last_annotation_lineno_ = -1;
  std::vector<std::shared_ptr<const t_const>> structured_annotations_;
  std::vector<const t_const*> structured_annotations_raw_;

  static const std::string kEmptyString;
};

using t_annotation = std::map<std::string, std::string>::value_type;

} // namespace compiler
} // namespace thrift
} // namespace apache
