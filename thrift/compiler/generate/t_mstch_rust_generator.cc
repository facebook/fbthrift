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

#include <cctype>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>

using namespace std;

namespace apache {
namespace thrift {
namespace compiler {

namespace {
std::string mangle(const std::string& name) {
  auto reserved = {
      // Keywords
      "abstract",
      "alignof",
      "as",
      "async",
      "await",
      "become",
      "box",
      "break",
      "const",
      "continue",
      "crate",
      "do",
      "else",
      "enum",
      "extern",
      "false",
      "final",
      "fn",
      "for",
      "if",
      "impl",
      "in",
      "let",
      "loop",
      "macro",
      "match",
      "mod",
      "move",
      "mut",
      "offsetof",
      "override",
      "priv",
      "proc",
      "pub",
      "pure",
      "ref",
      "return",
      "Self",
      "self",
      "sizeof",
      "static",
      "struct",
      "super",
      "trait",
      "true",
      "type",
      "typeof",
      "unsafe",
      "unsized",
      "use",
      "virtual",
      "where",
      "while",
      "yield",

      // Core Types
      "i8",
      "u8",
      "i16",
      "u16",
      "i32",
      "u32",
      "i64",
      "u64",
      "i128",
      "u128",
      "f32",
      "f64",
      "isize",
      "usize",
      "str",
      "bool",
  };

  for (auto s : reserved) {
    if (name == s) {
      return name + '_';
    }
  }

  return name;
}

// Convert CamelCase to snake_case.
std::string snakecase(const std::string& name) {
  std::ostringstream snake;

  for (auto ch : name) {
    if (isupper(ch)) {
      if (snake.tellp() > 0) {
        snake << '_';
      }
      snake << (char)tolower(ch);
    } else {
      snake << ch;
    }
  }

  return snake.str();
}

// Convert snake_case to UpperCamelCase.
std::string camelcase(const std::string& name) {
  std::ostringstream camel;

  size_t i = 0;
  for (; i < name.size() && name[i] == '_'; i++) {
    // Copy same number of leading underscores.
    camel << '_';
  }

  auto underscore = true;
  for (; i < name.size(); i++) {
    if (name[i] == '_') {
      underscore = true;
    } else if (underscore) {
      camel << (char)toupper(name[i]);
      underscore = false;
    } else {
      camel << name[i];
    }
  }

  return camel.str();
}

std::string quote(const std::string& data) {
  std::ostringstream quoted;
  quoted << '"';

  for (auto ch : data) {
    if (ch == '\t') {
      quoted << '\\' << 't';
    } else if (ch == '\r') {
      quoted << '\\' << 'r';
    } else if (ch == '\n') {
      quoted << '\\' << 'n';
    } else if (ch == '\\' || ch == '"') {
      quoted << '\\' << ch;
    } else if (ch < '\x7f') {
      quoted << ch;
    } else {
      throw "Non-ASCII string literal not implemented";
    }
  }

  quoted << '"';
  return quoted.str();
}
} // namespace

class t_mstch_rust_generator : public t_mstch_generator {
 public:
  t_mstch_rust_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /* option_string */)
      : t_mstch_generator(program, std::move(context), "rust", parsed_options) {
    out_dir_base_ = "gen-rust2";
  }

  void generate_program() override;

 private:
  void set_mstch_generators();
};

class mstch_rust_program : public mstch_program {
 public:
  mstch_rust_program(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_program(program, generators, cache, pos) {
    register_methods(
        this,
        {
            {"program:ident", &mstch_rust_program::rust_ident},
            {"program:types?", &mstch_rust_program::rust_has_types},
        });
  }
  mstch::node rust_ident() {
    return mangle(program_->get_name());
  }
  mstch::node rust_has_types() {
    return !program_->get_structs().empty() || !program_->get_enums().empty() ||
        !program_->get_typedefs().empty() || !program_->get_xceptions().empty();
  }
};

class mstch_rust_struct : public mstch_struct {
 public:
  mstch_rust_struct(
      const t_struct* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_struct(strct, generators, cache, pos) {
    register_methods(
        this,
        {
            {"struct:package", &mstch_rust_struct::rust_package},
            {"struct:derive_eq", &mstch_rust_struct::rust_derive_eq},
            {"struct:derive_hash", &mstch_rust_struct::rust_derive_hash},
            {"struct:derive_ord", &mstch_rust_struct::rust_derive_ord},
        });
  }
  mstch::node rust_package() {
    return mangle(strct_->get_program()->get_name());
  }
  mstch::node rust_derive_eq() {
    // TODO
    return false;
  }
  mstch::node rust_derive_hash() {
    // TODO
    return false;
  }
  mstch::node rust_derive_ord() {
    // TODO
    return false;
  }
};

class mstch_rust_service : public mstch_service {
 public:
  mstch_rust_service(
      const t_service* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_service(service, generators, cache, pos) {
    register_methods(
        this,
        {
            {"service:package", &mstch_rust_service::rust_package},
            {"service:snake", &mstch_rust_service::rust_snake},
        });
  }
  mstch::node rust_package() {
    return mangle(service_->get_program()->get_name());
  }
  mstch::node rust_snake() {
    return mangle(snakecase(service_->get_name()));
  }
};

class mstch_rust_function : public mstch_function {
 public:
  mstch_rust_function(
      const t_function* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index)
      : mstch_function(function, generators, cache, pos), index_(index) {
    register_methods(
        this,
        {
            {"function:upcamel", &mstch_rust_function::rust_upcamel},
            {"function:index", &mstch_rust_function::rust_index},
        });
  }
  mstch::node rust_upcamel() {
    return camelcase(function_->get_name());
  }
  mstch::node rust_index() {
    return index_;
  }

 private:
  int32_t index_;
};

class mstch_rust_field : public mstch_field {
 public:
  mstch_rust_field(
      const t_field* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index)
      : mstch_field(field, generators, cache, pos, index) {
    register_methods(
        this,
        {
            {"field:ident", &mstch_rust_field::rust_ident},
        });
  }
  mstch::node rust_ident() {
    return mangle(field_->get_name());
  }
};

class mstch_rust_enum : public mstch_enum {
 public:
  mstch_rust_enum(
      const t_enum* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum(enm, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enum:package", &mstch_rust_enum::rust_package},
        });
  }
  mstch::node rust_package() {
    return mangle(enm_->get_program()->get_name());
  }
};

class mstch_rust_const : public mstch_const {
 public:
  mstch_rust_const(
      const t_const* cnst,
      const t_const* current_const,
      const t_type* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index,
      const std::string& field_name)
      : mstch_const(
            cnst,
            current_const,
            expected_type,
            generators,
            cache,
            pos,
            index,
            field_name) {
    register_methods(
        this,
        {
            {"const:package", &mstch_rust_const::rust_package},
        });
  }
  mstch::node rust_package() {
    return mangle(cnst_->get_program()->get_name());
  }
};

class mstch_rust_const_value : public mstch_const_value {
 public:
  mstch_rust_const_value(
      const t_const_value* const_value,
      const t_const* current_const,
      const t_type* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_const_value(
            const_value,
            current_const,
            expected_type,
            generators,
            cache,
            pos,
            index) {
    register_methods(
        this,
        {
            {"value:quoted", &mstch_rust_const_value::rust_quoted},
            {"value:enum_package", &mstch_rust_const_value::rust_enum_package},
        });
  }
  mstch::node rust_quoted() {
    return quote(const_value_->get_string());
  }
  mstch::node rust_enum_package() {
    if (type_ == cv::CV_INTEGER && const_value_->is_enum()) {
      return const_value_->get_enum()->get_program()->get_name();
    }
    return mstch::node();
  }
  bool same_type_as_expected() const override {
    return true;
  }
};

class mstch_rust_type : public mstch_type {
 public:
  mstch_rust_type(
      const t_type* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_type(type, generators, cache, pos) {
    register_methods(
        this,
        {
            {"type:package", &mstch_rust_type::rust_package},
        });
  }
  mstch::node rust_package() {
    return mangle(type_->get_program()->get_name());
  }
};

class program_rust_generator : public program_generator {
 public:
  program_rust_generator() = default;
  ~program_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_program>(
        program, generators, cache, pos);
  }
};

class struct_rust_generator : public struct_generator {
 public:
  explicit struct_rust_generator() = default;
  ~struct_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_struct* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_struct>(strct, generators, cache, pos);
  }
};

class service_rust_generator : public service_generator {
 public:
  explicit service_rust_generator() = default;
  ~service_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_service* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_service>(
        service, generators, cache, pos);
  }
};

class function_rust_generator : public function_generator {
 public:
  function_rust_generator() = default;
  ~function_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_function* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index) const override {
    return std::make_shared<mstch_rust_function>(
        function, generators, cache, pos, index);
  }
};

class field_rust_generator : public field_generator {
 public:
  field_rust_generator() = default;
  ~field_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_field* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index) const override {
    return std::make_shared<mstch_rust_field>(
        field, generators, cache, pos, index);
  }
};

class enum_rust_generator : public enum_generator {
 public:
  explicit enum_rust_generator() = default;
  ~enum_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_enum* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_enum>(enm, generators, cache, pos);
  }
};

class type_rust_generator : public type_generator {
 public:
  type_rust_generator() = default;
  ~type_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_type* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_type>(type, generators, cache, pos);
  }
};

class const_rust_generator : public const_generator {
 public:
  const_rust_generator() = default;
  ~const_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_const* cnst,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      const t_const* current_const,
      const t_type* expected_type,
      const std::string& field_name) const override {
    return std::make_shared<mstch_rust_const>(
        cnst,
        current_const,
        expected_type,
        generators,
        cache,
        pos,
        index,
        field_name);
  }
};

class const_value_rust_generator : public const_value_generator {
 public:
  const_value_rust_generator() = default;
  ~const_value_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_const_value* const_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      const t_const* current_const,
      const t_type* expected_type) const override {
    return std::make_shared<mstch_rust_const_value>(
        const_value,
        current_const,
        expected_type,
        generators,
        cache,
        pos,
        index);
  }
};

void t_mstch_rust_generator::generate_program() {
  // disable mstch escaping
  mstch::config::escape = [](const std::string& s) { return s; };

  set_mstch_generators();

  const auto* program = get_program();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }

  render_to_file(cache_->programs_[id], "lib.rs", "lib.rs");
}

void t_mstch_rust_generator::set_mstch_generators() {
  generators_->set_program_generator(
      std::make_unique<program_rust_generator>());
  generators_->set_struct_generator(std::make_unique<struct_rust_generator>());
  generators_->set_service_generator(
      std::make_unique<service_rust_generator>());
  generators_->set_function_generator(
      std::make_unique<function_rust_generator>());
  generators_->set_field_generator(std::make_unique<field_rust_generator>());
  generators_->set_enum_generator(std::make_unique<enum_rust_generator>());
  generators_->set_type_generator(std::make_unique<type_rust_generator>());
  generators_->set_const_generator(std::make_unique<const_rust_generator>());
  generators_->set_const_value_generator(
      std::make_unique<const_value_rust_generator>());
}

THRIFT_REGISTER_GENERATOR(mstch_rust, "Rust", "");

} // namespace compiler
} // namespace thrift
} // namespace apache
