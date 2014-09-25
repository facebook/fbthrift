/*
 * Copyright 2014 Facebook, Inc.
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
#include <thrift/lib/cpp/util/SocketRetriever.h>
#include <algorithm>

using std::vector;
using std::remove;
using std::shared_ptr;

namespace apache { namespace thrift {

using apache::thrift::util::SocketRetriever;

vector<shared_ptr<TProcessorEventHandlerFactory>>*
  TClientBase::registeredHandlerFactoriesPtr_ =
          new vector<shared_ptr<TProcessorEventHandlerFactory>>();
vector<shared_ptr<TProcessorEventHandlerFactory>>*
  TProcessorBase::registeredHandlerFactoriesPtr_ =
          new vector<shared_ptr<TProcessorEventHandlerFactory>>();
std::shared_ptr<server::TServerObserverFactory> observerFactory_(nullptr);

concurrency::ReadWriteMutex* TClientBase::handlerFactoriesMutexPtr_ =
        new concurrency::ReadWriteMutex();
concurrency::ReadWriteMutex* TProcessorBase::handlerFactoriesMutexPtr_ =
        new concurrency::ReadWriteMutex();

TProcessorBase::TProcessorBase() {
  concurrency::RWGuard lock(*handlerFactoriesMutexPtr_, concurrency::RW_READ);
  for (auto factory: *registeredHandlerFactoriesPtr_) {
    auto handler = factory->getEventHandler();
    if (handler) {
      addEventHandler(handler);
    }
  }
}

void TProcessorBase::addProcessorEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory) {
  concurrency::RWGuard lock(*handlerFactoriesMutexPtr_, concurrency::RW_WRITE);
  assert(find(registeredHandlerFactoriesPtr_->begin(),
              registeredHandlerFactoriesPtr_->end(),
              factory) ==
         registeredHandlerFactoriesPtr_->end());
  registeredHandlerFactoriesPtr_->push_back(factory);
}

void TProcessorBase::removeProcessorEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory) {
  concurrency::RWGuard lock(*handlerFactoriesMutexPtr_, concurrency::RW_WRITE);
  assert(find(registeredHandlerFactoriesPtr_->begin(),
              registeredHandlerFactoriesPtr_->end(),
              factory) !=
         registeredHandlerFactoriesPtr_->end());
  registeredHandlerFactoriesPtr_->erase(
      remove(registeredHandlerFactoriesPtr_->begin(),
             registeredHandlerFactoriesPtr_->end(),
             factory),
      registeredHandlerFactoriesPtr_->end());
}

TClientBase::TClientBase()
    : s_() {
  // Automatically ask all registered factories to produce an event
  // handler, and attach the handlers
  concurrency::RWGuard lock(*handlerFactoriesMutexPtr_, concurrency::RW_READ);
  for (auto factory: *registeredHandlerFactoriesPtr_) {
    auto handler = factory->getEventHandler();
    if (handler) {
      addEventHandler(handler);
    }
  }
}

void TClientBase::addClientEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory) {
  concurrency::RWGuard lock(*handlerFactoriesMutexPtr_, concurrency::RW_WRITE);
  assert(find(registeredHandlerFactoriesPtr_->begin(),
              registeredHandlerFactoriesPtr_->end(),
              factory) ==
         registeredHandlerFactoriesPtr_->end());
  registeredHandlerFactoriesPtr_->push_back(factory);
}

void TClientBase::removeClientEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory) {
  concurrency::RWGuard lock(*handlerFactoriesMutexPtr_, concurrency::RW_WRITE);
  assert(find(registeredHandlerFactoriesPtr_->begin(),
              registeredHandlerFactoriesPtr_->end(),
              factory) !=
         registeredHandlerFactoriesPtr_->end());
  registeredHandlerFactoriesPtr_->erase(
      remove(registeredHandlerFactoriesPtr_->begin(),
             registeredHandlerFactoriesPtr_->end(),
             factory),
      registeredHandlerFactoriesPtr_->end());
}

TClientBase::ConnContext::ConnContext(
    std::shared_ptr<protocol::TProtocol> inputProtocol,
    std::shared_ptr<protocol::TProtocol> outputProtocol)
    : header_(nullptr)
    , manager_(nullptr) {
  const folly::SocketAddress* address = nullptr;

  if (outputProtocol) {
    auto socket = SocketRetriever::getSocket(outputProtocol);
    if (socket && socket->isOpen()) {
      address = socket->getPeerAddress();
    }
  }

  init(address, inputProtocol, outputProtocol);
}

TClientBase::ConnContext::ConnContext(
    std::shared_ptr<apache::thrift::async::TAsyncChannel> channel,
    std::shared_ptr<protocol::TProtocol> inputProtocol,
    std::shared_ptr<protocol::TProtocol> outputProtocol)
    : header_(nullptr)
    , manager_(nullptr) {

  if (channel) {
    auto transport = channel->getTransport();
    if (transport) {
      folly::SocketAddress address;
      transport->getPeerAddress(&address);
      init(&address, inputProtocol, outputProtocol);
      return;
    }
  }

  init(nullptr, inputProtocol, outputProtocol);

}

void TClientBase::ConnContext::init(
    const folly::SocketAddress* address,
    std::shared_ptr<protocol::TProtocol> inputProtocol,
    std::shared_ptr<protocol::TProtocol> outputProtocol) {
  if (address == nullptr) {
    address_ = nullptr;
  } else {
    internalAddress_ = *address;
    address_ = &internalAddress_;
  }
  inputProtocol_ = inputProtocol;
  outputProtocol_ = outputProtocol;
}

}} // apache::thrift
