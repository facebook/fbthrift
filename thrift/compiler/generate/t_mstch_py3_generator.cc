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
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>

#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>
#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/lib/py3/util.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

bool containsInParsedOptions(const mstch_cache* cache, const std::string& key) {
  const auto& parsed_options = cache->parsed_options_;
  return parsed_options.find(key) != parsed_options.end();
}

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

template <class T>
std::string get_cppname(const T& elem) {
  auto& annotation = elem.annotations_;
  auto it = annotation.find("cpp.name");
  if (it != annotation.end()) {
    return it->second;
  }
  return elem.get_name();
}

std::vector<std::string> get_py3_namespace(const t_program* prog) {
  return split_namespace(prog->get_namespace("py3"));
}

std::vector<std::string> get_py3_namespace_with_name(const t_program* prog) {
  auto ns = get_py3_namespace(prog);
  ns.push_back(prog->get_name());
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
  const auto& annotations = type.annotations_;
  auto it = annotations.find("cpp.template");
  if (it == annotations.end()) {
    it = annotations.find("cpp2.template");
  }
  if (it != annotations.end()) {
    return it->second;
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

class mstch_py3_type : public mstch_type {
 public:
  struct CachedProperties {
    const std::string cppTemplate;
    const std::string cppType;
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
    register_methods(
        this,
        {
            {"type:modulePath", &mstch_py3_type::modulePath},
            {"type:externalProgram?", &mstch_py3_type::isExternalProgram},
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

  mstch::node modulePath() {
    return createStringArray(get_type_py3_namespace());
  }

  mstch::node isExternalProgram() {
    auto p = type_->get_program();
    return p && p != prog_;
  }

  mstch::node flatName() {
    return cachedProps_.flatName;
  }

  mstch::node cppNamespaces() {
    return createStringArray(get_type_cpp2_namespace());
  }

  mstch::node cppTemplate() {
    return cachedProps_.cppTemplate;
  }

  mstch::node cythonTemplate() {
    return to_cython_template();
  }

  mstch::node isDefaultTemplate() {
    return is_default_template();
  }

  mstch::node cppType() {
    return cachedProps_.cppType;
  }

  mstch::node cythonType() {
    return to_cython_type();
  }

  mstch::node hasCustomType() {
    return has_custom_cpp_type();
  }

  mstch::node isNumber() {
    return is_number();
  }

  mstch::node isInteger() {
    return is_integer();
  }

  mstch::node isContainerOfString() {
    return is_list_of_string() || is_set_of_string();
  }

  mstch::node cythonTypeNoneable() {
    return !is_number() && has_cython_type();
  }

  mstch::node hasCythonType() {
    return has_cython_type();
  }

  mstch::node isIOBuf() {
    return is_iobuf();
  }

  mstch::node isIOBufRef() {
    return is_iobuf_ref();
  }

  mstch::node isIOBufWrapper() {
    return is_iobuf() || is_iobuf_ref();
  }

  mstch::node isFlexibleBinary() {
    return is_flexible_binary();
  }

  mstch::node hasCustomTypeBehavior() {
    return has_custom_type_behavior();
  }

  mstch::node isSimple() {
    return (type_->is_base_type() || type_->is_enum()) &&
        !has_custom_type_behavior();
  }

  mstch::node resolves_to_complex_return() {
    return resolved_type_->is_container() ||
        resolved_type_->is_string_or_binary() || resolved_type_->is_struct() ||
        resolved_type_->is_xception();
  }

  const std::string& get_flat_name() const {
    return cachedProps_.flatName;
  }

  void set_flat_name(std::string extra) {
    std::string custom_prefix;
    if (!is_default_template()) {
      custom_prefix = to_cython_template() + "__";
    } else {
      if (cachedProps_.cppType != "") {
        custom_prefix = to_cython_type() + "__";
      }
    }
    const t_program* typeProgram = type_->get_program();
    if (typeProgram && typeProgram != prog_) {
      custom_prefix += typeProgram->get_name() + "_";
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

  bool has_custom_cpp_type() const {
    return cachedProps_.cppType != "";
  }

 protected:
  const t_program* get_type_program() const {
    if (const t_program* p = type_->get_program()) {
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
    strip_comments(cython_type);
    boost::algorithm::replace_all(cython_type, "::", "_");
    boost::algorithm::replace_all(cython_type, "<", "_");
    boost::algorithm::replace_all(cython_type, ">", "");
    boost::algorithm::replace_all(cython_type, " ", "");
    boost::algorithm::replace_all(cython_type, ", ", "_");
    boost::algorithm::replace_all(cython_type, ",", "_");
    return cython_type;
  }

  bool is_integer() const {
    return type_->is_any_int() || type_->is_byte();
  }

  bool is_number() const {
    return is_integer() || type_->is_floating_point();
  }

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

  bool has_cython_type() const {
    return !type_->is_container();
  }

  bool is_iobuf() const {
    return cachedProps_.cppType == "folly::IOBuf";
  }

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
            {"program:typeContext?", &mstch_py3_program::isTypeContext},
            {"program:has_stream?", &mstch_py3_program::hasStream},
            {"program:stream_types", &mstch_py3_program::getStreamTypes},
            {"program:response_and_stream_types",
             &mstch_py3_program::getResponseAndStreamTypes},
            {"program:stream_exceptions",
             &mstch_py3_program::getStreamExceptions},
        });
    gather_included_program_namespaces();
    visit_types_for_services();
    visit_types_for_objects();
    visit_types_for_constants();
    visit_types_for_typedefs();
  }

  mstch::node getContainerTypes() {
    type_py3_generator<true> generator{program_};
    return generate_elements(containers_, &generator, generators_, cache_);
  }

  mstch::node getCppIncludes() {
    mstch::array a;
    for (const auto& include : program_->get_cpp_includes()) {
      a.push_back(include);
    }
    return a;
  }
  mstch::node unique_functions_by_return_type() {
    std::vector<const t_function*> functions;
    for (auto& kv : uniqueFunctionsByReturnType_) {
      functions.push_back(kv.second);
    }

    return generate_elements(
        functions, generators_->function_generator_.get(), generators_, cache_);
  }

  mstch::node getCustomTemplates() {
    return generate_types_array(customTemplates_);
  }

  mstch::node getCustomTypes() {
    return generate_types_array(customTypes_);
  }

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
    for (auto& it : streamTypes_) {
      types.push_back(it.second);
    }
    return generate_types_array(types);
  }

  mstch::node includeNamespaces() {
    mstch::array a;
    for (auto& it : includeNamespaces_) {
      a.push_back(
          mstch::map{{"includeNamespace", createStringArray(it.second.ns)},
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
    const auto& services = program_->get_services();
    return std::any_of(services.begin(), services.end(), [](const auto& s) {
      return !s->get_functions().empty();
    });
  }

  mstch::node hasStream() {
    return !streamTypes_.empty();
  }

  mstch::node isTypeContext() {
    return typeContext_;
  }

  void setTypeContext(bool val) {
    typeContext_ = val;
  }

 protected:
  struct Namespace {
    std::vector<std::string> ns;
    bool hasServices;
    bool hasTypes;
  };
  mstch::array generate_types_array(
      const std::vector<const t_type*>& types) const {
    return generate_elements(
        types, generators_->type_generator_.get(), generators_, cache_);
  }

  void gather_included_program_namespaces() {
    for (const t_program* included_program :
         program_->get_included_programs()) {
      bool hasTypes =
          !(included_program->get_objects().empty() &&
            included_program->get_enums().empty() &&
            included_program->get_typedefs().empty() &&
            included_program->get_consts().empty());
      includeNamespaces_[included_program->get_path()] = Namespace{
          get_py3_namespace_with_name(included_program),
          !included_program->get_services().empty(),
          hasTypes,
      };
    }
  }

  void add_typedef_namespace(const t_type* type) {
    auto prog = type->get_program();
    if (prog && prog != program_) {
      const auto& path = prog->get_path();
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

  void visit_types_for_services() {
    for (const auto service : program_->get_services()) {
      for (const auto function : service->get_functions()) {
        for (const auto field : function->get_arglist()->get_members()) {
          visit_type(field->get_type());
        }
        for (const auto field :
             function->get_stream_xceptions()->get_members()) {
          const t_type* exType = field->get_type();
          streamExceptions_.emplace(visit_type(field->get_type()), exType);
        }
        const t_type* returnType = function->get_returntype();
        auto sa = cpp2::is_stack_arguments(cache_->parsed_options_, *function);
        uniqueFunctionsByReturnType_.insert(
            {{visit_type(returnType), sa}, function});
      }
    }
  }

  void visit_types_for_objects() {
    for (const auto& object : program_->get_objects()) {
      for (const auto& field : object->get_members()) {
        visit_type(field->get_type());
      }
    }
  }

  void visit_types_for_constants() {
    for (const auto& constant : program_->get_consts()) {
      visit_type(constant->get_type());
    }
  }

  void visit_types_for_typedefs() {
    for (const auto typedef_def : program_->get_typedefs()) {
      visit_type(typedef_def->get_type());
    }
  }

  std::string visit_type(const t_type* orig_type) {
    auto trueType = orig_type->get_true_type();
    auto baseType =
        generators_->type_generator_->generate(trueType, generators_, cache_);
    mstch_py3_type* type = dynamic_cast<mstch_py3_type*>(baseType.get());
    const std::string& flatName = type->get_flat_name();
    if (flatName.empty()) {
      std::string extra;
      if (trueType->is_list()) {
        extra = "List__" + visit_type(get_list_elem_type(*trueType));
      } else if (trueType->is_set()) {
        extra = "Set__" + visit_type(get_set_elem_type(*trueType));
      } else if (trueType->is_map()) {
        extra = "Map__" + visit_type(get_map_key_type(*trueType)) + "_" +
            visit_type(get_map_val_type(*trueType));
      } else if (trueType->is_binary()) {
        extra = "binary";
      } else if (trueType->is_streamresponse()) {
        const t_type* respType = get_stream_first_response_type(*trueType);
        const t_type* elemType = get_stream_elem_type(*trueType);
        if (respType) {
          extra += "ResponseAndStream__" + visit_type(respType) + "_";
        } else {
          extra = "Stream__";
        }
        const auto& elemTypeName = visit_type(elemType);
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
    // If the original type is a typedef, then add the namespace of the
    // *resolved* type:
    if (orig_type->is_typedef()) {
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

  bool typeContext_ = false;
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
      : mstch_function(function, generators, cache, pos) {
    register_methods(
        this,
        {
            {"function:eb", &mstch_py3_function::event_based},
            {"function:stack_arguments?", &mstch_py3_function::stack_arguments},
        });
  }
  mstch::node event_based() {
    return function_->annotations_.count("thread") &&
        function_->annotations_.at("thread") == "eb";
  }
  mstch::node stack_arguments() {
    return cpp2::is_stack_arguments(cache_->parsed_options_, *function_);
  }
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
        });
  }

  mstch::node isExternalProgram() {
    return prog_ != service_->get_program();
  }

  mstch::node cppNamespaces() {
    return createStringArray(
        cpp2::get_gen_namespace_components(*service_->get_program()));
  }

  mstch::node py3Namespaces() {
    return createStringArray(
        split_namespace(service_->get_program()->get_namespace("py3")));
  }

  mstch::node programName() {
    return service_->get_program()->get_name();
  }

  mstch::node includePrefix() {
    return service_->get_program()->get_include_prefix();
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
      int32_t index)
      : mstch_field(field, generators, cache, pos, index),
        pyName_{py3::get_py3_name(*field)},
        cppName_{get_cppname<t_field>(*field)},
        hasOptionalsFlag_{containsInParsedOptions(cache.get(), "optionals")} {
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
            {"field:follyOptional?", &mstch_py3_field::isFollyOptional},
            {"field:PEP484Optional?", &mstch_py3_field::isPEP484Optional},
            {"field:isset?", &mstch_py3_field::isSet},
            {"field:cppName", &mstch_py3_field::cppName},
            {"field:hasModifiedName?", &mstch_py3_field::hasModifiedName},
            {"field:hasPyName?", &mstch_py3_field::hasPyName},
        });
  }

  mstch::node isRef() {
    return is_ref();
  }

  mstch::node isUniqueRef() {
    return get_ref_type() == RefType::Unique;
  }

  mstch::node isSharedRef() {
    return get_ref_type() == RefType::Shared;
  }

  mstch::node isSharedConstRef() {
    return get_ref_type() == RefType::SharedConst;
  }

  mstch::node isIOBufRef() {
    return get_ref_type() == RefType::IOBuf;
  }

  mstch::node hasRefAccessor() {
    return hasOptionalsFlag_ ? false : !is_required() && !is_ref();
  }

  mstch::node hasDefaultValue() {
    return has_default_value();
  }

  mstch::node isFollyOptional() {
    return is_folly_optional();
  }

  mstch::node isPEP484Optional() {
    return !has_default_value() || is_folly_optional();
  }

  mstch::node isSet() {
    auto ref_type = get_ref_type();
    return !hasOptionalsFlag_ &&
        (ref_type == RefType::NotRef || ref_type == RefType::IOBuf) &&
        !is_required();
  }

  mstch::node pyName() {
    return pyName_;
  }

  mstch::node cppName() {
    return cppName_;
  }

  mstch::node hasModifiedName() {
    return pyName_ != cppName_;
  }

  mstch::node hasPyName() {
    return pyName_ != field_->get_name();
  }

  bool has_default_value() {
    return !is_folly_optional() && !is_ref() &&
        (field_->get_value() != nullptr || !is_optional());
  }

  bool is_required() const {
    return field_->get_req() == t_field::e_req::T_REQUIRED;
  }

 protected:
  bool is_folly_optional() {
    return hasOptionalsFlag_ && is_optional();
  }

  RefType get_ref_type() {
    if (ref_type_cached_) {
      return ref_type_;
    }
    ref_type_cached_ = true;
    const std::map<std::string, std::string>& annotations =
        field_->annotations_;

    // backward compatibility with 'ref' annotation
    if (annotations.find("cpp.ref") != annotations.end() ||
        annotations.find("cpp2.ref") != annotations.end()) {
      return ref_type_ = RefType::Unique;
    }

    auto it = annotations.find("cpp.ref_type");
    if (it == annotations.end()) {
      it = annotations.find("cpp2.ref_type");
    }

    if (it == annotations.end() && field_->get_type() != nullptr) {
      const t_type* resolved_type = field_->get_type()->get_true_type();
      if (cpp2::get_cpp_type(resolved_type) ==
          "std::unique_ptr<folly::IOBuf>") {
        return ref_type_ = RefType::IOBuf;
      }
      return ref_type_ = RefType::NotRef;
    }

    const std::string& reftype = it->second;

    if (reftype == "unique" || reftype == "std::unique_ptr") {
      return ref_type_ = RefType::Unique;
    } else if (reftype == "shared" || reftype == "std::shared_ptr") {
      return ref_type_ = RefType::Shared;
    } else if (reftype == "shared_const") {
      return ref_type_ = RefType::SharedConst;
    } else {
      // It is legal to get here but hopefully nobody will in practice, since
      // we're not set up to handle other kinds of refs:
      std::ostringstream err;
      err << "Unhandled ref_type " << reftype;
      throw std::runtime_error{err.str()};
    }
  }
  bool is_optional() const {
    return field_->get_req() == t_field::e_req::T_OPTIONAL;
  }

  bool is_ref() {
    return get_ref_type() != RefType::NotRef;
  }

  RefType ref_type_{RefType::NotRef};
  bool ref_type_cached_ = false;
  const std::string pyName_;
  const std::string cppName_;
  bool hasOptionalsFlag_;
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
            {"struct:is_always_set?", &mstch_py3_struct::isAlwaysSet},
            {"struct:cpp_noncomparable", &mstch_py3_struct::cppNonComparable},
            {"struct:exception_message?",
             &mstch_py3_struct::hasExceptionMessage},
            {"struct:exception_message", &mstch_py3_struct::exceptionMessage},

        });
  }

  mstch::node getSize() {
    return std::to_string(strct_->get_members().size());
  }

  mstch::node isStructOrderable() {
    return cpp2::is_orderable(*strct_) &&
        strct_->annotations_.find("no_default_comparators") ==
        strct_->annotations_.end();
  }

  mstch::node isAlwaysSet() {
    const std::vector<t_field*>& members = strct_->get_members();
    return std::any_of(
        members.begin(), members.end(), [this](const auto* field) {
          mstch_py3_field f{field, generators_, cache_, pos_, 0};
          return f.has_default_value();
        });
  }

  mstch::node cppNonComparable() {
    return strct_->annotations_.find("cpp2.noncomparable") !=
        strct_->annotations_.end();
  }

  mstch::node hasExceptionMessage() {
    return strct_->annotations_.find("message") != strct_->annotations_.end();
  }

  mstch::node exceptionMessage() {
    const auto it = strct_->annotations_.find("message");
    if (it == strct_->annotations_.end()) {
      return {};
    }
    return it->second;
  }
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

  mstch::node hasFlags() {
    return enm_->annotations_.find("py3.flags") != enm_->annotations_.end();
  }
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
            {"enumValue:py_name", &mstch_py3_enum_value::pyName},
            {"enumValue:cppName", &mstch_py3_enum_value::cppName},
            {"enumValue:hasPyName?", &mstch_py3_enum_value::hasPyName},
        });
  }

  mstch::node pyName() {
    return py3::get_py3_name(*enm_value_);
  }

  mstch::node cppName() {
    return get_cppname<t_enum_value>(*enm_value_);
  }

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
            annotation.key,
            annotation.val,
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

  mstch::node hasValue() {
    return !val_.empty();
  }

  mstch::node pyQuotedKey() {
    return to_python_string_literal(key_);
  }

  mstch::node pyQuotedValue() {
    return to_python_string_literal(val_);
  }

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
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    const std::string& id = program->get_path();
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
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
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
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
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
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
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
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const override {
    return std::make_shared<mstch_py3_field>(
        field, generators, cache, pos, index);
  }
};

class enum_py3_generator : public enum_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_enum* enm,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_py3_enum>(enm, generators, cache, pos);
  }
};

class enum_value_py3_generator : public enum_value_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_enum_value* enm_value,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
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
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const override {
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
    boost::split(
        fields, prog->get_path(), boost::is_any_of(filepath_delimiters));
    for (const auto& field : fields) {
      if (py3_reserved_keys.find(field) != py3_reserved_keys.end()) {
        std::ostringstream ss;
        ss << "Path '" << prog->get_path() << "' contains reserved keyword '"
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
    for (const t_field* f : s->get_members()) {
      validate(f, f->get_name());
    }
    return true;
  }

 private:
  void validate(const t_annotated* node, const std::string& name) {
    const auto found = node->annotations_.find("py3.name");
    const auto& pyname =
        found == node->annotations_.end() ? name : found->second;
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
  enum class ModuleType {
    TYPES,
    CLIENTS,
    SERVICES,
    BUILDERS,
    REFLECTIONS,
  };

  t_mstch_py3_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /* option_string unused */)
      : t_mstch_generator(program, std::move(context), "py3", parsed_options),
        generateRootPath_{package_to_path()} {
    out_dir_base_ = "gen-py3";
    auto include_prefix = get_option("include_prefix");
    if (include_prefix && !include_prefix->empty()) {
      program->set_include_prefix(*include_prefix);
    }
  }

  void generate_program() override {
    set_mstch_generators();
    generate_init_files();
    generate_module(ModuleType::TYPES);
    generate_module(ModuleType::SERVICES);
    generate_module(ModuleType::CLIENTS);
    generate_module(ModuleType::BUILDERS);
    generate_module(ModuleType::REFLECTIONS);
  }

  void fill_validator_list(validator_list& vl) const override {
    vl.add<no_reserved_key_in_namespace_validator>();
    vl.add<enum_member_union_field_names_validator>();
  }

 protected:
  bool should_resolve_typedefs() const override {
    return true;
  }

  std::string get_module_name(ModuleType module) {
    switch (module) {
      case ModuleType::TYPES:
        return "types";
      case ModuleType::CLIENTS:
        return "clients";
      case ModuleType::SERVICES:
        return "services";
      case ModuleType::BUILDERS:
        return "builders";
      case ModuleType::REFLECTIONS:
        return "services_reflection";
    }
    // Should not happen
    assert(false);
    return {};
  }

  void set_mstch_generators();
  void generate_init_files();
  void generate_module(ModuleType moduleType);
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
            get_cpp_template(*trueType), cpp2::get_cpp_type(trueType), {}});
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
    render_to_file(nodePtr, "common/AutoGeneratedPy", p / "__init__.py");
    p = p.parent_path();
  }
}
boost::filesystem::path t_mstch_py3_generator::package_to_path() {
  auto package = get_program()->get_namespace("py3");
  return boost::algorithm::replace_all_copy(package, ".", "/");
}

void t_mstch_py3_generator::generate_module(ModuleType moduleType) {
  auto program = get_program();
  if (moduleType != ModuleType::TYPES && moduleType != ModuleType::BUILDERS &&
      program->get_services().empty()) {
    // There is no need to generate empty / broken code for non existent
    // services.
    return;
  }
  auto nodePtr = generators_->program_generator_->generate(
      get_program(), generators_, cache_);
  auto programNodePtr = std::static_pointer_cast<mstch_py3_program>(nodePtr);
  programNodePtr->setTypeContext(moduleType == ModuleType::TYPES);
  const auto& name = program->get_name();
  auto module = get_module_name(moduleType);

  std::vector<std::string> exts = {".pyx", ".pxd"};
  if (moduleType != ModuleType::REFLECTIONS) {
    exts.push_back(".pyi");
  }
  if (moduleType == ModuleType::TYPES) {
    exts.push_back(".h");
  }
  for (auto ext : exts) {
    render_to_file(
        nodePtr, module + ext, generateRootPath_ / name / (module + ext));
  }
  auto cpp_path = boost::filesystem::path{name};
  if (moduleType == ModuleType::CLIENTS || moduleType == ModuleType::SERVICES) {
    auto basename = module + "_wrapper";
    for (auto ext : {".h", ".cpp"}) {
      render_to_file(nodePtr, basename + ext, cpp_path / (basename + ext));
    }

    render_to_file(
        nodePtr,
        basename + ".pxd",
        generateRootPath_ / name / (basename + ".pxd"));
  }

  if (moduleType == ModuleType::TYPES) {
    render_to_file(nodePtr, "types.h", cpp_path / "types.h");
    programNodePtr->setTypeContext(false);
    for (auto ext : {".pxd", ".pyx"}) {
      const auto filename = std::string{"types_reflection"} + ext;
      render_to_file(nodePtr, filename, generateRootPath_ / name / filename);
    }
  }
}

THRIFT_REGISTER_GENERATOR(
    mstch_py3,
    "Python 3",
    "    include_prefix:  Use full include paths in generated files.\n");

} // namespace compiler
} // namespace thrift
} // namespace apache
