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
#pragma once

#include <folly/ThreadLocal.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>

namespace apache { namespace thrift {

/*
 * RequestEventBaseCache manages objects of type T that are dependent on the
 * current request's EventBase on a thread local basis.
 *
 * Provide a factory that creates T objects given an EventBase, and get() will
 * lazily create T objects based on the EventBase associated with the request
 * which is active at the time of the get() call. These are stored thread
 * locally - for a given pair of event base and calling thread there will only
 * be one T object created.
 *
 * The factory should ideally behave well when passed a null EventBase, i.e.
 * when no EventBase is in the RequestContext. For instance, you might hook in
 * mock T objects for tests in that case.
 *
 * The primary use case is for managing objects that need to do async IO on an
 * event base (e.g. servicerouter/thrift clients, memcache/tao clients, etc)
 * that can be used outside the IO thread without much hassle. For instance,
 * you could use this to manage Thrift clients that are only ever called from
 * within ThreadManager threads without the calling thread needing to know
 * anything about the IO threads that the clients will do their work on.
 */
template <class T>
class RequestEventBaseCache {
 public:
  typedef std::function<std::shared_ptr<T>(folly::EventBase*)> TFactory;

  RequestEventBaseCache() = default;
  explicit RequestEventBaseCache(TFactory factory)
    : factory_(std::move(factory)) {}

  std::shared_ptr<T> get() {
    CHECK(factory_);
    auto eb = folly::RequestEventBase::get();
    if (!eb) {
      return factory_(nullptr);
    }
    auto it = cache_->find(eb);
    if (it == cache_->end()) {
      auto p = cache_->insert(std::make_pair(eb, factory_(eb)));
      it = p.first;
    }
    return it->second;
  };

  void setFactory(TFactory factory) {
    factory_ = std::move(factory);
  }

 private:
  folly::ThreadLocal<std::map<folly::EventBase*, std::shared_ptr<T>>> cache_;
  TFactory factory_;
};

}} // apache::thrift
