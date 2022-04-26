/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <stdint.h>

#include <deque>
#include <string>
#include <vector>

#include <fmt/core.h>

namespace apache {
namespace thrift {
namespace compiler {

struct source;

class source_manager {
 private:
  struct source_info {
    std::string file_name;
    std::vector<char> text;
    std::vector<uint_least32_t> line_offsets;
  };
  // This is a deque to make sure that file_name is not reallocated when
  // sources_ grows.
  std::vector<source_info> sources_;

  friend class resolved_location;

  source add_source(std::string file_name, std::vector<char> text);

 public:
  // Adds a file source.
  source add_file(std::string file_name);

  // Adds a string source; file_name is used for source locations.
  source add_string(std::string file_name, std::string src);
};

// A lightweight source location that can be resolved via source_manager.
class source_location {
 private:
  uint_least32_t source_id_ = 0;
  uint_least32_t offset_ = 0;

  friend class resolved_location;
  friend class source_manager;

  source_location(uint_least32_t source_id, uint_least32_t offset)
      : source_id_(source_id), offset_(offset) {}

 public:
  source_location() = default;

  friend bool operator==(source_location lhs, source_location rhs) {
    return lhs.source_id_ == rhs.source_id_ && lhs.offset_ == rhs.offset_;
  }
  friend bool operator!=(source_location lhs, source_location rhs) {
    return !(lhs == rhs);
  }

  friend source_location operator+(source_location lhs, int rhs) {
    lhs.offset_ += rhs;
    return lhs;
  }
};

struct source_range {
  source_location begin;
  source_location end;
};

// A resolved (source) location that provides the file name, line and column.
class resolved_location {
 private:
  const char* file_name_;
  unsigned line_;
  unsigned column_;

 public:
  resolved_location(source_location loc, const source_manager& sm);

  // Returns the source file name. It can include directory components and/or be
  // a virtual file name that doesn't have a correspondent entry in the system's
  // directory structure.
  const char* file_name() const { return file_name_; }

  unsigned line() const { return line_; }
  unsigned column() const { return column_; }
};

struct source {
  source_location start;
  fmt::string_view text;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
