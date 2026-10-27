/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/DefaultServerFragmenter.h>

namespace apache::thrift::fast_thrift::thrift::server::test {

/** Fails every write, so a test can tell which fragmenter a server uses. */
class RejectingFragmentationHandler : public DefaultServerFragmenter::Handler {
 public:
  using Base = DefaultServerFragmenter::Handler;
  using Base::Base;

  template <typename Context>
  [[nodiscard]] channel_pipeline::Result onWrite(
      Context&, channel_pipeline::TypeErasedBox&&) noexcept {
    return channel_pipeline::Result::Error;
  }
};

/** Test-only fragmenter that selects RejectingFragmentationHandler. */
struct RejectingServerFragmenter {
  using Handler = RejectingFragmentationHandler;
};

} // namespace apache::thrift::fast_thrift::thrift::server::test
