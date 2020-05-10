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

#include <string>

#include <boost/variant.hpp>

#include <folly/Function.h>
#include <folly/Optional.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/transport/THeader.h>

namespace apache {
namespace thrift {

namespace detail {
class VisitorHelper : public boost::static_visitor<> {
 public:
  VisitorHelper& with(folly::Function<void(AppServerException&) const>&& f) {
    aseFunc_ = std::move(f);
    return *this;
  }

  VisitorHelper& with(folly::Function<void(AppClientException&) const>&& f) {
    aceFunc_ = std::move(f);
    return *this;
  }

  void operator()(AppServerException& value) const {
    aseFunc_(value);
  }

  void operator()(AppClientException& value) const {
    aceFunc_(value);
  }

 private:
  folly::Function<void(AppClientException&) const> aceFunc_;
  folly::Function<void(AppServerException&) const> aseFunc_;
};
} // namespace detail
} // namespace thrift
} // namespace apache
