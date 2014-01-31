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

#include "thrift/lib/cpp2/security/SecurityKillSwitch.h"
#include <gflags/gflags.h>
#include <sys/stat.h>
#include <time.h>

using namespace std;
using namespace apache::thrift;

// DO NOT modify this flag from your application
DEFINE_string(
  security_kill_switch_file,
  "/var/security_kill_switch/enabled",
  "A file, which when present acts as a kill switch and disables security \
  everywhere. This includes thrift security and acl checking both.");

// Time in seconds after kill switch expires
static const time_t killSwitchExpired = 86400;

namespace apache { namespace thrift {

bool isSecurityKillSwitchEnabled() {
  struct stat info;
  int ret = stat(FLAGS_security_kill_switch_file.c_str(), &info);
  if (ret == 0 && time(nullptr) - info.st_mtime < killSwitchExpired) {
    return true;
  }
  return false;
}

}} // apache::thrift
