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

namespace fbthrift {
#if FOLLY_HAVE_WEAK_SYMBOLS
FOLLY_ATTR_WEAK std::unique_ptr<apache::thrift::LoggingEventRegistry>
makeLoggingEventRegistry();
#else
constexpr std::unique_ptr<apache::thrift::LoggingEventRegistry> (
    *makeLoggingEventRegistry)() = nullptr;
#endif
} // namespace fbthrift

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

std::unique_ptr<LoggingEventRegistry> makeRegistry() {
  if (fbthrift::makeLoggingEventRegistry) {
    return fbthrift::makeLoggingEventRegistry();
  }
  return std::make_unique<DefaultLoggingEventRegistry>();
}

class Registry {
 public:
  Registry() : reg_(makeRegistry()) {}

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

} // namespace thrift
} // namespace apache
