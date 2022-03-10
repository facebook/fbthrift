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

#include <memory>
#include <string>

#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_const_value.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_struct.h>

namespace apache::thrift::compiler::gen::cpp {

struct structured_annotation_builder {
  t_program* program;
  t_struct type;

  explicit structured_annotation_builder(
      t_program* p, const std::string& name, const std::string& uri)
      : program(p), type(p, name) {
    type.set_uri(uri);
  }

  static std::unique_ptr<t_const_value> make_string(const char* value) {
    auto name = std::make_unique<t_const_value>();
    name->set_string(value);
    return name;
  }
};

struct adapter_builder : private structured_annotation_builder {
  explicit adapter_builder(t_program* p)
      : structured_annotation_builder(
            p, "Adapter", "facebook.com/thrift/annotation/cpp/Adapter") {}

  std::unique_ptr<t_const> make() & {
    auto map = std::make_unique<t_const_value>();
    map->set_map();
    map->add_map(make_string("name"), make_string("MyAdapter"));
    map->set_ttype(t_type_ref::from_ptr(&type));
    return std::make_unique<t_const>(program, &type, "", std::move(map));
  }
};

struct box_builder : private structured_annotation_builder {
  explicit box_builder(t_program* p)
      : structured_annotation_builder(
            p, "Box", "facebook.com/thrift/annotation/thrift/Box") {}

  std::unique_ptr<t_const> make() & {
    auto map = std::make_unique<t_const_value>();
    return std::make_unique<t_const>(program, &type, "", std::move(map));
  }
};

} // namespace apache::thrift::compiler::gen::cpp
