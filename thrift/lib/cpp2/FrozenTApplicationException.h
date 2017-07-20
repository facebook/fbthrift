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
#pragma once

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/frozen/Frozen.h>

namespace apache {
namespace thrift {
namespace frozen {

FROZEN_TYPE(
  ::apache::thrift::TApplicationException,
  FROZEN_FIELD(message, 1, std::string)
  FROZEN_FIELD(type, 2, int32_t)
  FROZEN_VIEW(
    FROZEN_VIEW_FIELD(message, std::string)
    FROZEN_VIEW_FIELD(type, int32_t))
  FROZEN_SAVE_INLINE(
    FROZEN_SAVE_FIELD(message)
    FROZEN_SAVE_FIELD(type))
  FROZEN_LOAD_INLINE(
    FROZEN_LOAD_FIELD(message, 1)
    FROZEN_LOAD_FIELD(type, 2)));
}
}
} // apache::thrift::frozen
