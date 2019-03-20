/*
 * Copyright 2016-present Facebook, Inc.
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
#include <cctype>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <thrift/compiler/lib/java/util.h>

#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>

namespace {
using namespace std;

class swift_generator_context {
 public:
  swift_generator_context(
      bool legacy_extend_runtime_exception,
      bool legacy_generate_beans,
      std::unique_ptr<std::string> default_package,
      std::string namespace_identifier)
      : legacy_extend_runtime_exception_(legacy_extend_runtime_exception),
        legacy_generate_beans_(legacy_generate_beans),
        default_package_(std::move(default_package)),
        namespace_identifier_(std::move(namespace_identifier)) {}

  /**
   * Gets the swift namespace, or, if it doesn't exist, uses the default.
   * If no default specified, throws runtime error
   */
  std::string get_namespace_or_default(const t_program& prog) {
    const auto& prog_namespace = prog.get_namespace(namespace_identifier_);
    if (prog_namespace != "") {
      return prog_namespace;
    } else if (default_package_) {
      return *(default_package_);
    } else {
      throw std::runtime_error{"No namespace '" + namespace_identifier_ +
                               "' in " + prog.get_name()};
    }
  }

  bool is_extend_runtime_exception() {
    return legacy_extend_runtime_exception_;
  }

  bool is_mutable_bean() {
    return legacy_generate_beans_;
  }

 private:
  const bool legacy_extend_runtime_exception_;
  const bool legacy_generate_beans_;
  std::unique_ptr<std::string> default_package_;
  const std::string namespace_identifier_;
};

class t_mstch_swift_generator : public t_mstch_generator {
 public:
  t_mstch_swift_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /* option_string */)
      : t_mstch_generator(
            program,
            std::move(context),
            "java/swift",
            parsed_options),
        swift_context_(std::make_shared<swift_generator_context>(
            has_option("legacy_extend_runtime_exception"),
            has_option("legacy_generate_beans"),
            get_option("default_package"),
            has_option("use_java_namespace") ? "java" : "java.swift")) {
    out_dir_base_ = "gen-swift";
  }

  void generate_program() override;

 private:
  std::shared_ptr<swift_generator_context> swift_context_;

  void set_mstch_generators();

  /*
   * Generate multiple Java items according to the given template. Writes
   * output to package_dir underneath the global output directory.
   */

  template <typename T, typename Generator, typename Cache>
  void generate_items(
      Generator const* generator,
      Cache& c,
      const t_program* program,
      const std::vector<T*>& items,
      const std::string& tpl_path) {
    const auto& id = program->get_path();
    if (!cache_->programs_.count(id)) {
      cache_->programs_[id] = generators_->program_generator_->generate(
          program, generators_, cache_);
    }
    auto package_dir = boost::filesystem::path{java::package_to_path(
        swift_context_->get_namespace_or_default(*program))};

    for (const T* item : items) {
      auto filename = java::mangle_java_name(item->get_name(), true) + ".java";
      const auto& item_id = id + item->get_name();
      if (!c.count(item_id)) {
        c[item_id] = generator->generate(item, generators_, cache_);
      }

      render_to_file(c[item_id], tpl_path, package_dir / filename);
    }
  }

  void generate_constants(const t_program* program) {
    if (program->get_consts().empty()) {
      // Only generate Constants.java if we actually have constants
      return;
    }
    auto name = program->get_name();
    const auto& id = program->get_path();
    if (!cache_->programs_.count(id)) {
      cache_->programs_[id] = generators_->program_generator_->generate(
          program, generators_, cache_);
    }
    auto package_dir = boost::filesystem::path{java::package_to_path(
        swift_context_->get_namespace_or_default(*program))};
    render_to_file(
        cache_->programs_[id], "Constants", package_dir / "Constants.java");
  }
};

class mstch_swift_program : public mstch_program {
 public:
  mstch_swift_program(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      std::shared_ptr<swift_generator_context> swift_context)
      : mstch_program(program, generators, cache, pos),
        swift_context_(std::move(swift_context)) {
    register_methods(
        this,
        {
            {"program:javaPackage", &mstch_swift_program::java_package},
        });
  }
  mstch::node java_package() {
    return swift_context_->get_namespace_or_default(*program_);
  }

 private:
  std::shared_ptr<swift_generator_context> swift_context_;
};

class mstch_swift_struct : public mstch_struct {
 public:
  mstch_swift_struct(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      std::shared_ptr<swift_generator_context> swift_context)
      : mstch_struct(strct, generators, cache, pos),
        swift_context_(std::move(swift_context)) {
    register_methods(
        this,
        {
            {"struct:javaPackage", &mstch_swift_struct::java_package},
            {"struct:extendRuntimeException?",
             &mstch_swift_struct::is_extend_runtime_exception},
            {"struct:asBean?", &mstch_swift_struct::is_as_bean},
            {"struct:javaCapitalName", &mstch_swift_struct::java_capital_name},
        });
  }
  mstch::node java_package() {
    return swift_context_->get_namespace_or_default(*(strct_->get_program()));
  }
  mstch::node is_extend_runtime_exception() {
    return swift_context_->is_extend_runtime_exception();
  }
  mstch::node is_as_bean() {
    return swift_context_->is_mutable_bean();
  }
  mstch::node java_capital_name() {
    return java::mangle_java_name(strct_->get_name(), true);
  }

 private:
  std::shared_ptr<swift_generator_context> swift_context_;
};

class mstch_swift_service : public mstch_service {
 public:
  mstch_swift_service(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      std::shared_ptr<swift_generator_context> swift_context)
      : mstch_service(service, generators, cache, pos),
        swift_context_(std::move(swift_context)) {
    register_methods(
        this,
        {
            {"service:javaPackage", &mstch_swift_service::java_package},
            {"service:javaCapitalName",
             &mstch_swift_service::java_capital_name},
        });
  }
  mstch::node java_package() {
    return swift_context_->get_namespace_or_default(*(service_->get_program()));
  }
  mstch::node java_capital_name() {
    return java::mangle_java_name(service_->get_name(), true);
  }

 private:
  std::shared_ptr<swift_generator_context> swift_context_;
};

class mstch_swift_function : public mstch_function {
 public:
  mstch_swift_function(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_function(function, generators, cache, pos) {
    register_methods(
        this,
        {
            {"function:javaName", &mstch_swift_function::java_name},
        });
  }
  mstch::node java_name() {
    return java::mangle_java_name(function_->get_name(), false);
  }
};

class mstch_swift_field : public mstch_field {
 public:
  mstch_swift_field(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index)
      : mstch_field(field, generators, cache, pos, index) {
    register_methods(
        this,
        {
            {"field:javaName", &mstch_swift_field::java_name},
            {"field:javaCapitalName", &mstch_swift_field::java_capital_name},
        });
  }
  mstch::node java_name() {
    return java::mangle_java_name(field_->get_name(), false);
  }
  mstch::node java_capital_name() {
    return java::mangle_java_name(field_->get_name(), true);
  }
};

class mstch_swift_enum : public mstch_enum {
 public:
  mstch_swift_enum(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      std::shared_ptr<swift_generator_context> swift_context)
      : mstch_enum(enm, generators, cache, pos), swift_context_(swift_context) {
    register_methods(
        this,
        {
            {"enum:javaPackage", &mstch_swift_enum::java_package},
            {"enum:javaCapitalName", &mstch_swift_enum::java_capital_name},
        });
  }
  mstch::node java_package() {
    return swift_context_->get_namespace_or_default(*(enm_->get_program()));
  }
  mstch::node java_capital_name() {
    return java::mangle_java_name(enm_->get_name(), true);
  }

 private:
  std::shared_ptr<swift_generator_context> swift_context_;
};

class mstch_swift_enum_value : public mstch_enum_value {
 public:
  mstch_swift_enum_value(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum_value(enm_value, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enumValue:javaConstantName",
             &mstch_swift_enum_value::java_constant_name},
        });
  }
  mstch::node java_constant_name() {
    return java::mangle_java_constant_name(enm_value_->get_name());
  }
};

class mstch_swift_const_value : public mstch_const_value {
 public:
  mstch_swift_const_value(
      t_const_value const* const_value,
      t_const const* current_const,
      t_type const* expected_type,
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
            {"value:quotedString", &mstch_swift_const_value::quote_java_string},
        });
  }
  mstch::node quote_java_string() {
    return java::quote_java_string(const_value_->get_string());
  }
};

class mstch_swift_type : public mstch_type {
 public:
  mstch_swift_type(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_type(type, generators, cache, pos) {
    register_methods(
        this,
        {
            {"type:primitive?", &mstch_swift_type::is_primitive},
        });
  }
  mstch::node is_primitive() {
    return type_->is_void() || type_->is_bool() || type_->is_byte() ||
        type_->is_i16() || type_->is_i32() || type_->is_i64() ||
        type_->is_double() || type_->is_float();
  }
};

class program_swift_generator : public program_generator {
 public:
  explicit program_swift_generator(
      std::shared_ptr<swift_generator_context> context)
      : context_(std::move(context)) {}
  ~program_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_swift_program>(
        program, generators, cache, pos, context_);
  }

 private:
  std::shared_ptr<swift_generator_context> context_;
};

class struct_swift_generator : public struct_generator {
 public:
  explicit struct_swift_generator(
      std::shared_ptr<swift_generator_context> context)
      : context_(std::move(context)) {}
  ~struct_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_swift_struct>(
        strct, generators, cache, pos, context_);
  }

 private:
  std::shared_ptr<swift_generator_context> context_;
};

class service_swift_generator : public service_generator {
 public:
  explicit service_swift_generator(
      std::shared_ptr<swift_generator_context> context)
      : context_(std::move(context)) {}
  ~service_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_swift_service>(
        service, generators, cache, pos, context_);
  }

 private:
  std::shared_ptr<swift_generator_context> context_;
};

class function_swift_generator : public function_generator {
 public:
  function_swift_generator() = default;
  ~function_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_swift_function>(
        function, generators, cache, pos);
  }
};

class field_swift_generator : public field_generator {
 public:
  field_swift_generator() = default;
  ~field_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const override {
    return std::make_shared<mstch_swift_field>(
        field, generators, cache, pos, index);
  }
};

class enum_swift_generator : public enum_generator {
 public:
  explicit enum_swift_generator(
      std::shared_ptr<swift_generator_context> context)
      : context_(std::move(context)) {}
  ~enum_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_swift_enum>(
        enm, generators, cache, pos, context_);
  }

 private:
  std::shared_ptr<swift_generator_context> context_;
};

class enum_value_swift_generator : public enum_value_generator {
 public:
  enum_value_swift_generator() = default;
  ~enum_value_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_swift_enum_value>(
        enm_value, generators, cache, pos);
  }
};

class type_swift_generator : public type_generator {
 public:
  type_swift_generator() = default;
  ~type_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_swift_type>(type, generators, cache, pos);
  }
};

class const_value_swift_generator : public const_value_generator {
 public:
  const_value_swift_generator() = default;
  ~const_value_swift_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_const_value const* const_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      t_type const* expected_type = nullptr) const override {
    return std::make_shared<mstch_swift_const_value>(
        const_value,
        current_const,
        expected_type,
        generators,
        cache,
        pos,
        index);
  }
};

void t_mstch_swift_generator::generate_program() {
  // disable mstch escaping
  mstch::config::escape = [](const std::string s) { return s; };

  set_mstch_generators();

  auto name = get_program()->get_name();
  const auto& id = get_program()->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] = generators_->program_generator_->generate(
        get_program(), generators_, cache_);
  }

  generate_items(
      generators_->struct_generator_.get(),
      cache_->structs_,
      get_program(),
      get_program()->get_objects(),
      "Object");
  generate_items(
      generators_->service_generator_.get(),
      cache_->services_,
      get_program(),
      get_program()->get_services(),
      "Service");
  generate_items(
      generators_->enum_generator_.get(),
      cache_->enums_,
      get_program(),
      get_program()->get_enums(),
      "Enum");
  generate_constants(get_program());
}

void t_mstch_swift_generator::set_mstch_generators() {
  generators_->set_program_generator(
      std::make_unique<program_swift_generator>(swift_context_));
  generators_->set_struct_generator(
      std::make_unique<struct_swift_generator>(swift_context_));
  generators_->set_service_generator(
      std::make_unique<service_swift_generator>(swift_context_));
  generators_->set_function_generator(
      std::make_unique<function_swift_generator>());
  generators_->set_field_generator(std::make_unique<field_swift_generator>());
  generators_->set_enum_generator(
      std::make_unique<enum_swift_generator>(swift_context_));
  generators_->set_enum_value_generator(
      std::make_unique<enum_value_swift_generator>());
  generators_->set_type_generator(std::make_unique<type_swift_generator>());
  generators_->set_const_value_generator(
      std::make_unique<const_value_swift_generator>());
}
} // namespace

THRIFT_REGISTER_GENERATOR(mstch_swift, "Java Swift", "");
