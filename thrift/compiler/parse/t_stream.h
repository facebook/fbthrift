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

#ifndef T_STREAM_H
#define T_STREAM_H

/**
 * A stream is a lightweight object type that just wraps another data type.
 *
 */
class t_stream : public t_type {
 public:
  explicit t_stream(t_type* elem_type) :
    elem_type_(elem_type) {}

  t_type* get_elem_type() const {
    return elem_type_;
  }

  bool is_stream() const override { return true; }

  std::string get_full_name() const override {
    return "stream<" + elem_type_->get_full_name() + ">";
  }

  std::string get_impl_full_name() const override {
    return "stream<" + elem_type_->get_impl_full_name() + ">";
  }

  TypeValue get_type_value() const override { return TypeValue::TYPE_STREAM; }

 private:
  t_type* elem_type_;
};

#endif
