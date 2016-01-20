/*
 * Copyright 2016 Facebook, Inc.
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

#include <thrift/lib/cpp2/security/SecurityKillSwitchPoller.h>
#include <thrift/lib/cpp2/security/SecurityKillSwitch.h>
#include <folly/Singleton.h>

namespace apache { namespace thrift {

namespace {
auto defaultInstance = folly::Singleton<SecurityKillSwitchPoller>()
  .shouldEagerInit();
}

using namespace std;
using namespace folly;

SecurityKillSwitchPoller::SecurityKillSwitchPoller()
    : SecurityKillSwitchPoller(chrono::milliseconds(1000),
                               apache::thrift::isTlsKillSwitchEnabled) {}

SecurityKillSwitchPoller::SecurityKillSwitchPoller(
  const chrono::milliseconds& timeout, function<bool()> pollFunc)
    : timeout_(timeout), pollFunc_(pollFunc), thread_(true) {
  setup();
}

SecurityKillSwitchPoller::~SecurityKillSwitchPoller() {
  thread_.stop();
}

void SecurityKillSwitchPoller::setup() {
  auto evb = thread_.getEventBase();
  evb->runInEventBaseThread([this, evb]() {
      attachEventBase(evb);
      updateSwitchState();
      scheduleTimeout(timeout_);
  });
}

void SecurityKillSwitchPoller::updateSwitchState() noexcept {
  try {
    switchEnabled_ = pollFunc_();
  } catch (...) {
    VLOG(5) << "Error checking kill switch.  Disabling.";
    switchEnabled_ = false;
  }
}

void SecurityKillSwitchPoller::timeoutExpired() noexcept {
  updateSwitchState();
  scheduleTimeout(timeout_);
}

}}
