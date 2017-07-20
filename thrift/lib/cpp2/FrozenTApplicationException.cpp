/*
 * Copyright 2004-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/FrozenTApplicationException.h>

namespace apache {
namespace thrift {
namespace frozen {

FROZEN_CTOR(
    ::apache::thrift::TApplicationException,
    FROZEN_CTOR_FIELD(message, 1) FROZEN_CTOR_FIELD(type, 2))
FROZEN_MAXIMIZE(
    ::apache::thrift::TApplicationException,
    FROZEN_MAXIMIZE_FIELD(message) FROZEN_MAXIMIZE_FIELD(type))
FROZEN_DEBUG(
    ::apache::thrift::TApplicationException,
    FROZEN_DEBUG_FIELD(message) FROZEN_DEBUG_FIELD(type))
FROZEN_CLEAR(
    ::apache::thrift::TApplicationException,
    FROZEN_CLEAR_FIELD(message) FROZEN_CLEAR_FIELD(type))

FieldPosition Layout<TApplicationException>::layout(
    LayoutRoot& root,
    const T& x,
    LayoutPosition self) {
  FieldPosition pos = startFieldPosition();
  pos = root.layoutField(self, pos, messageField, x.getMessage());
  pos =
      root.layoutField(self, pos, typeField, static_cast<int32_t>(x.getType()));
  return pos;
}

void Layout<TApplicationException>::freeze(
    FreezeRoot& root,
    const T& x,
    FreezePosition self) const {
  root.freezeField(self, messageField, x.getMessage());
  int32_t type = static_cast<int32_t>(x.getType());
  root.freezeField(self, typeField, type);
}

void Layout<TApplicationException>::thaw(ViewPosition self, T& out) const {
  std::string msg;
  int32_t type;
  thawField(self, messageField, msg);
  thawField(self, typeField, type);
  out.setMessage(std::move(msg));
  out.setType(
      static_cast<TApplicationException::TApplicationExceptionType>(type));
}
}
}
} // apache::thrift::frozen
