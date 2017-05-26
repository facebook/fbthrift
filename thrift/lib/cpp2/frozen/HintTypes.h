/*
 * Copyright 2014 Facebook, Inc.
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

#include <utility>
#include <vector>

#include <thrift/lib/cpp2/frozen/Traits.h>

namespace apache {
namespace thrift {
namespace frozen {

/*
 * For representing sequences of unpacked integral types.
 *
 * Use this in Thrift IDL like:
 *
 *   cpp_include "thrift/lib/cpp2/frozen/HintTypes.h"
 *
 *   struct MyStruct {
 *     7: list<i32>
 *        (cpp.template = "apache::thrift::frozen::VectorUnpacked")
 *        ids,
 *   }
 */
template <class T>
class VectorUnpacked : public std::vector<T> {
  static_assert(
      std::is_arithmetic<T>::value || std::is_enum<T>::value,
      "Unpacked storage is only available for simple item types");
  using std::vector<T>::vector;
};
}
}
}
THRIFT_DECLARE_TRAIT_TEMPLATE(IsString, apache::thrift::frozen::VectorUnpacked)
