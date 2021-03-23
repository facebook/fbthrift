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

namespace apache {
namespace thrift {
namespace compiler {

/**
 * class t_node
 *
 * Base data structure for every parsed element in
 * a thrift program.
 */
class t_node {
 public:
  virtual ~t_node() = default;

  void set_doc(const std::string& doc) {
    doc_ = doc;
    has_doc_ = true;
  }

  const std::string& get_doc() const { return doc_; }

  bool has_doc() const { return has_doc_; }

  void set_lineno(int lineno) { lineno_ = lineno; }

  int get_lineno() const { return lineno_; }

 protected:
  // t_node is abstract.
  t_node() = default;

 private:
  std::string doc_;
  bool has_doc_{false};
  int lineno_{-1};
};

} // namespace compiler
} // namespace thrift
} // namespace apache
