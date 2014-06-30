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

#include <thrift/lib/cpp2/security/SecurityKillSwitch.h>
#include <gflags/gflags.h>
#include <sys/stat.h>
#include <time.h>

using namespace std;
using namespace apache::thrift;

// DO NOT modify this flag from your application
DEFINE_string(
  thrift_security_kill_switch_file,
  "/var/thrift_security/disable_thrift_security",
  "A file, which when present acts as a kill switch and disables thrift "
  " security everywhere. This DOES NOT disable ACL checking / authorization.");

// Time in seconds after which thrift kill switch expires
static const time_t thriftKillSwitchExpired = 86400;

namespace apache { namespace thrift {

bool isSecurityKillSwitchEnabled() {
  struct stat info;
  int ret = stat(FLAGS_thrift_security_kill_switch_file.c_str(), &info);
  if (ret == 0 && time(nullptr) - info.st_mtime < thriftKillSwitchExpired) {
    return true;
  }
  return false;
}

}} // apache::thrift
