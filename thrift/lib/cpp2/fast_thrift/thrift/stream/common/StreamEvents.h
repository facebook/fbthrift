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

namespace apache::thrift::fast_thrift::thrift::stream {

/**
 * Type-based flow-control events for the established stream sub-pipeline's
 * control plane (fireEvent / on<E>). These readiness signals let
 * InboundCreditHandler keep the credit budget private while still telling an
 * upstream buffer when to stop and start. Both are pure signals — they carry no
 * payload (EventTag<>).
 *
 * These are stream-layer policy signals, distinct from transport write
 * backpressure, which flows on the data path as Result::Backpressure.
 */

/**
 * The credit budget just reached zero; no more Payloads may be sent until
 * credit is regranted. Emitted as the exhausting Payload is forwarded.
 */
struct FlowControlPauseEvent : channel_pipeline::EventTag<> {};

/**
 * Credit became available after being exhausted (an inbound RequestN grant); a
 * writer that paused upstream may resume.
 */
struct FlowControlResumeEvent : channel_pipeline::EventTag<> {};

} // namespace apache::thrift::fast_thrift::thrift::stream
