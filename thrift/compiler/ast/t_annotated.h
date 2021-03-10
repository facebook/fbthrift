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
// Like std::span and std::string_view, it is the caller's responsibility to
// ensure the underlying data outlives the span.
class alias_span {
 public:
  using value_type = std::string;
  using const_reference = const std::string&;
  using reference = const_reference;
  using size_type = std::size_t;
  using const_iterator = const std::string*;
  using iterator = const_iterator;

  alias_span(const std::string* data, size_type size)
      : data_(data), size_(size) {}

  /* implicit */ alias_span(std::initializer_list<std::string> name)
      : alias_span(name.begin(), name.size()) {}
  /* implicit */ alias_span(const std::string& name) : alias_span(&name, 1) {}
  template <
      typename C = std::vector<std::string>,
      typename = decltype(
          std::declval<const C&>().data() + std::declval<const C&>().size())>
  /* implicit */ alias_span(const C& list)
      : alias_span(list.data(), list.size()) {}

  const_iterator begin() const {
    return data_;
  }
  const_iterator end() const {
    return data_ + size_;
  }
  size_type size() const {
    return size_;
  }

 private:
  const std::string* data_;
  size_type size_;
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
  bool has_annotation(alias_span name) const {
    return get_annotation_or_null(name) != nullptr;
  }

  // Returns the pointer to the value of the first annotation found with the
  // given name.
  //
  // If not found returns nullptr.
  const std::string* get_annotation_or_null(alias_span name) const;

  // Returns the value of an annotation with the given name.
  //
  // If not found returns the provided default or "".
  //
  // TODO(afuller): Require all call sites to use {} and replace `const N&` with
  // alias_span.
  template <
      typename N = std::vector<std::string>,
      typename D = const std::string*>
  decltype(auto) get_annotation(const N& name, D&& default_value = nullptr)
      const {
    return annotation_or(
        get_annotation_or_null(alias_span{name}),
        std::forward<D>(default_value));
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

 public:
  // TODO(afuller): Require arg to be wrapped in {} instead of providing these
  // overloads.
  bool has_annotation(const char* name) const {
    return has_annotation(alias_span{name});
  }
  const std::string* get_annotation_or_null(const char* name) const {
    return get_annotation_or_null(alias_span{name});
  }
};

using t_annotation = std::map<std::string, std::string>::value_type;

} // namespace compiler
} // namespace thrift
} // namespace apache
