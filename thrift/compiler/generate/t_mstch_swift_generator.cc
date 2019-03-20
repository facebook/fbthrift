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
        swift_context_{
            has_option("legacy_extend_runtime_exception"),
            has_option("legacy_generate_beans"),
            get_option("default_package"),
            has_option("use_java_namespace") ? "java" : "java.swift"} {
    out_dir_base_ = "gen-swift";
  }

  void generate_program() override;

 private:
  swift_generator_context swift_context_;

  /*
   * Generate multiple Java items according to the given template. Writes
   * output to package_dir underneath the global output directory.
   */
  template <typename T>
  void generate_items(
      const std::string& tpl_path,
      const std::vector<T*>& items) {
    for (const T* item : items) {
      auto package_dir = boost::filesystem::path{java::package_to_path(
          swift_context_.get_namespace_or_default(*item->get_program()))};
      auto filename = java::mangle_java_name(item->get_name(), true) + ".java";
      render_to_file(*item, tpl_path, package_dir / filename);
    }
  }

  void generate_constants(const t_program& prog) {
    if (prog.get_consts().empty()) {
      // Only generate Constants.java if we actually have constants
      return;
    }
    auto package_dir = boost::filesystem::path{
        java::package_to_path(swift_context_.get_namespace_or_default(prog))};
    render_to_file(prog, "Constants", package_dir / "Constants.java");
  }

  mstch::map extend_program(const t_program& program) override {
    // Sort constant members to match java swift generator
    auto constants = program.get_consts();
    std::sort(
        constants.begin(),
        constants.end(),
        [](const t_const* x, const t_const* y) {
          return std::less<std::string>{}(x->get_name(), y->get_name());
        });
    return mstch::map{
        {"javaPackage", swift_context_.get_namespace_or_default(program)},
        {"sortedConstants", dump_elems(constants)},
    };
  }

  mstch::map extend_struct(const t_struct& strct) override {
    mstch::map result{
        {"javaPackage",
         swift_context_.get_namespace_or_default(*strct.get_program())},
        {"extendRuntimeException?",
         swift_context_.is_extend_runtime_exception()},
        {"asBean?", swift_context_.is_mutable_bean()}};
    add_java_names(result, strct.get_name());
    return result;
  }

  mstch::map extend_service(const t_service& service) override {
    mstch::map result{
        {"javaPackage",
         swift_context_.get_namespace_or_default(*service.get_program())}};
    add_java_names(result, service.get_name());
    return result;
  }

  mstch::map extend_function(const t_function& func) override {
    mstch::map result;
    add_java_names(result, func.get_name());
    return result;
  }

  mstch::map extend_field(const t_field& field) override {
    mstch::map result;
    add_java_names(result, field.get_name());
    return result;
  }

  mstch::map extend_enum(const t_enum& enm) override {
    mstch::map result{
        {"javaPackage",
         swift_context_.get_namespace_or_default(*enm.get_program())},
    };
    add_java_names(result, enm.get_name());
    return result;
  }

  mstch::map extend_enum_value(const t_enum_value& value) override {
    mstch::map result;
    add_java_names(result, value.get_name());
    return result;
  }

  /**
   * Extend types to have a field set if the corresponding Java type has
   * a primitive representation. We need this because these types are treated
   * differently when they are arguments to type constructors in Java.
   */
  mstch::map extend_type(const t_type& type) override {
    return mstch::map{
        {"primitive?",
         type.is_void() || type.is_bool() || type.is_byte() || type.is_i16() ||
             type.is_i32() || type.is_i64() || type.is_double() ||
             type.is_float()},
    };
  }

  mstch::map extend_const_value(const t_const_value& const_value) override {
    mstch::map result;
    if (const_value.get_type() == t_const_value::CV_STRING) {
      result.emplace(
          "quotedString", java::quote_java_string(const_value.get_string()));
    }
    return result;
  }

  /**
   * Extends the map to contain elements for the java-mangled versions
   * of a Thrift identifier.
   * @modifies - map
   */
  static void add_java_names(mstch::map& map, const std::string& rawName) {
    map.emplace("javaName", java::mangle_java_name(rawName, false));
    map.emplace("javaCapitalName", java::mangle_java_name(rawName, true));
    map.emplace("javaConstantName", java::mangle_java_constant_name(rawName));
  }
};

void t_mstch_swift_generator::generate_program() {
  // disable mstch escaping
  mstch::config::escape = [](const std::string s) { return s; };

  generate_items("Object", get_program()->get_objects());

  generate_items("Service", get_program()->get_services());

  generate_items("Enum", get_program()->get_enums());

  generate_constants(*get_program());
}
} // namespace
THRIFT_REGISTER_GENERATOR(mstch_swift, "Java Swift", "");
