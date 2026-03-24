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

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <fmt/core.h>

#include <openssl/evp.h>
#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/ast/uri.h>
#include <thrift/compiler/generate/cpp/name_resolver.h>
#include <thrift/compiler/generate/java/util.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/whisker/mstch_compat.h>

using namespace std;

namespace apache::thrift::compiler {

namespace {

/**
 * Gets the java namespace, throws a runtime error if not found.
 */
std::string get_namespace_or_default(const t_program& prog) {
  auto prog_namespace = fmt::format(
      "{}", fmt::join(prog.gen_namespace_or_default("java.swift", {}), "."));
  if (prog_namespace != "") {
    return prog_namespace;
  } else {
    throw std::runtime_error{"No namespace 'java.swift' in " + prog.name()};
  }
}

std::string get_constants_class_name(const t_program& prog) {
  const auto& constant_name = prog.get_namespace("java.swift.constants");
  if (constant_name == "") {
    return "Constants";
  } else {
    auto java_name_space = get_namespace_or_default(prog);
    std::string java_class_name;
    if (constant_name.rfind(java_name_space) == 0) {
      java_class_name = constant_name.substr(java_name_space.length() + 1);
    } else {
      java_class_name = constant_name;
    }

    if (java_class_name == "" ||
        java_class_name.find('.') != std::string::npos) {
      throw std::runtime_error{
          "Java Constants Class Name `" + java_class_name +
          "` is not well formatted."};
    }

    return java_class_name;
  }
}

template <typename Node>
mstch::node get_structed_annotation_attribute(
    const Node* node, const char* uri, const std::string& key) {
  if (auto annotation = node->find_structured_annotation_or_null(uri)) {
    for (const auto& item : annotation->value()->get_map()) {
      if (item.first->get_string() == key) {
        return item.second->get_string();
      }
    }
  }

  return nullptr;
}

template <typename Node>
std::string get_java_swift_name(const Node* node) {
  return node->get_unstructured_annotation(
      "java.swift.name", java::mangle_java_name(node->name(), false));
}

struct MdCtxDeleter {
  void operator()(EVP_MD_CTX* ctx) const { EVP_MD_CTX_free(ctx); }
};
using ctx_ptr = std::unique_ptr<EVP_MD_CTX, MdCtxDeleter>;

ctx_ptr newMdContext() {
  auto* ctx = EVP_MD_CTX_new();
  return ctx_ptr(ctx);
}

std::string hash(std::string st) {
  // Save an initialized context.
  static EVP_MD_CTX* kBase = []() {
    auto ctx = newMdContext();
    EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr);
    return ctx.release(); // Leaky singleton.
  }();

  // Copy the base context.
  auto ctx = newMdContext();
  EVP_MD_CTX_copy_ex(ctx.get(), kBase);
  // Digest the st.
  EVP_DigestUpdate(ctx.get(), st.data(), st.size());

  // Get the result.
  std::string result(EVP_MD_CTX_size(ctx.get()), 0);
  uint32_t size;
  EVP_DigestFinal_ex(
      ctx.get(), reinterpret_cast<uint8_t*>(result.data()), &size);
  assert(size == result.size()); // Should already be the correct size.
  result.resize(size);
  return result;
}

string toHex(const string& s) {
  ostringstream ret;

  unsigned int c;
  for (string::size_type i = 0; i < s.length(); ++i) {
    c = (unsigned int)(unsigned char)s[i];
    ret << hex << setfill('0') << setw(2) << c;
  }
  return ret.str().substr(0, 8);
}

std::string compute_type_list_hash(const t_program& program) {
  std::string uri_concat;
  for (const auto* s : program.structured_definitions()) {
    auto uri = s->uri();
    if (!uri.empty()) {
      uri_concat += uri;
    }
  }
  for (const auto* e : program.enums()) {
    auto uri = e->uri();
    if (!uri.empty()) {
      uri_concat += uri;
    }
  }
  return toHex(
      hash(program.name() + get_namespace_or_default(program) + uri_concat));
}

std::string java_constant_name(std::string name) {
  std::string constant_str;
  bool is_first = true;
  bool was_previous_char_upper = false;
  for (auto iter = name.begin(); iter != name.end(); ++iter) {
    auto character = *iter;
    bool is_upper = isupper(character);
    if (is_upper && !is_first && !was_previous_char_upper) {
      constant_str += '_';
    }
    constant_str += toupper(character);
    is_first = false;
    was_previous_char_upper = is_upper;
  }
  return constant_str;
}

set<std::string> adapterDefinitionSet;
std::string prevTypeName;

bool is_annotation_map_field_equal(
    const t_const* annotation, const char* field, const int value) {
  for (const auto& item : annotation->value()->get_map()) {
    if (item.first->get_string() == field) {
      return item.second->get_integer() == value;
    }
  }
  return annotation->value()->get_map().empty();
}

void validate_java_enum_intrinsic_default(
    sema_context& ctx, const t_enum& enm) {
  if (enm.has_structured_annotation(kJavaUseIntrinsicDefaultUri) &&
      enm.find_value(0) == nullptr) {
    ctx.error(
        enm,
        "Enum {} does not have a value for 0! You have to have value for 0 "
        "to use intrinsic default annotation.",
        enm.name());
  }
}

class t_mstch_java_generator : public t_mstch_generator {
 public:
  using t_mstch_generator::t_mstch_generator;

  whisker_options render_options() const override {
    whisker_options opts;
    opts.allowed_undefined_variables = {
        "type:typedef_type", // in UnionWrite.mustache
        "struct:union?", // in WriteResponseType.mustache
        "field:hasAdapter?", // in BoxedType.mustache
    };
    return opts;
  }

  std::string template_prefix() const override { return "java"; }

  void generate_program() override;

  void fill_validator_visitors(ast_validator& validator) const override {
    validator.add_enum_visitor(validate_java_enum_intrinsic_default);
  }

 private:
  void set_mstch_factories();

  prototype<t_named>::ptr make_prototype_for_named(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_named(proto);
    auto def = whisker::dsl::prototype_builder<h_named>::extends(base);

    def.property("java_name", [](const t_named& self) {
      return java::mangle_java_name(self.name(), true);
    });
    def.property("java_qualified_name", [](const t_named& self) {
      return fmt::format(
          "{}.{}",
          get_namespace_or_default(*self.program()),
          java::mangle_java_name(self.name(), true));
    });

    return std::move(def).make();
  }

  prototype<t_const>::ptr make_prototype_for_const(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_const(proto);
    auto def = whisker::dsl::prototype_builder<h_const>::extends(base);

    def.property("javaCapitalName", [](const t_const& self) {
      return java::mangle_java_constant_name(self.name());
    });
    def.property("javaIgnoreConstant?", [](const t_const& self) {
      // we have to ignore constants if they are enums that we handled as ints,
      // as we don't have the constant values to work with.
      if (const t_map* map = self.type()->try_as<t_map>();
          map != nullptr && map->key_type()->is<t_enum>()) {
        return map->key_type()->has_unstructured_annotation(
            "java.swift.skip_enum_name_map");
      }
      if (const t_list* list = self.type()->try_as<t_list>();
          list != nullptr && list->elem_type()->is<t_enum>()) {
        return list->elem_type()->has_unstructured_annotation(
            "java.swift.skip_enum_name_map");
      }
      if (const t_set* set = self.type()->try_as<t_set>();
          set != nullptr && set->elem_type()->is<t_enum>()) {
        return set->elem_type()->has_unstructured_annotation(
            "java.swift.skip_enum_name_map");
      }
      // T194272441 generated schema const is rendered incorrectly.
      return self.generated();
    });

    return std::move(def).make();
  }

  prototype<t_const_value>::ptr make_prototype_for_const_value(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_const_value(proto);
    auto def = whisker::dsl::prototype_builder<h_const_value>::extends(base);

    def.property("quotedString", [](const t_const_value& self) {
      return java::quote_java_string(self.get_string());
    });

    return std::move(def).make();
  }

  prototype<t_enum>::ptr make_prototype_for_enum(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_enum(proto);
    auto def = whisker::dsl::prototype_builder<h_enum>::extends(base);
    def.property("java_enums_compat?", [](const t_enum& self) {
      return self.has_structured_annotation(kEnumsUri) ||
          self.program()->has_structured_annotation(kEnumsUri);
    });
    def.property("java_enum_type_open?", [&](const t_enum& self) {
      constexpr auto kEnumType = "type";
      constexpr int kOpenEnum = 1;
      if (const t_const* annotation =
              self.find_structured_annotation_or_null(kEnumsUri)) {
        return is_annotation_map_field_equal(annotation, kEnumType, kOpenEnum);
      }
      if (const t_const* annotation =
              self.program()->find_structured_annotation_or_null(kEnumsUri)) {
        return is_annotation_map_field_equal(annotation, kEnumType, kOpenEnum);
      }
      return false;
    });
    def.property("skipEnumNameMap?", [](const t_enum& self) {
      return self.has_unstructured_annotation("java.swift.skip_enum_name_map");
    });
    def.property("useIntrinsicDefault?", [](const t_enum& self) {
      if (self.has_structured_annotation(kJavaUseIntrinsicDefaultUri)) {
        if (self.find_value(0) == nullptr) {
          throw std::runtime_error(
              "Enum " + self.name() +
              " does not have a value for 0! You have to have value for 0 to use intrinsic default annotation.");
        }
        return true;
      }
      return false;
    });
    def.property("findValueZero", [](const t_enum& self) -> whisker::object {
      if (self.has_structured_annotation(kJavaUseIntrinsicDefaultUri)) {
        return whisker::make::string(
            java::mangle_java_constant_name(self.find_value(0)->name()));
      }
      return whisker::make::null;
    });
    return std::move(def).make();
  }

  prototype<t_enum_value>::ptr make_prototype_for_enum_value(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_enum_value(proto);
    auto def = whisker::dsl::prototype_builder<h_enum_value>::extends(base);
    def.property("javaConstantName", [](const t_enum_value& self) {
      return java::mangle_java_constant_name(self.name());
    });
    return std::move(def).make();
  }

  prototype<t_field>::ptr make_prototype_for_field(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_field(proto);
    auto def = whisker::dsl::prototype_builder<h_field>::extends(base);

    def.property("javaCapitalName", [](const t_field& self) {
      return java::mangle_java_name(get_java_swift_name(&self), true);
    });
    def.property(
        "negativeId?", [](const t_field& self) { return self.id() < 0; });
    def.property("isSensitive?", [](const t_field& self) {
      return self.has_unstructured_annotation("java.sensitive");
    });
    def.property("recursive?", [](const t_field& self) {
      return self.get_unstructured_annotation("swift.recursive_reference") ==
          "true" ||
          self.has_structured_annotation(kJavaRecursiveUri);
    });
    def.property("FieldNameUnmangled?", [](const t_field& self) {
      return self.has_structured_annotation(kJavaFieldUseUnmangledNameUri);
    });
    def.property("isEnum?", [](const t_field& self) {
      return self.type()->get_true_type()->is<t_enum>();
    });
    def.property("isObject?", [](const t_field& self) {
      return self.type()->get_true_type()->is<t_structured>();
    });
    def.property("isUnion?", [](const t_field& self) {
      return self.type()->get_true_type()->is<t_union>();
    });
    def.property("isNumericOrVoid?", [](const t_field& self) {
      auto type = self.type()->get_true_type();
      return type->is_void() || type->is_bool() || type->is_byte() ||
          type->is_i16() || type->is_i32() || type->is_i64() ||
          type->is_double() || type->is_float();
    });
    def.property("isNullableOrOptionalNotEnum?", [](const t_field& self) {
      if (self.qualifier() == t_field_qualifier::optional) {
        return true;
      }
      const t_type* field_type = self.type()->get_true_type();
      return !(
          field_type->is_bool() || field_type->is_byte() ||
          field_type->is_float() || field_type->is_i16() ||
          field_type->is_i32() || field_type->is_i64() ||
          field_type->is_double() || field_type->is<t_enum>());
    });
    def.property("hasInitialValue?", [](const t_field& self) {
      if (self.qualifier() == t_field_qualifier::optional) {
        return false;
      }
      return self.default_value() != nullptr;
    });
    def.property("javaAnnotations?", [](const t_field& self) {
      return self.has_unstructured_annotation("java.swift.annotations") ||
          self.has_structured_annotation(kJavaAnnotationUri);
    });
    def.property("javaAnnotations", [](const t_field& self) -> whisker::object {
      if (self.has_unstructured_annotation("java.swift.annotations")) {
        return whisker::make::string(
            self.get_unstructured_annotation("java.swift.annotations"));
      }
      if (auto annotation =
              self.find_structured_annotation_or_null(kJavaAnnotationUri)) {
        for (const auto& item : annotation->value()->get_map()) {
          if (item.first->get_string() == "java_annotation") {
            return whisker::make::string(item.second->get_string());
          }
        }
      }
      return whisker::make::null;
    });
    def.property("javaName", [](const t_field& self) {
      return get_java_swift_name(&self);
    });
    def.property("hasWrapper?", [](const t_field& self) {
      return self.has_structured_annotation(kJavaWrapperUri);
    });
    def.property("wrapper", [](const t_field& self) -> whisker::object {
      if (auto annotation =
              self.find_structured_annotation_or_null(kJavaWrapperUri)) {
        for (const auto& item : annotation->value()->get_map()) {
          if (item.first->get_string() == "wrapperClassName") {
            return whisker::make::string(item.second->get_string());
          }
        }
      }
      return whisker::make::null;
    });
    def.property("wrapperType", [](const t_field& self) -> whisker::object {
      if (auto annotation =
              self.find_structured_annotation_or_null(kJavaWrapperUri)) {
        for (const auto& item : annotation->value()->get_map()) {
          if (item.first->get_string() == "typeClassName") {
            return whisker::make::string(item.second->get_string());
          }
        }
      }
      return whisker::make::null;
    });
    def.property("hasFieldAdapter?", [](const t_field& self) {
      return self.has_structured_annotation(kJavaAdapterUri);
    });
    def.property(
        "fieldAdapterTypeClassName",
        [](const t_field& self) -> whisker::object {
          if (auto annotation =
                  self.find_structured_annotation_or_null(kJavaAdapterUri)) {
            for (const auto& item : annotation->value()->get_map()) {
              if (item.first->get_string() == "typeClassName") {
                return whisker::make::string(item.second->get_string());
              }
            }
          }
          return whisker::make::null;
        });
    def.property(
        "fieldAdapterClassName", [](const t_field& self) -> whisker::object {
          if (auto annotation =
                  self.find_structured_annotation_or_null(kJavaAdapterUri)) {
            for (const auto& item : annotation->value()->get_map()) {
              if (item.first->get_string() == "adapterClassName") {
                return whisker::make::string(item.second->get_string());
              }
            }
          }
          return whisker::make::null;
        });
    def.property("hasTypeAdapter?", [](const t_field& self) {
      auto type = self.type().get_type();
      return type->is<t_typedef>() &&
          t_typedef::get_first_structured_annotation_or_null(
              type, kJavaAdapterUri) != nullptr;
    });
    def.property(
        "typeAdapterTypeClassName", [](const t_field& self) -> whisker::object {
          auto type = self.type().get_type();
          if (type->is<t_typedef>()) {
            if (auto annotation =
                    t_typedef::get_first_structured_annotation_or_null(
                        type, kJavaAdapterUri)) {
              for (const auto& item : annotation->value()->get_map()) {
                if (item.first->get_string() == "typeClassName") {
                  return whisker::make::string(item.second->get_string());
                }
              }
            }
          }
          return whisker::make::null;
        });
    def.property(
        "typeAdapterClassName", [](const t_field& self) -> whisker::object {
          auto type = self.type().get_type();
          if (type->is<t_typedef>()) {
            if (auto annotation =
                    t_typedef::get_first_structured_annotation_or_null(
                        type, kJavaAdapterUri)) {
              for (const auto& item : annotation->value()->get_map()) {
                if (item.first->get_string() == "adapterClassName") {
                  return whisker::make::string(item.second->get_string());
                }
              }
            }
          }
          return whisker::make::null;
        });
    def.property("hasAdapter?", [](const t_field& self) {
      if (self.has_structured_annotation(kJavaAdapterUri)) {
        return true;
      }
      auto type = self.type().get_type();
      return type->is<t_typedef>() &&
          t_typedef::get_first_structured_annotation_or_null(
              type, kJavaAdapterUri) != nullptr;
    });
    def.property("hasAdapterOrWrapper?", [](const t_field& self) {
      if (self.has_structured_annotation(kJavaAdapterUri) ||
          self.has_structured_annotation(kJavaWrapperUri)) {
        return true;
      }
      auto type = self.type().get_type();
      return type->is<t_typedef>() &&
          t_typedef::get_first_structured_annotation_or_null(
              type, kJavaAdapterUri) != nullptr;
    });
    def.property("javaAllCapsName", [](const t_field& self) {
      auto field_name = self.name();
      boost::to_upper(field_name);
      return field_name;
    });
    def.property("javaConstantName", [](const t_field& self) {
      return java_constant_name(get_java_swift_name(&self));
    });
    def.property("javaTFieldName", [](const t_field& self) {
      return java_constant_name(get_java_swift_name(&self)) + "_FIELD_DESC";
    });
    def.property("typeFieldName", [](const t_field& self) {
      auto type_name = self.type()->get_true_type()->get_full_name();
      return java::mangle_java_name(type_name, true);
    });

    return std::move(def).make();
  }

  prototype<t_program>::ptr make_prototype_for_program(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_program(proto);
    auto def = whisker::dsl::prototype_builder<h_program>::extends(base);
    def.property("javaPackage", &get_namespace_or_default);
    def.property("constantClassName", &get_constants_class_name);
    def.property("type_list_batches", [&proto](const t_program& self) {
      constexpr int32_t BATCH_SIZE = 512;
      whisker::array::raw all_types;
      for (const auto* s : self.structured_definitions()) {
        if (!s->uri().empty()) {
          all_types.emplace_back(resolve_derived_t_type(proto, *s));
        }
      }
      for (const auto* e : self.enums()) {
        if (!e->uri().empty()) {
          all_types.emplace_back(resolve_derived_t_type(proto, *e));
        }
      }
      whisker::array::raw batches;
      for (size_t i = 0; i < all_types.size(); i += BATCH_SIZE) {
        whisker::array::raw batch;
        auto end =
            std::min(i + static_cast<size_t>(BATCH_SIZE), all_types.size());
        for (size_t j = i; j < end; ++j) {
          batch.emplace_back(std::move(all_types[j]));
        }
        batches.emplace_back(whisker::make::array(std::move(batch)));
      }
      return whisker::make::array(std::move(batches));
    });
    def.property("type_list_hash", [](const t_program& self) {
      return compute_type_list_hash(self);
    });
    return std::move(def).make();
  }

  prototype<t_interface>::ptr make_prototype_for_interface(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_interface(proto);
    auto def = whisker::dsl::prototype_builder<h_interface>::extends(base);
    def.property("function_batches", [&proto](const t_interface& self) {
      constexpr int32_t BATCH_SIZE = 100;
      whisker::array::raw all_funcs;
      for (const auto& func : self.functions()) {
        if (!func.is_interaction_constructor()) {
          all_funcs.emplace_back(proto.create<t_function>(func));
        }
      }
      whisker::array::raw batches;
      for (size_t i = 0; i < all_funcs.size(); i += BATCH_SIZE) {
        whisker::array::raw batch;
        auto end =
            std::min(i + static_cast<size_t>(BATCH_SIZE), all_funcs.size());
        for (size_t j = i; j < end; ++j) {
          batch.emplace_back(std::move(all_funcs[j]));
        }
        batches.emplace_back(whisker::make::array(std::move(batch)));
      }
      return whisker::make::array(std::move(batches));
    });
    return std::move(def).make();
  }

  prototype<t_function>::ptr make_prototype_for_function(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_function(proto);
    auto def = whisker::dsl::prototype_builder<h_function>::extends(base);
    def.property("java_name", [](const t_function& self) {
      return java::mangle_java_name(self.name(), false);
    });
    return std::move(def).make();
  }

  prototype<t_structured>::ptr make_prototype_for_structured(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_structured(proto);
    auto def = whisker::dsl::prototype_builder<h_structured>::extends(base);

    def.property("asBean?", [](const t_structured& self) {
      return self.is<t_struct>() &&
          (self.get_unstructured_annotation("java.swift.mutable") == "true" ||
           self.has_structured_annotation(kJavaMutableUri));
    });
    def.property("hasTerseField?", [](const t_structured& self) {
      for (const auto& field : self.fields()) {
        if (field.qualifier() == t_field_qualifier::terse) {
          return true;
        }
      }
      return false;
    });
    def.property("hasWrapper?", [](const t_structured& self) {
      for (const auto& field : self.fields()) {
        if (field.has_structured_annotation(kJavaWrapperUri)) {
          return true;
        }
      }
      return false;
    });

    def.property("isBigStruct?", [](const t_structured& self) {
      static constexpr uint64_t bigStructThreshold = 127;
      return (self.is<t_struct>() || self.is<t_union>()) &&
          self.fields().size() > bigStructThreshold;
    });

    def.property("javaAnnotations?", [](const t_structured& self) {
      return self.has_unstructured_annotation("java.swift.annotations") ||
          self.has_structured_annotation(kJavaAnnotationUri);
    });
    def.property(
        "javaAnnotations", [](const t_structured& self) -> std::string {
          if (self.has_unstructured_annotation("java.swift.annotations")) {
            return self.get_unstructured_annotation("java.swift.annotations");
          }
          if (auto annotation =
                  self.find_structured_annotation_or_null(kJavaAnnotationUri)) {
            for (const auto& item : annotation->value()->get_map()) {
              if (item.first->get_string() == "java_annotation") {
                return item.second->get_string();
              }
            }
          }
          return "";
        });

    def.property("needsExceptionMessage?", [](const t_structured& self) {
      return self.is<t_exception>() &&
          dynamic_cast<const t_exception&>(self).get_message_field() !=
          nullptr &&
          self.get_field_by_name("message") == nullptr;
    });
    def.property(
        "exceptionMessage", [](const t_structured& self) -> std::string {
          const auto* message_field =
              dynamic_cast<const t_exception&>(self).get_message_field();
          return get_java_swift_name(message_field);
        });

    return std::move(def).make();
  }

  prototype<t_type>::ptr make_prototype_for_type(
      const prototype_database& proto) const override {
    auto base = t_whisker_generator::make_prototype_for_type(proto);
    auto def = whisker::dsl::prototype_builder<h_type>::extends(base);

    def.property("javaCapitalName", [](const t_type& self) {
      return java::mangle_java_name(self.name(), true);
    });
    def.property("hasAdapter?", [](const t_type& self) {
      return self.is<t_typedef>() &&
          t_typedef::get_first_structured_annotation_or_null(
              &self, kJavaAdapterUri) != nullptr;
    });
    def.property("typedefWithoutJavaType?", [](const t_type& self) {
      if (!self.is<t_typedef>()) {
        return false;
      }
      return t_typedef::get_first_unstructured_annotation_or_null(
                 &self, {"java.swift.type"}) == nullptr;
    });
    def.property("javaType", [](const t_type& self) {
      auto* val = t_typedef::get_first_unstructured_annotation_or_null(
          &self, {"java.swift.type"});
      return val ? *val : "";
    });
    def.property("isBinaryString?", [](const t_type& self) {
      if (t_typedef::get_first_structured_annotation_or_null(
              &self, kJavaBinaryStringUri) != nullptr) {
        return true;
      }
      return t_typedef::get_first_unstructured_annotation_or_null(
                 &self, {"java.swift.binary_string"}) != nullptr;
    });
    def.property("adapterClassName", [](const t_type& self) {
      if (const t_const* annotation =
              t_typedef::get_first_structured_annotation_or_null(
                  &self, kJavaAdapterUri);
          annotation != nullptr && self.is<t_typedef>()) {
        for (const auto& item : annotation->value()->get_map()) {
          if (item.first->get_string() == "adapterClassName") {
            return whisker::make::string(item.second->get_string());
          }
        }
      }
      return whisker::make::null;
    });
    def.property("typeClassName", [](const t_type& self) {
      if (const t_const* annotation =
              t_typedef::get_first_structured_annotation_or_null(
                  &self, kJavaAdapterUri);
          annotation != nullptr && self.is<t_typedef>()) {
        for (const auto& item : annotation->value()->get_map()) {
          if (item.first->get_string() == "typeClassName") {
            return whisker::make::string(item.second->get_string());
          }
        }
      }
      return whisker::make::null;
    });
    def.property("enumSkipNameMap?", [](const t_type& self) {
      if (self.get_true_type()->is<t_enum>()) {
        return self.get_true_type()->has_unstructured_annotation(
            "java.swift.skip_enum_name_map");
      }
      return false;
    });

    return std::move(def).make();
  }

  /*
   * Generate multiple Java items according to the given template. Writes
   * output to package_dir underneath the global output directory.
   */
  void generate_rpc_interfaces() {
    const t_program* program = get_program();
    const auto& id = program->path();
    if (!mstch_context_.program_cache.count(id)) {
      mstch_context_.program_cache[id] =
          mstch_context_.program_factory->make_mstch_object(
              program, mstch_context_);
    }
    auto raw_package_dir = std::filesystem::path{
        java::package_to_path(get_namespace_or_default(*program))};
    auto package_dir = has_option("separate_data_type_from_services")
        ? "services" / raw_package_dir
        : raw_package_dir;

    for (const t_service* service : program->services()) {
      auto& cache = mstch_context_.service_cache;
      auto filename = java::mangle_java_name(service->name(), true) + ".java";
      const auto& item_id = id + service->name();
      if (!cache.count(item_id)) {
        cache[item_id] = mstch_context_.service_factory->make_mstch_object(
            service, mstch_context_);
      }

      render_to_file(cache[item_id], "Service", package_dir / filename);
    }
  }

  template <typename T, typename Factory, typename Cache>
  void generate_items(
      const Factory& factory,
      Cache& c,
      const t_program* program,
      const std::vector<T*>& items,
      const std::string& tpl_path) {
    const auto& id = program->path();
    if (!mstch_context_.program_cache.count(id)) {
      mstch_context_.program_cache[id] =
          mstch_context_.program_factory->make_mstch_object(
              program, mstch_context_);
    }
    auto raw_package_dir = std::filesystem::path{
        java::package_to_path(get_namespace_or_default(*program))};
    auto package_dir = has_option("separate_data_type_from_services")
        ? "data-type" / raw_package_dir
        : raw_package_dir;

    for (const T* item : items) {
      auto classname = java::mangle_java_name(item->name(), true);
      auto filename = classname + ".java";
      const auto& item_id = id + item->name();
      if (!c.count(item_id)) {
        c[item_id] = factory.make_mstch_object(item, mstch_context_);
      }

      render_to_file(c[item_id], tpl_path, package_dir / filename);
    }
  }

  /*
   * Generate Service Client implementation - Sync & Async. Writes
   * output to package_dir
   */
  void generate_services() {
    const auto& service_factory = *mstch_context_.service_factory;
    auto& cache = mstch_context_.service_cache;
    const t_program* program = get_program();
    const auto& id = program->path();
    if (!mstch_context_.program_cache.count(id)) {
      mstch_context_.program_cache[id] =
          mstch_context_.program_factory->make_mstch_object(
              program, mstch_context_);
    }

    auto raw_package_dir = std::filesystem::path{
        java::package_to_path(get_namespace_or_default(*program))};

    auto package_dir = has_option("separate_data_type_from_services")
        ? "services" / raw_package_dir
        : raw_package_dir;

    // Iterate through services
    for (const t_service* service : program->services()) {
      auto service_name = java::mangle_java_name(service->name(), true);
      // NOTE: generation of ClientImpl and AsyncClientImpl is deprecated and
      // only customers who are still using netty3 for some reason should use it
      // for backward compatibility reasons.
      if (has_option("deprecated_allow_leagcy_reflection_client")) {
        // Generate deprecated sync client
        auto sync_filename = service_name + "ClientImpl.java";
        const auto& sync_service_id = id + service->name() + "Client";
        if (!cache.count(sync_service_id)) {
          cache[sync_service_id] =
              service_factory.make_mstch_object(service, mstch_context_);
        }

        render_to_file(
            cache[sync_service_id],
            "deprecated/ServiceClient",
            package_dir / sync_filename);

        // Generate deprecated async client
        auto async_filename = service_name + "AsyncClientImpl.java";
        const auto& async_service_id = id + service->name() + "AsyncClient";
        if (!cache.count(async_service_id)) {
          cache[async_service_id] =
              service_factory.make_mstch_object(service, mstch_context_);
        }

        render_to_file(
            cache[async_service_id],
            "deprecated/ServiceAsyncClient",
            package_dir / async_filename);
      }

      // Generate Async to Reactive Wrapper
      auto async_reactive_wrapper_filename =
          service_name + "AsyncReactiveWrapper.java";
      const auto& async_reactive_wrapper_id =
          id + service->name() + "AsyncReactiveWrapper";
      if (!cache.count(async_reactive_wrapper_id)) {
        cache[async_reactive_wrapper_id] =
            service_factory.make_mstch_object(service, mstch_context_);
      }

      render_to_file(
          cache[async_reactive_wrapper_id],
          "AsyncReactiveWrapper",
          package_dir / async_reactive_wrapper_filename);

      // Generate Blocking to Reactive Wrapper
      auto blocking_reactive_wrapper_filename =
          service_name + "BlockingReactiveWrapper.java";
      const auto& blocking_reactive_wrapper_id =
          id + service->name() + "BlockingReactiveWrapper";
      if (!cache.count(blocking_reactive_wrapper_id)) {
        cache[blocking_reactive_wrapper_id] =
            service_factory.make_mstch_object(service, mstch_context_);
      }

      render_to_file(
          cache[blocking_reactive_wrapper_id],
          "BlockingReactiveWrapper",
          package_dir / blocking_reactive_wrapper_filename);

      // Generate Reactive to Async Wrapper
      auto reactive_async_wrapper_filename =
          service_name + "ReactiveAsyncWrapper.java";
      const auto& reactive_async_wrapper_id =
          id + service->name() + "ReactiveAsyncWrapper";
      if (!cache.count(reactive_async_wrapper_id)) {
        cache[reactive_async_wrapper_id] =
            service_factory.make_mstch_object(service, mstch_context_);
      }

      render_to_file(
          cache[reactive_async_wrapper_id],
          "ReactiveAsyncWrapper",
          package_dir / reactive_async_wrapper_filename);

      // Generate Reactive to Blocking Wrapper
      auto reactive_blocking_wrapper_filename =
          service_name + "ReactiveBlockingWrapper.java";
      const auto& reactive_blocking_wrapper_id =
          id + service->name() + "ReactiveBlockingWrapper";
      if (!cache.count(reactive_blocking_wrapper_id)) {
        cache[reactive_blocking_wrapper_id] =
            service_factory.make_mstch_object(service, mstch_context_);
      }

      render_to_file(
          cache[reactive_blocking_wrapper_id],
          "ReactiveBlockingWrapper",
          package_dir / reactive_blocking_wrapper_filename);

      // Generate Reactive Client
      auto reactive_client_filename = service_name + "ReactiveClient.java";
      const auto& reactive_client_wrapper_id =
          id + service->name() + "ReactiveClient";
      if (!cache.count(reactive_client_wrapper_id)) {
        cache[reactive_client_wrapper_id] =
            service_factory.make_mstch_object(service, mstch_context_);
      }

      render_to_file(
          cache[reactive_client_wrapper_id],
          "ReactiveClient",
          package_dir / reactive_client_filename);

      // Generate RpcServerHandler
      auto rpc_server_handler_filename = service_name + "RpcServerHandler.java";
      const auto& rpc_server_handler_id =
          id + service->name() + "RpcServerHandler";
      if (!cache.count(rpc_server_handler_id)) {
        cache[rpc_server_handler_id] =
            service_factory.make_mstch_object(service, mstch_context_);
      }

      render_to_file(
          cache[rpc_server_handler_id],
          "RpcServerHandler",
          package_dir / rpc_server_handler_filename);

      auto metadata_handler_filename =
          service_name + "ThriftMetadataHandler.java";
      const auto& metadata_handler_id =
          id + service->name() + "ThriftMetadataHandler";
      if (!cache.count(metadata_handler_id)) {
        cache[metadata_handler_id] =
            service_factory.make_mstch_object(service, mstch_context_);
      }

      render_to_file(
          cache[metadata_handler_id],
          "ThriftMetadataHandler",
          package_dir / metadata_handler_filename);
    }
  }

  void generate_constants(const t_program* program) {
    if (program->consts().empty()) {
      // Only generate Constants.java if we actually have constants
      return;
    }
    const auto& prog = cached_program(program);

    auto raw_package_dir = std::filesystem::path{
        java::package_to_path(get_namespace_or_default(*program))};
    auto package_dir = has_option("separate_data_type_from_services")
        ? "data-type" / raw_package_dir
        : raw_package_dir;

    auto constant_file_name = get_constants_class_name(*program) + ".java";
    render_to_file(prog, "Constants", package_dir / constant_file_name);
  }

  void generate_placeholder(const t_program* program) {
    auto package_dir = std::filesystem::path{
        java::package_to_path(get_namespace_or_default(*program))};
    auto placeholder_file_name = ".generated_" + program->name();
    if (has_option("separate_data_type_from_services")) {
      write_output("data-type" / package_dir / placeholder_file_name, "");
      write_output("services" / package_dir / placeholder_file_name, "");
    } else {
      write_output(package_dir / placeholder_file_name, "");
    }
  }

  void generate_type_list(const t_program* program) {
    // NOTE: ideally, we do not really need to generated type list if
    // `type_list.size() == 0`, but due to build system limitations.
    // all java_library need at least one .java file.

    auto java_namespace = get_namespace_or_default(*program);
    auto raw_package_dir =
        std::filesystem::path{java::package_to_path(java_namespace)};
    auto package_dir = has_option("separate_data_type_from_services")
        ? "data-type" / raw_package_dir
        : raw_package_dir;

    auto list_hash = compute_type_list_hash(*program);

    std::string file_name = "__fbthrift_TypeList_" + list_hash + ".java";

    const auto& prog = cached_program(program);
    render_to_file(prog, "TypeList", package_dir / file_name);
  }
};

class mstch_java_struct : public mstch_struct {
 public:
  mstch_java_struct(
      const t_structured* s, mstch_context& ctx, mstch_element_position pos)
      : mstch_struct(s, ctx, pos) {
    register_methods(
        this,
        {
            {"struct:unionFieldTypeUnique?",
             &mstch_java_struct::is_union_field_type_unique},
            {"struct:clearAdapter", &mstch_java_struct::clear_adapter},
        });
  }
  mstch::node is_union_field_type_unique() {
    std::set<std::string> field_types;
    for (const auto& field : struct_->fields()) {
      auto type_name = field.type()->get_true_type()->get_full_name();
      std::string type_with_erasure = type_name.substr(0, type_name.find('<'));
      if (field_types.find(type_with_erasure) != field_types.end()) {
        return false;
      } else {
        field_types.insert(type_with_erasure);
      }
    }
    return true;
  }
  mstch::node clear_adapter() {
    adapterDefinitionSet.clear();
    return mstch::node();
  }
};

class mstch_java_service : public mstch_service {
 public:
  mstch_java_service(
      const t_service* s,
      mstch_context& ctx,
      mstch_element_position pos,
      const t_service* containing_service = nullptr)
      : mstch_service(s, ctx, pos, containing_service) {
    register_methods(
        this,
        {
            {"service:onewayFunctions",
             &mstch_java_service::get_oneway_functions},
            {"service:requestResponseFunctions",
             &mstch_java_service::get_request_response_functions},
            {"service:singleRequestFunctions",
             &mstch_java_service::get_single_request_functions},
            {"service:streamingFunctions",
             &mstch_java_service::get_streaming_functions},
            {"service:sinkFunctions", &mstch_java_service::get_sink_functions},
            {"service:metaReferencedStructs",
             &mstch_java_service::meta_referenced_structs},
            {"service:metaReferencedEnums",
             &mstch_java_service::meta_referenced_enums},
            {"service:metaReferencedExceptions",
             &mstch_java_service::meta_referenced_exceptions},
        });
  }
  mstch::node get_oneway_functions() {
    std::vector<const t_function*> funcs;
    for (auto& func : service_->functions()) {
      if (func.qualifier() == t_function_qualifier::oneway) {
        funcs.push_back(&func);
      }
    }
    return make_mstch_functions(funcs);
  }
  mstch::node get_request_response_functions() {
    std::vector<const t_function*> funcs;
    for (auto& func : service_->functions()) {
      if (!func.sink_or_stream() && !func.is_interaction_constructor() &&
          func.qualifier() != t_function_qualifier::oneway) {
        funcs.push_back(&func);
      }
    }
    return make_mstch_functions(funcs);
  }
  mstch::node get_single_request_functions() {
    std::vector<const t_function*> funcs;
    for (auto& func : service_->functions()) {
      if (!func.sink_or_stream() && !func.is_interaction_constructor()) {
        funcs.push_back(&func);
      }
    }
    return make_mstch_functions(funcs);
  }

  mstch::node get_streaming_functions() {
    std::vector<const t_function*> funcs;
    for (auto& func : service_->functions()) {
      if (func.stream() && !func.is_bidirectional_stream()) {
        funcs.push_back(&func);
      }
    }
    return make_mstch_functions(funcs);
  }

  mstch::node get_sink_functions() {
    std::vector<const t_function*> funcs;
    for (auto& func : service_->functions()) {
      if (func.sink() && !func.is_bidirectional_stream()) {
        funcs.push_back(&func);
      }
    }
    return make_mstch_functions(funcs);
  }

  // TODO(leochashnikov): Traverse type graph to populate
  // structs/enums/exceptions.
  mstch::node meta_referenced_structs() { return mstch::array(); }
  mstch::node meta_referenced_enums() { return mstch::array(); }
  mstch::node meta_referenced_exceptions() { return mstch::array(); }
};

class mstch_java_interaction : public mstch_java_service {
 public:
  using ast_type = t_interaction;

  mstch_java_interaction(
      const t_interaction* interaction,
      mstch_context& ctx,
      mstch_element_position pos,
      const t_service* containing_service)
      : mstch_java_service(interaction, ctx, pos, containing_service) {
    register_methods(
        this,
        {{"interaction:javaParentCapitalName",
          &mstch_java_interaction::java_parent_capital_name}});
  }

  mstch::node java_parent_capital_name() {
    return java::mangle_java_name(containing_service_->name(), true);
  }
};

class mstch_java_function : public mstch_function {
 public:
  mstch_java_function(
      const t_function* f, mstch_context& ctx, mstch_element_position pos)
      : mstch_function(f, ctx, pos) {
    register_methods(
        this,
        {
            {"function:voidType", &mstch_java_function::is_void_type},
            {"function:nestedDepth",
             {with_no_caching, &mstch_java_function::get_nested_depth}},
            {"function:nestedDepth++",
             {with_no_caching, &mstch_java_function::increment_nested_depth}},
            {"function:nestedDepth--",
             {with_no_caching, &mstch_java_function::decrement_nested_depth}},
            {"function:prevNestedDepth",
             {with_no_caching, &mstch_java_function::preceding_nested_depth}},
            {"function:isNested?",
             {with_no_caching,
              &mstch_java_function::get_nested_container_flag}},
            {"function:setIsNested",
             {with_no_caching,
              &mstch_java_function::set_nested_container_flag}},
            {"function:unsetIsNested",
             {with_no_caching,
              &mstch_java_function::unset_nested_container_flag}},
        });
  }

  int32_t nestedDepth = 0;
  bool isNestedContainerFlag = false;

  mstch::node get_nested_depth() { return nestedDepth; }
  mstch::node preceding_nested_depth() { return (nestedDepth - 1); }
  mstch::node get_nested_container_flag() { return isNestedContainerFlag; }
  mstch::node set_nested_container_flag() {
    isNestedContainerFlag = true;
    return mstch::node();
  }
  mstch::node unset_nested_container_flag() {
    isNestedContainerFlag = false;
    return mstch::node();
  }
  mstch::node increment_nested_depth() {
    nestedDepth++;
    return mstch::node();
  }
  mstch::node decrement_nested_depth() {
    nestedDepth--;
    return mstch::node();
  }

  mstch::node is_void_type() {
    return function_->return_type()->is_void() && !function_->stream();
  }
};

class mstch_java_field : public mstch_field {
 public:
  mstch_java_field(
      const t_field* f, mstch_context& ctx, mstch_element_position pos)
      : mstch_field(f, ctx, pos) {
    register_methods(
        this,
        {
            {"field:nestedDepth",
             {with_no_caching, &mstch_java_field::get_nested_depth}},
            {"field:nestedDepth++",
             {with_no_caching, &mstch_java_field::increment_nested_depth}},
            {"field:nestedDepth--",
             {with_no_caching, &mstch_java_field::decrement_nested_depth}},
            {"field:prevNestedDepth",
             {with_no_caching, &mstch_java_field::preceding_nested_depth}},
            {"field:isNested?",
             {with_no_caching, &mstch_java_field::get_nested_container_flag}},
            {"field:setIsNested",
             {with_no_caching, &mstch_java_field::set_nested_container_flag}},

            {"field:java_strings_compat?",
             &mstch_java_field::is_strings_compat},
            {"field:java_coding_error_action_report?",
             &mstch_java_field::is_coding_error_action_report},
        });
  }

  int32_t nestedDepth = 0;
  bool isNestedContainerFlag = false;

  mstch::node get_nested_depth() { return nestedDepth; }
  mstch::node preceding_nested_depth() { return (nestedDepth - 1); }
  mstch::node get_nested_container_flag() { return isNestedContainerFlag; }
  mstch::node set_nested_container_flag() {
    isNestedContainerFlag = true;
    return mstch::node();
  }
  mstch::node increment_nested_depth() {
    nestedDepth++;
    return mstch::node();
  }
  mstch::node decrement_nested_depth() {
    nestedDepth--;
    return mstch::node();
  }

  mstch::node is_strings_compat() {
    if (field_->has_structured_annotation(kStringsUri)) {
      return true;
    }
    auto type = field_->type().get_type();
    if (type->is<t_typedef>() &&
        t_typedef::get_first_structured_annotation_or_null(type, kStringsUri) !=
            nullptr) {
      return true;
    }
    const t_structured* parent = whisker_context().get_field_parent(field_);
    return parent != nullptr &&
        (parent->has_structured_annotation(kStringsUri) ||
         parent->program()->has_structured_annotation(kStringsUri));
  }
  mstch::node is_coding_error_action_report() {
    constexpr auto kOnInvalidUtf8 = "onInvalidUtf8";
    constexpr int kActionReport = 1;

    auto type = field_->type().get_type();
    if (type->is<t_typedef>()) {
      if (const t_const* annotation =
              t_typedef::get_first_structured_annotation_or_null(
                  type, kStringsUri)) {
        return is_annotation_map_field_equal(
            annotation, kOnInvalidUtf8, kActionReport);
      }
    }
    if (const t_const* annotation =
            field_->find_structured_annotation_or_null(kStringsUri)) {
      return is_annotation_map_field_equal(
          annotation, kOnInvalidUtf8, kActionReport);
    }
    const t_structured* parent = whisker_context().get_field_parent(field_);
    if (parent == nullptr) {
      return false;
    }
    if (const t_const* annotation =
            parent->find_structured_annotation_or_null(kStringsUri)) {
      return is_annotation_map_field_equal(
          annotation, kOnInvalidUtf8, kActionReport);
    }
    if (const t_const* annotation =
            parent->program()->find_structured_annotation_or_null(
                kStringsUri)) {
      return is_annotation_map_field_equal(
          annotation, kOnInvalidUtf8, kActionReport);
    }
    return false;
  }
};

class mstch_java_type : public mstch_type {
 public:
  mstch_java_type(
      const t_type* t, mstch_context& ctx, mstch_element_position pos)
      : mstch_type(t, ctx, pos) {
    register_methods(
        this,
        {
            {"type:setIsMapKey",
             {with_no_caching, &mstch_java_type::set_is_map_key}},
            {"type:isMapKey?",
             {with_no_caching, &mstch_java_type::get_map_key_flag}},
            {"type:setIsMapValue",
             {with_no_caching, &mstch_java_type::set_is_map_value}},
            {"type:isMapValue?",
             {with_no_caching, &mstch_java_type::get_map_value_flag}},
            {"type:setIsNotMap",
             {with_no_caching, &mstch_java_type::set_is_not_map}},
            {"type:setAdapter",
             {with_no_caching, &mstch_java_type::set_adapter}},
            {"type:unsetAdapter",
             {with_no_caching, &mstch_java_type::unset_adapter}},
            {"type:isAdapterSet?",
             {with_no_caching, &mstch_java_type::is_adapter_set}},
            {"type:addAdapter",
             {with_no_caching, &mstch_java_type::add_type_adapter}},
            {"type:adapterDefined?",
             {with_no_caching, &mstch_java_type::is_type_adapter_defined}},
            {"type:lastAdapter?",
             {with_no_caching, &mstch_java_type::is_last_type_adapter}},
            {"type:setTypeName",
             {with_no_caching, &mstch_java_type::set_type_name}},
            {"type:getTypeName",
             {with_no_caching, &mstch_java_type::get_type_name}},
        });
  }
  bool isMapValueFlag = false;
  bool isMapKeyFlag = false;
  bool hasTypeAdapter = false;

  mstch::node set_is_not_map() {
    isMapValueFlag = false;
    isMapKeyFlag = false;
    return mstch::node();
  }
  mstch::node get_map_value_flag() { return isMapValueFlag; }
  mstch::node get_map_key_flag() { return isMapKeyFlag; }
  mstch::node set_is_map_value() {
    isMapValueFlag = true;
    return mstch::node();
  }
  mstch::node set_is_map_key() {
    isMapKeyFlag = true;
    return mstch::node();
  }

  mstch::node set_adapter() {
    hasTypeAdapter = true;
    return mstch::node();
  }

  mstch::node unset_adapter() {
    hasTypeAdapter = false;
    return mstch::node();
  }

  mstch::node is_adapter_set() { return hasTypeAdapter; }

  mstch::node add_type_adapter() {
    adapterDefinitionSet.insert(type_->name());
    return mstch::node();
  }

  mstch::node is_type_adapter_defined() {
    return adapterDefinitionSet.find(type_->name()) !=
        adapterDefinitionSet.end();
  }

  int32_t nested_adapter_count() {
    int32_t count = 0;
    auto type = type_;
    while (type) {
      if (type_->is<t_typedef>() &&
          type->has_structured_annotation(kJavaAdapterUri)) {
        count++;
        if (const auto* as_typedef = type->try_as<t_typedef>()) {
          type = &as_typedef->type().deref();
        } else {
          type = nullptr;
        }
      } else {
        type = nullptr;
      }
    }

    return count;
  }

  mstch::node is_last_type_adapter() { return nested_adapter_count() < 2; }

  mstch::node set_type_name() {
    if (nested_adapter_count() > 0) {
      prevTypeName = type_->name();
    }

    return mstch::node();
  }

  mstch::node get_type_name() { return prevTypeName; }
};

void t_mstch_java_generator::generate_program() {
  out_dir_base_ = "gen-java";
  set_mstch_factories();

  auto name = get_program()->name();
  const auto& id = get_program()->path();
  if (!mstch_context_.program_cache.count(id)) {
    mstch_context_.program_cache[id] =
        mstch_context_.program_factory->make_mstch_object(
            get_program(), mstch_context_);
  }

  generate_items(
      *mstch_context_.struct_factory,
      mstch_context_.struct_cache,
      get_program(),
      get_program()->structured_definitions(),
      "Object");
  generate_rpc_interfaces();
  generate_services();
  generate_items(
      *mstch_context_.enum_factory,
      mstch_context_.enum_cache,
      get_program(),
      get_program()->enums(),
      "Enum");
  generate_constants(get_program());
  generate_placeholder(get_program());
  generate_type_list(get_program());
}

void t_mstch_java_generator::set_mstch_factories() {
  mstch_context_.add<mstch_java_service>();
  mstch_context_.add<mstch_java_interaction>();
  mstch_context_.add<mstch_java_function>();
  mstch_context_.add<mstch_java_type>();
  mstch_context_.add<mstch_java_struct>();
  mstch_context_.add<mstch_java_field>();
}

} // namespace

THRIFT_REGISTER_GENERATOR(mstch_java, "Java", "");

} // namespace apache::thrift::compiler
