/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#pragma once

#include <chrono>
#include <string>

namespace apache::thrift::test {

struct AdaptTestMsAdapter {
  static std::chrono::milliseconds fromThrift(int64_t ms) {
    return std::chrono::milliseconds{ms};
  }

  static int64_t toThrift(std::chrono::milliseconds duration) {
    return duration.count();
  }
};

struct Num {
  int64_t val;
};
struct String {
  std::string val;
};

struct OverloadedAdatper {
  static Num fromThrift(int64_t val) {
    return Num{val};
  }
  static String fromThrift(std::string&& val) {
    return String{std::move(val)};
  }

  static int64_t toThrift(const Num& num) {
    return num.val;
  }
  static const std::string& toThrift(const String& str) {
    return str.val;
  }
};

} // namespace apache::thrift::test
