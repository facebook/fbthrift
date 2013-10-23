/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_SCHEMA_INL_H_
#define THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_SCHEMA_INL_H_

#ifndef THRIFT_INCLUDE_SCHEMA_INL
#error This file may only be included from Schema.h
#endif

namespace apache {
namespace thrift {
namespace protocol {
namespace neutronium {

namespace detail {
extern const TType typeToTType[];
}  // namespace detail

inline TType toTType(reflection::Type t) {
  return detail::typeToTType[t];
}

}  // namespace neutronium
}  // namespace protocol
}  // namespace thrift
}  // namespace apache

#endif /* THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_SCHEMA_INL_H_ */
