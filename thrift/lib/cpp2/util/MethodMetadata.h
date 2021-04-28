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

#include <memory>
#include <string>
#include <string_view>
#include <folly/Range.h>

namespace apache::thrift {

enum class FunctionQualifier {
  None = 0,
  OneWay,
  Idempotent,
  ReadOnly,
};

/*
 * A move-only structure for storing thrift method metadata.
 * Metadata : method name & function qualifier.
 * When created from static data, the structure creates a view of the data
 * When created from temporary data, the structure takes the ownership of
 * the data by creating a dynamic storage for it.
 */
class MethodMetadata {
 public:
  struct Data {
    Data(std::string name, FunctionQualifier qualifier)
        : methodName(std::move(name)), functionQualifier(qualifier) {}
    std::string methodName;
    FunctionQualifier functionQualifier;
  };

  MethodMetadata() = delete;
  MethodMetadata(const MethodMetadata&) = delete;
  MethodMetadata& operator=(const MethodMetadata&) = delete;

  MethodMetadata(MethodMetadata&& that) noexcept
      : isOwning_(std::exchange(that.isOwning_, false)),
        data_(std::exchange(that.data_, nullptr)) {}

  MethodMetadata& operator=(MethodMetadata&& that) noexcept {
    MethodMetadata tmp(std::move(that));
    swap(*this, tmp);
    return *this;
  }

  ~MethodMetadata() noexcept {
    if (isOwning()) {
      delete data_;
    }
  }

  /* implicit */ MethodMetadata(std::string_view methodName)
      : isOwning_(true),
        data_(new Data(std::string(methodName), FunctionQualifier::None)) {}

  /* implicit */ MethodMetadata(std::string&& methodName)
      : isOwning_(true),
        data_(new Data(std::move(methodName), FunctionQualifier::None)) {}

  /* implicit */ MethodMetadata(const std::string& methodName)
      : isOwning_(true), data_(new Data(methodName, FunctionQualifier::None)) {}

  /* implicit */ MethodMetadata(const char* methodName)
      : isOwning_(true), data_(new Data(methodName, FunctionQualifier::None)) {}

  /* implicit */ MethodMetadata(folly::StringPiece methodName)
      : isOwning_(true),
        data_(new Data(std::string(methodName), FunctionQualifier::None)) {}

  static MethodMetadata from_static(Data* mPtr) {
    return MethodMetadata(mPtr, NonOwningTag{});
  }

  std::string name_str() const& { return data_->methodName; }

  std::string name_str() && {
    return isOwning() ? std::move(data_->methodName) : data_->methodName;
  }

  std::string_view name_view() const {
    return std::string_view(data_->methodName);
  }

  FunctionQualifier qualifier() const { return data_->functionQualifier; }

  bool isOwning() const { return isOwning_; }

  MethodMetadata getCopy() const {
    if (isOwning()) {
      return MethodMetadata(data_, OwningTag{});
    } else {
      return MethodMetadata(data_, NonOwningTag{});
    }
  }

 private:
  struct NonOwningTag {};
  struct OwningTag {};

  MethodMetadata(Data* mPtr, OwningTag)
      : isOwning_(true), data_(new Data(*mPtr)) {}

  MethodMetadata(Data* mPtr, NonOwningTag) : isOwning_(false), data_(mPtr) {}

  friend void swap(MethodMetadata& left, MethodMetadata& right) noexcept {
    std::swap(left.isOwning_, right.isOwning_);
    std::swap(left.data_, right.data_);
  }

  bool isOwning_{false};
  Data* data_{nullptr};
};

} // namespace apache::thrift
