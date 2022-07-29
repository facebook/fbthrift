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

#pragma once

#include <map>
#include <string>
#include <vector>

#include <thrift/compiler/generate/t_java_deprecated_generator.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Java code generator for Android.
 */
class t_android_generator : public t_java_deprecated_generator {
 public:
  t_android_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& options)
      : t_java_deprecated_generator(program, std::move(context), options) {
    generate_field_metadata_ = false;
    generate_immutable_structs_ = true;
    generate_boxed_primitive = true;
    generate_builder = false;

    out_dir_base_ = "gen-android";
  }

  void init_generator() override;

  bool has_bit_vector(const t_struct*) override { return false; }

  bool is_comparable(const t_type*, std::vector<const t_type*>*) override {
    return false;
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
