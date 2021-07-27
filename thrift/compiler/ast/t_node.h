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

#include <cassert>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <thrift/compiler/ast/alias_span.h>
#include <thrift/compiler/ast/source_range.h>

namespace apache {
namespace thrift {
namespace compiler {

struct annotation_value {
  source_range src_range;
  std::string value;
};

/**
 * class t_node
 *
 * Base data structure for every parsed element in
 * a thrift program.
 */
class t_node {
 public:
  virtual ~t_node() = default;

  const source_range& src_range() const { return source_range_; }
  void set_src_range(const source_range& source_range) {
    source_range_ = source_range;
  }

  const std::string& doc() const { return doc_; }
  bool has_doc() const { return has_doc_; }
  void set_doc(const std::string& doc) {
    doc_ = doc;
    has_doc_ = true;
  }

  int lineno() const { return lineno_; }
  void set_lineno(int lineno) { lineno_ = lineno; }

  // The annotations declared directly on this node.
  const auto& annotations() const { return annotations_; }

  // Returns true if there exists an annotation with the given name.
  bool has_annotation(alias_span name) const {
    return get_annotation_or_null(name) != nullptr;
  }
  bool has_annotation(const char* name) const {
    return has_annotation(alias_span{name});
  }

  // Returns the pointer to the value of the first annotation found with the
  // given name.
  //
  // If not found returns nullptr.
  const std::string* get_annotation_or_null(alias_span name) const;
  const std::string* get_annotation_or_null(const char* name) const {
    return get_annotation_or_null(alias_span{name});
  }

  // Returns the value of an annotation with the given name.
  //
  // If not found returns the provided default or "".
  template <
      typename T = std::vector<std::string>,
      typename D = const std::string*>
  decltype(auto) get_annotation(
      const T& name, D&& default_value = nullptr) const {
    return annotation_or(
        get_annotation_or_null(alias_span{name}),
        std::forward<D>(default_value));
  }

  void reset_annotations(
      std::map<std::string, annotation_value> annotations, int last_lineno) {
    annotations_ = std::move(annotations);
    last_annotation_lineno_ = last_lineno;
  }

  void set_annotation(
      const std::string& key,
      const std::string& value = {},
      const source_range& range = {}) {
    annotations_[key] = {range, value};
  }

  int last_annotation_lineno() const { return last_annotation_lineno_; }

 protected:
  // t_node is abstract.
  t_node() = default;

  static const std::string kEmptyString;

  template <typename D>
  static std::string annotation_or(const std::string* val, D&& def) {
    if (val != nullptr) {
      return *val;
    }
    return std::forward<D>(def);
  }

  static const std::string& annotation_or(
      const std::string* val, const std::string* def) {
    return val ? *val : (def ? *def : kEmptyString);
  }

  static const std::string& annotation_or(
      const std::string* val, std::string* def) {
    return val ? *val : (def ? *def : kEmptyString);
  }

 private:
  std::string doc_;
  bool has_doc_{false};

  /*
   * TODO(urielrivas): Remove this lineno_, it will no longer be needed after
   * source_range.
   */
  int lineno_{-1};
  source_range source_range_;

  std::map<std::string, annotation_value> annotations_;
  // TODO(afuller): Looks like only this is only used by t_json_generator.
  // Consider removing.
  int last_annotation_lineno_{-1};

  // TODO(afuller): Remove everything below this comment. It is only provideed
  // for backwards compatibility.
 public:
  const std::string& get_doc() const { return doc_; }
  int get_lineno() const { return lineno_; }
};

using t_annotation = std::map<std::string, annotation_value>::value_type;

} // namespace compiler
} // namespace thrift
} // namespace apache
