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

#include <memory>

#include <folly/Indestructible.h>

namespace thrift {
namespace py3 {

template <typename T>
std::shared_ptr<T> constant_shared_ptr(const T& x) {
  return std::shared_ptr<T>(std::shared_ptr<T>{}, const_cast<T*>(&x));
}

template <typename T, typename S>
std::shared_ptr<T> reference_shared_ptr(S& owner, const T& ref) {
  return std::shared_ptr<T>(owner, const_cast<T*>(&ref));
}

template <typename T>
const T& default_inst() {
  static const folly::Indestructible<T> inst{};
  return *inst;
}

} // namespace py3
} // namespace thrift
