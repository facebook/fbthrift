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

#ifndef T_ENUM_VALUE_H
#define T_ENUM_VALUE_H

#include <string>
#include "thrift/compiler/parse/t_doc.h"

/**
 * A constant. These are used inside of enum definitions. Constants are just
 * symbol identifiers that may or may not have an explicit value associated
 * with them.
 *
 */
class t_enum_value : public t_doc {
 public:
  t_enum_value(std::string name, int32_t value) :
    name_(name),
    value_(value) {}

  ~t_enum_value() {}

  const std::string& get_name() const {
    return name_;
  }

  // Open source thrift supports enums with auto-incrementing values. This is
  // a stub, replace it if we ever implement this feature in facebook.
  bool has_value() {
    return true;
  }

  int32_t get_value() const {
    return value_;
  }

 private:
  std::string name_;
  int32_t value_;
};

#endif
