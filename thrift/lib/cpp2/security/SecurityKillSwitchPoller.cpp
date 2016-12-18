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
#include <folly/Singleton.h>

#include <gflags/gflags.h>

DEFINE_string(
    thrift_security_tls_kill_switch_file,
    "/var/thrift_security/disable_thrift_security_tls",
    "A file, which when present on a client, disables the use of TLS to "
    "endpoints, and on servers will downgrade a 'required' SSL policy to "
    "'permitted'");

namespace apache { namespace thrift {

static constexpr std::chrono::seconds kThriftKillSwitchExpired =
    std::chrono::seconds(86400);
static constexpr std::chrono::seconds kPollInterval =
    std::chrono::seconds(1);

using namespace std;
using namespace folly;

SecurityKillSwitchPoller::SecurityKillSwitchPoller()
    : SecurityKillSwitchPoller(true) {}

SecurityKillSwitchPoller::SecurityKillSwitchPoller(bool autostart)
    : poller_(folly::make_unique<FilePoller>(kPollInterval)) {
  auto yCob = [this]() { switchEnabled_ = true; };
  auto nCob = [this]() { switchEnabled_ = false; };
  auto condition = FilePoller::fileTouchedWithinCond(kThriftKillSwitchExpired);
  if (autostart) {
    poller_->addFileToTrack(
        FLAGS_thrift_security_tls_kill_switch_file, yCob, nCob, condition);
  }
}

void SecurityKillSwitchPoller::addFileToTrack(const std::string& filePath,
                                              FilePoller::Cob yCob,
                                              FilePoller::Cob nCob,
                                              FilePoller::Condition cond) {
  poller_->addFileToTrack(filePath, yCob, nCob, cond);
}

SecurityKillSwitchPoller::~SecurityKillSwitchPoller() {
  poller_->stop();
}
}}
