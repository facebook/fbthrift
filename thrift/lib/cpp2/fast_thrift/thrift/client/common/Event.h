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

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>

#include <thrift/lib/cpp2/fast_thrift/transport/WriteCompletion.h>

namespace apache::thrift::fast_thrift::thrift {

/** Graceful-drain signal published by a transport adapter. */
struct ThriftClientCloseConnectionEvent : channel_pipeline::EventTag<> {};

/**
 * `ThriftClientWriteCompleteEvent` reports the completion of one
 * individual request write, relayed up from the rocket pipeline.
 * `requestContext` is a NON-OWNING borrow valid for the duration of the
 * event callback; the owning layer static_casts it to the concrete context
 * type.
 */
struct ThriftClientWriteCompleteEvent
    : channel_pipeline::EventTag<ThriftClientWriteCompleteEvent> {
  void* requestContext;
  apache::thrift::fast_thrift::transport::WriteCompletionStatus status;
};

} // namespace apache::thrift::fast_thrift::thrift
