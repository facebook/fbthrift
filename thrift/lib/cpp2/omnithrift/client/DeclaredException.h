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

#include <optional>
#include <string>

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

namespace apache {
namespace thrift {
namespace omniclient {

/**
 * Wraps a function-defined exception thrown when calling a function via an
 * OmniClient (e.g. via `semifuture_send()`).
 */
class DeclaredException : public apache::thrift::TException {
 public:
  DeclaredException(
      apache::thrift::ProtocolType protocol,
      int16_t id,
      std::string&& serializedException,
      std::optional<std::string> exceptionName = std::nullopt)
      : protocol_(protocol),
        id_(id),
        serialized_(serializedException),
        exceptionName_(exceptionName) {
    if (exceptionName) {
      message_ =
          folly::to<std::string>(*exceptionName, ": ", serializedException);
    } else {
      message_ = "UnknownException";
    }
  }

  virtual ~DeclaredException() throw() override {}

  /**
   * Return the ID of the exception defined by the function.
   *
   * For example, in the following Thrift function definition,
   * <code>
   * list<Animal> queryAnimals() throws (1: ZooServiceException e);
   * </code>
   * the `ZooServiceException` exception will have an ID of 1.
   */
  int16_t getId() const { return id_; }

  /**
   * @returns The name of the declared exception, if known.
   */
  const std::optional<std::string>& getExceptionName() const {
    return exceptionName_;
  }

  /**
   * Get the serialized exception.
   */
  const std::string& getSerializedException() const { return serialized_; }

  /**
   * The protocol the exception is serialized in.
   */
  apache::thrift::ProtocolType getProtocol() const { return protocol_; }

  const char* what() const throw() override { return message_.c_str(); }

 private:
  apache::thrift::ProtocolType protocol_;
  int16_t id_;
  std::string message_;
  std::string serialized_;
  std::optional<std::string> exceptionName_;
};

} // namespace omniclient
} // namespace thrift
} // namespace apache
