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

#include <cstdint>

namespace apache::thrift::fast_thrift::thrift::stream {

/**
 * User-event enum for the established stream sub-pipeline's control plane
 * (fireEvent/onEvent), used to parameterize the pipeline
 * (PipelineBuilder<Head, Tail, Alloc, StreamEvent>).
 *
 * Flow-control readiness signals let InboundCreditHandler keep the credit
 * budget private while still telling an upstream buffer when to stop and start:
 *   - FlowControlPause: the credit budget just reached zero; no more Payloads
 *     may be sent until credit is regranted. Emitted as the exhausting Payload
 *     is forwarded.
 *   - FlowControlResume: credit became available after being exhausted (an
 *     inbound RequestN grant); a paused writer may resume.
 *
 * These are stream-layer policy signals, distinct from transport write
 * backpressure, which flows on the data path as Result::Backpressure.
 */
enum class StreamEvent : std::uint32_t {
  FlowControlPause,
  FlowControlResume,
  Count,
};

} // namespace apache::thrift::fast_thrift::thrift::stream
