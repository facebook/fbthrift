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

#ifndef SECURITY_LOGGER_H
#define SECURITY_LOGGER_H

#include <glog/logging.h>
#include <string>
#include <vector>

#include <folly/String.h>

namespace apache { namespace thrift {

/**
 * Basic dummy logger for security-related things
 */
class SecurityLogger {
 public:
  SecurityLogger() {}
  virtual ~SecurityLogger() {}

  enum TracingOptions {
    NONE = 0,
    START = 1,
    END = 2
  };

  /**
   * Log key, optional data.
   */
  virtual void log(const std::string& key,
      const std::vector<std::string>& data,
      TracingOptions tracing) {
    if (!VLOG_IS_ON(9)) {
      return;
    }

    std::string data_str = folly::join(", ", data);
    std::string key_suffix;
    if (tracing == START) {
      key_suffix = "_start";
    } else if (tracing == END) {
      key_suffix = "_end";
    }

    VLOG(9) << key << key_suffix
            << (data_str.empty() ? "" : ": ") << data_str;
  }

  virtual void logValue(const std::string& key, int64_t value) {
    SecurityLogger::log(key, {folly::to<std::string>(value)}, NONE);
  }

  /**
   * Log function overloads.
   */
  void log(const std::string& key, const std::string& data) {
    log(key, {data}, NONE);
  }
  void log(const std::string& key) {
    log(key, {}, NONE);
  }
  void log(const std::string& key,
      const std::initializer_list<std::string>& data) {
    log(key, data, NONE);
  }

  void logStart(const std::string& key) {
    log(key, {}, START);
  }
  void logStart(const std::string& key, const std::string& data) {
    log(key, {data}, START);
  }
  void logEnd(const std::string& key) {
    log(key, {}, END);
  }
  void logEnd(const std::string& key, const std::string& data) {
    log(key, {data}, END);
  }

  // Only to be used for testing
  virtual void moveMockTime() {}
};

}}  // apache::thrift

#endif
