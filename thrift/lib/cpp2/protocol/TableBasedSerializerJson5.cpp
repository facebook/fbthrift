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

// Separate from TableBasedSerializer.cpp, which every Thrift binary links, so
// that only JSON5 consumers pay for these.

// Including the detail headers rather than Json5Protocol.h: that facade depends
// on this target, not the reverse.
// NOLINTBEGIN(facebook-unused-include-check) used by the explicit template
// instantiations below
#include <thrift/lib/cpp2/protocol/TableBasedSerializerImpl.h>
#include <thrift/lib/cpp2/protocol/detail/Json5ProtocolReader.h>
#include <thrift/lib/cpp2/protocol/detail/Json5ProtocolWriter.h>
// NOLINTEND(facebook-unused-include-check)

namespace apache::thrift::detail {

template void read<json5::detail::Json5ProtocolReader>(
    json5::detail::Json5ProtocolReader* iprot,
    const StructInfo& structInfo,
    void* object);
template size_t write<json5::detail::Json5ProtocolWriter>(
    json5::detail::Json5ProtocolWriter* iprot,
    const StructInfo& structInfo,
    const void* object);

} // namespace apache::thrift::detail
