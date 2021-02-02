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

#include <variant>

#include <folly/Overload.h>
#include <folly/Range.h>

namespace apache::thrift {
// A view into a string that may or may not be stored inline.
// Unlike string_view, should be passed by move by default.
class ManagedStringView {
  using Storage = std::variant<std::string, folly::StringPiece>;

 public:
  /* implicit */ ManagedStringView(
      folly::StringPiece str,
      bool isTemporary = true)
      : str_(
            isTemporary ? Storage(std::in_place_index_t<0>{}, str.str())
                        : Storage(std::in_place_index_t<1>{}, str)) {}
  /* implicit */ ManagedStringView(
      const std::string& str,
      bool isTemporary = true)
      : str_(
            isTemporary ? Storage(std::in_place_index_t<0>{}, str)
                        : Storage(std::in_place_index_t<1>{}, str)) {}
  /* implicit */ ManagedStringView(std::string&& str)
      : str_(std::in_place_index_t<0>{}, std::move(str)) {}
  /* implicit */ ManagedStringView(const char* str, bool isTemporary = true)
      : str_(
            isTemporary ? Storage(std::in_place_index_t<0>{}, str)
                        : Storage(std::in_place_index_t<1>{}, str)) {}

  std::string str() const& {
    return folly::variant_match(
        str_,
        [](folly::StringPiece str) { return str.str(); },
        [](const std::string& str) { return str; });
  }
  std::string str() && {
    return folly::variant_match(
        std::move(str_),
        [](folly::StringPiece str) { return str.str(); },
        [](std::string&& str) { return std::move(str); });
  }

  folly::StringPiece view() const {
    return folly::variant_match(
        str_,
        [](folly::StringPiece str) { return str; },
        [](const std::string& str) { return folly::StringPiece(str); });
  }

 private:
  Storage str_;
};
} // namespace apache::thrift
