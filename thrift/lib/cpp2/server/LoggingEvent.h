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

#include <atomic>
#include <memory>
#include <string_view>
#include <utility>

#include <folly/io/async/AsyncTransport.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

#define THRIFT_LOGGING_EVENT(KEY, FETCH_FUNC)                      \
  ([]() -> auto& {                                                 \
    static auto& handler =                                         \
        apache::thrift::getLoggingEventRegistry().FETCH_FUNC(KEY); \
    return handler;                                                \
  }())

#define THRIFT_SERVER_EVENT(NAME) \
  THRIFT_LOGGING_EVENT(#NAME, getServerEventHandler)

#define THRIFT_CONNECTION_EVENT(NAME) \
  THRIFT_LOGGING_EVENT(#NAME, getConnectionEventHandler)

#define THRIFT_APPLICATION_EVENT(NAME) \
  THRIFT_LOGGING_EVENT(#NAME, getApplicationEventHandler)

class ThriftServer;
class Cpp2Worker;

class LoggingEventHandler {
 public:
  virtual ~LoggingEventHandler() {}
};

class ServerEventHandler : public LoggingEventHandler {
 public:
  virtual void log(const ThriftServer&) {}
  virtual ~ServerEventHandler() {}
};

class ConnectionLoggingContext {
 public:
  enum class TransportType {
    HEADER,
    ROCKET,
  };

  explicit ConnectionLoggingContext(
      TransportType transportType,
      const Cpp2Worker& worker,
      const folly::AsyncTransport& transport)
      : transportType_(transportType), worker_(worker), transport_(transport) {}

  const Cpp2Worker& getWorker() const {
    return worker_;
  }
  const folly::AsyncTransport& getTransport() const {
    return transport_;
  }
  void setClientAgent(std::string clientAgent) {
    clientAgent_ = clientAgent;
  }
  void setClientHostId(std::string clientHostId) {
    clientHostId_ = clientHostId;
  }
  void setInterfaceKind(apache::thrift::InterfaceKind kind) {
    interfaceKind_ = kind;
  }
  const std::string& getClientAgent() const {
    return clientAgent_;
  }
  const std::string& getClientHostId() const {
    return clientHostId_;
  }
  TransportType getTransportType() const {
    return transportType_;
  }
  InterfaceKind getInterfaceKind() const {
    return interfaceKind_;
  }

 private:
  TransportType transportType_;
  const Cpp2Worker& worker_;
  const folly::AsyncTransport& transport_;

  std::string clientAgent_;
  std::string clientHostId_;
  apache::thrift::InterfaceKind interfaceKind_;
};

class ConnectionEventHandler : public LoggingEventHandler {
 public:
  virtual void log(const ConnectionLoggingContext&) {}

  virtual ~ConnectionEventHandler() {}
};

class ApplicationEventHandler : public LoggingEventHandler {
 public:
  virtual void log() {}
  virtual ~ApplicationEventHandler() {}
};

class LoggingEventRegistry {
 public:
  virtual ServerEventHandler& getServerEventHandler(
      std::string_view eventKey) const = 0;
  virtual ConnectionEventHandler& getConnectionEventHandler(
      std::string_view eventKey) const = 0;
  virtual ApplicationEventHandler& getApplicationEventHandler(
      std::string_view eventKey) const = 0;
  virtual ~LoggingEventRegistry() {}
};

const LoggingEventRegistry& getLoggingEventRegistry();

void useMockLoggingEventRegistry();

void logSetupConnectionEventsOnce(
    folly::once_flag& flag,
    std::string_view methodName,
    const ConnectionLoggingContext& context);

} // namespace thrift
} // namespace apache
