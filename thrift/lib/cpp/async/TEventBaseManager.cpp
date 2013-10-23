/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "thrift/lib/cpp/async/TEventBaseManager.h"

#include "thrift/lib/cpp/async/TEventBase.h"

namespace apache { namespace thrift { namespace async {

std::atomic<TEventBaseManager*> globalManager(nullptr);

TEventBaseManager* TEventBaseManager::get() {
  TEventBaseManager* mgr = globalManager;
  if (mgr) {
    return mgr;
  }

  TEventBaseManager* new_mgr = new TEventBaseManager;
  bool exchanged = globalManager.compare_exchange_strong(mgr, new_mgr);
  if (!exchanged) {
    delete new_mgr;
    return mgr;
  } else {
    return new_mgr;
  }

}

/*
 * TEventBaseManager methods
 */

void TEventBaseManager::setEventBase(TEventBase *eventBase,
                                     bool takeOwnership) {
  EventBaseInfo *info = localStore_.get();
  if (info != nullptr) {
    throw TLibraryException("TEventBaseManager: cannot set a new TEventBase "
                            "for this thread when one already exists");
  }

  info = new EventBaseInfo(eventBase, takeOwnership);
  localStore_.reset(info);
  this->trackEventBase(eventBase);
}

void TEventBaseManager::clearEventBase() {
  EventBaseInfo *info = localStore_.get();
  if (info != nullptr) {
    this->untrackEventBase(info->eventBase);
    this->localStore_.reset(nullptr);
  }
}

// XXX should this really be "const"?
TEventBase * TEventBaseManager::getEventBase() const {
  // have one?
  auto *info = localStore_.get();
  if (! info) {
    info = new EventBaseInfo();
    localStore_.reset(info);

    if (observer_) {
      info->eventBase->setObserver(observer_);
    }

    // start tracking the event base
    // XXX
    // note: ugly cast because this does something mutable
    // even though this method is defined as "const".
    // Simply removing the const causes trouble all over fbcode;
    // lots of services build a const TEventBaseManager and errors
    // abound when we make this non-const.
    (const_cast<TEventBaseManager *>(this))->trackEventBase(info->eventBase);
  }

  return info->eventBase;
}

}}} // apache::thrift::async
