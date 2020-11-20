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

#include <thrift/lib/cpp2/server/LoggingEvent.h>

#include <folly/Portability.h>
#include <folly/Singleton.h>
#include <folly/Synchronized.h>
#include <folly/io/async/AsyncSSLSocket.h>
#include <thrift/lib/cpp2/PluggableFunction.h>

namespace apache {
namespace thrift {
namespace {

class DefaultLoggingEventRegistry : public LoggingEventRegistry {
 public:
  ServerEventHandler& getServerEventHandler(std::string_view) const override {
    static auto* handler = new ServerEventHandler();
    return *handler;
  }
  ConnectionEventHandler& getConnectionEventHandler(
      std::string_view) const override {
    static auto* handler = new ConnectionEventHandler();
    return *handler;
  }
  ApplicationEventHandler& getApplicationEventHandler(
      std::string_view) const override {
    static auto* handler = new ApplicationEventHandler();
    return *handler;
  }
};

THRIFT_PLUGGABLE_FUNC_REGISTER(
    std::unique_ptr<apache::thrift::LoggingEventRegistry>,
    makeLoggingEventRegistry) {
  return std::make_unique<DefaultLoggingEventRegistry>();
}

class Registry {
 public:
  Registry() : reg_(THRIFT_PLUGGABLE_FUNC(makeLoggingEventRegistry)()) {}

  LoggingEventRegistry& getRegistry() const {
    return *reg_.get();
  }

 private:
  std::unique_ptr<LoggingEventRegistry> reg_;
};

struct RegistryTag {};
folly::LeakySingleton<Registry, RegistryTag> registryStorage;

} // namespace

void useMockLoggingEventRegistry() {
  folly::LeakySingleton<Registry, RegistryTag>::make_mock();
}

const LoggingEventRegistry& getLoggingEventRegistry() {
  return registryStorage.get().getRegistry();
}

void logSetupConnectionEventsOnce(
    folly::once_flag& flag,
    std::string_view methodName,
    const Cpp2Worker& worker,
    const folly::AsyncTransport& transport) {
  static_cast<void>(folly::try_call_once(flag, [&]() noexcept {
    if (methodName == "getCounters" || methodName == "getStatus" ||
        methodName == "getRegexCounters") {
      return false;
    }
    try {
      if (!transport.getUnderlyingTransport<folly::AsyncSSLSocket>()) {
        THRIFT_CONNECTION_EVENT(non_tls).log(worker, transport);
      }
    } catch (...) {
      LOG(ERROR)
          << "Exception thrown during Thrift server connection events logging: "
          << folly::exceptionStr(std::current_exception());
    }
    return true;
  }));
}

} // namespace thrift
} // namespace apache
