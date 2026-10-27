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

#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/FragmentCompletionTracker.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/FrameFragmentationHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/RocketServerEventFactory.h>

namespace apache::thrift::fast_thrift::thrift::server {

/** Selects current SRPT with the server's fragment completion tracker. */
struct DefaultServerFragmenter {
  using Handler = frame::write::handler::FrameFragmentationHandlerT<
      frame::write::handler::FragmentCompletionTrackerT<
          rocket::server::RocketServerEventFactory>>;
};

} // namespace apache::thrift::fast_thrift::thrift::server
