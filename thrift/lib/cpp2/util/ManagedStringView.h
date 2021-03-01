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

#include <folly/Conv.h>
#include <folly/Overload.h>
#include <folly/Range.h>
#include <thrift/lib/cpp2/Thrift.h>

namespace apache::thrift {
// A view into a string that may or may not be stored inline.
// Unlike string_view, should be passed by move by default.
class ManagedStringView {
  using Storage = std::variant<std::string, folly::StringPiece>;

 public:
  ManagedStringView() : ManagedStringView("") {}
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

  friend bool operator==(const ManagedStringView& a, folly::StringPiece b) {
    return a.view() == b;
  }
  friend bool operator==(folly::StringPiece b, const ManagedStringView& a) {
    return a.view() == b;
  }
  friend bool operator==(
      const ManagedStringView& a,
      const ManagedStringView& b) {
    return a.view() == b.view();
  }
  friend bool operator!=(const ManagedStringView& a, folly::StringPiece b) {
    return a.view() != b;
  }
  friend bool operator!=(folly::StringPiece b, const ManagedStringView& a) {
    return a.view() != b;
  }
  friend bool operator!=(
      const ManagedStringView& a,
      const ManagedStringView& b) {
    return a.view() != b.view();
  }
  friend bool operator<(const ManagedStringView& a, folly::StringPiece b) {
    return a.view() < b;
  }
  friend bool operator<(folly::StringPiece b, const ManagedStringView& a) {
    return a.view() > b;
  }
  friend bool operator<(
      const ManagedStringView& a,
      const ManagedStringView& b) {
    return a.view() < b.view();
  }

 protected:
  Storage str_;
};

// Separate adaptor to work with FieldRef without making the main type
// convertible to everything.
class ManagedStringViewWithConversions : public ManagedStringView {
 public:
  using value_type = char;
  explicit ManagedStringViewWithConversions(ManagedStringView self)
      : ManagedStringView(std::move(self)) {}
  /* implicit */ ManagedStringViewWithConversions(folly::StringPiece str)
      : ManagedStringView(str) {}
  /* implicit */ ManagedStringViewWithConversions(const std::string& str)
      : ManagedStringView(str) {}
  /* implicit */ ManagedStringViewWithConversions(std::string&& str)
      : ManagedStringView(std::move(str)) {}
  /* implicit */ ManagedStringViewWithConversions(const char* str)
      : ManagedStringView(str) {}
  ManagedStringViewWithConversions() = default;

  /* implicit */ operator folly::StringPiece() const {
    return view();
  }
  void clear() {
    *this = ManagedStringViewWithConversions("");
  }
  void reserve(size_t n) {
    folly::variant_match(
        str_,
        [](folly::StringPiece) {
          folly::throw_exception<std::runtime_error>("Can't reserve in view");
        },
        [=](std::string& str) { str.reserve(n); });
  }
  void append(const char* data, size_t n) {
    folly::variant_match(
        str_,
        [](folly::StringPiece) {
          folly::throw_exception<std::runtime_error>("Can't append to view");
        },
        [=](std::string& str) { str.append(data, n); });
  }
  ManagedStringView& operator+=(folly::StringPiece in) {
    folly::variant_match(
        str_,
        [](folly::StringPiece) {
          folly::throw_exception<std::runtime_error>("Can't append to view");
        },
        [=](std::string& str) { str.append(in.data(), in.size()); });
    return *this;
  }
  ManagedStringView& operator+=(unsigned char in) {
    folly::variant_match(
        str_,
        [](folly::StringPiece) {
          folly::throw_exception<std::runtime_error>("Can't append to view");
        },
        [=](std::string& str) { str += in; });
    return *this;
  }
};

inline folly::Expected<folly::StringPiece, folly::ConversionCode> parseTo(
    folly::StringPiece in,
    ManagedStringViewWithConversions& out) noexcept {
  out = ManagedStringViewWithConversions(in);
  return folly::StringPiece{in.end(), in.end()};
}

} // namespace apache::thrift
