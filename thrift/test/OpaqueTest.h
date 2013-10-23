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
#pragma once

// Implement opaque typedef similar to BOOST_STRONG_TYPEDEF or Boost.Units.
// In real life it would have operators defined that access underlying value in
// a type-safe manner

#define OPAQUE_TYPEDEF_FOR_TEST(RawType, NewType)                           \
struct NewType {                                                            \
  typedef RawType raw_type;                                                 \
  RawType val_;                                                             \
  explicit NewType(const RawType val) : val_(val) {}                        \
  NewType() {}                                                              \
  RawType& __value() { return val_; }                                       \
  const RawType& __value() const { return val_; }                           \
  explicit operator RawType() const { return val_; }                        \
  bool operator==(const NewType & rhs) const { return val_ == rhs.val_; }   \
};

OPAQUE_TYPEDEF_FOR_TEST(double, OpaqueDouble1)
OPAQUE_TYPEDEF_FOR_TEST(double, OpaqueDouble2)
OPAQUE_TYPEDEF_FOR_TEST(int64_t, NonConvertibleId)
