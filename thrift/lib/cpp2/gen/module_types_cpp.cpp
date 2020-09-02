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

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

namespace apache {
namespace thrift {
namespace detail {

namespace st {

FOLLY_NOINLINE void translate_field_name(
    folly::StringPiece fname,
    int16_t& fid,
    protocol::TType& ftype,
    translate_field_name_table const& table) noexcept {
  for (size_t i = 0; i < table.size; ++i) {
    if (fname == table.names[i]) {
      fid = table.ids[i];
      ftype = table.types[i];
      break;
    }
  }
}

} // namespace st

} // namespace detail
} // namespace thrift
} // namespace apache
