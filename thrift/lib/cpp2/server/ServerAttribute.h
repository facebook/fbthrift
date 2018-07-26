/*
 * Copyright 2004-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/Optional.h>

namespace apache {
namespace thrift {

// source of a server's attribute, precedence takes place in descending order
// (APP will override CONF). see comment on ServerAttribute to learn more
enum class AttributeSource : uint32_t {
  OVERRIDE, // when set directly in application code
  BASELINE, // e.g., may come from external configuration mechanism
};

/*
 * ServerAttribute helps to keep track of who set what value on this
 * attribute. there can be two sources for now. One is from explicit
 * application override, the other may from configuration mechanism. The
 * former one will have higher precedence than the latter one.
 */
template <class T>
class ServerAttribute {
 public:
  explicit ServerAttribute(T value)
      : default_(std::move(value)), deduced_(default_) {}

  template <class U>
  void set(U value, AttributeSource source) {
    choose(source) = value;
    merge();
  }

  void unset(AttributeSource source) {
    choose(source).reset();
    merge();
  }

  T const& get() const {
    return deduced_;
  }

 private:
  folly::Optional<T>& choose(AttributeSource source) {
    if (source == AttributeSource::OVERRIDE) {
      return fromOverride_;
    } else {
      return fromBaseline_;
    }
  }

  void merge() {
    deduced_ = fromOverride_ ? *fromOverride_
                             : fromBaseline_ ? *fromBaseline_ : default_;
  }

  folly::Optional<T> fromBaseline_;
  folly::Optional<T> fromOverride_;
  T default_;
  std::reference_wrapper<T> deduced_;
};

} // namespace thrift
} // namespace apache
