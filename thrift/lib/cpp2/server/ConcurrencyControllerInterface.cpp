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

#include <atomic>
#include <mutex>
#include <folly/Indestructible.h>
#include <folly/Synchronized.h>
#include <thrift/lib/cpp2/server/ConcurrencyControllerInterface.h>

namespace apache::thrift {

using Observer = ConcurrencyControllerInterface::Observer;

ConcurrencyControllerInterface::~ConcurrencyControllerInterface() {}

void ConcurrencyControllerInterface::onRequestFinished(UserData) {
  LOG(FATAL) << "Unimplemented onRequestFinished called";
}

auto& getObserverStorage() {
  class Storage {
   public:
    void set(std::shared_ptr<Observer> o) {
      static std::once_flag observerSetFlag;
      std::call_once(observerSetFlag, [&] {
        instance_ = std::move(o);
        isset_.store(true, std::memory_order_release);
      });
    }
    Observer* get() {
      if (!isset_.load(std::memory_order_acquire)) {
        return {};
      }
      return instance_.get();
    }

   private:
    std::atomic<bool> isset_{false};
    std::shared_ptr<Observer> instance_;
  };

  static auto observer = folly::Indestructible<Storage>();
  return *observer;
}

Observer* ConcurrencyControllerInterface::getGlobalObserver() {
  return getObserverStorage().get();
}

void ConcurrencyControllerInterface::setGlobalObserver(
    std::shared_ptr<Observer> observer) {
  getObserverStorage().set(std::move(observer));
}

} // namespace apache::thrift
