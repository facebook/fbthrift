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

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>

#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/gen/cpp/reference_type.h>
#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>
#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/lib/py3/util.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

mstch::array createStringArray(const std::vector<std::string>& values) {
  mstch::array a;
  for (auto it = values.begin(); it != values.end(); ++it) {
    a.push_back(mstch::map{
        {"value", *it},
        {"first?", it == values.begin()},
        {"last?", std::next(it) == values.end()},
    });
  }
  return a;
}

std::vector<std::string> get_py3_namespace(const t_program* prog) {
  return split_namespace(prog->get_namespace("py3"));
}

std::vector<std::string> get_py3_namespace_with_name(const t_program* prog) {
  auto ns = get_py3_namespace(prog);
  ns.push_back(prog->name());
  return ns;
}

const t_type* get_list_elem_type(const t_type& type) {
  assert(type.is_list());
  return dynamic_cast<const t_list&>(type).get_elem_type();
}

const t_type* get_set_elem_type(const t_type& type) {
  assert(type.is_set());
  return dynamic_cast<const t_set&>(type).get_elem_type();
}

const t_type* get_map_key_type(const t_type& type) {
  assert(type.is_map());
  return dynamic_cast<const t_map&>(type).get_key_type();
}

const t_type* get_map_val_type(const t_type& type) {
  assert(type.is_map());
  return dynamic_cast<const t_map&>(type).get_val_type();
}

std::string get_cpp_template(const t_type& type) {
  if (const auto* val =
          type.find_annotation_or_null({"cpp.template", "cpp2.template"})) {
    return *val;
  } else if (type.is_list()) {
    return "std::vector";
  } else if (type.is_set()) {
    return "std::set";
  } else if (type.is_map()) {
    return "std::map";
  }
  return {};
}

const t_type* get_stream_first_response_type(const t_type& type) {
  assert(type.is_streamresponse());
  return dynamic_cast<const t_stream_response&>(type).get_first_response_type();
}

const t_type* get_stream_elem_type(const t_type& type) {
  assert(type.is_streamresponse());
  return dynamic_cast<const t_stream_response&>(type).get_elem_type();
}

bool is_func_supported(bool no_stream, const t_function* func) {
  return !(no_stream && func->returns_stream()) && !func->returns_sink() &&
      !func->get_returntype()->is_service();
}

class mstch_py3_type : public mstch_type {
 public:
  struct CachedProperties {
    const std::string cppTemplate;
    std::string cppType;
    std::string flatName;
  };
  mstch_py3_type(
      const t_type* type,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const t_program* prog,
      CachedProperties& cachedProps)
      : mstch_type(type, generators, cache, pos),
        prog_{prog},
        cachedProps_{cachedProps} {
    strip_cpp_comments_and_newlines(cachedProps_.cppType);
    register_methods(
        this,
        {
            {"type:modulePath", &mstch_py3_type::modulePath},
            {"type:need_module_path?", &mstch_py3_type::need_module_path},
            {"type:flat_name", &mstch_py3_type::flatName},
            {"type:cppNamespaces", &mstch_py3_type::cppNamespaces},
            {"type:cppTemplate", &mstch_py3_type::cppTemplate},
            {"type:cythonTemplate", &mstch_py3_type::cythonTemplate},
            {"type:defaultTemplate?", &mstch_py3_type::isDefaultTemplate},
            {"type:cppCustomType", &mstch_py3_type::cppType},
            {"type:cythonCustomType", &mstch_py3_type::cythonType},
            {"type:hasCustomType?", &mstch_py3_type::hasCustomType},
            {"type:number?", &mstch_py3_type::isNumber},
            {"type:integer?", &mstch_py3_type::isInteger},
            {"type:containerOfString?", &mstch_py3_type::isContainerOfString},
            {"type:cythonTypeNoneable?", &mstch_py3_type::cythonTypeNoneable},
            {"type:hasCythonType?", &mstch_py3_type::hasCythonType},
            {"type:iobuf?", &mstch_py3_type::isIOBuf},
            {"type:iobufRef?", &mstch_py3_type::isIOBufRef},
            {"type:iobufWrapper?", &mstch_py3_type::isIOBufWrapper},
            {"type:flexibleBinary?", &mstch_py3_type::isFlexibleBinary},
            {"type:hasCustomTypeBehavior?",
             &mstch_py3_type::hasCustomTypeBehavior},
            {"type:simple?", &mstch_py3_type::isSimple},
            {"type:resolves_to_complex_return?",
             &mstch_py3_type::resolves_to_complex_return},
        });
  }

  mstch::node need_module_path() {
    if (!has_option("is_types_file")) {
      return true;
    }
    if (const t_program* prog = type_->program()) {
      if (prog != prog_) {
        return true;
      }
    }
    return false;
  }

  mstch::node modulePath() {
    return "_" + boost::algorithm::join(get_type_py3_namespace(), "_");
  }

  mstch::node flatName() { return cachedProps_.flatName; }

  mstch::node cppNamespaces() {
    return createStringArray(get_type_cpp2_namespace());
  }

  mstch::node cppTemplate() { return cachedProps_.cppTemplate; }

  mstch::node cythonTemplate() { return to_cython_template(); }

  mstch::node isDefaultTemplate() { return is_default_template(); }

  mstch::node cppType() { return cachedProps_.cppType; }

  mstch::node cythonType() { return to_cython_type(); }

  mstch::node hasCustomType() { return has_custom_cpp_type(); }

  mstch::node isNumber() { return is_number(); }

  mstch::node isInteger() { return is_integer(); }

  mstch::node isContainerOfString() {
    return is_list_of_string() || is_set_of_string();
  }

  mstch::node cythonTypeNoneable() { return !is_number() && has_cython_type(); }

  mstch::node hasCythonType() { return has_cython_type(); }

  mstch::node isIOBuf() { return is_iobuf(); }

  mstch::node isIOBufRef() { return is_iobuf_ref(); }

  mstch::node isIOBufWrapper() { return is_iobuf() || is_iobuf_ref(); }

  mstch::node isFlexibleBinary() { return is_flexible_binary(); }

  mstch::node hasCustomTypeBehavior() { return has_custom_type_behavior(); }

  mstch::node isSimple() {
    return (type_->is_base_type() || type_->is_enum()) &&
        !has_custom_type_behavior();
  }

  mstch::node resolves_to_complex_return() {
    return resolved_type_->is_container() ||
        resolved_type_->is_string_or_binary() || resolved_type_->is_struct() ||
        resolved_type_->is_xception();
  }

  const std::string& get_flat_name() const { return cachedProps_.flatName; }

  void set_flat_name(std::string extra) {
    std::string custom_prefix;
    if (!is_default_template()) {
      custom_prefix = to_cython_template() + "__";
    } else {
      if (cachedProps_.cppType != "") {
        custom_prefix = to_cython_type() + "__";
      }
    }
    const t_program* typeProgram = type_->program();
    if (typeProgram && typeProgram != prog_) {
      custom_prefix += typeProgram->name() + "_";
    }
    custom_prefix += extra;
    cachedProps_.flatName = std::move(custom_prefix);
  }

  bool is_default_template() const {
    return (!type_->is_container() && cachedProps_.cppTemplate == "") ||
        (type_->is_list() && cachedProps_.cppTemplate == "std::vector") ||
        (type_->is_set() && cachedProps_.cppTemplate == "std::set") ||
        (type_->is_map() && cachedProps_.cppTemplate == "std::map");
  }

  bool has_custom_cpp_type() const { return cachedProps_.cppType != ""; }

 protected:
  const t_program* get_type_program() const {
    if (const t_program* p = type_->program()) {
      return p;
    }
    return prog_;
  }

  std::vector<std::string> get_type_py3_namespace() const {
    auto ns = get_py3_namespace_with_name(get_type_program());
    ns.push_back("types");
    return ns;
  }

  std::vector<std::string> get_type_cpp2_namespace() const {
    return cpp2::get_gen_namespace_components(*get_type_program());
  }

  std::string to_cython_template() const {
    // handle special built-ins first:
    if (cachedProps_.cppTemplate == "std::vector") {
      return "vector";
    } else if (cachedProps_.cppTemplate == "std::set") {
      return "cset";
    } else if (cachedProps_.cppTemplate == "std::map") {
      return "cmap";
    }
    // then default handling:
    return boost::algorithm::replace_all_copy(
        cachedProps_.cppTemplate, "::", "_");
  }

  std::string to_cython_type() const {
    if (cachedProps_.cppType == "") {
      return "";
    }
    std::string cython_type = cachedProps_.cppType;
    boost::algorithm::replace_all(cython_type, "::", "_");
    boost::algorithm::replace_all(cython_type, "<", "_");
    boost::algorithm::replace_all(cython_type, ">", "");
    boost::algorithm::replace_all(cython_type, " ", "");
    boost::algorithm::replace_all(cython_type, ", ", "_");
    boost::algorithm::replace_all(cython_type, ",", "_");
    return cython_type;
  }

  bool is_integer() const { return type_->is_any_int() || type_->is_byte(); }

  bool is_number() const { return is_integer() || type_->is_floating_point(); }

  bool is_list_of_string() {
    if (!type_->is_list()) {
      return false;
    }
    return get_list_elem_type(*type_)->is_string_or_binary();
  }

  bool is_set_of_string() {
    if (!type_->is_set()) {
      return false;
    }
    return get_set_elem_type(*type_)->is_string_or_binary();
  }

  bool has_cython_type() const { return !type_->is_container(); }

  bool is_iobuf() const { return cachedProps_.cppType == "folly::IOBuf"; }

  bool is_iobuf_ref() const {
    return cachedProps_.cppType == "std::unique_ptr<folly::IOBuf>";
  }

  bool is_flexible_binary() const {
    return type_->is_binary() && has_custom_cpp_type() && !is_iobuf() &&
        !is_iobuf_ref() &&
        // We know that folly::fbstring is completely substitutable for
        // std::string and it's a common-enough type to special-case:
        cachedProps_.cppType != "folly::fbstring" &&
        cachedProps_.cppType != "::folly::fbstring";
  }

  bool has_custom_type_behavior() const {
    return is_iobuf() || is_iobuf_ref() || is_flexible_binary();
  }

  const t_program* prog_;
  CachedProperties& cachedProps_;
};

template <bool ForContainers = false>
class type_py3_generator : public type_generator {
 public:
  explicit type_py3_generator(const t_program* prog) : prog_{prog} {}
  std::shared_ptr<mstch_base> generate(
      const t_type* type,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index) const override;

 protected:
  const t_program* prog_;
};

class mstch_py3_program : public mstch_program {
 public:
  mstch_py3_program(
      const t_program* program,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_program{program, generators, cache, pos} {
    register_methods(
        this,
        {
            {"program:unique_functions_by_return_type",
             &mstch_py3_program::unique_functions_by_return_type},
            {"program:cppNamespaces", &mstch_py3_program::getCpp2Namespace},
            {"program:py3Namespaces", &mstch_py3_program::getPy3Namespace},
            {"program:hasServiceFunctions?",
             &mstch_py3_program::hasServiceFunctions},
            {"program:includeNamespaces",
             &mstch_py3_program::includeNamespaces},
            {"program:cppIncludes", &mstch_py3_program::getCppIncludes},
            {"program:containerTypes", &mstch_py3_program::getContainerTypes},
            {"program:customTemplates", &mstch_py3_program::getCustomTemplates},
            {"program:customTypes", &mstch_py3_program::getCustomTypes},
            {"program:moveContainerTypes",
             &mstch_py3_program::getMoveContainerTypes},
            {"program:has_stream?", &mstch_py3_program::hasStream},
            {"program:stream_types", &mstch_py3_program::getStreamTypes},
            {"program:response_and_stream_types",
             &mstch_py3_program::getResponseAndStreamTypes},
            {"program:stream_exceptions",
             &mstch_py3_program::getStreamExceptions},
            {"program:cpp_gen_path", &mstch_py3_program::getCppGenPath},
        });
    gather_included_program_namespaces();
    visit_types_for_services_and_interactions();
    visit_types_for_objects();
    visit_types_for_constants();
    visit_types_for_typedefs();
    visit_types_for_mixin_fields();
  }

  mstch::node getCppGenPath() {
    return std::string(has_option("py3cpp") ? "gen-py3cpp" : "gen-cpp2");
  }

  mstch::node getContainerTypes() {
    type_py3_generator<true> generator{program_};
    return generate_elements(containers_, &generator);
  }

  mstch::node getCppIncludes() {
    mstch::array a;
    for (const auto& include : program_->cpp_includes()) {
      a.push_back(include);
    }
    return a;
  }
  mstch::node unique_functions_by_return_type() {
    std::vector<const t_function*> functions;
    bool no_stream = has_option("no_stream");
    for (auto& kv : uniqueFunctionsByReturnType_) {
      if (is_func_supported(no_stream, kv.second)) {
        functions.push_back(kv.second);
      }
    }

    return generate_functions(functions);
  }

  mstch::node getCustomTemplates() {
    return generate_types_array(customTemplates_);
  }

  mstch::node getCustomTypes() { return generate_types_array(customTypes_); }

  mstch::node getMoveContainerTypes() {
    std::vector<const t_type*> types;
    for (auto& it : moveContainers_) {
      types.push_back(it.second);
    }
    return generate_types_array(types);
  }

  mstch::node getResponseAndStreamTypes() {
    return generate_types_array(responseAndStreamTypes_);
  }

  mstch::node getStreamExceptions() {
    std::vector<const t_type*> types;
    for (auto& it : streamExceptions_) {
      types.push_back(it.second);
    }
    return generate_types_array(types);
  }

  mstch::node getStreamTypes() {
    std::vector<const t_type*> types;
    if (!has_option("no_stream")) {
      for (auto& it : streamTypes_) {
        types.push_back(it.second);
      }
    }
    return generate_types_array(types);
  }

  mstch::node includeNamespaces() {
    mstch::array a;
    for (auto& it : includeNamespaces_) {
      a.push_back(mstch::map{
          {"includeNamespace", createStringArray(it.second.ns)},
          {"hasServices?", it.second.hasServices},
          {"hasTypes?", it.second.hasTypes}});
    }
    return a;
  }

  mstch::node getCpp2Namespace() {
    return createStringArray(cpp2::get_gen_namespace_components(*program_));
  }

  mstch::node getPy3Namespace() {
    return createStringArray(get_py3_namespace(program_));
  }

  mstch::node hasServiceFunctions() {
    const auto& services = program_->services();
    return std::any_of(services.begin(), services.end(), [](const auto& s) {
      // TODO(ffrancet): This is here because service interaction functions
      // aren't supported yet and won't be generated, so this shouldn't include
      // them
      const auto& functions = s->get_functions();
      return std::any_of(functions.begin(), functions.end(), [](const auto& f) {
        return !f->get_returntype()->is_service();
      });
    });
  }

  mstch::node hasStream() {
    return !has_option("no_stream") && !streamTypes_.empty();
  }

 protected:
  struct Namespace {
    std::vector<std::string> ns;
    bool hasServices;
    bool hasTypes;
  };

  mstch::array generate_types_array(const std::vector<const t_type*>& types) {
    return generate_types(types);
  }

  void gather_included_program_namespaces() {
    for (const t_program* included_program :
         program_->get_included_programs()) {
      bool hasTypes =
          !(included_program->objects().empty() &&
            included_program->enums().empty() &&
            included_program->typedefs().empty() &&
            included_program->consts().empty());
      includeNamespaces_[included_program->path()] = Namespace{
          get_py3_namespace_with_name(included_program),
          !included_program->services().empty(),
          hasTypes,
      };
    }
  }

  void add_typedef_namespace(const t_type* type) {
    auto prog = type->program();
    if (prog && prog != program_) {
      const auto& path = prog->path();
      if (includeNamespaces_.find(path) != includeNamespaces_.end()) {
        return;
      }
      auto ns = Namespace();
      ns.ns = get_py3_namespace_with_name(prog);
      ns.hasServices = false;
      ns.hasTypes = true;
      includeNamespaces_[path] = std::move(ns);
    }
  }

  void visit_type_single_service(const t_service* service) {
    for (const auto& function : service->functions()) {
      for (const auto& field : function.get_paramlist()->fields()) {
        visit_type(field.get_type());
      }
      for (const auto& field : function.get_stream_xceptions()->fields()) {
        const t_type* exType = field.get_type();
        streamExceptions_.emplace(visit_type(field.get_type()), exType);
      }
      const t_type* return_type = function.get_returntype();
      auto sa = cpp2::is_stack_arguments(cache_->parsed_options_, function);
      uniqueFunctionsByReturnType_.insert(
          {{visit_type(return_type), sa}, &function});
    }
  }

  void visit_types_for_services_and_interactions() {
    for (const auto* service : program_->services()) {
      visit_type_single_service(service);
    }
    for (const auto* interaction : program_->interactions()) {
      visit_type_single_service(interaction);
    }
  }

  void visit_types_for_objects() {
    for (const auto& object : program_->objects()) {
      for (auto&& field : object->fields()) {
        visit_type(field.get_type());
      }
    }
  }

  void visit_types_for_constants() {
    for (const auto& constant : program_->consts()) {
      visit_type(constant->get_type());
    }
  }

  void visit_types_for_typedefs() {
    for (const auto typedef_def : program_->typedefs()) {
      visit_type(typedef_def->get_type());
    }
  }

  void visit_types_for_mixin_fields() {
    for (const auto& strct : program_->structs()) {
      for (const auto& m : cpp2::get_mixins_and_members(*strct)) {
        visit_type(m.member->get_type());
      }
    }
  }

  enum TypeDef { NoTypedef, HasTypedef };

  std::string visit_type(const t_type* orig_type) {
    return visit_type_with_typedef(orig_type, TypeDef::NoTypedef);
  }

  std::string visit_type_with_typedef(
      const t_type* orig_type, TypeDef isTypedef) {
    auto trueType = orig_type->get_true_type();
    auto baseType =
        generators_->type_generator_->generate(trueType, generators_, cache_);
    mstch_py3_type* type = dynamic_cast<mstch_py3_type*>(baseType.get());
    const std::string& flatName = type->get_flat_name();
    // Import all types either beneath a typedef, even if the current type is
    // not directly a typedef
    isTypedef = isTypedef == TypeDef::HasTypedef || orig_type->is_typedef()
        ? TypeDef::HasTypedef
        : TypeDef::NoTypedef;
    if (flatName.empty()) {
      std::string extra;
      if (trueType->is_list()) {
        extra = "List__" +
            visit_type_with_typedef(get_list_elem_type(*trueType), isTypedef);
      } else if (trueType->is_set()) {
        extra = "Set__" +
            visit_type_with_typedef(get_set_elem_type(*trueType), isTypedef);
      } else if (trueType->is_map()) {
        extra = "Map__" +
            visit_type_with_typedef(get_map_key_type(*trueType), isTypedef) +
            "_" +
            visit_type_with_typedef(get_map_val_type(*trueType), isTypedef);
      } else if (trueType->is_binary()) {
        extra = "binary";
      } else if (trueType->is_streamresponse()) {
        const t_type* respType = get_stream_first_response_type(*trueType);
        const t_type* elemType = get_stream_elem_type(*trueType);
        if (respType) {
          extra += "ResponseAndStream__" +
              visit_type_with_typedef(respType, isTypedef) + "_";
        } else {
          extra = "Stream__";
        }
        const auto& elemTypeName = visit_type_with_typedef(elemType, isTypedef);
        extra += elemTypeName;
        streamTypes_.emplace(elemTypeName, elemType);
      } else if (trueType->is_sink()) {
        return "";
      } else {
        extra = trueType->get_name();
      }
      type->set_flat_name(std::move(extra));
    }
    assert(!flatName.empty());
    // If this type or a parent of this type is a typedef,
    // then add the namespace of the *resolved* type:
    // (parent matters if you have eg. typedef list<list<type>>)
    if (isTypedef == TypeDef::HasTypedef) {
      add_typedef_namespace(trueType);
    }
    bool inserted = seenTypeNames_.insert(flatName).second;
    if (inserted) {
      if (trueType->is_container()) {
        containers_.push_back(trueType);
        moveContainers_.emplace(
            boost::algorithm::replace_all_copy(flatName, "binary", "string"),
            trueType);
      }
      if (!type->is_default_template()) {
        customTemplates_.push_back(trueType);
      }
      if (type->has_custom_cpp_type()) {
        customTypes_.push_back(trueType);
      }
      if (trueType->is_streamresponse() &&
          get_stream_first_response_type(*trueType)) {
        responseAndStreamTypes_.push_back(trueType);
      }
    }
    return flatName;
  }

  std::vector<const t_type*> containers_;
  std::map<std::string, const t_type*> moveContainers_;
  std::vector<const t_type*> customTemplates_;
  std::vector<const t_type*> customTypes_;
  std::unordered_set<std::string> seenTypeNames_;
  std::map<std::string, Namespace> includeNamespaces_;
  std::map<std::tuple<std::string, bool>, t_function const*>
      uniqueFunctionsByReturnType_;
  std::vector<const t_type*> responseAndStreamTypes_;
  std::map<std::string, const t_type*> streamTypes_;
  std::map<std::string, const t_type*> streamExceptions_;
};

class mstch_py3_function : public mstch_function {
 public:
  mstch_py3_function(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_function(function, generators, cache, pos),
        cppName_{cpp2::get_name(function)} {
    register_methods(
        this,
        {{"function:eb", &mstch_py3_function::event_based},
         {"function:stack_arguments?", &mstch_py3_function::stack_arguments},
         {"function:cppName", &mstch_py3_function::cppName}});
  }

  mstch::node cppName() { return cppName_; }

  mstch::node event_based() {
    return function_->get_annotation("thread") == "eb";
  }

  mstch::node stack_arguments() {
    return cpp2::is_stack_arguments(cache_->parsed_options_, *function_);
  }

 protected:
  const std::string cppName_;
};

class mstch_py3_service : public mstch_service {
 public:
  mstch_py3_service(
      const t_service* service,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const t_program* prog)
      : mstch_service(service, generators, cache, pos), prog_{prog} {
    register_methods(
        this,
        {
            {"service:externalProgram?", &mstch_py3_service::isExternalProgram},
            {"service:cppNamespaces", &mstch_py3_service::cppNamespaces},
            {"service:py3Namespaces", &mstch_py3_service::py3Namespaces},
            {"service:programName", &mstch_py3_service::programName},
            {"service:includePrefix", &mstch_py3_service::includePrefix},
            {"service:parent_service_name",
             &mstch_py3_service::parent_service_name},
            {"service:supportedFunctions",
             &mstch_py3_service::get_supported_functions},
        });
  }

  mstch::node isExternalProgram() { return prog_ != service_->program(); }

  mstch::node cppNamespaces() {
    return createStringArray(
        cpp2::get_gen_namespace_components(*service_->program()));
  }

  mstch::node py3Namespaces() {
    return createStringArray(
        split_namespace(service_->program()->get_namespace("py3")));
  }

  mstch::node programName() { return service_->program()->name(); }

  mstch::node includePrefix() { return service_->program()->include_prefix(); }

  mstch::node parent_service_name() {
    return cache_->parsed_options_.at("parent_service_name");
  }

  mstch::node get_supported_functions() {
    std::vector<t_function*> funcs;
    bool no_stream = has_option("no_stream");
    for (auto func : service_->get_functions()) {
      if (is_func_supported(no_stream, func)) {
        funcs.push_back(func);
      }
    }
    return generate_functions(funcs);
  }

 protected:
  const t_program* prog_;
};

class mstch_py3_field : public mstch_field {
 public:
  enum class RefType : uint8_t {
    NotRef,
    Unique,
    Shared,
    SharedConst,
    IOBuf,
  };
  mstch_py3_field(
      const t_field* field,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index,
      field_generator_context const* field_context)
      : mstch_field(field, generators, cache, pos, index, field_context),
        pyName_{py3::get_py3_name(*field)},
        cppName_{cpp2::get_name(field)} {
    register_methods(
        this,
        {
            {"field:py_name", &mstch_py3_field::pyName},
            {"field:reference?", &mstch_py3_field::isRef},
            {"field:unique_ref?", &mstch_py3_field::isUniqueRef},
            {"field:shared_ref?", &mstch_py3_field::isSharedRef},
            {"field:shared_const_ref?", &mstch_py3_field::isSharedConstRef},
            {"field:iobuf_ref?", &mstch_py3_field::isIOBufRef},
            {"field:has_ref_accessor?", &mstch_py3_field::hasRefAccessor},
            {"field:hasDefaultValue?", &mstch_py3_field::hasDefaultValue},
            {"field:PEP484Optional?", &mstch_py3_field::isPEP484Optional},
            {"field:isset?", &mstch_py3_field::isSet},
            {"field:cppName", &mstch_py3_field::cppName},
            {"field:hasModifiedName?", &mstch_py3_field::hasModifiedName},
            {"field:hasPyName?", &mstch_py3_field::hasPyName},
            {"field:boxed_ref?", &mstch_py3_field::boxed_ref},
        });
  }

  mstch::node isRef() { return is_ref(); }

  mstch::node isUniqueRef() { return get_ref_type() == RefType::Unique; }

  mstch::node isSharedRef() { return get_ref_type() == RefType::Shared; }

  mstch::node isSharedConstRef() {
    return get_ref_type() == RefType::SharedConst;
  }

  mstch::node isIOBufRef() { return get_ref_type() == RefType::IOBuf; }

  mstch::node hasRefAccessor() {
    auto ref_type = get_ref_type();
    return (ref_type == RefType::NotRef || ref_type == RefType::IOBuf);
  }

  mstch::node hasDefaultValue() { return has_default_value(); }

  mstch::node isPEP484Optional() { return !has_default_value(); }

  mstch::node isSet() {
    auto ref_type = get_ref_type();
    return (ref_type == RefType::NotRef || ref_type == RefType::IOBuf) &&
        field_->get_req() != t_field::e_req::required;
  }

  mstch::node pyName() { return pyName_; }
  mstch::node cppName() { return cppName_; }
  mstch::node hasModifiedName() { return pyName_ != cppName_; }
  mstch::node hasPyName() { return pyName_ != field_->get_name(); }

  bool has_default_value() {
    return !is_ref() && (field_->get_value() != nullptr || !is_optional());
  }

  mstch::node boxed_ref() {
    return gen::cpp::find_ref_type(*field_) == gen::cpp::reference_type::boxed;
  }

 protected:
  RefType get_ref_type() {
    if (ref_type_cached_) {
      return ref_type_;
    }
    ref_type_cached_ = true;
    switch (gen::cpp::find_ref_type(*field_)) {
      case gen::cpp::reference_type::unique: {
        return ref_type_ = RefType::Unique;
      }
      case gen::cpp::reference_type::shared_const: {
        return ref_type_ = RefType::SharedConst;
      }
      case gen::cpp::reference_type::shared_mutable: {
        return ref_type_ = RefType::Shared;
      }
      case gen::cpp::reference_type::boxed: {
        return ref_type_ = RefType::NotRef;
      }
      case gen::cpp::reference_type::none: {
        const t_type* resolved_type = field_->get_type()->get_true_type();
        if (cpp2::get_type(resolved_type) == "std::unique_ptr<folly::IOBuf>") {
          return ref_type_ = RefType::IOBuf;
        }
        return ref_type_ = RefType::NotRef;
      }
      case gen::cpp::reference_type::unrecognized: {
        // It is legal to get here but hopefully nobody will in practice, since
        // we're not set up to handle other kinds of refs:
        throw std::runtime_error{"Unrecognized ref_type"};
      }
    }
    // Suppress "control reaches end of non-void function" warning
    throw std::logic_error{"Unhandled ref_type"};
  }

  bool is_optional() const {
    return field_->get_req() == t_field::e_req::optional;
  }

  bool is_ref() { return get_ref_type() != RefType::NotRef; }

  RefType ref_type_{RefType::NotRef};
  bool ref_type_cached_ = false;
  const std::string pyName_;
  const std::string cppName_;
};

class mstch_py3_struct : public mstch_struct {
 public:
  mstch_py3_struct(
      const t_struct* strct,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_struct(strct, generators, cache, pos) {
    register_methods(
        this,
        {
            {"struct:size", &mstch_py3_struct::getSize},
            {"struct:is_struct_orderable?",
             &mstch_py3_struct::isStructOrderable},
            {"struct:cpp_noncomparable", &mstch_py3_struct::cppNonComparable},
            {"struct:cpp_noncopyable?", &mstch_py3_struct::cppNonCopyable},
            {"struct:exception_message?",
             &mstch_py3_struct::hasExceptionMessage},
            {"struct:exception_message", &mstch_py3_struct::exceptionMessage},
            {"struct:fields_and_mixin_fields",
             &mstch_py3_struct::fields_and_mixin_fields},
            {"struct:py3_fields", &mstch_py3_struct::py3_fields},
            {"struct:py3_fields?", &mstch_py3_struct::has_py3_fields},
        });
    py3_fields_ = strct_->fields().copy();
    py3_fields_.erase(
        std::remove_if(
            py3_fields_.begin(),
            py3_fields_.end(),
            [](t_field const* field) {
              return field->has_annotation("py3.hidden");
            }),
        py3_fields_.end());
  }

  mstch::node getSize() { return std::to_string(py3_fields_.size()); }

  mstch::node isStructOrderable() {
    return cpp2::is_orderable(*strct_) &&
        !strct_->has_annotation("no_default_comparators");
  }

  mstch::node cppNonComparable() {
    return strct_->has_annotation({"cpp.noncomparable", "cpp2.noncomparable"});
  }

  mstch::node cppNonCopyable() {
    return strct_->has_annotation({"cpp.noncopyable", "cpp2.noncopyable"});
  }

  mstch::node hasExceptionMessage() {
    return strct_->has_annotation("message");
  }

  mstch::node exceptionMessage() { return strct_->get_annotation("message"); }

  mstch::node py3_fields() { return generate_fields(py3_fields_); }

  mstch::node has_py3_fields() { return !py3_fields_.empty(); }

  mstch::node fields_and_mixin_fields() {
    std::vector<t_field const*> fields = py3_fields_;
    for (auto m : cpp2::get_mixins_and_members(*strct_)) {
      fields.push_back(m.member);
    }
    return generate_fields(fields);
  }

 private:
  std::vector<t_field const*> py3_fields_;
};

class mstch_py3_enum : public mstch_enum {
 public:
  mstch_py3_enum(
      const t_enum* enm,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum(enm, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enum:flags?", &mstch_py3_enum::hasFlags},
        });
  }

  mstch::node hasFlags() { return enm_->has_annotation("py3.flags"); }
};

class mstch_py3_enum_value : public mstch_enum_value {
 public:
  mstch_py3_enum_value(
      const t_enum_value* enm_value,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum_value(enm_value, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enum_value:py_name", &mstch_py3_enum_value::pyName},
            {"enum_value:cppName", &mstch_py3_enum_value::cppName},
            {"enum_value:hasPyName?", &mstch_py3_enum_value::hasPyName},
        });
  }

  mstch::node pyName() { return py3::get_py3_name(*enm_value_); }

  mstch::node cppName() { return cpp2::get_name(enm_value_); }

  mstch::node hasPyName() {
    return py3::get_py3_name(*enm_value_) != enm_value_->get_name();
  }
};

class mstch_py3_container_type : public mstch_py3_type {
 public:
  mstch_py3_container_type(
      const t_type* type,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const t_program* prog,
      CachedProperties& cachedProps)
      : mstch_py3_type(type, generators, cache, pos, prog, cachedProps) {
    register_methods(
        this,
        {
            {"containerType:flat_name",
             &mstch_py3_container_type::containerTypeFlatName},

        });
  }

  mstch::node containerTypeFlatName() {
    assert(type_->is_container());
    return cachedProps_.flatName;
  }
};

class mstch_py3_annotation : public mstch_annotation {
 public:
  mstch_py3_annotation(
      const t_annotation& annotation,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_annotation(
            annotation.first,
            annotation.second,
            generators,
            cache,
            pos,
            index) {
    register_methods(
        this,
        {
            {"annotation:value?", &mstch_py3_annotation::hasValue},
            {"annotation:py_quoted_key", &mstch_py3_annotation::pyQuotedKey},
            {"annotation:py_quoted_value",
             &mstch_py3_annotation::pyQuotedValue},
        });
  }

  mstch::node hasValue() { return !val_.value.empty(); }

  mstch::node pyQuotedKey() { return to_python_string_literal(key_); }

  mstch::node pyQuotedValue() { return to_python_string_literal(val_.value); }

 protected:
  std::string to_python_string_literal(std::string val) const {
    std::string quotes = "\"\"\"";
    boost::algorithm::replace_all(val, "\\", "\\\\");
    boost::algorithm::replace_all(val, "\"", "\\\"");
    return quotes + val + quotes;
  }
};

class program_py3_generator : public program_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_program* program,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    const std::string& id = program->path();
    auto it = cache->programs_.find(id);
    if (it != cache->programs_.end()) {
      return it->second;
    }
    auto r = cache->programs_.emplace(
        id,
        std::make_shared<mstch_py3_program>(program, generators, cache, pos));
    return r.first->second;
  }
  std::unordered_map<const t_type*, mstch_py3_type::CachedProperties>
      typePropsCache;
};

class struct_py3_generator : public struct_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_struct* strct,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_py3_struct>(strct, generators, cache, pos);
  }
};

class function_py3_generator : public function_generator {
 public:
  function_py3_generator() = default;
  ~function_py3_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_py3_function>(
        function, generators, cache, pos);
  }
};

class service_py3_generator : public service_generator {
 public:
  explicit service_py3_generator(const t_program* prog) : prog_{prog} {}
  std::shared_ptr<mstch_base> generate(
      const t_service* service,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_py3_service>(
        service, generators, cache, pos, prog_);
  }

 protected:
  const t_program* prog_;
};

class field_py3_generator : public field_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_field* field,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      field_generator_context const* field_context) const override {
    return std::make_shared<mstch_py3_field>(
        field, generators, cache, pos, index, field_context);
  }
};

class enum_py3_generator : public enum_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_enum* enm,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_py3_enum>(enm, generators, cache, pos);
  }
};

class enum_value_py3_generator : public enum_value_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_enum_value* enm_value,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_py3_enum_value>(
        enm_value, generators, cache, pos);
  }
};

class annotation_py3_generator : public annotation_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_annotation& annotation,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index) const override {
    return std::make_shared<mstch_py3_annotation>(
        annotation, generators, cache, pos, index);
  }
};

/**
 * Generator-specific validator that enforces that reserved key is
 * not used as namespace field name.
 *
 */
class no_reserved_key_in_namespace_validator : virtual public validator {
 public:
  using validator::visit;

  bool visit(t_program* const prog) override {
    set_program(prog);
    validate(prog);
    return true;
  }

 private:
  void validate(t_program* const prog) {
    static const std::unordered_set<std::string> py3_reserved_keys = {
        "include",
    };

    const auto& py3_namespace = prog->get_namespace("py3");
    if (py3_namespace.empty()) {
      return;
    }

    std::vector<std::string> namespace_tokens = split_namespace(py3_namespace);
    for (const auto& field_name : namespace_tokens) {
      if (py3_reserved_keys.find(field_name) != py3_reserved_keys.end()) {
        std::ostringstream ss;
        ss << "Namespace '" << py3_namespace << "' contains reserved keyword '"
           << field_name << "'";
        add_error(boost::none, ss.str());
      }
    }

    std::string filepath_delimiters("\\/.");
    std::vector<std::string> fields;
    boost::split(fields, prog->path(), boost::is_any_of(filepath_delimiters));
    for (const auto& field : fields) {
      if (py3_reserved_keys.find(field) != py3_reserved_keys.end()) {
        std::ostringstream ss;
        ss << "Path '" << prog->path() << "' contains reserved keyword '"
           << field << "'";
        add_error(boost::none, ss.str());
      }
    }
  }
};

/**
 * Generator-specific validator that enforces "name" and "value" is not used as
 * enum member or union field names (thrift-py3)
 */
class enum_member_union_field_names_validator : virtual public validator {
 public:
  using validator::visit;

  bool visit(t_enum* enm) override {
    for (const t_enum_value* ev : enm->get_enum_values()) {
      validate(ev, ev->get_name());
    }
    return true;
  }

  bool visit(t_struct* s) override {
    if (!s->is_union()) {
      return false;
    }
    for (const t_field& f : s->fields()) {
      validate(&f, f.name());
    }
    return true;
  }

 private:
  void validate(const t_named* node, const std::string& name) {
    const auto& pyname = node->get_annotation("py3.name", &name);
    if (pyname == "name" || pyname == "value") {
      std::ostringstream ss;
      ss << "'" << pyname
         << "' should not be used as an enum/union field name in thrift-py3. "
         << "Use a different name or annotate the field with `(py3.name=\"<new_py_name>\")`";
      add_error(node->get_lineno(), ss.str());
    }
  }
};

class t_mstch_py3_generator : public t_mstch_generator {
 public:
  t_mstch_py3_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /* option_string unused */)
      : t_mstch_generator(program, std::move(context), "py3", parsed_options),
        generateRootPath_{package_to_path()} {
    out_dir_base_ = "gen-py3";
    auto include_prefix = get_option("include_prefix");
    if (!include_prefix.empty()) {
      program->set_include_prefix(std::move(include_prefix));
    }
  }

  void generate_program() override {
    set_mstch_generators();
    generate_init_files();
    generate_types();
    generate_services();
  }

  void fill_validator_list(validator_list& vl) const override {
    vl.add<no_reserved_key_in_namespace_validator>();
    vl.add<enum_member_union_field_names_validator>();
  }

  enum TypesFile { IsTypesFile, NotTypesFile };

 protected:
  bool should_resolve_typedefs() const override { return true; }
  void set_mstch_generators();
  void generate_init_files();
  void generate_file(
      const std::string& file,
      TypesFile is_types_file,
      const boost::filesystem::path& base);
  void generate_types();
  void generate_services();
  boost::filesystem::path package_to_path();

  const boost::filesystem::path generateRootPath_;
};

} // namespace

template <bool ForContainers>
std::shared_ptr<mstch_base> type_py3_generator<ForContainers>::generate(
    const t_type* type,
    std::shared_ptr<const mstch_generators> generators,
    std::shared_ptr<mstch_cache> cache,
    ELEMENT_POSITION pos,
    int32_t /*index*/) const {
  using T = std::
      conditional_t<ForContainers, mstch_py3_container_type, mstch_py3_type>;
  auto trueType = type->get_true_type();
  auto& propsCache =
      dynamic_cast<program_py3_generator*>(generators->program_generator_.get())
          ->typePropsCache;
  auto it = propsCache.find(trueType);
  if (it == propsCache.end()) {
    propsCache.emplace(
        trueType,
        mstch_py3_type::CachedProperties{
            get_cpp_template(*trueType), cpp2::get_type(trueType), {}});
  }
  return std::make_shared<T>(
      trueType, generators, cache, pos, prog_, propsCache.at(trueType));
}

void t_mstch_py3_generator::set_mstch_generators() {
  generators_->set_program_generator(std::make_unique<program_py3_generator>());
  generators_->set_struct_generator(std::make_unique<struct_py3_generator>());
  generators_->set_function_generator(
      std::make_unique<function_py3_generator>());
  generators_->set_service_generator(
      std::make_unique<service_py3_generator>(get_program()));
  generators_->set_field_generator(std::make_unique<field_py3_generator>());
  generators_->set_enum_generator(std::make_unique<enum_py3_generator>());
  generators_->set_enum_value_generator(
      std::make_unique<enum_value_py3_generator>());
  generators_->set_type_generator(
      std::make_unique<type_py3_generator<false>>(get_program()));
  generators_->set_annotation_generator(
      std::make_unique<annotation_py3_generator>());
}

void t_mstch_py3_generator::generate_init_files() {
  boost::filesystem::path p = generateRootPath_;
  auto nodePtr = generators_->program_generator_->generate(
      get_program(), generators_, cache_);
  while (!p.empty()) {
    render_to_file(nodePtr, "common/auto_generated_py", p / "__init__.py");
    p = p.parent_path();
  }
}

boost::filesystem::path t_mstch_py3_generator::package_to_path() {
  auto package = get_program()->get_namespace("py3");
  return boost::algorithm::replace_all_copy(package, ".", "/");
}

void t_mstch_py3_generator::generate_file(
    const std::string& file,
    TypesFile is_types_file,
    const boost::filesystem::path& base = {}) {
  auto program = get_program();
  const auto& name = program->name();
  if (is_types_file == IsTypesFile) {
    cache_->parsed_options_["is_types_file"] = "";
  } else {
    cache_->parsed_options_.erase("is_types_file");
  }
  auto nodePtr =
      generators_->program_generator_->generate(program, generators_, cache_);
  render_to_file(nodePtr, file, base / name / file);
}

void t_mstch_py3_generator::generate_types() {
  std::vector<std::string> cythonFilesWithTypeContext{
      "types.pyx",
      "types.pxd",
      "types.pyi",
  };

  std::vector<std::string> cythonFilesNoTypeContext{
      "types_reflection.pxd",
      "types_reflection.pyx",
      "types_fields.pxd",
      "types_fields.pyx",
      "builders.pxd",
      "builders.pyx",
      "builders.pyi",
      "metadata.pxd",
      "metadata.pyi",
      "metadata.pyx",
  };

  std::vector<std::string> cppFilesWithTypeContext{
      "types.h",
  };

  std::vector<std::string> cppFilesWithNoTypeContext{
      "metadata.h",
      "metadata.cpp",
  };

  for (const auto& file : cythonFilesWithTypeContext) {
    generate_file(file, IsTypesFile, generateRootPath_);
  }
  for (const auto& file : cppFilesWithTypeContext) {
    generate_file(file, IsTypesFile);
  }
  for (const auto& file : cythonFilesNoTypeContext) {
    generate_file(file, NotTypesFile, generateRootPath_);
  }
  for (const auto& file : cppFilesWithNoTypeContext) {
    generate_file(file, NotTypesFile);
  }
}

void t_mstch_py3_generator::generate_services() {
  if (get_program()->services().empty()) {
    // There is no need to generate empty / broken code for non existent
    // services.
    return;
  }

  std::vector<std::string> cythonFiles{
      "clients.pxd",
      "clients.pyx",
      "clients.pyi",
      "clients_wrapper.pxd",
      "services.pxd",
      "services.pyx",
      "services.pyi",
      "services_wrapper.pxd",
      "services_reflection.pxd",
      "services_reflection.pyx",
  };

  std::vector<std::string> cppFiles{
      "clients_wrapper.h",
      "clients_wrapper.cpp",
      "services_wrapper.h",
      "services_wrapper.cpp",
  };

  for (const auto& file : cythonFiles) {
    generate_file(file, NotTypesFile, generateRootPath_);
  }
  for (const auto& file : cppFiles) {
    generate_file(file, NotTypesFile);
  }
}

THRIFT_REGISTER_GENERATOR(
    mstch_py3,
    "Python 3",
    "    include_prefix:  Use full include paths in generated files.\n");

} // namespace compiler
} // namespace thrift
} // namespace apache
