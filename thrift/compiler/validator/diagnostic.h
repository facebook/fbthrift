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

#include <iostream>
#include <memory>
#include <string>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Diagnostic message for validation use
 *
 */
class diagnostic {
 public:
  /**
   * Diagnostic type
   */
  enum class type {
    failure,
    warning,
    info,
  };

  /**
   * Constructor for diagnostic
   *
   * @param type       - diagnostic type
   * @param file       - file path location of diagnostic
   * @param line       - line location of diagnostic in the file
   * @param message    - detailed diagnostic message
   */
  diagnostic(
      type type,
      std::string const& file,
      int line,
      std::string const& message)
      : file_(file), line_(line), message_(message), type_(type) {}

  type getType() const {
    return type_;
  }

  std::string str();

  friend std::ostream& operator<<(std::ostream& os, diagnostic e);

 private:
  std::string file_;
  int line_;
  std::string message_;
  type type_;

  static std::string getStringFromType(type type);
};

std::ostream& operator<<(std::ostream& os, diagnostic e);

} // namespace compiler
} // namespace thrift
} // namespace apache
