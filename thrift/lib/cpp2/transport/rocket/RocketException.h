/*
 * Copyright 2018-present Facebook, Inc.
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

#include <exception>
#include <utility>

#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>

namespace apache {
namespace thrift {
namespace rocket {

class RocketException : public std::exception {
 public:
  explicit RocketException(ErrorCode errorCode)
      : rsocketErrorCode_(errorCode) {}

  RocketException(ErrorCode errorCode, folly::IOBuf&& errorData)
      : rsocketErrorCode_(errorCode), errorData_(std::move(errorData)) {}

  ErrorCode getErrorCode() const noexcept {
    return rsocketErrorCode_;
  }

  const char* what() const noexcept override {
    return "RocketException";
  }

  folly::IOBuf moveErrorData() {
    return std::move(errorData_);
  }

 private:
  ErrorCode rsocketErrorCode_{ErrorCode::RESERVED};
  folly::IOBuf errorData_;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
