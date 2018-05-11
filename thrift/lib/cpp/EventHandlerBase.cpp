/*
 * Copyright 2014-present Facebook, Inc.
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

#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/async/TAsyncChannel.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <algorithm>

using std::vector;
using std::remove;
using std::shared_ptr;

namespace apache { namespace thrift {

std::shared_ptr<server::TServerObserverFactory> observerFactory_(nullptr);

TProcessorBase::TProcessorBase() {
  concurrency::RWGuard lock(getRWMutex(), concurrency::RW_READ);
  for (auto factory: getFactories()) {
    auto handler = factory->getEventHandler();
    if (handler) {
      addEventHandler(handler);
    }
  }
}

void TProcessorBase::addProcessorEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory) {
  concurrency::RWGuard lock(getRWMutex(), concurrency::RW_WRITE);
  assert(find(getFactories().begin(),
              getFactories().end(),
              factory) ==
         getFactories().end());
  getFactories().push_back(factory);
}

void TProcessorBase::removeProcessorEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory) {
  concurrency::RWGuard lock(getRWMutex(), concurrency::RW_WRITE);
  assert(find(getFactories().begin(),
              getFactories().end(),
              factory) !=
         getFactories().end());
  getFactories().erase(
      remove(getFactories().begin(),
             getFactories().end(),
             factory),
      getFactories().end());
}

concurrency::ReadWriteMutex& TProcessorBase::getRWMutex() {
  static concurrency::ReadWriteMutex* mutex = new concurrency::ReadWriteMutex();
  return *mutex;
}

vector<shared_ptr<TProcessorEventHandlerFactory>>&
TProcessorBase::getFactories() {
  static vector<shared_ptr<TProcessorEventHandlerFactory>>* factories =
    new vector<shared_ptr<TProcessorEventHandlerFactory>>();
  return *factories;
}

TClientBase::TClientBase() {
  // Automatically ask all registered factories to produce an event
  // handler, and attach the handlers
  concurrency::RWGuard lock(getRWMutex(), concurrency::RW_READ);
  for (auto factory: getFactories()) {
    auto handler = factory->getEventHandler();
    if (handler) {
      addEventHandler(handler);
    }
  }
}

void TClientBase::addClientEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory) {
  concurrency::RWGuard lock(getRWMutex(), concurrency::RW_WRITE);
  assert(find(getFactories().begin(),
              getFactories().end(),
              factory) ==
         getFactories().end());
  getFactories().push_back(factory);
}

void TClientBase::removeClientEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory) {
  concurrency::RWGuard lock(getRWMutex(), concurrency::RW_WRITE);
  assert(find(getFactories().begin(),
              getFactories().end(),
              factory) !=
         getFactories().end());
  getFactories().erase(
      remove(getFactories().begin(),
             getFactories().end(),
             factory),
      getFactories().end());
}

concurrency::ReadWriteMutex& TClientBase::getRWMutex() {
  static concurrency::ReadWriteMutex* mutex = new concurrency::ReadWriteMutex();
  return *mutex;
}
vector<shared_ptr<TProcessorEventHandlerFactory>>&
TClientBase::getFactories() {
  static vector<shared_ptr<TProcessorEventHandlerFactory>>* factories =
    new vector<shared_ptr<TProcessorEventHandlerFactory>>();
  return *factories;
}

}} // apache::thrift
