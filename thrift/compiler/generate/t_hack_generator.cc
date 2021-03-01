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

#include <stdlib.h>

#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <thrift/compiler/ast/base_types.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/generate/t_oop_generator.h>

using namespace std;

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Hack code generator.
 *
 */
class t_hack_generator : public t_oop_generator {
 public:
  t_hack_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/)
      : t_oop_generator(program, std::move(context)) {
    std::map<std::string, std::string>::const_iterator iter;

    json_ = option_is_specified(parsed_options, "json");
    phps_ = option_is_specified(parsed_options, "server");
    strict_types_ = option_is_specified(parsed_options, "stricttypes");
    arraysets_ = option_is_specified(parsed_options, "arraysets");
    no_nullables_ = option_is_specified(parsed_options, "nonullables");
    from_map_construct_ =
        option_is_specified(parsed_options, "frommap_construct");
    struct_trait_ = option_is_specified(parsed_options, "structtrait");
    shapes_ = option_is_specified(parsed_options, "shapes");
    shape_arraykeys_ = option_is_specified(parsed_options, "shape_arraykeys");
    shapes_allow_unknown_fields_ =
        option_is_specified(parsed_options, "shapes_allow_unknown_fields");
    array_migration_ = option_is_specified(parsed_options, "array_migration");
    arrays_ = option_is_specified(parsed_options, "arrays");
    no_use_hack_collections_ =
        option_is_specified(parsed_options, "no_use_hack_collections");
    nullable_everything_ =
        option_is_specified(parsed_options, "nullable_everything");
    const_collections_ =
        option_is_specified(parsed_options, "const_collections");
    enum_extratype_ = option_is_specified(parsed_options, "enum_extratype");
    enum_transparenttype_ =
        option_is_specified(parsed_options, "enum_transparenttype");
    soft_attribute_ = option_is_specified(parsed_options, "soft_attribute");
    arrprov_skip_frames_ =
        option_is_specified(parsed_options, "arrprov_skip_frames");
    protected_unions_ = option_is_specified(parsed_options, "protected_unions");
    mangled_services_ = option_is_set(parsed_options, "mangledsvcs", false);
    has_hack_namespace = !hack_namespace(program).empty();

    // no_use_hack_collections_ is only used to migrate away from php gen
    if (no_use_hack_collections_ && strict_types_) {
      throw std::runtime_error(
          "Don't use no_use_hack_collections with strict_types");
    } else if (no_use_hack_collections_ && !arraysets_) {
      throw std::runtime_error(
          "Don't use no_use_hack_collections without arraysets");
    } else if (no_use_hack_collections_ && arrays_) {
      throw std::runtime_error(
          "Don't use no_use_hack_collections with arrays. Just use arrays");
    } else if (mangled_services_ && has_hack_namespace) {
      throw std::runtime_error("Don't use mangledsvcs with hack namespaces");
    }

    out_dir_base_ = "gen-hack";
    array_keyword_ = array_migration_ ? "darray" : "dict";
  }

  /**
   * Init and close methods
   */

  void init_generator() override;
  void close_generator() override;

  /**
   * Program-level generation functions
   */

  void generate_typedef(const t_typedef* ttypedef) override;
  void generate_enum(const t_enum* tenum) override;
  void generate_const(const t_const* tconst) override;
  void generate_struct(const t_struct* tstruct) override;
  void generate_xception(const t_struct* txception) override;
  void generate_service(const t_service* tservice) override;

  std::string render_const_value(
      const t_type* type,
      const t_const_value* value,
      bool immutable_collections = false);
  std::string render_default_value(const t_type* type);

  /**
   * Metadata Types functions
   */

  /**
   * Keep synced with : thrift/lib/thrift/metadata.thrift
   */
  enum ThriftPrimitiveType {
    THRIFT_BOOL_TYPE = 1,
    THRIFT_BYTE_TYPE = 2,
    THRIFT_I16_TYPE = 3,
    THRIFT_I32_TYPE = 4,
    THRIFT_I64_TYPE = 5,
    THRIFT_FLOAT_TYPE = 6,
    THRIFT_DOUBLE_TYPE = 7,
    THRIFT_BINARY_TYPE = 8,
    THRIFT_STRING_TYPE = 9,
    THRIFT_VOID_TYPE = 10,
  };

  ThriftPrimitiveType base_to_t_primitive(t_base_type* base_type);

  std::unique_ptr<t_const_value> type_to_tmeta(const t_type* type);
  std::unique_ptr<t_const_value> field_to_tmeta(const t_field* field);
  std::unique_ptr<t_const_value> function_to_tmeta(const t_function* function);
  std::unique_ptr<t_const_value> service_to_tmeta(const t_service* service);

  void append_to_t_enum(
      t_enum* tenum,
      t_program* program,
      ThriftPrimitiveType value);

  const t_type* tmeta_ThriftType_type();
  const t_type* tmeta_ThriftField_type();
  const t_type* tmeta_ThriftFunction_type();
  const t_type* tmeta_ThriftService_type();

  /**
   * Structs!
   */

  void generate_php_struct(const t_struct* tstruct, bool is_exception);
  void generate_php_struct_definition(
      std::ofstream& out,
      const t_struct* tstruct,
      bool is_xception = false,
      bool is_result = false,
      bool is_args = false,
      const std::string& name = "");
  void _generate_php_struct_definition(
      std::ofstream& out,
      const t_struct* tstruct,
      bool is_xception,
      bool is_result,
      bool is_args,
      const std::string& name);
  void generate_php_function_helpers(
      const t_service* tservice,
      const t_function* tfunction);
  void generate_php_interaction_function_helpers(
      const t_service* tservice,
      const t_service* interaction,
      const t_function* tfunction);

  void generate_php_union_enum(
      std::ofstream& out,
      const t_struct* tstruct,
      const std::string& name);
  void generate_php_union_methods(std::ofstream& out, const t_struct* tstruct);

  void generate_php_struct_shape_spec(
      std::ofstream& out,
      const t_struct* tstruct,
      bool is_constructor_shape = false);
  void generate_php_struct_shape_collection_value_lambda(
      std::ostream& out,
      t_name_generator& namer,
      const t_type* t);
  void generate_hack_array_from_shape_lambda(
      std::ostream& out,
      t_name_generator& namer,
      const t_type* t);
  void generate_shape_from_hack_array_lambda(
      std::ostream& out,
      t_name_generator& namer,
      const t_type* t);
  void generate_php_struct_from_shape(
      std::ofstream& out,
      const t_struct* tstruct);
  void generate_php_struct_from_map(
      std::ofstream& out,
      const t_struct* tstruct);
  bool type_has_nested_struct(const t_type* t);
  bool
  field_is_nullable(const t_struct* tstruct, const t_field* field, string dval);
  void generate_php_struct_shape_methods(
      std::ofstream& out,
      const t_struct* tstruct);

  void generate_adapter_type_checks(ofstream& out, const t_struct* tstruct);

  void generate_php_type_spec(std::ofstream& out, const t_type* t);
  void generate_php_struct_spec(std::ofstream& out, const t_struct* tstruct);
  void generate_php_struct_struct_trait(
      std::ofstream& out,
      const t_struct* tstruct,
      const std::string& name);
  void generate_php_structural_id(
      std::ofstream& out,
      const t_struct* tstruct,
      bool asFunction);

  /**
   * Service-level generation functions
   */

  void generate_service(const t_service* tservice, bool mangle);
  void generate_service_helpers(const t_service* tservice, bool mangle);
  void generate_service_interactions(const t_service* tservice, bool mangle);
  void generate_service_interface(
      const t_service* tservice,
      bool mangle,
      bool async,
      bool rpc_options,
      bool client);
  void generate_service_client(const t_service* tservice, bool mangle);
  void _generate_service_client(
      std::ofstream& out,
      const t_service* tservice,
      bool mangle);
  void _generate_recvImpl(
      ofstream& out,
      const t_service* tservice,
      const t_function* tfunction);
  void _generate_sendImpl(
      ofstream& out,
      const t_service* tservice,
      const t_function* tfunction);
  void _generate_sendImpl_arg(
      ofstream& out,
      t_name_generator& namer,
      const string& var,
      const t_type* t);
  void _generate_service_client_children(
      std::ofstream& out,
      const t_service* tservice,
      bool mangle,
      bool async,
      bool rpc_options);
  void _generate_service_client_child_fn(
      std::ofstream& out,
      const t_service* tservice,
      const t_function* tfunction,
      bool rpc_options,
      bool legacy_arrays = false);
  void generate_service_processor(
      const t_service* tservice,
      bool mangle,
      bool async);
  void generate_process_function(
      const t_service* tservice,
      const t_function* tfunction,
      bool async);
  void generate_processor_event_handler_functions(std::ofstream& out);
  void generate_client_event_handler_functions(std::ofstream& out);
  void generate_event_handler_functions(std::ofstream& out, string cl);

  /**
   * Read thrift object from JSON string, generated using the
   * TSimpleJSONProtocol.
   */

  void generate_json_enum(
      std::ofstream& out,
      t_name_generator& namer,
      const t_enum* tenum,
      const string& prefix_thrift,
      const string& prefix_json);

  void generate_json_struct(
      std::ofstream& out,
      t_name_generator& namer,
      const t_struct* tstruct,
      const string& prefix_thrift,
      const string& prefix_json);

  void generate_json_field(
      std::ofstream& out,
      t_name_generator& namer,
      const t_field* tfield,
      const string& prefix_thrift = "",
      const string& suffix_thrift = "",
      const string& prefix_json = "");

  void generate_json_container(
      std::ofstream& out,
      t_name_generator& namer,
      const t_type* ttype,
      const string& prefix_thrift = "",
      const string& prefix_json = "");

  void generate_json_set_element(
      std::ofstream& out,
      t_name_generator& namer,
      const t_set* tset,
      const string& value,
      const string& prefix_thrift);

  void generate_json_list_element(
      ofstream& out,
      t_name_generator& namer,
      const t_list* list,
      const string& value,
      const string& prefix_thrift);

  void generate_json_map_element(
      std::ofstream& out,
      t_name_generator& namer,
      const t_map* tmap,
      const string& key,
      const string& value,
      const string& prefix_thrift);

  void generate_json_reader(std::ofstream& out, const t_struct* tstruct);

  /**
   * Helper rendering functions
   */

  std::string declare_field(
      const t_field* tfield,
      bool init = false,
      bool obj = false,
      bool thrift = false);
  std::string function_signature(
      const t_function* tfunction,
      std::string more_head_parameters = "",
      std::string more_tail_parameters = "",
      std::string typehint = "");
  std::string argument_list(
      const t_struct* tstruct,
      std::string more_head_parameters = "",
      std::string more_tail_parameters = "",
      bool typehints = true,
      bool force_nullable = false);
  std::string generate_rpc_function_name(
      const t_service* tservice,
      const t_function* tfunction) const;
  std::string generate_function_helper_name(
      const t_service* tservice,
      const t_function* tfunction,
      bool is_args);
  std::string type_to_cast(const t_type* ttype);
  std::string type_to_enum(const t_type* ttype);
  void generate_php_docstring(ofstream& out, const t_node* tdoc);
  void generate_php_docstring(ofstream& out, const t_enum* tenum);
  void generate_php_docstring(ofstream& out, const t_service* tservice);
  void generate_php_docstring(ofstream& out, const t_const* tconst);
  void generate_php_docstring(ofstream& out, const t_function* tfunction);
  void generate_php_docstring(ofstream& out, const t_field* tfield);
  void generate_php_docstring(
      ofstream& out,
      const t_struct* tstruct,
      bool is_exception = false);
  void generate_php_docstring_args(
      ofstream& out,
      int start_pos,
      const t_struct* arg_list);
  std::string render_string(std::string value);

  std::string type_to_typehint(
      const t_type* ttype,
      bool nullable = false,
      bool shape = false,
      bool immutable_collections = false,
      bool ignore_adapter = false);
  std::string type_to_param_typehint(
      const t_type* ttype,
      bool nullable = false);

  bool is_type_arraykey(const t_type* type);

  std::string union_enum_name(
      const std::string& name,
      const t_program* program,
      bool decl = false) {
    // <StructName>Type
    return hack_name(name, program, decl) + "Enum";
  }

  std::string union_enum_name(const t_struct* tstruct, bool decl = false) {
    return union_enum_name(tstruct->get_name(), tstruct->get_program(), decl);
  }

  std::string union_field_to_enum(
      const t_struct* tstruct,
      const t_field* tfield,
      const std::string& name) {
    // If null is passed,  it refer to empty;
    if (tfield) {
      return union_enum_name(name, tstruct->get_program()) +
          "::" + tfield->get_name();
    } else {
      return union_enum_name(name, tstruct->get_program()) + "::" + UNION_EMPTY;
    }
  }

  bool is_bitmask_enum(const t_enum* tenum) {
    return tenum->has_annotation("bitmask");
  }

  // Recursively traverse any typdefs and return the first found adapter name.
  const string* get_hack_adapter(const t_type* type) {
    while (true) {
      if (const auto* adapter = type->get_annotation_or_null("hack.adapter")) {
        return adapter;
      }
      if (!type->is_typedef()) {
        return nullptr;
      }
      type = static_cast<const t_typedef*>(type)->get_type();
    }
  }

  std::string hack_namespace(const t_program* p) {
    std::string ns;
    ns = p->get_namespace("hack");
    std::replace(ns.begin(), ns.end(), '.', '\\');
    return ns;
  }

  std::string php_namespace(const t_program* p) {
    std::string ns = hack_namespace(p);
    p->get_namespace("hack");
    if (!ns.empty()) {
      return ns;
    }
    ns = p->get_namespace("php");
    if (!ns.empty()) {
      ns.push_back('_');
      return ns;
    }
    return "";
  }

  string php_namespace(const t_service* s) {
    return php_namespace(s->get_program());
  }

  string hack_name(string name, const t_program* prog, bool decl = false) {
    string ns;
    ns = hack_namespace(prog);
    if (!ns.empty()) {
      if (decl) {
        return name;
      }
      return "\\" + ns + "\\" + name;
    }
    ns = prog->get_namespace("php");
    return (!decl && has_hack_namespace ? "\\" : "") +
        (!ns.empty() ? ns + "_" : "") + name;
  }

  string hack_name(const t_type* t, bool decl = false) {
    return hack_name(t->get_name(), t->get_program(), decl);
  }

  string hack_name(const t_service* s, bool decl = false) {
    return hack_name(s->get_name(), s->get_program(), decl);
  }

  std::string php_path(const t_program* p) {
    std::string ns = p->get_namespace("php_path");
    if (ns.empty()) {
      return p->get_name();
    }

    // Transform the java-style namespace into a path.
    for (std::string::iterator it = ns.begin(); it != ns.end(); ++it) {
      if (*it == '.') {
        *it = '/';
      }
    }

    return ns + '/' + p->get_name();
  }

  std::string php_path(const t_service* s) {
    return php_path(s->get_program());
  }

  const char* UNION_EMPTY = "_EMPTY_";

  std::string generate_array_typehint(
      const std::string& key_type,
      const std::string& value_type);

  bool is_base_exception_property(const t_field*);

  std::string render_structured_annotations(
      const std::vector<const t_const*>& annotations);

 private:
  /**
   * Generate the namespace mangled string, if necessary
   */
  std::string php_servicename_mangle(
      bool mangle,
      const t_service* svc,
      bool extends = false) {
    if (extends && !hack_namespace(svc->get_program()).empty()) {
      return hack_name(svc);
    }
    return (extends && has_hack_namespace ? "\\" : "") +
        (mangle ? php_namespace(svc) : "") + svc->get_name();
  }

  /**
   * Return the correct function to be used on a Hack Collection, only when
   * generating shape structures.
   * - If array_migration_ is set, we'll want to use varray / darray
   * - If we're operating on a list, we'll want to use varray / vec over
   *   darray / dict
   */
  std::string generate_to_array_method(const t_type* t) {
    if (!t->is_container()) {
      throw std::logic_error("not a container");
    }
    if (array_migration_) {
      return t->is_list() ? "varray" : "ThriftUtil::toDArray";
    } else {
      return t->is_list() ? "vec" : "dict";
    }
  }

  bool is_hack_const_type(const t_type* type);

  std::vector<t_function*> get_supported_functions(const t_service* tservice) {
    std::vector<t_function*> funcs;
    for (auto func : tservice->get_functions()) {
      if (!func->returns_stream() && !func->returns_sink() &&
          !func->get_returntype()->is_service()) {
        funcs.push_back(func);
      }
    }
    return funcs;
  }

  std::vector<t_service*> get_interactions(const t_service* tservice) const {
    std::vector<t_service*> interactions;
    for (const auto& func : tservice->get_functions()) {
      if (func->get_returntype()->is_service()) {
        interactions.push_back(
            dynamic_cast<t_service*>(func->get_returntype()));
      }
    }
    return interactions;
  }
  /**
   * File streams
   */
  std::ofstream f_types_;
  std::ofstream f_consts_;
  std::ofstream f_helpers_;
  std::ofstream f_service_;

  /**
   * True iff we should generate a function parse json to thrift object.
   */
  bool json_;

  /**
   * Generate stubs for a PHP server
   */
  bool phps_;

  /**
   * * Whether to use collection classes everywhere vs KeyedContainer
   */
  bool strict_types_;

  /**
   * Whether to generate protected members for thrift unions
   */
  bool protected_unions_;

  /**
   * Whether to generate array sets or Set objects
   */
  bool arraysets_;

  /**
   * memory of the values of the constants in array initialisation form
   * for use with generate_const
   */
  std::vector<string> constants_values_;

  /**
   * True iff mangled service classes should be emitted
   */
  bool mangled_services_;

  /**
   * True if struct fields within structs should be instantiated rather than
   * nullable typed
   */
  bool no_nullables_;

  /**
   * True if struct should generate fromMap_DEPRECATED method with a semantic
   * of a legacy map constructor.
   */
  bool from_map_construct_;

  /**
   * True if we should add a "use StructNameTrait" to the generated class
   */
  bool struct_trait_;

  /**
   * True if we should generate Shape types for the generated structs
   */
  bool shapes_;

  /**
   * True if we should generate array<arraykey, TValue> instead of array<string,
   * TValue>
   */
  bool shape_arraykeys_;

  /**
   * True if we should allow implicit subtyping for shapes (i.e. '...')
   */
  bool shapes_allow_unknown_fields_;

  /**
   * True to use darrays instead of dicts for internal constructs
   */
  bool array_migration_;

  /**
   * True to use Hack arrays instead of collections
   */
  bool arrays_;

  /**
   * True to never use hack collection objects. Only used for migrations
   */
  bool no_use_hack_collections_;

  /**
   * True to force client methods to accept null arguments. Only used for
   * migrations
   */
  bool nullable_everything_;

  /**
   * True to force hack collection members be const collection objects
   */
  bool const_collections_;

  /**
   * True to generate explicit types for Hack enums: 'type FooType = Foo'
   */
  bool enum_extratype_;

  /**
   * True to use transparent typing for Hack enums: 'enum FooBar: int as int'.
   */
  bool enum_transparenttype_;

  /**
   * True to generate soft typehints as __Soft instead of @
   */
  bool soft_attribute_;

  /**
   * True to add the `__ProvenanceSkipFrame` attribute to applicable methods.
   * This will allow array provenance (arrprov) instrumentation to pretend that
   * arrays created inside a given function were "created" at the particular
   * callsite of said function.
   */
  bool arrprov_skip_frames_;

  std::string array_keyword_;

  bool has_hack_namespace;
};

void t_hack_generator::generate_json_enum(
    std::ofstream& out,
    t_name_generator& /* namer */,
    const t_enum* tenum,
    const string& prefix_thrift,
    const string& prefix_json) {
  indent(out) << prefix_thrift << " = " << hack_name(tenum) << "::coerce("
              << prefix_json << ");";
}

void t_hack_generator::generate_json_struct(
    ofstream& out,
    t_name_generator& namer,
    const t_struct* tstruct,
    const string& prefix_thrift,
    const string& prefix_json) {
  string enc = namer("$_tmp");
  indent(out) << enc << " = "
              << "json_encode(" << prefix_json << ");\n";
  string tmp = namer("$_tmp");
  t_field felem(tstruct, tmp);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << "\n";
  indent(out) << tmp << "->readFromJson(" << enc << ");\n";
  indent(out) << prefix_thrift << " = " << tmp << ";\n";
}

void t_hack_generator::generate_json_field(
    ofstream& out,
    t_name_generator& namer,
    const t_field* tfield,
    const string& prefix_thrift,
    const string& suffix_thrift,
    const string& prefix_json) {
  const t_type* type = tfield->get_type()->get_true_type();

  if (type->is_void()) {
    throw std::runtime_error(
        "CANNOT READ JSON FIELD WITH void TYPE: " + prefix_thrift +
        tfield->get_name());
  }

  string name = prefix_thrift + tfield->get_name() + suffix_thrift;

  if (type->is_struct() || type->is_xception()) {
    generate_json_struct(out, namer, (t_struct*)type, name, prefix_json);
  } else if (type->is_container()) {
    generate_json_container(out, namer, (t_container*)type, name, prefix_json);
  } else if (type->is_enum()) {
    generate_json_enum(
        out, namer, static_cast<const t_enum*>(type), name, prefix_json);
  } else if (type->is_base_type()) {
    string typeConversionString = "";
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    string number_limit = "";
    switch (tbase) {
      case t_base_type::TYPE_VOID:
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_BINARY:
      case t_base_type::TYPE_BOOL:
      case t_base_type::TYPE_I64:
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        break;
      case t_base_type::TYPE_BYTE:
        number_limit = "0x7f";
        typeConversionString = "(int)";
        break;
      case t_base_type::TYPE_I16:
        number_limit = "0x7fff";
        typeConversionString = "(int)";
        break;
      case t_base_type::TYPE_I32:
        number_limit = "0x7fffffff";
        typeConversionString = "(int)";
        break;
      default:
        throw std::runtime_error(
            "compiler error: no PHP reader for base type " +
            t_base_type::t_base_name(tbase) + name);
    }

    if (number_limit.empty()) {
      indent(out) << name << " = " << typeConversionString << prefix_json
                  << ";\n";
    } else {
      string temp = namer("$_tmp");
      indent(out) << temp << " = (int)" << prefix_json << ";\n";
      indent(out) << "if (" << temp << " > " << number_limit << ") {\n";
      indent_up();
      indent(out) << "throw new \\TProtocolException(\"number exceeds "
                  << "limit in field\");\n";
      indent_down();
      indent(out) << "} else {\n";
      indent_up();
      indent(out) << name << " = " << typeConversionString << temp << ";\n";
      indent_down();
      indent(out) << "}\n";
    }
  }
}

void t_hack_generator::generate_json_container(
    std::ofstream& out,
    t_name_generator& namer,
    const t_type* ttype,
    const string& prefix_thrift,
    const string& prefix_json) {
  string size = namer("$_size");
  string key = namer("$_key");
  string value = namer("$_value");
  string json = namer("$_json");
  string container = namer("$_container");

  indent(out) << json << " = " << prefix_json << ";\n";
  if (ttype->is_map()) {
    if (arrays_) {
      indent(out) << container << " = dict[];\n";
    } else if (no_use_hack_collections_) {
      indent(out) << container << " = darray[];\n";
    } else {
      indent(out) << container << " = Map {};\n";
    }
  } else if (ttype->is_list()) {
    if (arrays_) {
      indent(out) << container << " = vec[];\n";
    } else if (no_use_hack_collections_) {
      indent(out) << container << " = varray[];\n";
    } else {
      indent(out) << container << " = Vector {};\n";
    }
  } else if (ttype->is_set()) {
    if (arrays_) {
      indent(out) << container << " = keyset[];\n";
    } else if (arraysets_) {
      indent(out) << container << " = " << array_keyword_ << "[];\n";
    } else {
      indent(out) << container << " = Set {};\n";
    }
  }
  indent(out) << "foreach(/* HH_FIXME[4110] */ " << json << " as " << key
              << " => " << value << ") {\n";
  indent_up();

  if (ttype->is_list()) {
    generate_json_list_element(out, namer, (t_list*)ttype, value, container);
  } else if (ttype->is_set()) {
    generate_json_set_element(out, namer, (t_set*)ttype, value, container);
  } else if (ttype->is_map()) {
    generate_json_map_element(out, namer, (t_map*)ttype, key, value, container);
  } else {
    throw std::runtime_error("compiler error: no PHP reader for this type.");
  }
  indent_down();
  indent(out) << "}\n";
  indent(out) << prefix_thrift << " = " << container << ";\n";
}

void t_hack_generator::generate_json_list_element(
    ofstream& out,
    t_name_generator& namer,
    const t_list* tlist,
    const string& value,
    const string& prefix_thrift) {
  string elem = namer("$_elem");
  t_field felem(tlist->get_elem_type(), elem);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << "\n";
  generate_json_field(out, namer, &felem, "", "", value);
  indent(out) << prefix_thrift << " []= " << elem << ";\n";
}

void t_hack_generator::generate_json_set_element(
    std::ofstream& out,
    t_name_generator& namer,
    const t_set* tset,
    const string& value,
    const string& prefix_thrift) {
  string elem = namer("$_elem");
  t_field felem(tset->get_elem_type(), elem);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << "\n";
  generate_json_field(out, namer, &felem, "", "", value);
  if (arrays_) {
    indent(out) << prefix_thrift << " []= " << elem << ";\n";
  } else if (arraysets_) {
    indent(out) << prefix_thrift << "[" << elem << "] = true;\n";
  } else {
    indent(out) << prefix_thrift << "->add(" << elem << ");\n";
  }
}

void t_hack_generator::generate_json_map_element(
    std::ofstream& out,
    t_name_generator& namer,
    const t_map* tmap,
    const string& key,
    const string& value,
    const string& prefix_thrift) {
  const t_type* keytype = tmap->get_key_type()->get_true_type();
  string error_msg =
      "compiler error: Thrift Hack compiler"
      "does not support complex types as the key of a map.";

  if (!keytype->is_enum() && !keytype->is_base_type()) {
    throw error_msg;
  }
  if (keytype->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)keytype)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_VOID:
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        throw error_msg;
      default:
        break;
    }
  }
  string _value = namer("$_value");
  t_field vfelem(tmap->get_val_type(), _value);
  indent(out) << declare_field(&vfelem, true, true, true).substr(1) << "\n";
  generate_json_field(out, namer, &vfelem, "", "", value);
  indent(out) << prefix_thrift << "[" << key << "] = " << _value << ";\n";
}

void t_hack_generator::generate_json_reader(
    ofstream& out,
    const t_struct* tstruct) {
  if (!json_) {
    return;
  }
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  t_name_generator namer;

  string name = tstruct->get_name();
  indent(out) << "public function readFromJson(string $jsonText): void {\n";
  indent_up();
  if (tstruct->is_union()) {
    indent(out) << "$this->_type = "
                << union_field_to_enum(tstruct, nullptr, tstruct->get_name())
                << ";\n";
  }
  indent(out) << "$parsed = json_decode($jsonText, true);\n\n";

  indent(out)
      << "if ($parsed === null || !($parsed is KeyedContainer<_, _>)) {\n";
  indent_up();
  indent(out) << "throw new \\TProtocolException(\"Cannot parse the given json"
              << " string.\");\n";
  indent_down();
  indent(out) << "}\n\n";
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    const t_field* tf = *f_iter;
    indent(out) << "if (idx($parsed, '" << tf->get_name() << "') !== null) {\n";
    indent_up();
    generate_json_field(
        out,
        namer,
        tf,
        "$this->",
        "",
        "/* HH_FIXME[4110] */ $parsed['" + tf->get_name() + "']");
    if (tstruct->is_union()) {
      indent(out) << "$this->_type = "
                  << union_field_to_enum(tstruct, tf, tstruct->get_name())
                  << ";\n";
    }
    indent_down();
    indent(out) << "}";
    if (tf->get_req() == t_field::e_req::required) {
      out << " else {\n";
      indent_up();
      indent(out) << "throw new \\TProtocolException(\"Required field "
                  << tf->get_name() << " cannot be found.\");\n";
      indent_down();
      indent(out) << "}";
    }
    indent(out) << "\n";
  }
  indent_down();
  indent(out) << "}\n\n";
}

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_hack_generator::init_generator() {
  // Make output directory
  boost::filesystem::create_directory(get_out_dir());

  // Make output file
  string f_types_name = get_out_dir() + program_name_ + "_types.php";
  f_types_.open(f_types_name.c_str());
  record_genfile(f_types_name);

  // Print header
  f_types_ << "<?hh // strict\n" << autogen_comment() << "\n";

  string hack_ns = hack_namespace(program_);
  if (!hack_ns.empty()) {
    f_types_ << "namespace " << hack_ns << ";\n\n";
  }

  // Print header
  if (!program_->get_consts().empty()) {
    string f_consts_name = get_out_dir() + program_name_ + "_constants.php";
    f_consts_.open(f_consts_name.c_str());
    record_genfile(f_consts_name);
    f_consts_ << "<?hh // strict\n" << autogen_comment();
    constants_values_.clear();
    string const_namespace = php_namespace(program_);
    if (!hack_ns.empty()) {
      f_consts_ << "namespace " << hack_ns << ";\n\n";
    }
    f_consts_ << "class "
              << (!hack_ns.empty() || const_namespace == ""
                      ? program_name_ + "_"
                      : const_namespace)
              << "CONSTANTS implements \\IThriftConstants {\n";
  }
}

/**
 * Close up (or down) some filez.
 */
void t_hack_generator::close_generator() {
  // Close types file
  f_types_.close();

  if (!program_->get_consts().empty()) {
    // write out the values array
    indent_up();
    f_consts_ << "\n";
    // write structured annotations
    f_consts_ << indent()
              << "public static function getAllStructuredAnnotations(): "
                 "dict<string, dict<string, \\IThriftStruct>> {\n";
    indent_up();
    f_consts_ << indent() << "return dict[\n";
    indent_up();
    for (const auto& tconst : program_->get_consts()) {
      if (tconst->structured_annotations().empty()) {
        continue;
      }
      f_consts_ << indent() << "'" << tconst->get_name() << "' => "
                << render_structured_annotations(
                       tconst->structured_annotations())
                << ",\n";
    }
    indent_down();
    f_consts_ << indent() << "];\n";
    indent_down();
    f_consts_ << indent() << "}\n";
    indent_down();
    // close constants class
    f_consts_ << "}\n\n";
    f_consts_.close();
  }
}

/**
 * Generates a typedef. This is not done in PHP, types are all implicit.
 *
 * @param ttypedef The type definition
 */
void t_hack_generator::generate_typedef(const t_typedef* /*ttypedef*/) {}

/**
 * Generates code for an enumerated type. Since define is expensive to lookup
 * in PHP, we use a global array for this.
 *
 * @param tenum The enumeration
 */
void t_hack_generator::generate_enum(const t_enum* tenum) {
  vector<t_enum_value*> constants = tenum->get_enum_values();
  vector<t_enum_value*>::iterator c_iter;

  std::string typehint;
  generate_php_docstring(f_types_, tenum);
  bool hack_enum = false;
  if (is_bitmask_enum(tenum)) {
    typehint = "int";
    f_types_ << "final class " << hack_name(tenum, true)
             << " extends \\Flags {\n";
  } else {
    hack_enum = true;
    typehint = hack_name(tenum, true);
    if (std::string const* attributes =
            tenum->get_annotation_or_null("hack.attributes")) {
      f_types_ << "<<" << *attributes << ">>\n";
    }
    f_types_ << "enum " << hack_name(tenum, true) << ": int"
             << (enum_transparenttype_ ? " as int" : "") << " {\n";
  }

  indent_up();

  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int32_t value = (*c_iter)->get_value();

    generate_php_docstring(f_types_, *c_iter);
    if (!hack_enum) {
      indent(f_types_) << "const " << typehint << " ";
    }
    indent(f_types_) << (*c_iter)->get_name() << " = " << value << ";\n";
  }

  indent_down();
  f_types_ << "}\n";
  if (hack_enum && enum_extratype_) {
    f_types_ << "type " << typehint << "Type = " << typehint << ";\n";
  }
  f_types_ << "\n";

  f_types_ << indent() << "class " << hack_name(tenum, true)
           << "_TEnumStaticMetadata implements \\IThriftEnumStaticMetadata {\n";
  indent_up();
  // Structured annotations
  f_types_ << indent()
           << "public static function getAllStructuredAnnotations(): "
              "\\TEnumAnnotations {\n";
  indent_up();
  f_types_ << indent() << "return shape(\n";
  indent_up();
  f_types_ << indent() << "'enum' => "
           << render_structured_annotations(tenum->structured_annotations())
           << ",\n";
  f_types_ << indent() << "'constants' => dict[\n";
  indent_up();
  for (const auto& constant : constants) {
    if (constant->structured_annotations().empty()) {
      continue;
    }
    f_types_ << indent() << "'" << constant->get_name() << "' => "
             << render_structured_annotations(
                    constant->structured_annotations())
             << ",\n";
  }
  indent_down();
  f_types_ << indent() << "],\n";
  indent_down();
  f_types_ << indent() << ");\n";
  indent_down();
  f_types_ << indent() << "}\n";
  indent_down();
  f_types_ << indent() << "}\n\n";
}

/**
 * Generate a constant value
 */
void t_hack_generator::generate_const(const t_const* tconst) {
  const t_type* type = tconst->get_type();
  string name = tconst->get_name();
  t_const_value* value = tconst->get_value();

  indent_up();
  generate_php_docstring(f_consts_, tconst);
  bool is_hack_const = is_hack_const_type(type);
  f_consts_ << indent();
  // for base hack types, use const (guarantees optimization in hphp)
  if (is_hack_const) {
    f_consts_ << "const " << type_to_typehint(type) << " " << name << " = ";
    // cannot use const for objects (incl arrays). use static
  } else {
    f_consts_ << "<<__Memoize>>\n"
              << indent() << "public static function " << name
              << "(): " << type_to_typehint(type, false, false, true) << "{\n";
    indent_up();
    f_consts_ << indent() << "return ";
  }
  f_consts_ << render_const_value(type, value, true) << ";\n";
  if (!is_hack_const) {
    indent_down();
    f_consts_ << indent() << "}\n";
  }
  f_consts_ << "\n";
  indent_down();
}

bool t_hack_generator::is_hack_const_type(const t_type* type) {
  type = type->get_true_type();
  if (type->is_base_type() || type->is_enum()) {
    return true;
  } else if (arrays_ && type->is_container()) {
    if (type->is_list()) {
      return is_hack_const_type(((t_list*)type)->get_elem_type());
    } else if (type->is_set()) {
      return is_hack_const_type(((t_set*)type)->get_elem_type());
    } else if (type->is_map()) {
      return is_hack_const_type(((t_map*)type)->get_key_type()) &&
          is_hack_const_type(((t_map*)type)->get_val_type());
    }
  }
  return false;
}

std::string t_hack_generator::generate_array_typehint(
    const std::string& key_type,
    const std::string& value_type) {
  std::ostringstream stream;
  stream << array_keyword_ << "<" << key_type << ", " << value_type << ">";
  return stream.str();
}

string t_hack_generator::render_string(string value) {
  std::ostringstream out;
  size_t pos = 0;
  while ((pos = value.find('"', pos)) != string::npos) {
    value.insert(pos, 1, '\\');
    pos += 2;
  }
  out << "\"" << value << "\"";
  return out.str();
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
string t_hack_generator::render_const_value(
    const t_type* type,
    const t_const_value* value,
    bool immutable_collections) {
  std::ostringstream out;
  type = type->get_true_type();
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_BINARY:
        out << render_string(value->get_string());
        break;
      case t_base_type::TYPE_BOOL:
        out << (value->get_integer() > 0 ? "true" : "false");
        break;
      case t_base_type::TYPE_BYTE:
      case t_base_type::TYPE_I16:
      case t_base_type::TYPE_I32:
      case t_base_type::TYPE_I64:
        out << value->get_integer();
        break;
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        if (value->get_type() == t_const_value::CV_INTEGER) {
          out << value->get_integer();
        } else {
          out << value->get_double();
        }
        if (out.str().find('.') == string::npos &&
            out.str().find('e') == string::npos) {
          out << ".0";
        }
        break;
      default:
        throw std::runtime_error(
            "compiler error: no const of base type " +
            t_base_type::t_base_name(tbase));
    }
  } else if (type->is_enum()) {
    const t_enum* tenum = (t_enum*)type;
    const t_enum_value* val = tenum->find_value(value->get_integer());
    if (val != nullptr) {
      out << hack_name(tenum) << "::" << val->get_name();
    } else {
      out << hack_name(tenum) << "::coerce(" << value->get_integer() << ")";
    }
  } else if (type->is_struct() || type->is_xception()) {
    out << hack_name(type) << "::fromShape(\n";
    indent_up();
    indent(out) << "shape(\n";
    indent_up();

    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      const t_type* field_type = nullptr;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == nullptr) {
        throw std::runtime_error(
            "type error: " + type->get_name() + " has no field " +
            v_iter->first->get_string());
      }
    }
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      t_const_value* k = nullptr;
      t_const_value* v = nullptr;
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          k = v_iter->first;
          v = v_iter->second;
        }
      }
      if (v != nullptr) {
        indent(out) << render_const_value(string_type(), k) << " => "
                    << render_const_value((*f_iter)->get_type(), v) << ",\n";
      }
    }
    indent_down();
    indent(out) << ")\n";
    indent_down();
    indent(out) << ")";
  } else if (type->is_map()) {
    const t_type* ktype = ((t_map*)type)->get_key_type();
    const t_type* vtype = ((t_map*)type)->get_val_type();
    if (arrays_) {
      out << "dict[\n";
    } else if (no_use_hack_collections_) {
      out << "darray[\n";
    } else {
      out << (immutable_collections ? "Imm" : "") << "Map {\n";
    }
    indent_up();
    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(ktype, v_iter->first, immutable_collections);
      out << " => ";
      out << render_const_value(vtype, v_iter->second, immutable_collections);
      out << ",\n";
    }
    indent_down();
    if (arrays_ || no_use_hack_collections_) {
      indent(out) << "]";
    } else {
      indent(out) << "}";
    }
  } else if (type->is_list()) {
    const t_type* etype = ((t_list*)type)->get_elem_type();
    if (arrays_) {
      out << "vec[\n";
    } else if (no_use_hack_collections_) {
      out << "varray[\n";
    } else {
      out << (immutable_collections ? "Imm" : "") << "Vector {\n";
    }
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(etype, *v_iter, immutable_collections);
      out << ",\n";
    }
    indent_down();
    if (arrays_ || no_use_hack_collections_) {
      indent(out) << "]";
    } else {
      indent(out) << "}";
    }
  } else if (type->is_set()) {
    const t_type* etype = ((t_set*)type)->get_elem_type();
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    if (arrays_) {
      out << "keyset[\n";
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        out << indent();
        out << render_const_value(etype, *v_iter, immutable_collections);
        out << ",\n";
      }
      indent_down();
      indent(out) << "]";
    } else if (arraysets_) {
      out << array_keyword_ << "[\n";
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        out << indent();
        out << render_const_value(etype, *v_iter, immutable_collections);
        out << " => true";
        out << ",\n";
      }
      indent_down();
      indent(out) << "]";
    } else {
      out << (immutable_collections ? "Imm" : "") << "Set {\n";
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        out << indent();
        out << render_const_value(etype, *v_iter, immutable_collections);
        out << ",\n";
      }
      indent_down();
      indent(out) << "}";
    }
  }
  return out.str();
}

string t_hack_generator::render_default_value(const t_type* type) {
  string dval;
  type = type->get_true_type();
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_BINARY:
        dval = "''";
        break;
      case t_base_type::TYPE_BOOL:
        dval = "false";
        break;
      case t_base_type::TYPE_BYTE:
      case t_base_type::TYPE_I16:
      case t_base_type::TYPE_I32:
      case t_base_type::TYPE_I64:
        dval = "0";
        break;
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        dval = "0.0";
        break;
      default:
        throw std::runtime_error(
            "compiler error: no const of base type " +
            t_base_type::t_base_name(tbase));
    }
  } else if (type->is_enum()) {
    dval = "null";
  } else if (type->is_struct() || type->is_xception()) {
    const t_struct* tstruct = (t_struct*)type;
    if (no_nullables_) {
      dval = hack_name(tstruct) + "::withDefaultValues()";
    } else {
      dval = "null";
    }
  } else if (type->is_map()) {
    if (arrays_) {
      dval = "dict[]";
    } else if (no_use_hack_collections_) {
      dval = "darray[]";
    } else {
      dval = "Map {}";
    }
  } else if (type->is_list()) {
    if (arrays_) {
      dval = "vec[]";
    } else if (no_use_hack_collections_) {
      dval = "varray[]";
    } else {
      dval = "Vector {}";
    }
  } else if (type->is_set()) {
    if (arrays_) {
      dval = "keyset[]";
    } else if (arraysets_) {
      dval = array_keyword_ + "[]";
    } else {
      dval = "Set {}";
    }
  }
  return dval;
}

t_hack_generator::ThriftPrimitiveType t_hack_generator::base_to_t_primitive(
    t_base_type* tbase) {
  switch (tbase->get_base()) {
    case t_base_type::TYPE_BOOL:
      return ThriftPrimitiveType::THRIFT_BOOL_TYPE;
    case t_base_type::TYPE_BYTE:
      return ThriftPrimitiveType::THRIFT_BYTE_TYPE;
    case t_base_type::TYPE_I16:
      return ThriftPrimitiveType::THRIFT_I16_TYPE;
    case t_base_type::TYPE_I32:
      return ThriftPrimitiveType::THRIFT_I32_TYPE;
    case t_base_type::TYPE_I64:
      return ThriftPrimitiveType::THRIFT_I64_TYPE;
    case t_base_type::TYPE_FLOAT:
      return ThriftPrimitiveType::THRIFT_FLOAT_TYPE;
    case t_base_type::TYPE_DOUBLE:
      return ThriftPrimitiveType::THRIFT_DOUBLE_TYPE;
    case t_base_type::TYPE_BINARY:
      return ThriftPrimitiveType::THRIFT_BINARY_TYPE;
    case t_base_type::TYPE_STRING:
      return ThriftPrimitiveType::THRIFT_STRING_TYPE;
    case t_base_type::TYPE_VOID:
      return ThriftPrimitiveType::THRIFT_VOID_TYPE;
    default:
      throw std::invalid_argument(
          "compiler error: no ThriftPrimitiveType mapped to base type " +
          t_base_type::t_base_name(tbase->get_base()));
  }
}

std::unique_ptr<t_const_value> t_hack_generator::type_to_tmeta(
    const t_type* type) {
  auto tmeta_ThriftType = std::make_unique<t_const_value>();

  if (type->is_base_type()) {
    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_primitive"),
        std::make_unique<t_const_value>(
            base_to_t_primitive((t_base_type*)type)));
  } else if (type->is_list()) {
    const t_list* tlist = (t_list*)type;
    auto tlist_tmeta = std::make_unique<t_const_value>();
    tlist_tmeta->add_map(
        std::make_unique<t_const_value>("valueType"),
        type_to_tmeta(tlist->get_elem_type()));

    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_list"), std::move(tlist_tmeta));
  } else if (type->is_set()) {
    const t_set* tset = (t_set*)type;
    auto tset_tmeta = std::make_unique<t_const_value>();
    tset_tmeta->add_map(
        std::make_unique<t_const_value>("valueType"),
        type_to_tmeta(tset->get_elem_type()));

    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_set"), std::move(tset_tmeta));
  } else if (type->is_map()) {
    const t_map* tmap = (t_map*)type;
    auto tmap_tmeta = std::make_unique<t_const_value>();
    tmap_tmeta->add_map(
        std::make_unique<t_const_value>("keyType"),
        type_to_tmeta(tmap->get_key_type()));
    tmap_tmeta->add_map(
        std::make_unique<t_const_value>("valueType"),
        type_to_tmeta(tmap->get_val_type()));

    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_map"), std::move(tmap_tmeta));
  } else if (type->is_enum()) {
    auto tenum_tmeta = std::make_unique<t_const_value>();
    tenum_tmeta->add_map(
        std::make_unique<t_const_value>("name"),
        std::make_unique<t_const_value>(type->get_name()));

    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_enum"), std::move(tenum_tmeta));
  } else if (type->is_struct()) {
    auto tstruct_tmeta = std::make_unique<t_const_value>();
    tstruct_tmeta->add_map(
        std::make_unique<t_const_value>("name"),
        std::make_unique<t_const_value>(type->get_name()));

    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_struct"), std::move(tstruct_tmeta));
  } else if (type->is_union()) {
    auto tunion_tmeta = std::make_unique<t_const_value>();
    tunion_tmeta->add_map(
        std::make_unique<t_const_value>("name"),
        std::make_unique<t_const_value>(type->get_name()));

    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_union"), std::move(tunion_tmeta));
  } else if (type->is_typedef()) {
    const t_typedef* ttypedef = (t_typedef*)type;
    auto ttypedef_tmeta = std::make_unique<t_const_value>();
    ttypedef_tmeta->add_map(
        std::make_unique<t_const_value>("name"),
        std::make_unique<t_const_value>(ttypedef->get_name()));
    ttypedef_tmeta->add_map(
        std::make_unique<t_const_value>("underlyingType"),
        type_to_tmeta(ttypedef->get_type()));

    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_typedef"),
        std::move(ttypedef_tmeta));
  } else if (type->is_sink()) {
    const t_sink* tsink = (t_sink*)type;
    auto tsink_tmeta = std::make_unique<t_const_value>();
    tsink_tmeta->add_map(
        std::make_unique<t_const_value>("elemType"),
        type_to_tmeta(tsink->get_sink_type()));
    tsink_tmeta->add_map(
        std::make_unique<t_const_value>("finalResponseType"),
        type_to_tmeta(tsink->get_final_response_type()));
    tsink_tmeta->add_map(
        std::make_unique<t_const_value>("initialResponseType"),
        type_to_tmeta(tsink->get_first_response_type()));

    tmeta_ThriftType->add_map(
        std::make_unique<t_const_value>("t_sink"), std::move(tsink_tmeta));
  } else {
    // Unsupported type
  }

  return tmeta_ThriftType;
}

std::unique_ptr<t_const_value> t_hack_generator::field_to_tmeta(
    const t_field* field) {
  auto tmeta_ThriftField = std::make_unique<t_const_value>();

  tmeta_ThriftField->add_map(
      std::make_unique<t_const_value>("id"),
      std::make_unique<t_const_value>(field->get_key()));

  tmeta_ThriftField->add_map(
      std::make_unique<t_const_value>("type"),
      type_to_tmeta(field->get_type()));

  tmeta_ThriftField->add_map(
      std::make_unique<t_const_value>("name"),
      std::make_unique<t_const_value>(field->get_name()));

  if (field->get_req() == t_field::e_req::optional) {
    auto is_optional = std::make_unique<t_const_value>();
    is_optional->set_bool(true);
    tmeta_ThriftField->add_map(
        std::make_unique<t_const_value>("is_optional"), std::move(is_optional));
  }

  return tmeta_ThriftField;
}

std::unique_ptr<t_const_value> t_hack_generator::function_to_tmeta(
    const t_function* function) {
  auto tmeta_ThriftFunction = std::make_unique<t_const_value>();

  tmeta_ThriftFunction->add_map(
      std::make_unique<t_const_value>("name"),
      std::make_unique<t_const_value>(function->get_name()));

  tmeta_ThriftFunction->add_map(
      std::make_unique<t_const_value>("return_type"),
      type_to_tmeta(function->get_returntype()));

  vector<t_field*> arguments_fields = function->get_paramlist()->get_members();
  if (!arguments_fields.empty()) {
    auto arguments = std::make_unique<t_const_value>();
    arguments->set_list();
    for (const auto& field : arguments_fields) {
      arguments->add_list(field_to_tmeta(field));
    }
    tmeta_ThriftFunction->add_map(
        make_unique<t_const_value>("arguments"), std::move(arguments));
  }

  vector<t_field*> exceptions_fields = function->get_xceptions()->get_members();
  if (!exceptions_fields.empty()) {
    auto exceptions = std::make_unique<t_const_value>();
    exceptions->set_list();
    for (const auto& field : exceptions_fields) {
      exceptions->add_list(field_to_tmeta(field));
    }
    tmeta_ThriftFunction->add_map(
        make_unique<t_const_value>("exceptions"), std::move(exceptions));
  }

  if (function->is_oneway()) {
    auto is_oneway = std::make_unique<t_const_value>();
    is_oneway->set_bool(true);
    tmeta_ThriftFunction->add_map(
        std::make_unique<t_const_value>("is_oneway"), std::move(is_oneway));
  }

  return tmeta_ThriftFunction;
}

std::unique_ptr<t_const_value> t_hack_generator::service_to_tmeta(
    const t_service* service) {
  auto tmeta_ThriftService = std::make_unique<t_const_value>();

  tmeta_ThriftService->add_map(
      std::make_unique<t_const_value>("name"),
      std::make_unique<t_const_value>(service->get_name()));

  vector<t_function*> functions = get_supported_functions(service);
  if (!functions.empty()) {
    auto tmeta_functions = std::make_unique<t_const_value>();
    tmeta_functions->set_list();
    for (const auto& function : functions) {
      tmeta_functions->add_list(function_to_tmeta(function));
    }
    tmeta_ThriftService->add_map(
        std::make_unique<t_const_value>("functions"),
        std::move(tmeta_functions));
  }

  const t_service* parent = service->get_extends();
  if (parent) {
    tmeta_ThriftService->add_map(
        std::make_unique<t_const_value>("parent"),
        std::make_unique<t_const_value>(parent->get_name()));
  }
  return tmeta_ThriftService;
}

void t_hack_generator::append_to_t_enum(
    t_enum* tenum,
    t_program* program,
    ThriftPrimitiveType value) {
  auto enum_value = std::make_unique<t_enum_value>();
  enum_value->set_value(value);
  switch (value) {
    case ThriftPrimitiveType::THRIFT_BOOL_TYPE:
      enum_value->set_name("THRIFT_BOOL_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_BYTE_TYPE:
      enum_value->set_name("THRIFT_BYTE_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_I16_TYPE:
      enum_value->set_name("THRIFT_I16_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_I32_TYPE:
      enum_value->set_name("THRIFT_I32_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_I64_TYPE:
      enum_value->set_name("THRIFT_I64_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_FLOAT_TYPE:
      enum_value->set_name("THRIFT_FLOAT_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_DOUBLE_TYPE:
      enum_value->set_name("THRIFT_DOUBLE_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_BINARY_TYPE:
      enum_value->set_name("THRIFT_BINARY_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_STRING_TYPE:
      enum_value->set_name("THRIFT_STRING_TYPE");
      break;
    case ThriftPrimitiveType::THRIFT_VOID_TYPE:
      enum_value->set_name("THRIFT_VOID_TYPE");
      break;
  }
  auto const_value = std::make_unique<t_const>(
      program,
      tenum,
      enum_value->get_name(),
      std::make_unique<t_const_value>(value));
  tenum->append(std::move(enum_value), std::move(const_value));
}

const t_type* t_hack_generator::tmeta_ThriftType_type() {
  static t_program empty_program("");
  static t_union type(&empty_program, "tmeta_ThriftType");
  static t_enum primitive_type(&empty_program);
  static t_struct list_type(&empty_program, "tmeta_ThriftListType");
  static t_struct set_type(&empty_program, "tmeta_ThriftSetType");
  static t_struct map_type(&empty_program, "tmeta_ThriftMapType");
  static t_struct enum_type(&empty_program, "tmeta_ThriftEnumType");
  static t_struct struct_type(&empty_program, "tmeta_ThriftStructType");
  static t_struct union_type(&empty_program, "tmeta_ThriftUnionType");
  static t_struct typedef_type(&empty_program, "tmeta_ThriftTypedefType");
  static t_struct stream_type(&empty_program, "tmeta_ThriftStreamType");
  static t_struct sink_type(&empty_program, "tmeta_ThriftSinkType");
  if (!type.get_members().empty()) {
    return (t_type*)&type;
  }

  primitive_type.set_name("tmeta_ThriftPrimitiveType");
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_BOOL_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_BYTE_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_I16_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_I32_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_I64_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_FLOAT_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_DOUBLE_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_BINARY_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_STRING_TYPE);
  append_to_t_enum(
      &primitive_type, &empty_program, ThriftPrimitiveType::THRIFT_VOID_TYPE);

  list_type.append(std::make_unique<t_field>((t_type*)&type, "valueType"));
  set_type.append(std::make_unique<t_field>((t_type*)&type, "valueType"));
  map_type.append(std::make_unique<t_field>((t_type*)&type, "keyType"));
  map_type.append(std::make_unique<t_field>((t_type*)&type, "valueType"));
  enum_type.append(std::make_unique<t_field>(string_type(), "name"));
  struct_type.append(std::make_unique<t_field>(string_type(), "name"));
  union_type.append(std::make_unique<t_field>(string_type(), "name"));
  typedef_type.append(std::make_unique<t_field>(string_type(), "name"));
  typedef_type.append(
      std::make_unique<t_field>((t_type*)&type, "underlyingType"));
  stream_type.append(std::make_unique<t_field>((t_type*)&type, "elemType"));
  stream_type.append(
      std::make_unique<t_field>((t_type*)&type, "initialResponseType"));
  sink_type.append(std::make_unique<t_field>((t_type*)&type, "elemType"));
  sink_type.append(
      std::make_unique<t_field>((t_type*)&type, "finalResponseType"));
  sink_type.append(
      std::make_unique<t_field>((t_type*)&type, "initialResponseType"));

  type.append(
      std::make_unique<t_field>((t_type*)&primitive_type, "t_primitive"));
  type.append(std::make_unique<t_field>((t_type*)&list_type, "t_list"));
  type.append(std::make_unique<t_field>((t_type*)&set_type, "t_set"));
  type.append(std::make_unique<t_field>((t_type*)&map_type, "t_map"));
  type.append(std::make_unique<t_field>((t_type*)&enum_type, "t_enum"));
  type.append(std::make_unique<t_field>((t_type*)&struct_type, "t_struct"));
  type.append(std::make_unique<t_field>((t_type*)&union_type, "t_union"));
  type.append(std::make_unique<t_field>((t_type*)&typedef_type, "t_typedef"));
  type.append(std::make_unique<t_field>((t_type*)&stream_type, "t_stream"));
  type.append(std::make_unique<t_field>((t_type*)&sink_type, "t_sink"));
  return (t_type*)&type;
}

const t_type* t_hack_generator::tmeta_ThriftField_type() {
  static t_program empty_program("");
  static t_struct type(&empty_program, "tmeta_ThriftField");
  if (!type.get_members().empty()) {
    return (t_type*)&type;
  }

  type.append(std::make_unique<t_field>(i64_type(), "id"));
  type.append(std::make_unique<t_field>(tmeta_ThriftType_type(), "type"));
  type.append(std::make_unique<t_field>(string_type(), "name"));
  type.append(std::make_unique<t_field>(bool_type(), "is_optional"));
  return (t_type*)&type;
}

const t_type* t_hack_generator::tmeta_ThriftFunction_type() {
  static t_program empty_program("");
  static t_struct type(&empty_program, "tmeta_ThriftFunction");
  static t_list tlist(tmeta_ThriftField_type());
  if (!type.get_members().empty()) {
    return (t_type*)&type;
  }

  type.append(std::make_unique<t_field>(string_type(), "name"));
  type.append(
      std::make_unique<t_field>(tmeta_ThriftType_type(), "return_type"));
  type.append(std::make_unique<t_field>((t_type*)&tlist, "arguments"));
  type.append(std::make_unique<t_field>((t_type*)&tlist, "exceptions"));
  type.append(std::make_unique<t_field>(bool_type(), "is_oneway"));
  return (t_type*)&type;
}

const t_type* t_hack_generator::tmeta_ThriftService_type() {
  static t_program empty_program("");
  static t_struct type(&empty_program, "tmeta_ThriftService");
  static t_list tlist(tmeta_ThriftFunction_type());
  if (!type.get_members().empty()) {
    return (t_type*)&type;
  }

  type.append(std::make_unique<t_field>(string_type(), "name"));
  type.append(std::make_unique<t_field>((t_type*)&tlist, "functions"));
  type.append(std::make_unique<t_field>(string_type(), "parent"));
  return (t_type*)&type;
}

/**
 * Make a struct
 */
void t_hack_generator::generate_struct(const t_struct* tstruct) {
  generate_php_struct(tstruct, false);
}

/**
 * Generates a struct definition for a thrift exception. Basically the same
 * as a struct but extends the Exception class.
 *
 * @param txception The struct definition
 */
void t_hack_generator::generate_xception(const t_struct* txception) {
  generate_php_struct(txception, true);
}

/**
 * Structs can be normal or exceptions.
 */
void t_hack_generator::generate_php_struct(
    const t_struct* tstruct,
    bool is_exception) {
  generate_php_struct_definition(f_types_, tstruct, is_exception, false);
}

void t_hack_generator::generate_php_type_spec(ofstream& out, const t_type* t) {
  // Check the adapter before resolving typedefs.
  if (const auto* adapter = get_hack_adapter(t)) {
    indent(out) << "'adapter' => " << *adapter << "::class,\n";
  }
  t = t->get_true_type();
  indent(out) << "'type' => " << type_to_enum(t) << ",\n";

  if (t->is_base_type()) {
    // Noop, type is all we need
  } else if (t->is_enum()) {
    indent(out) << "'enum' => " << hack_name(t) << "::class,\n";
  } else if (t->is_struct() || t->is_xception()) {
    indent(out) << "'class' => " << hack_name(t) << "::class,\n";
  } else if (t->is_map()) {
    const t_type* ktype = ((t_map*)t)->get_key_type();
    const t_type* vtype = ((t_map*)t)->get_val_type();
    if (get_hack_adapter(ktype)) {
      throw std::runtime_error(
          "using hack.adapter annotation with map keys is not supported yet");
    }
    indent(out) << "'ktype' => " << type_to_enum(ktype) << ",\n";
    indent(out) << "'vtype' => " << type_to_enum(vtype) << ",\n";
    indent(out) << "'key' => shape(\n";
    indent_up();
    generate_php_type_spec(out, ktype);
    indent_down();
    indent(out) << "),\n";
    indent(out) << "'val' => shape(\n";
    indent_up();
    generate_php_type_spec(out, vtype);
    indent_down();
    indent(out) << "),\n";
    if (arrays_) {
      indent(out) << "'format' => 'harray',\n";
    } else if (no_use_hack_collections_) {
      indent(out) << "'format' => 'array',\n";
    } else {
      indent(out) << "'format' => 'collection',\n";
    }
  } else if (t->is_list()) {
    const t_type* etype = ((t_list*)t)->get_elem_type();
    indent(out) << "'etype' => " << type_to_enum(etype) << ",\n";
    indent(out) << "'elem' => shape(\n";
    indent_up();
    generate_php_type_spec(out, etype);
    indent_down();
    indent(out) << "),\n";
    if (arrays_) {
      indent(out) << "'format' => 'harray',\n";
    } else if (no_use_hack_collections_) {
      indent(out) << "'format' => 'array',\n";
    } else {
      indent(out) << "'format' => 'collection',\n";
    }
  } else if (t->is_set()) {
    const t_type* etype = ((t_set*)t)->get_elem_type();
    if (get_hack_adapter(etype)) {
      throw std::runtime_error(
          "using hack.adapter annotation with set keys is not supported yet");
    }
    indent(out) << "'etype' => " << type_to_enum(etype) << ",\n";
    indent(out) << "'elem' => shape(\n";
    indent_up();
    generate_php_type_spec(out, etype);
    indent_down();
    indent(out) << "),\n";
    if (arrays_) {
      indent(out) << "'format' => 'harray',\n";
    } else if (arraysets_) {
      indent(out) << "'format' => 'array',\n";
    } else {
      indent(out) << "'format' => 'collection',\n";
    }
  } else {
    throw std::runtime_error(
        "compiler error: no type for php struct spec field");
  }
}

/**
 * Generates the struct specification structure, which fully qualifies enough
 * type information to generalize serialization routines.
 */
void t_hack_generator::generate_php_struct_spec(
    ofstream& out,
    const t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  indent(out) << "const dict<int, this::TFieldSpec> SPEC = dict[\n";
  indent_up();
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    const t_type* t = (*m_iter)->get_type();
    indent(out) << (*m_iter)->get_key() << " => shape(\n";
    indent_up();
    out << indent() << "'var' => '" << (*m_iter)->get_name() << "',\n";
    if (tstruct->is_union()) {
      // Optimally, we shouldn't set this per field but rather per struct.
      // However, the tspec is a field_id => data array, and if we set it
      // at the top level people might think the 'union' key is a field
      // id, which isn't cool. It's safer and more bc to instead set this
      // key on all fields.
      out << indent() << "'union' => true,\n";
    }
    generate_php_type_spec(out, t);
    indent_down();
    indent(out) << "),\n";
  }
  indent_down();
  indent(out) << "];\n";

  indent(out) << "const dict<string, int> FIELDMAP = dict[\n";
  indent_up();
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out) << "'" << (*m_iter)->get_name() << "' => "
                << (*m_iter)->get_key() << ",\n";
  }
  indent_down();
  indent(out) << "];\n";
}

void t_hack_generator::generate_php_struct_struct_trait(
    std::ofstream& out,
    const t_struct* tstruct,
    const std::string& name) {
  string traitName;
  if (const auto* structtrait =
          tstruct->get_annotation_or_null("php.structtrait")) {
    if (structtrait->empty() || *structtrait == "1") {
      traitName = hack_name(name, tstruct->get_program()) + "Trait";
    } else {
      traitName = hack_name(*structtrait, tstruct->get_program());
    }
  } else if (struct_trait_) {
    traitName = hack_name(name, tstruct->get_program()) + "Trait";
  }

  if (!traitName.empty()) {
    indent(out) << "use " << traitName << ";\n\n";
  }
}

void t_hack_generator::generate_php_struct_shape_spec(
    std::ofstream& out,
    const t_struct* tstruct,
    bool is_constructor_shape) {
  indent(out) << "const type "
              << (is_constructor_shape ? "TConstructorShape" : "TShape")
              << " = shape(\n";
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    const t_type* t = (*m_iter)->get_type();
    // Compute typehint before resolving typedefs to avoid missing any adapter
    // annotations.
    string typehint = type_to_typehint(t, false, !is_constructor_shape);

    t = t->get_true_type();
    string dval = "";
    if ((*m_iter)->get_value() != nullptr &&
        !(t->is_struct() || t->is_xception())) {
      dval = render_const_value(t, (*m_iter)->get_value());
    } else {
      dval = render_default_value(t);
    }

    bool nullable =
        nullable_everything_ || field_is_nullable(tstruct, *m_iter, dval);

    string prefix = nullable || is_constructor_shape ? "?" : "";

    indent(out) << "  " << prefix << "'" << (*m_iter)->get_name() << "' => "
                << prefix << typehint << ",\n";
  }
  if (!is_constructor_shape && shapes_allow_unknown_fields_) {
    indent(out) << "  ...\n";
  }
  indent(out) << ");\n";
}

/**
 * Generate a Lambda on a Collection Value.
 *
 * For example, if our structure is:
 *
 * 1: map<string, list<i32>> map_of_string_to_list_of_i32;
 *
 * Then our __toShape() routine results in:
 *
 *   'map_of_string_to_list_of_i32' => $this->map_of_string_to_list_of_i32->map(
 *     $_val0 ==> $_val0->toVArray(),
 *   )
 *     |> ThriftUtil::toDArray($$),
 *
 * And this method here will get called with
 *
 *  generate_php_struct_shape_collection_value_lambda(..., list<i32>)
 *
 * And returns the string:
 *
 *   "  $_val0 ==> $_val0->toVArray(),"
 *
 * This method operates via recursion on complex types.
 */
void t_hack_generator::generate_php_struct_shape_collection_value_lambda(
    std::ostream& out,
    t_name_generator& namer,
    const t_type* t) {
  string tmp = namer("_val");
  indent(out);
  if (arrprov_skip_frames_) {
    out << "<<__ProvenanceSkipFrame>> ";
  }
  out << "($" << tmp << ") ==> ";
  if (t->is_struct()) {
    out << "$" << tmp << "->__toShape(),\n";
  } else if (t->is_set()) {
    if (arraysets_ || no_use_hack_collections_) {
      out << "darray($" << tmp << "),\n";
    } else {
      out << "ThriftUtil::toDArray(Dict\\fill_keys($" << tmp << ", true)),\n";
    }
  } else if (t->is_map() || t->is_list()) {
    const t_type* val_type;
    if (t->is_map()) {
      val_type = ((t_map*)t)->get_val_type();
    } else {
      val_type = ((t_list*)t)->get_elem_type();
    }
    val_type = val_type->get_true_type();

    if (!val_type->is_container() && !val_type->is_struct()) {
      out << generate_to_array_method(t) << "($" << tmp << "),\n";
      return;
    }

    out << "$" << tmp << "->map(\n";
    indent_up();
    generate_php_struct_shape_collection_value_lambda(out, namer, val_type);
    indent_down();
    indent(out) << ")\n";
    indent_up();
    indent(out) << "|> " << generate_to_array_method(t) << "($$),\n";
    indent_down();
  }
}

void t_hack_generator::generate_hack_array_from_shape_lambda(
    std::ostream& out,
    t_name_generator& namer,
    const t_type* t) {
  if (t->is_map()) {
    out << "Dict\\map(\n";
  } else {
    out << "Vec\\map(\n";
  }
  indent_up();
  indent(out) << "$$,\n";
  string tmp = namer("_val");
  indent(out) << "$" << tmp << " ==> $" << tmp;

  const t_type* val_type;
  if (t->is_map()) {
    val_type = ((t_map*)t)->get_val_type();
  } else {
    val_type = ((t_list*)t)->get_elem_type();
  }
  val_type = val_type->get_true_type();

  if (val_type->is_map() || val_type->is_list() || val_type->is_struct()) {
    indent_up();
    out << "\n";
    indent(out) << "|> ";

    if (val_type->is_struct()) {
      string type = hack_name(val_type);
      out << type << "::__fromShape($$),\n";
    } else {
      generate_hack_array_from_shape_lambda(out, namer, val_type);
      out << ",\n";
    }

    indent_down();
  } else {
    out << ",\n";
  }

  indent_down();
  indent(out) << ")";
  if (no_use_hack_collections_) {
    if (t->is_map()) {
      out << " |> darray($$)";
    } else {
      out << " |> varray($$)";
    }
  }
}

void t_hack_generator::generate_shape_from_hack_array_lambda(
    std::ostream& out,
    t_name_generator& namer,
    const t_type* t) {
  if (no_use_hack_collections_) {
    out << "(\n";
    indent_up();
    indent(out);
  }
  if (t->is_map()) {
    out << "Dict\\map(\n";
  } else {
    out << "Vec\\map(\n";
  }
  indent_up();
  indent(out) << "$$,\n";
  string tmp = namer("_val");
  indent(out);
  if (arrprov_skip_frames_) {
    out << "<<__ProvenanceSkipFrame>> ";
  }
  out << "($" << tmp << ") ==> $" << tmp;

  const t_type* val_type;
  if (t->is_map()) {
    val_type = ((t_map*)t)->get_val_type();
  } else {
    val_type = ((t_list*)t)->get_elem_type();
  }
  val_type = val_type->get_true_type();

  if (val_type->is_struct()) {
    out << "->__toShape(),\n";
  } else if (val_type->is_map() || val_type->is_list()) {
    indent_up();
    out << "\n";
    indent(out) << "|> ";
    generate_shape_from_hack_array_lambda(out, namer, val_type);
    indent_down();
  } else {
    out << ",\n";
  }

  indent_down();
  if (no_use_hack_collections_) {
    if (t->is_map()) {
      indent(out) << ") |> darray($$)\n";
    } else {
      indent(out) << ") |> varray($$)\n";
    }
    indent_down();
  }
  indent(out) << "),\n";
}

bool t_hack_generator::type_has_nested_struct(const t_type* t) {
  bool has_struct = false;
  const t_type* val_type = t;
  while (true) {
    if (val_type->is_map()) {
      val_type = ((t_map*)val_type)->get_val_type();
    } else {
      val_type = ((t_list*)val_type)->get_elem_type();
    }
    val_type = val_type->get_true_type();

    if (!(val_type->is_map() || val_type->is_list())) {
      if (val_type->is_struct()) {
        has_struct = true;
      }
      break;
    }
  }

  return has_struct;
}

/**
 * Determine whether a field should be marked nullable.
 */
bool t_hack_generator::field_is_nullable(
    const t_struct* tstruct,
    const t_field* field,
    string dval) {
  const t_type* t = field->get_type()->get_true_type();
  return (dval == "null") || tstruct->is_union() ||
      (field->get_req() == t_field::e_req::optional &&
       field->get_value() == nullptr) ||
      (t->is_enum() && field->get_req() != t_field::e_req::required);
}

void t_hack_generator::generate_php_struct_shape_methods(
    std::ofstream& out,
    const t_struct* tstruct) {
  if (shape_arraykeys_) {
    string arg_return_type;
    if (arrays_) {
      arg_return_type = "dict";
    } else if (no_use_hack_collections_) {
      arg_return_type = "darray";
    } else if (const_collections_) {
      arg_return_type = "ConstMap";
    } else {
      arg_return_type = "Map";
    }
    indent(out) << "public static function __stringifyMapKeys<T>("
                << arg_return_type << "<arraykey, T> $m)[]: " << arg_return_type
                << "<string, T> {\n";
    indent_up();
    if (arrays_) {
      indent(out) << "return Dict\\map_keys($m, $key ==> (string)$key);\n";
    } else if (no_use_hack_collections_) {
      // There doesn't seem to be an eqivalent for map_keys.
      indent(out)
          << "return darray(Dict\\map_keys($m, $key ==> (string)$key));\n";
    } else {
      indent(out) << "return Map::fromItems(\n";
      indent_up();
      indent(out)
          << "$m->items()->map($item ==> Pair {(string)$item[0], $item[1]}),\n";
      indent_down();
      indent(out) << ");\n";
    }
    indent_down();
    indent(out) << "}\n\n";
  }

  if (arrprov_skip_frames_)
    indent(out) << "<<__ProvenanceSkipFrame>>\n";
  indent(out)
      << "public static function __fromShape(self::TShape $shape)[]: this {\n";
  indent_up();
  indent(out) << "return new static(\n";
  indent_up();

  const vector<t_field*>& members = tstruct->get_members();
  t_name_generator namer;
  for (const auto& member : members) {
    const t_type* t = member->get_type()->get_true_type();

    string dval = "";
    if (member->get_value() != nullptr &&
        !(t->is_struct() || t->is_xception())) {
      dval = render_const_value(t, member->get_value());
    } else {
      dval = render_default_value(t);
    }

    bool nullable =
        field_is_nullable(tstruct, member, dval) || nullable_everything_;

    stringstream source;
    source << "$shape['" << member->get_name() << "']";

    stringstream val;
    indent(val);

    if (tstruct->is_union() || nullable) {
      val << "Shapes::idx($shape, '" << member->get_name()
          << "') === null ? null : (";
    }

    if (t->is_set()) {
      if (arraysets_ || arrays_ || no_use_hack_collections_) {
        val << source.str();
      } else {
        val << "new Set(Keyset\\keys(" << source.str() << "))";
      }
    } else if (t->is_map() || t->is_list()) {
      bool stringify_map_keys = false;
      if (t->is_map() && shape_arraykeys_) {
        const t_type* key_type = ((t_map*)t)->get_key_type();
        if (key_type->is_base_type() && key_type->is_string_or_binary()) {
          stringify_map_keys = true;
        }
      }

      if (stringify_map_keys) {
        val << "self::__stringifyMapKeys(";
      }

      if (arrays_ || no_use_hack_collections_) {
        val << source.str();
        if (type_has_nested_struct(t)) {
          indent_up();
          val << endl;
          indent(val) << "|> ";
          generate_hack_array_from_shape_lambda(val, namer, t);
          indent_down();
        }
        if (stringify_map_keys) {
          val << ")";
        }
      } else {
        val << (stringify_map_keys ? "" : "(");
        if (t->is_map()) {
          val << "new Map(";
        } else {
          val << "new Vector(";
        }

        val << source.str() << "))";

        int nest = 0;
        while (true) {
          const t_type* val_type;
          if (t->is_map()) {
            val_type = ((t_map*)t)->get_val_type();
          } else {
            val_type = ((t_list*)t)->get_elem_type();
          }
          val_type = val_type->get_true_type();

          if ((val_type->is_set() && !arraysets_) || val_type->is_map() ||
              val_type->is_list() || val_type->is_struct()) {
            indent_up();
            nest++;
            val << "->map(\n";

            if (val_type->is_set()) {
              string tmp = namer("val");
              indent(val) << "$" << tmp << " ==> new Set(Keyset\\keys($" << tmp
                          << ")),\n";
              break;
            } else if (val_type->is_map() || val_type->is_list()) {
              string tmp = namer("val");

              stringify_map_keys = false;
              if (val_type->is_map() && shape_arraykeys_) {
                const t_type* key_type = ((t_map*)val_type)->get_key_type();
                if (key_type->is_base_type() &&
                    key_type->is_string_or_binary()) {
                  stringify_map_keys = true;
                }
              }

              indent(val) << "$" << tmp << " ==> "
                          << (stringify_map_keys ? "self::__stringifyMapKeys"
                                                 : "")
                          << "(new ";
              if (val_type->is_map()) {
                val << "Map";
              } else {
                val << "Vector";
              }
              val << "($" << tmp << "))";
              t = val_type;
            } else if (val_type->is_struct()) {
              string tmp = namer("val");
              string type = hack_name(val_type);
              indent(val) << "$" << tmp << " ==> " << type << "::__fromShape("
                          << "$" << tmp << "),\n";
              break;
            }
          } else {
            if (nest > 0) {
              val << ",\n";
            }
            break;
          }
        }
        while (nest-- > 0) {
          indent_down();
          indent(val) << ")";
          if (nest > 0) {
            val << ",\n";
          }
        }
      }
    } else if (t->is_struct()) {
      string type = hack_name(t);
      val << type << "::__fromShape(" << source.str() << ")";
    } else {
      val << source.str();
    }
    if (tstruct->is_union() || nullable) {
      val << ")";
    }
    val << ",\n";
    out << val.str();
  }
  indent_down();
  indent(out) << ");\n";
  indent_down();
  indent(out) << "}\n";
  out << "\n";

  if (arrprov_skip_frames_) {
    indent(out) << "<<__ProvenanceSkipFrame>>\n";
  }
  indent(out) << "public function __toShape()[]: self::TShape {\n";
  indent_up();
  indent(out) << "return shape(\n";
  indent_up();
  for (const auto& member : members) {
    const t_type* t = member->get_type()->get_true_type();
    t_name_generator ngen;

    indent(out) << "'" << member->get_name() << "' => ";

    stringstream val;

    bool nullable =
        field_is_nullable(tstruct, member, render_default_value(t)) ||
        nullable_everything_;

    if (t->is_container()) {
      if (t->is_map() || t->is_list()) {
        if (arrays_ || no_use_hack_collections_) {
          val << "$this->" << member->get_name();
          if (type_has_nested_struct(t)) {
            val << "\n";
            indent_up();
            indent(val) << "|> ";
            if (nullable) {
              val << "$$ === null \n";
              indent_up();
              indent(val) << "? null \n";
              indent(val) << ": ";
            }
            generate_shape_from_hack_array_lambda(val, ngen, t);
            if (nullable) {
              indent_down();
            }
            indent_down();
          } else {
            val << ",\n";
          }
        } else {
          const t_type* val_type;
          if (t->is_map()) {
            val_type = ((t_map*)t)->get_val_type();
          } else {
            val_type = ((t_list*)t)->get_elem_type();
          }
          val_type = val_type->get_true_type();

          if (val_type->is_container() || val_type->is_struct() || nullable) {
            val << "$this->" << member->get_name();
            if (val_type->is_container() || val_type->is_struct()) {
              val << (nullable ? "?" : "") << "->map(\n";
              indent_up();
              generate_php_struct_shape_collection_value_lambda(
                  val, ngen, val_type);
              indent_down();
              indent(val) << ")";
            }
            val << endl;
            indent_up();
            indent(val) << "|> " << (nullable ? "$$ === null ? null : " : "")
                        << generate_to_array_method(t) << "($$),\n";
            indent_down();
          } else {
            val << generate_to_array_method(t) << "($this->"
                << member->get_name() << "),\n";
          }
        }
      } else if (arraysets_ || arrays_ || no_use_hack_collections_) {
        val << "$this->" << member->get_name() << ",\n";
      } else {
        if (nullable) {
          val << "$this->" << member->get_name() << "\n";
          indent_up();
          indent(val) << "|> $$ === null ? null : ";
        }
        val << "ThriftUtil::toDArray(Dict\\fill_keys(";
        if (nullable) {
          val << "$$";
        } else {
          val << "$this->" << member->get_name();
        }
        val << "->toValuesArray(), true)),\n";
        if (nullable) {
          indent_down();
        }
      }
    } else if (t->is_struct()) {
      val << "$this->" << member->get_name();
      val << (nullable ? "?" : "") << "->__toShape(),\n";
    } else {
      val << "$this->" << member->get_name() << ",\n";
    }

    out << val.str();
  }
  indent_down();
  indent(out) << ");\n";
  indent_down();
  indent(out) << "}\n";
}

/**
 * Generates the structural ID definition, see generate_structural_id()
 * for information about the structural ID.
 */
void t_hack_generator::generate_php_structural_id(
    ofstream& out,
    const t_struct* tstruct,
    bool asFunction) {
  if (asFunction) {
    indent(out) << "static function getStructuralID(): int {\n";
    indent_up();
    indent(out) << "return " << generate_structural_id(tstruct) << ";\n";
    indent_down();
    indent(out) << "}\n";
  } else {
    indent(out) << "const int STRUCTURAL_ID = "
                << generate_structural_id(tstruct) << ";\n";
  }
}

void t_hack_generator::generate_php_struct_definition(
    ofstream& out,
    const t_struct* tstruct,
    bool is_exception,
    bool is_result,
    bool is_args,
    const std::string& name) {
  const std::string& real_name = name.empty() ? tstruct->get_name() : name;
  if (tstruct->is_union()) {
    // Generate enum for union before the actual class
    generate_php_union_enum(out, tstruct, real_name);
  }
  _generate_php_struct_definition(
      out, tstruct, is_exception, is_result, is_args, real_name);
}

void t_hack_generator::generate_php_union_methods(
    ofstream& out,
    const t_struct* tstruct) {
  vector<t_field*>::const_iterator m_iter;
  const vector<t_field*>& members = tstruct->get_members();
  auto enumName = union_enum_name(tstruct);

  indent(out) << "public function getType(): " << enumName << " {\n";
  indent(out) << indent() << "return $this->_type;\n";
  indent(out) << "}\n\n";

  out << indent() << "public function reset(): void {\n";
  indent_up();
  out << indent() << "switch ($this->_type) {\n";
  indent_up();
  for (const auto& m : members) {
    auto fieldName = m->get_name();
    out << indent() << "case " << enumName << "::" << fieldName << ":\n";
    out << indent(get_indent() + 1) << "$this->" << fieldName << " = null;\n";
    out << indent(get_indent() + 1) << "break;\n";
  }
  out << indent() << "case " << enumName << "::_EMPTY_:\n";
  out << indent(get_indent() + 1) << "break;\n";
  indent_down();
  out << indent() << "}\n";
  out << indent() << "$this->_type = " << enumName << "::_EMPTY_;\n";
  indent_down();
  out << "}\n\n";

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    auto fieldName = (*m_iter)->get_name();
    auto typehint = type_to_typehint((*m_iter)->get_type());
    // set_<fieldName>()
    indent(out) << "public function set_" << fieldName << "(" << typehint
                << " $" << fieldName << "): this {\n";
    indent_up();
    indent(out) << "return $this->setx_" << fieldName << "($" << fieldName
                << ");\n ";
    indent_down();
    indent(out) << "}\n\n";

    // setx_<fieldName>()
    indent(out) << "public function setx_" << fieldName << "(" << typehint
                << " $" << fieldName << "): this {\n";
    indent_up();
    indent(out) << "$this->reset();\n";
    indent(out) << "$this->_type = " << enumName << "::" << fieldName << ";\n";
    indent(out) << "$this->" << fieldName << " = "
                << "$" << fieldName << ";\n";
    indent(out) << "return $this;\n";
    indent_down();
    indent(out) << "}\n\n";

    indent(out) << "public function get_" << fieldName << "(): " << typehint
                << " {\n";
    indent_up();
    indent(out) << "return $this->getx_" << fieldName << "();\n";
    indent_down();
    indent(out) << "}\n\n";

    indent(out) << "public function getx_" << fieldName << "(): " << typehint
                << " {\n";
    indent_up();
    indent(out) << "invariant(\n";
    indent_up();
    indent(out) << "$this->_type === " << enumName << "::" << fieldName
                << ",\n";
    indent(out) << "'get_" << fieldName << " called on an instance of "
                << tstruct->get_name() << " whose current type is %s',\n";
    indent(out) << "(string)$this->_type,\n";
    indent_down();
    indent(out) << ");\n";
    indent(out) << "return $this->" << fieldName << " as nonnull;\n";
    indent_down();
    indent(out) << "}\n\n";
  }
}

void t_hack_generator::generate_php_union_enum(
    ofstream& out,
    const t_struct* tstruct,
    const std::string& name) {
  // Generate enum class with this pattern
  // enum <UnionName>Enum: int {
  //   __EMPTY__ = 0;
  //   field1 = 1;
  // }
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  if (string const* union_enum_attributes =
          tstruct->get_annotation_or_null("hack.union_enum_attributes")) {
    indent(out) << "<<" << *union_enum_attributes << ">>\n";
  }
  out << "enum " << union_enum_name(name, tstruct->get_program(), true)
      << ": int {\n";

  indent_up();
  // If no member is set
  indent(out) << UNION_EMPTY << " = 0;\n";
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out) << (*m_iter)->get_name() << " = " << (*m_iter)->get_key()
                << ";\n";
  }
  indent_down();
  out << "}\n\n";
}

bool t_hack_generator::is_base_exception_property(const t_field* field) {
  static const std::unordered_set<string> kBaseExceptionProperties{
      "code", "message", "line", "file"};
  return kBaseExceptionProperties.find(field->get_name()) !=
      kBaseExceptionProperties.end();
}

std::string t_hack_generator::render_structured_annotations(
    const std::vector<const t_const*>& annotations) {
  std::ostringstream out;
  out << "dict[";
  if (!annotations.empty()) {
    out << "\n";
    indent_up();
    for (const auto& annotation : annotations) {
      indent(out) << "'" << hack_name(annotation->get_type()) << "' => "
                  << render_const_value(
                         annotation->get_type(), annotation->get_value())
                  << ",\n";
    }
    indent_down();
    indent(out);
  }
  out << "]";
  return out.str();
}

void t_hack_generator::generate_adapter_type_checks(
    ofstream& out,
    const t_struct* tstruct) {
  // Adapter name -> original type of the field that the adapter is for.
  set<pair<string, string>> adapter_types_;

  std::function<void(const t_type*)> collect_adapters_recursively =
      [&](const t_type* t) {
        // Check the adapter before resolving typedefs.
        if (const auto* adapter = get_hack_adapter(t)) {
          adapter_types_.emplace(
              *adapter,
              type_to_typehint(
                  t, false, false, false, /* ignore_adapter */ true));
        }

        t = t->get_true_type();
        if (t->is_map()) {
          collect_adapters_recursively(((t_map*)t)->get_key_type());
          collect_adapters_recursively(((t_map*)t)->get_val_type());
        } else if (t->is_list()) {
          collect_adapters_recursively(((t_list*)t)->get_elem_type());
        } else if (t->is_set()) {
          collect_adapters_recursively(((t_set*)t)->get_elem_type());
        }
      };

  for (const auto& m : tstruct->get_members()) {
    collect_adapters_recursively(m->get_type());
  }

  if (adapter_types_.empty()) {
    return;
  }

  indent(out) << "private static function __hackAdapterTypeChecks(): void {\n";
  indent_up();
  for (const auto& kv : adapter_types_) {
    indent(out) << "\\ThriftUtil::requireSameType<" << kv.first
                << "::TThriftType, " << kv.second << ">();\n";
  }
  indent_down();
  indent(out) << "}\n\n";
}
/**
 * Generates a struct definition for a thrift data type. This is nothing in PHP
 * where the objects are all just associative arrays (unless of course we
 * decide to start using objects for them...)
 *
 * @param tstruct The struct definition
 */
void t_hack_generator::_generate_php_struct_definition(
    ofstream& out,
    const t_struct* tstruct,
    bool is_exception,
    bool is_result,
    bool is_args,
    const std::string& name) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  bool generateAsTrait = tstruct->has_annotation("php.trait");

  if (!is_result && !is_args && (is_exception || !generateAsTrait)) {
    generate_php_docstring(out, tstruct, is_exception);
  }
  if (string const* attributes =
          tstruct->get_annotation_or_null("hack.attributes")) {
    f_types_ << "<<" << *attributes << ">>\n";
  }
  out << (generateAsTrait ? "trait " : "class ")
      << hack_name(name, tstruct->get_program(), true);
  if (generateAsTrait) {
    out << "Trait";
  } else if (is_exception) {
    out << " extends \\TException";
  }
  out << " implements \\IThriftStruct";

  if (tstruct->is_union()) {
    out << ", \\IThriftUnion<" << union_enum_name(name, tstruct->get_program())
        << ">";
  }

  bool gen_shapes = shapes_ && !is_exception && !is_result;

  if (gen_shapes) {
    out << ", \\IThriftShapishStruct";
  }

  out << " {\n";
  indent_up();

  if (tstruct->is_union()) {
    indent(out) << "use \\ThriftUnionSerializationTrait;\n\n";
  } else {
    indent(out) << "use \\ThriftSerializationTrait;\n\n";
  }

  if (generateAsTrait && is_exception) {
    indent(out) << "require extends \\TException;\n";
  }

  generate_php_struct_struct_trait(out, tstruct, name);
  generate_php_struct_spec(out, tstruct);
  out << "\n";
  generate_php_struct_shape_spec(out, tstruct, true);
  out << "\n";
  if (gen_shapes) {
    generate_php_struct_shape_spec(out, tstruct);
  }

  generate_php_structural_id(out, tstruct, generateAsTrait);

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    const t_type* t = (*m_iter)->get_type();
    // Compute typehint before resolving typedefs to avoid missing any adapter
    // annotations.
    string typehint = type_to_typehint(t);

    t = t->get_true_type();

    if (t->is_enum() && is_bitmask_enum((t_enum*)t)) {
      throw std::runtime_error(
          "Enum " + (((t_enum*)t)->get_name()) +
          "is actually a bitmask, cannot generate a field of this enum type");
    }

    string dval = "";
    if ((*m_iter)->get_value() != nullptr &&
        !(t->is_struct() || t->is_xception())) {
      dval = render_const_value(t, (*m_iter)->get_value());
    } else {
      dval = render_default_value(t);
    }

    // result structs only contain fields: success and e.
    // success is whatever type the method returns, but must be nullable
    // regardless, since if there is an exception we expect it to be null
    bool nullable = (is_result || field_is_nullable(tstruct, (*m_iter), dval) ||
                     nullable_everything_) &&
        !(is_exception && is_base_exception_property(*m_iter));
    if (nullable) {
      typehint = "?" + typehint;
    }

    if (!is_result && !is_args) {
      generate_php_docstring(out, *m_iter);
    }

    if (string const* field_attributes =
            (*m_iter)->get_annotation_or_null("hack.attributes")) {
      indent(out) << "<<" << *field_attributes << ">>\n";
    }

    if (is_exception && (*m_iter)->get_name() == "code") {
      if (!(t->is_any_int() || t->is_enum())) {
        throw tstruct->get_name() +
            "::code defined to be a non-integral type. " +
            "code fields for Exception classes must be integral";
      } else if (t->is_enum() && ((t_enum*)t)->get_enum_values().empty()) {
        throw std::runtime_error(
            "Enum " + t->get_name() + " is the type for the code property of " +
            tstruct->get_name() + ", but it has no values.");
      }
      if (t->is_enum()) {
        typehint = "/* Originally defined as " + typehint + " */ int";
      }
    }

    string visibility =
        (protected_unions_ && tstruct->is_union()) ? "protected" : "public";

    indent(out) << visibility << " " << typehint << " $"
                << (*m_iter)->get_name() << ";\n";

    if (is_exception && (*m_iter)->get_name() == "code" && t->is_enum()) {
      string enum_type = type_to_typehint(t);
      out << "\n";
      out << indent() << "public function setCodeAsEnum(" << enum_type
          << " $code): void {\n";
      if (!enum_transparenttype_) {
        out << indent() << "  /* HH_FIXME[4110] nontransparent enum */\n";
      }
      out << indent() << "  $this->code = $code;" << indent() << "\n"
          << indent() << "}\n\n";
      out << indent() << "public function getCodeAsEnum(): " << enum_type
          << " {\n"
          << indent()
          << "  /* HH_FIXME[4110] retain HHVM enforcement semantics */\n"
          << indent() << "  return $this->code;" << indent() << "\n"
          << indent() << "}\n";
    }
  }

  if (tstruct->is_union()) {
    // Generate _type to store which field is set and initialize it to _EMPTY_
    indent(out) << "protected " << union_enum_name(name, tstruct->get_program())
                << " $_type = " << union_field_to_enum(tstruct, nullptr, name)
                << ";\n";
  }

  out << "\n";

  if (arrprov_skip_frames_)
    indent(out) << "<<__ProvenanceSkipFrame>>\n";
  out << indent() << "public function __construct(";
  bool first = true;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (!first) {
      out << ", ";
    }
    out << "?" << type_to_typehint((*m_iter)->get_type()) << " $"
        << (*m_iter)->get_name() << " = null";
    first = false;
  }
  out << indent() << ")[] {\n";
  indent_up();

  if (is_exception) {
    out << indent() << "parent::__construct();\n";
  }
  if (tstruct->is_union()) {
    out << indent()
        << "$this->_type = " << union_field_to_enum(tstruct, nullptr, name)
        << ";\n";
  }
  for (m_iter = members.begin(); !is_result && m_iter != members.end();
       ++m_iter) {
    const t_type* t = (*m_iter)->get_type()->get_true_type();
    string dval = "";
    if ((*m_iter)->get_value() != nullptr &&
        !(t->is_struct() || t->is_xception())) {
      dval = render_const_value(t, (*m_iter)->get_value());
    } else if (
        is_exception &&
        ((*m_iter)->get_name() == "code" || (*m_iter)->get_name() == "line")) {
      if (t->is_any_int()) {
        dval = "0";
      } else {
        // just use the lowest value
        const t_enum* tenum = (t_enum*)t;
        dval = hack_name(tenum) +
            "::" + (*(tenum->get_enum_values().begin()))->get_name();
      }
    } else if (
        is_exception &&
        ((*m_iter)->get_name() == "message" ||
         (*m_iter)->get_name() == "file")) {
      dval = "''";
    } else if (tstruct->is_union() || nullable_everything_) {
      dval = "null";
    } else {
      dval = render_default_value(t);
    }
    if (dval != "null") {
      if (const auto* adapter = get_hack_adapter((*m_iter)->get_type())) {
        dval = *adapter + "::fromThrift(" + dval + ")";
      }
    }

    // result structs only contain fields: success and e.
    // success is whatever type the method returns, but must be nullable
    // regardless, since if there is an exception we expect it to be null
    // TODO(ckwalsh) Extract this logic into a helper function
    bool nullable = !(is_exception && is_base_exception_property(*m_iter)) &&
        (dval == "null" || is_result ||
         ((*m_iter)->get_req() == t_field::e_req::optional &&
          (*m_iter)->get_value() == nullptr));
    const string& field_name = (*m_iter)->get_name();
    bool need_enum_code_fixme = is_exception && field_name == "code" &&
        t->is_enum() && !enum_transparenttype_;
    if (tstruct->is_union()) {
      // Capture value from constructor and update _type field
      out << indent() << "if ($" << field_name << " !== null) {\n";
    }
    if (need_enum_code_fixme) {
      out << indent() << "  /* HH_FIXME[4110] nontransparent Enum */\n";
    }
    out << indent() << (tstruct->is_union() ? "  " : "") << "$this->"
        << field_name << " = $" << field_name
        << (!nullable ? " ?? " + dval : "") << ";\n";
    if (tstruct->is_union()) {
      out << indent()
          << "  $this->_type = " << union_field_to_enum(tstruct, *m_iter, name)
          << ";\n"
          << indent() << "}\n";
    }
  }

  scope_down(out);
  out << "\n";

  if (arrprov_skip_frames_)
    indent(out) << "<<__ProvenanceSkipFrame>>\n";
  indent(out) << "public static function withDefaultValues()[]: this {\n";
  indent_up();
  indent(out) << "return new static();\n";
  scope_down(out);
  out << "\n";

  generate_php_struct_from_shape(out, tstruct);
  out << "\n";

  if (from_map_construct_) {
    generate_php_struct_from_map(out, tstruct);
    out << "\n";
  }

  out << indent() << "public function getName(): string {\n"
      << indent() << "  return '" << name << "';\n"
      << indent() << "}\n\n";
  if (tstruct->is_union()) {
    generate_php_union_methods(out, tstruct);
  }
  if (is_exception) {
    const auto& value = tstruct->get_annotation("message");
    if (tstruct->has_annotation("message") && value != "message") {
      auto message_field = tstruct->get_member(value);
      out << indent() << "<<__Override>>\n"
          << indent() << "public function getMessage()[]: string {\n"
          << indent() << "  return $this->" << message_field->get_name();
      if (message_field->get_req() != t_field::e_req::required) {
        out << " ?? ''";
      }
      out << ";\n" << indent() << "}\n\n";
    }
  }

  // Structured annotations
  indent(out) << "public static function getAllStructuredAnnotations(): "
                 "\\TStructAnnotations {\n";
  indent_up();
  indent(out) << "return shape(\n";
  indent_up();
  indent(out) << "'struct' => "
              << render_structured_annotations(
                     tstruct->structured_annotations())
              << ",\n";
  indent(out) << "'fields' => dict[\n";
  indent_up();
  for (const auto& field : tstruct->get_members()) {
    if (field->structured_annotations().empty() &&
        field->get_type()->structured_annotations().empty()) {
      continue;
    }
    indent(out) << "'" << field->get_name() << "' => shape(\n";
    indent_up();
    indent(out) << "'field' => "
                << render_structured_annotations(
                       field->structured_annotations())
                << ",\n";
    indent(out) << "'type' => "
                << render_structured_annotations(
                       field->get_type()->structured_annotations())
                << ",\n";
    indent_down();
    indent(out) << "),\n";
  }
  indent_down();
  indent(out) << "],\n";
  indent_down();
  indent(out) << ");\n";
  indent_down();
  indent(out) << "}\n\n";

  if (gen_shapes) {
    generate_php_struct_shape_methods(out, tstruct);
  }
  generate_json_reader(out, tstruct);
  generate_adapter_type_checks(out, tstruct);
  indent_down();

  out << indent() << "}\n\n";
}

void t_hack_generator::generate_php_struct_from_shape(
    ofstream& out,
    const t_struct* tstruct) {
  if (arrprov_skip_frames_)
    indent(out) << "<<__ProvenanceSkipFrame>>\n";
  out << indent() << "public static function fromShape"
      << "(self::TConstructorShape $shape)[]: this {\n";
  indent_up();
  out << indent() << "return new static(\n";
  indent_up();
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    string name = (*m_iter)->get_name();
    out << indent() << "Shapes::idx($shape, '" << name << "'),\n";
  }
  indent_down();
  out << indent() << ");\n";
  indent_down();
  out << indent() << "}\n";
}

void t_hack_generator::generate_php_struct_from_map(
    ofstream& out,
    const t_struct* tstruct) {
  if (arrprov_skip_frames_)
    indent(out) << "<<__ProvenanceSkipFrame>>\n";
  out << indent() << "public static function fromMap_DEPRECATED(";
  if (strict_types_) {
    // Generate constructor from Map
    out << (const_collections_ ? "Const" : "") << "Map<string, mixed> $map";
  } else {
    // Generate constructor from KeyedContainer
    out << (soft_attribute_ ? "<<__Soft>> " : "@")
        << "KeyedContainer<string, mixed> $map";
  }
  out << ")[]: this {\n";
  indent_up();
  out << indent() << "return new static(\n";
  indent_up();
  for (const auto& tfield : tstruct->get_members()) {
    out << indent()
        << "/* HH_FIXME[4110] For backwards compatibility with map's mixed values. */\n";
    out << indent() << "idx($map, '" << tfield->get_name() << "'),\n";
  }
  indent_down();
  out << indent() << ");\n";
  indent_down();
  out << indent() << "}\n";
}

/**
 * Generates a thrift service.
 *
 * @param tservice The service definition
 */
void t_hack_generator::generate_service(const t_service* tservice) {
  if (mangled_services_) {
    if (php_namespace(tservice).empty()) {
      throw std::runtime_error(
          "cannot generate mangled services for " + tservice->get_name() +
          "; no php namespace found");
    }
    // Note: Because calling generate_service again "uses up" tmp variables,
    //   generating a mangled service has the effect of changing the files of
    //   unmangled services declared in the same thrift file (i.e., with
    //   different tmp variables). Thus we store/restore the tmp_ counter so
    //   that unmangled service files are not affected.
    int orig_tmp = get_tmp_counter();
    // generate new files for mangled services, if requested
    generate_service(tservice, true);
    set_tmp_counter(orig_tmp);
  } else {
    generate_service(tservice, false);
  }
}

void t_hack_generator::generate_service(
    const t_service* tservice,
    bool mangle) {
  string f_base_name = php_servicename_mangle(mangle, tservice);
  string f_service_name = get_out_dir() + f_base_name + ".php";
  f_service_.open(f_service_name.c_str());
  record_genfile(f_service_name);

  f_service_ << "<?hh // strict\n" << autogen_comment() << "\n";
  string hack_ns = hack_namespace(program_);
  if (!hack_ns.empty()) {
    f_service_ << "namespace " << hack_ns << ";\n\n";
  }

  // Generate the main parts of the service
  generate_service_interface(
      tservice,
      mangle,
      /*async*/ true,
      /*rpc_options*/ false,
      /*client*/ false);
  generate_service_interface(
      tservice,
      mangle,
      /*async*/ false,
      /*rpc_options*/ false,
      /*client*/ false);
  generate_service_interface(
      tservice,
      mangle,
      /*async*/ false,
      /*rpc_options*/ false,
      /*client*/ true);
  generate_service_interface(
      tservice, mangle, /*async*/ true, /*rpc_options*/ true, /*client*/ false);
  generate_service_client(tservice, mangle);
  generate_service_interactions(tservice, mangle);
  if (phps_) {
    generate_service_processor(tservice, mangle, /*async*/ true);
    generate_service_processor(tservice, mangle, /*async*/ false);
  }
  // Generate the structures passed around and helper functions
  generate_service_helpers(tservice, mangle);

  // Close service file
  f_service_.close();
}

/**
 * Generates event handler functions.
 */
void t_hack_generator::generate_processor_event_handler_functions(
    ofstream& out) {
  generate_event_handler_functions(out, "\\TProcessorEventHandler");
}
void t_hack_generator::generate_client_event_handler_functions(ofstream& out) {
  generate_event_handler_functions(out, "\\TClientEventHandler");
}
void t_hack_generator::generate_event_handler_functions(
    ofstream& /*out*/,
    string cl) {
  f_service_ << indent() << "public function setEventHandler(" << cl
             << " $event_handler): this {\n"
             << indent() << "  $this->eventHandler_ = $event_handler;\n"
             << indent() << "  return $this;\n"
             << indent() << "}\n\n";

  indent(f_service_) << "public function getEventHandler(): " << cl << " {\n"
                     << indent() << "  return $this->eventHandler_;\n"
                     << indent() << "}\n\n";
}

/**
 * Generates a service server definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_hack_generator::generate_service_processor(
    const t_service* tservice,
    bool mangle,
    bool async) {
  // Generate the dispatch methods
  vector<t_function*> functions = get_supported_functions(tservice);
  vector<t_function*>::iterator f_iter;

  string suffix = async ? "Async" : "Sync";
  string extends = "";
  string extends_processor = string("\\Thrift") + suffix + "Processor";
  if (tservice->get_extends() != nullptr) {
    extends = php_servicename_mangle(mangle, tservice->get_extends(), true);
    extends_processor = extends + suffix + "ProcessorBase";
  }

  string long_name = php_servicename_mangle(mangle, tservice);

  // I hate to make this abstract, but Hack doesn't support overriding const
  // types. Thus, we will have an inheritance change that does not define the
  // const type, then branch off at each service with the processor that does
  // define the const type.

  f_service_ << indent() << "abstract class " << long_name << suffix
             << "ProcessorBase extends " << extends_processor << " {\n"
             << indent() << "  abstract const type TThriftIf as " << long_name
             << (async ? "Async" : "") << "If;\n";

  indent_up();

  // Generate the process subfunctions
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    generate_process_function(tservice, *f_iter, async);
  }

  indent_down();
  f_service_ << "}\n";

  f_service_ << indent() << "class " << long_name << suffix
             << "Processor extends " << long_name << suffix
             << "ProcessorBase {\n"
             << indent() << "  const type TThriftIf = " << long_name
             << (async ? "Async" : "") << "If;\n"
             << indent() << "}\n";

  if (!async) {
    f_service_ << indent() << "// For backwards compatibility\n"
               << indent() << "class " << long_name << "Processor extends "
               << long_name << "SyncProcessor {}\n";
  }

  f_service_ << "\n";
}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_hack_generator::generate_process_function(
    const t_service* tservice,
    const t_function* tfunction,
    bool async) {
  // Open function
  indent(f_service_)
      << "protected" << (async ? " async" : "") << " function process_"
      << tfunction->get_name()
      << "(int $seqid, \\TProtocol $input, \\TProtocol $output): "
      << (async ? "Awaitable<void>" : "void") << " {\n";
  indent_up();

  string service_name = hack_name(tservice);
  string argsname =
      generate_function_helper_name(tservice, tfunction, /*is_args*/ true);
  string resultname =
      generate_function_helper_name(tservice, tfunction, /*is_args*/ false);
  const string& fn_name = tfunction->get_name();

  f_service_ << indent()
             << "$handler_ctx = $this->eventHandler_->getHandlerContext('"
             << fn_name << "');\n"
             << indent() << "$reply_type = \\TMessageType::REPLY;\n"
             << "\n"
             << indent() << "$this->eventHandler_->preRead($handler_ctx, '"
             << fn_name << "', " << array_keyword_ << "[]);\n"
             << "\n"
             << indent() << "if ($input is \\TBinaryProtocolAccelerated) {\n"
             << indent() << "  $args = \\thrift_protocol_read_binary_struct("
             << "$input, '" << argsname << "');\n"
             << indent()
             << "} else if ($input is \\TCompactProtocolAccelerated) {"
             << "\n"
             << indent()
             << "  $args = \\thrift_protocol_read_compact_struct($input, '"
             << argsname << "');\n"
             << indent() << "} else {\n"
             << indent() << "  $args = " << argsname
             << "::withDefaultValues();\n"
             << indent() << "  $args->read($input);\n"
             << indent() << "}\n";
  f_service_ << indent() << "$input->readMessageEnd();\n";
  f_service_ << indent() << "$this->eventHandler_->postRead($handler_ctx, '"
             << fn_name << "', $args);\n";

  const t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;

  // Declare result for non oneway function
  if (!tfunction->is_oneway()) {
    f_service_ << indent() << "$result = " << resultname
               << "::withDefaultValues();\n";
  }

  // Try block for a function with exceptions
  f_service_ << indent() << "try {\n";
  indent_up();

  // Generate the function call
  const t_struct* arg_struct = tfunction->get_paramlist();
  const std::vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator f_iter;

  indent(f_service_) << "$this->eventHandler_->preExec($handler_ctx, '"
                     << service_name << "', '" << fn_name << "', $args);\n";

  f_service_ << indent();
  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
    f_service_ << "$result->success = ";
  }
  f_service_ << (async ? "await " : "") << "$this->handler->"
             << tfunction->get_name() << "(";
  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      f_service_ << ", ";
    }
    f_service_ << "$args->" << (*f_iter)->get_name();
  }
  f_service_ << ");\n";

  if (!tfunction->is_oneway()) {
    indent(f_service_) << "$this->eventHandler_->postExec($handler_ctx, '"
                       << fn_name << "', $result);\n";
  }

  indent_down();
  int exc_num;
  for (exc_num = 0, x_iter = xceptions.begin(); x_iter != xceptions.end();
       ++x_iter, ++exc_num) {
    f_service_ << indent() << "} catch (" << hack_name((*x_iter)->get_type())
               << " $exc" << exc_num << ") {\n";
    if (!tfunction->is_oneway()) {
      indent_up();
      f_service_ << indent()
                 << "$this->eventHandler_->handlerException($handler_ctx, '"
                 << fn_name << "', $exc" << exc_num << ");\n"
                 << indent() << "$result->" << (*x_iter)->get_name()
                 << " = $exc" << exc_num << ";\n";
      indent_down();
    }
  }
  f_service_
      << indent() << "} catch (\\Exception $ex) {\n"
      << indent() << "  $reply_type = \\TMessageType::EXCEPTION;\n"
      << indent() << "  $this->eventHandler_->handlerError($handler_ctx, '"
      << fn_name << "', $ex);\n"
      << indent()
      << "  $result = new \\TApplicationException($ex->getMessage().\"\\n\".$ex->getTraceAsString());\n"
      << indent() << "}\n";

  // Shortcut out here for oneway functions
  if (tfunction->is_oneway()) {
    f_service_ << indent() << "return;\n";
    indent_down();
    f_service_ << indent() << "}\n";
    return;
  }

  f_service_ << indent() << "$this->eventHandler_->preWrite($handler_ctx, '"
             << fn_name << "', $result);\n";

  f_service_ << indent() << "if ($output is \\TBinaryProtocolAccelerated)\n";
  scope_up(f_service_);

  f_service_ << indent() << "\\thrift_protocol_write_binary($output, '"
             << tfunction->get_name()
             << "', $reply_type, $result, $seqid, $output->isStrictWrite());\n";

  scope_down(f_service_);
  f_service_ << indent()
             << "else if ($output is \\TCompactProtocolAccelerated)\n";
  scope_up(f_service_);

  f_service_ << indent() << "\\thrift_protocol_write_compact($output, '"
             << tfunction->get_name() << "', $reply_type, $result, $seqid);\n";

  scope_down(f_service_);
  f_service_ << indent() << "else\n";
  scope_up(f_service_);

  // Serialize the request header
  f_service_ << indent() << "$output->writeMessageBegin(\""
             << tfunction->get_name() << "\", $reply_type, $seqid);\n"
             << indent() << "$result->write($output);\n"
             << indent() << "$output->writeMessageEnd();\n"
             << indent() << "$output->getTransport()->flush();\n";

  scope_down(f_service_);

  f_service_ << indent() << "$this->eventHandler_->postWrite($handler_ctx, '"
             << fn_name << "', $result);\n";

  // Close function
  indent_down();
  f_service_ << indent() << "}\n";
}

/**
 * Generates helper functions for a service.
 *
 * @param tservice The service to generate a header definition for
 */
void t_hack_generator::generate_service_helpers(
    const t_service* tservice,
    bool mangle) {
  vector<t_function*> functions = get_supported_functions(tservice);
  vector<t_function*>::iterator f_iter;

  f_service_ << "// HELPER FUNCTIONS AND STRUCTURES\n\n";

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    generate_php_function_helpers(tservice, *f_iter);
  }

  const vector<t_service*>& interactions = get_interactions(tservice);
  for (const auto& interaction : interactions) {
    for (const auto& function : get_supported_functions(interaction)) {
      generate_php_interaction_function_helpers(
          tservice, interaction, function);
    }
  }

  f_service_ << indent() << "class " << php_servicename_mangle(mangle, tservice)
             << "StaticMetadata implements \\IThriftServiceStaticMetadata {\n";
  indent_up();

  // Expose service metadata
  f_service_ << indent() << "public static function getServiceMetadata(): "
             << "\\tmeta_ThriftService {\n";
  indent_up();

  bool saved_arrays_ = arrays_;
  arrays_ = true;
  f_service_ << indent() << "return "
             << render_const_value(
                    tmeta_ThriftService_type(),
                    service_to_tmeta(tservice).get())
             << ";\n";
  arrays_ = saved_arrays_;

  indent_down();
  f_service_ << indent() << "}\n";

  // Structured annotations
  f_service_ << indent()
             << "public static function getAllStructuredAnnotations(): "
                "\\TServiceAnnotations {\n";
  indent_up();
  f_service_ << indent() << "return shape(\n";
  indent_up();
  f_service_ << indent() << "'service' => "
             << render_structured_annotations(
                    tservice->structured_annotations())
             << ",\n";
  f_service_ << indent() << "'functions' => dict[\n";
  indent_up();
  for (const auto& function : functions) {
    if (function->structured_annotations().empty()) {
      continue;
    }
    f_service_ << indent() << "'" << function->get_name() << "' => "
               << render_structured_annotations(
                      function->structured_annotations())
               << ",\n";
  }
  indent_down();
  f_service_ << indent() << "],\n";
  indent_down();
  f_service_ << indent() << ");\n";
  indent_down();
  f_service_ << indent() << "}\n";

  indent_down();
  f_service_ << indent() << "}\n\n";
}

void t_hack_generator::generate_service_interactions(
    const t_service* tservice,
    bool mangle) {
  const std::vector<t_service*>& interactions = get_interactions(tservice);
  if (interactions.empty()) {
    return;
  }

  f_service_ << "// INTERACTION HANDLERS\n\n";

  const string& service_name = tservice->get_name();
  for (const auto& interaction : interactions) {
    const string interaction_name = interaction->get_name();
    interaction->set_name(service_name + "_" + interaction_name);
    f_service_ << indent() << "class "
               << php_servicename_mangle(mangle, interaction)
               << " extends \\ThriftClientBase {\n";
    interaction->set_name(interaction_name);
    indent_up();

    f_service_ << indent() << "private \\InteractionId $interactionId;\n\n";

    f_service_ << indent() << "public function __construct("
               << "\\TProtocol $input, "
               << "?\\TProtocol $output = null, "
               << "?\\IThriftMigrationAsyncChannel $channel = null) {\n";
    indent_up();
    f_service_ << indent()
               << "parent::__construct($input, $output, $channel);\n";
    f_service_ << indent() << "if ($this->channel_ is nonnull) {\n";
    indent_up();
    f_service_ << indent()
               << "$this->interactionId = $this->channel_->createInteraction("
               << render_string(interaction_name) << ");\n";
    indent_down();
    f_service_ << indent() << "} else {\n";
    indent_up();
    f_service_ << indent() << "throw new \\Exception("
               << render_string(
                      "The channel must be nonnull to create interactions.")
               << ");\n";
    indent_down();
    f_service_ << indent() << "}\n";
    indent_down();
    f_service_ << indent() << "}\n\n";

    // Generate interaction method implementations
    for (const auto& function : get_supported_functions(interaction)) {
      _generate_service_client_child_fn(
          f_service_, interaction, function, /*rpc_options*/ true);
      _generate_sendImpl(f_service_, interaction, function);
      if (!function->is_oneway()) {
        _generate_recvImpl(f_service_, interaction, function);
      }
    }

    indent_down();
    f_service_ << indent() << "}\n\n";
  }
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_hack_generator::generate_php_function_helpers(
    const t_service* tservice,
    const t_function* tfunction) {
  const string& service_name = tservice->get_name();
  const t_struct* params = tfunction->get_paramlist();
  string params_name = service_name + "_" + params->get_name();

  generate_php_struct_definition(
      f_service_, params, false, false, true, params_name);

  if (!tfunction->is_oneway()) {
    t_struct result(
        program_, service_name + "_" + tfunction->get_name() + "_result");
    auto success =
        std::make_unique<t_field>(tfunction->get_returntype(), "success", 0);
    if (!tfunction->get_returntype()->is_void()) {
      result.append(std::move(success));
    }

    const t_struct* xs = tfunction->get_xceptions();
    const vector<t_field*>& fields = xs->get_members();
    vector<t_field*>::const_iterator f_iter;
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      result.append((*f_iter)->clone_DO_NOT_USE());
    }

    generate_php_struct_definition(f_service_, &result, false, true);
  }
}

/**
 * Generates a struct and helpers for an interaction function
 */
void t_hack_generator::generate_php_interaction_function_helpers(
    const t_service* tservice,
    const t_service* interaction,
    const t_function* tfunction) {
  const string& prefix = tservice->get_name() + "_" + interaction->get_name();
  const t_struct* params = tfunction->get_paramlist();
  string params_name = prefix + "_" + params->get_name();

  generate_php_struct_definition(
      f_service_, params, false, false, true, params_name);

  if (!tfunction->is_oneway()) {
    t_struct result(program_, prefix + "_" + tfunction->get_name() + "_result");
    auto success =
        std::make_unique<t_field>(tfunction->get_returntype(), "success", 0);
    if (!tfunction->get_returntype()->is_void()) {
      result.append(std::move(success));
    }

    const t_struct* xs = tfunction->get_xceptions();
    const vector<t_field*>& fields = xs->get_members();
    vector<t_field*>::const_iterator f_iter;
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      result.append((*f_iter)->clone_DO_NOT_USE());
    }

    generate_php_struct_definition(f_service_, &result, false, true);
  }
}

/**
 * Generates the docstring for a generic object.
 */
void t_hack_generator::generate_php_docstring(
    ofstream& out,
    const t_node* tdoc) {
  if (tdoc->has_doc()) {
    generate_docstring_comment(
        out, // out
        "/**\n", // comment_start
        " * ", // line_prefix
        tdoc->get_doc(), // contents
        " */\n"); // comment_end
  }
}

/**
 * Generates the docstring for a function.
 *
 * This is how the generated docstring looks like:-
 *
 * <Original docstring goes here>
 *
 * Original thrift definition:-
 * returnType
 *   functionName(1: argType1 arg_name1,
 *                2: argType2 arg_name2)
 *   throws (1: exceptionType1 ex_name1,
 *           2: exceptionType2 ex_name2);
 */
void t_hack_generator::generate_php_docstring(
    ofstream& out,
    const t_function* tfunction) {
  indent(out) << "/**\n";
  // Copy the doc.
  if (tfunction->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tfunction->get_doc(), // contents
        ""); // comment_end
  }

  // Also write the original thrift function definition.
  if (tfunction->has_doc()) {
    indent(out) << " * \n";
  }
  indent(out) << " * "
              << "Original thrift definition:-\n";
  // Return type.
  indent(out) << " * ";
  if (tfunction->is_oneway()) {
    out << "oneway ";
  }
  out << thrift_type_name(tfunction->get_returntype()) << "\n";

  // Function name.
  indent(out) << " * " << indent(1) << tfunction->get_name() << "(";
  // Find the position after the " * " from where the function arguments should
  // be rendered.
  int start_pos = get_indent_size() + tfunction->get_name().size() + 1;

  // Parameters.
  generate_php_docstring_args(out, start_pos, tfunction->get_paramlist());
  out << ")";

  // Exceptions.
  const t_struct* xceptions = tfunction->get_xceptions();
  if (xceptions && xceptions->get_members().size()) {
    out << "\n" << indent() << " * " << indent(1) << "throws (";
    // Find the position after the " * " from where the exceptions should be
    // rendered.
    start_pos = get_indent_size() + strlen("throws (");
    generate_php_docstring_args(out, start_pos, xceptions);
    out << ")";
  }
  out << ";\n";
  indent(out) << " */\n";
}

/**
 * Generates the docstring for a field.
 *
 * This is how the generated docstring looks like:-
 *
 * <Original docstring goes here>
 *
 * Original thrift field:-
 * argNumber: argType argName
 */
void t_hack_generator::generate_php_docstring(
    ofstream& out,
    const t_field* tfield) {
  indent(out) << "/**\n";
  // Copy the doc.
  if (tfield->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tfield->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * \n";
  }
  indent(out) << " * "
              << "Original thrift field:-\n";
  indent(out) << " * " << tfield->get_key() << ": "
              << tfield->get_type()->get_full_name() << " "
              << tfield->get_name() << "\n";
  indent(out) << " */\n";
}

/**
 * Generates the docstring for a struct.
 *
 * This is how the generated docstring looks like:-
 *
 * <Original docstring goes here>
 *
 * Original thrift struct/exception:-
 * Name
 */
void t_hack_generator::generate_php_docstring(
    ofstream& out,
    const t_struct* tstruct,
    bool is_exception) {
  indent(out) << "/**\n";
  // Copy the doc.
  if (tstruct->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tstruct->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * \n";
  }
  indent(out) << " * "
              << "Original thrift " << (is_exception ? "exception" : "struct")
              << ":-\n";
  indent(out) << " * " << tstruct->get_name() << "\n";
  indent(out) << " */\n";
}

/**
 * Generates the docstring for an enum.
 *
 * This is how the generated docstring looks like:-
 *
 * <Original docstring goes here>
 *
 * Original thrift enum:-
 * Name
 */
void t_hack_generator::generate_php_docstring(
    ofstream& out,
    const t_enum* tenum) {
  indent(out) << "/**\n";
  // Copy the doc.
  if (tenum->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tenum->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * \n";
  }
  indent(out) << " * "
              << "Original thrift enum:-\n";
  indent(out) << " * " << tenum->get_name() << "\n";
  indent(out) << " */\n";
}

/**
 * Generates the docstring for a service.
 *
 * This is how the generated docstring looks like:-
 *
 * <Original docstring goes here>
 *
 * Original thrift service:-
 * Name
 */
void t_hack_generator::generate_php_docstring(
    ofstream& out,
    const t_service* tservice) {
  indent(out) << "/**\n";
  // Copy the doc.
  if (tservice->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tservice->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * \n";
  }
  indent(out) << " * "
              << "Original thrift service:-\n";
  indent(out) << " * " << tservice->get_name() << "\n";
  indent(out) << " */\n";
}

/**
 * Generates the docstring for a constant.
 *
 * This is how the generated docstring looks like:-
 *
 * <Original docstring goes here>
 *
 * Original thrift constant:-
 * TYPE NAME
 */
void t_hack_generator::generate_php_docstring(
    ofstream& out,
    const t_const* tconst) {
  indent(out) << "/**\n";
  // Copy the doc.
  if (tconst->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tconst->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * \n";
  }
  indent(out) << " * "
              << "Original thrift constant:-\n";
  indent(out) << " * " << tconst->get_type()->get_full_name() << " "
              << tconst->get_name() << "\n";
  // no value because it could have characters that mess up the comment
  indent(out) << " */\n";
}

/**
 * Generates the docstring for function arguments and exceptions.
 *
 * @param int start_pos the position (after " * ") from which the rendering of
 * arguments should start. In other words, we put that many space after " * "
 * and then render the argument.
 */
void t_hack_generator::generate_php_docstring_args(
    ofstream& out,
    int start_pos,
    const t_struct* arg_list) {
  if (arg_list) {
    vector<t_field*> params = arg_list->get_members();
    if (params.size()) {
      vector<t_field*>::const_iterator p_iter;
      for (p_iter = params.begin(); p_iter != params.end(); ++p_iter) {
        const t_field* p = *p_iter;
        if (p_iter != params.begin()) { // If not first argument
          out << ",\n" << indent() << " * " << std::string(start_pos, ' ');
        }
        out << p->get_key() << ": " << thrift_type_name(p->get_type()) << " "
            << p->get_name();
      }
    }
  }
}

/**
 * Generate an appropriate string for a php typehint
 */
string t_hack_generator::type_to_typehint(
    const t_type* ttype,
    bool nullable,
    bool shape,
    bool immutable_collections,
    bool ignore_adapter) {
  if (!ignore_adapter) {
    // Check the adapter before resolving typedefs.
    if (const auto* adapter = get_hack_adapter(ttype)) {
      return *adapter + "::THackType";
    }
  }
  ttype = ttype->get_true_type();
  immutable_collections = immutable_collections || const_collections_;
  if (ttype->is_base_type()) {
    switch (((t_base_type*)ttype)->get_base()) {
      case t_base_type::TYPE_VOID:
        return "void";
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_BINARY:
        return "string";
      case t_base_type::TYPE_BOOL:
        return "bool";
      case t_base_type::TYPE_BYTE:
      case t_base_type::TYPE_I16:
      case t_base_type::TYPE_I32:
      case t_base_type::TYPE_I64:
        return "int";
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        return "float";
      default:
        return "mixed";
    }
  } else if (ttype->is_enum()) {
    if (is_bitmask_enum((t_enum*)ttype)) {
      return "int";
    } else {
      return (nullable ? "?" : "") + hack_name(ttype);
    }
  } else if (ttype->is_struct() || ttype->is_xception()) {
    return (nullable ? "?" : "") + hack_name(ttype) + (shape ? "::TShape" : "");
  } else if (ttype->is_list()) {
    string prefix;
    if (arrays_) {
      prefix = "vec";
    } else if (no_use_hack_collections_) {
      prefix = "varray";
    } else if (shape) {
      prefix = array_migration_ ? "varray" : "vec";
    } else {
      prefix = immutable_collections ? "ConstVector" : "Vector";
    }
    return prefix + "<" +
        type_to_typehint(
               ((t_list*)ttype)->get_elem_type(),
               false,
               shape,
               immutable_collections) +
        ">";
  } else if (ttype->is_map()) {
    string prefix;
    if (arrays_) {
      prefix = "dict";
    } else if (no_use_hack_collections_) {
      prefix = "darray";
    } else if (shape) {
      prefix = array_keyword_;
    } else {
      prefix = immutable_collections ? "ConstMap" : "Map";
    }
    string key_type = type_to_typehint(
        ((t_map*)ttype)->get_key_type(), false, shape, immutable_collections);
    if (shape && shape_arraykeys_ && key_type == "string") {
      key_type = "arraykey";
    } else if (!is_type_arraykey(((t_map*)ttype)->get_key_type())) {
      key_type = "arraykey";
    }
    return prefix + "<" + key_type + ", " +
        type_to_typehint(
               ((t_map*)ttype)->get_val_type(),
               false,
               shape,
               immutable_collections) +
        ">";
  } else if (ttype->is_set()) {
    string prefix;
    if (arraysets_) {
      prefix = array_keyword_;
    } else if (arrays_) {
      prefix = "keyset";
    } else if (shape) {
      prefix = array_keyword_;
    } else {
      prefix = immutable_collections ? "ConstSet" : "Set";
    }
    string suffix = (arraysets_ || (shape && !arrays_)) ? ", bool>" : ">";
    string key_type = !is_type_arraykey(((t_set*)ttype)->get_elem_type())
        ? "arraykey"
        : type_to_typehint(
              ((t_set*)ttype)->get_elem_type(),
              false,
              shape,
              immutable_collections);
    return prefix + "<" + key_type + suffix;
  } else {
    return "mixed";
  }
}

/**
 * Generate an appropriate string for a parameter typehint.
 * The difference from type_to_typehint() is for parameters we should accept an
 * array or a collection type, so we return KeyedContainer
 */
string t_hack_generator::type_to_param_typehint(
    const t_type* ttype,
    bool nullable) {
  if (ttype->is_list()) {
    if (strict_types_) {
      return type_to_typehint(ttype, nullable);
    } else {
      return "KeyedContainer<int, " +
          type_to_param_typehint(((t_list*)ttype)->get_elem_type()) + ">";
    }
  } else if (ttype->is_map()) {
    if (strict_types_) {
      return type_to_typehint(ttype, nullable);
    } else {
      auto key_type = ((t_map*)ttype)->get_key_type();
      return "KeyedContainer<" +
          (!is_type_arraykey(key_type) ? "arraykey"
                                       : type_to_param_typehint(key_type)) +
          ", " + type_to_param_typehint(((t_map*)ttype)->get_val_type()) + ">";
    }
  } else {
    return type_to_typehint(ttype, nullable);
  }
}

bool t_hack_generator::is_type_arraykey(const t_type* type) {
  type = type->get_true_type();
  return type->is_string_or_binary() || type->is_any_int() || type->is_byte() ||
      type->is_enum();
}

/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 * @param mangle Generate mangled service classes
 */
void t_hack_generator::generate_service_interface(
    const t_service* tservice,
    bool mangle,
    bool async,
    bool rpc_options,
    bool client) {
  generate_php_docstring(f_service_, tservice);
  string suffix = string(async ? "Async" : "") +
      (rpc_options ? "RpcOptions" : "") + (client ? "Client" : "");
  string extends_if = string("\\IThrift") + (async ? "Async" : "Sync") +
      (rpc_options ? "RpcOptions" : "") + "If";
  if (tservice->get_extends() != nullptr) {
    string ext_prefix =
        php_servicename_mangle(mangle, tservice->get_extends(), true);
    extends_if = ext_prefix + suffix + "If";
  }
  string long_name = php_servicename_mangle(mangle, tservice);
  string head_parameters = rpc_options ? "\\RpcOptions $rpc_options" : "";

  f_service_ << "interface " << long_name << suffix << "If extends "
             << extends_if << " {\n";
  indent_up();
  vector<t_function*> functions = get_supported_functions(tservice);
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    // Add a blank line before the start of a new function definition
    if (f_iter != functions.begin()) {
      f_service_ << "\n";
    }

    // Add the doxygen style comments.
    generate_php_docstring(f_service_, *f_iter);

    // Finally, the function declaration.
    string return_typehint = type_to_typehint((*f_iter)->get_returntype());
    if (async || client) {
      return_typehint = "Awaitable<" + return_typehint + ">";
    }

    if (nullable_everything_) {
      string funname = (*f_iter)->get_name();
      indent(f_service_)
          << "public function " << funname << "("
          << argument_list(
                 (*f_iter)->get_paramlist(), head_parameters, "", true, true)
          << "): " << return_typehint << ";\n";
    } else {
      indent(f_service_) << "public function "
                         << function_signature(
                                *f_iter, head_parameters, "", return_typehint)
                         << ";\n";
    }
  }
  indent_down();
  f_service_ << "}\n\n";
}

void t_hack_generator::generate_service_client(
    const t_service* tservice,
    bool mangle) {
  _generate_service_client(f_service_, tservice, mangle);
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_hack_generator::_generate_service_client(
    ofstream& out,
    const t_service* tservice,
    bool mangle) {
  generate_php_docstring(out, tservice);

  string long_name = php_servicename_mangle(mangle, tservice);
  out << "trait " << long_name << "ClientBase {\n"
      << "  require extends \\ThriftClientBase;\n\n";
  indent_up();

  // Generate client method implementations
  vector<t_function*> functions = get_supported_functions(tservice);
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    _generate_sendImpl(out, tservice, *f_iter);

    if (!(*f_iter)->is_oneway()) {
      _generate_recvImpl(out, tservice, *f_iter);
    }

    out << "\n";
  }

  // Generate factory method for interactions
  const std::vector<t_service*>& interactions = get_interactions(tservice);
  if (!interactions.empty()) {
    out << indent() << "/* interaction handlers factory methods */\n";
    const string& service_name = tservice->get_name();
    for (const auto& interaction : interactions) {
      const string interaction_name = interaction->get_name();
      interaction->set_name(service_name + "_" + interaction_name);
      const string& handle_name = php_servicename_mangle(mangle, interaction);
      interaction->set_name(interaction_name);

      out << indent() << "public function create" << interaction_name << "(): ";
      out << handle_name << " {\n";
      indent_up();

      out << indent() << "$interaction = new " << handle_name
          << "($this->input_, $this->output_, $this->channel_);\n";
      out << indent() << "$interaction->setAsyncHandler($this->asyncHandler_)"
          << "->setEventHandler($this->eventHandler_);\n";
      out << indent() << "return $interaction;\n";

      indent_down();
      out << indent() << "}\n\n";
    }
  }

  scope_down(out);
  out << "\n";

  _generate_service_client_children(
      out, tservice, mangle, /*async*/ true, /*rpc_options*/ false);
  _generate_service_client_children(
      out, tservice, mangle, /*async*/ false, /*rpc_options*/ false);
  _generate_service_client_children(
      out, tservice, mangle, /*async*/ true, /*rpc_options*/ true);
}

void t_hack_generator::_generate_recvImpl(
    ofstream& out,
    const t_service* tservice,
    const t_function* tfunction) {
  const string& resultname =
      generate_function_helper_name(tservice, tfunction, /*is_args*/ false);
  const string& rpc_function_name =
      generate_rpc_function_name(tservice, tfunction);

  t_function recv_function(
      tfunction->get_returntype(),
      string("recvImpl_") + tfunction->get_name(),
      std::make_unique<t_paramlist>(program_));
  string return_typehint = type_to_typehint(tfunction->get_returntype());
  // Open function
  out << "\n";
  if (arrprov_skip_frames_) {
    indent(f_service_) << "<<__ProvenanceSkipFrame>>\n";
  }
  out << indent() << "protected function "
      << function_signature(
             &recv_function,
             "",
             "?int $expectedsequenceid = null"
             ", shape(?'read_options' => int) $options = shape()",
             return_typehint)
      << " {\n";
  indent_up();

  out << indent() << "try {\n";
  indent_up();

  out << indent() << "$this->eventHandler_->preRecv('" << rpc_function_name
      << "', $expectedsequenceid);\n";

  out << indent() << "if ($this->input_ is \\TBinaryProtocolAccelerated) {\n";

  indent_up();

  out << indent() << "$result = \\thrift_protocol_read_binary("
      << "$this->input_"
      << ", '" << resultname << "'"
      << ", $this->input_->isStrictRead()"
      << ", Shapes::idx($options, 'read_options', 0)"
      << ");\n";

  indent_down();

  out << indent()
      << "} else if ($this->input_ is \\TCompactProtocolAccelerated)\n";
  scope_up(out);
  out << indent() << "$result = \\thrift_protocol_read_compact("
      << "$this->input_"
      << ", '" << resultname << "'"
      << ", Shapes::idx($options, 'read_options', 0)"
      << ");\n";
  scope_down(out);

  out << indent() << "else\n";
  scope_up(out);

  out << indent() << "$rseqid = 0;\n"
      << indent() << "$fname = '';\n"
      << indent() << "$mtype = 0;\n\n";

  out << indent() << "$this->input_->readMessageBegin(\n"
      << indent() << "  inout $fname,\n"
      << indent() << "  inout $mtype,\n"
      << indent() << "  inout $rseqid,\n"
      << indent() << ");\n"
      << indent() << "if ($mtype == \\TMessageType::EXCEPTION) {\n"
      << indent() << "  $x = new \\TApplicationException();\n"
      << indent() << "  $x->read($this->input_);\n"
      << indent() << "  $this->input_->readMessageEnd();\n"
      << indent() << "  throw $x;\n"
      << indent() << "}\n";

  out << indent() << "$result = " << resultname << "::withDefaultValues();\n"
      << indent() << "$result->read($this->input_);\n";

  out << indent() << "$this->input_->readMessageEnd();\n";

  out << indent()
      << "if ($expectedsequenceid !== null && ($rseqid != $expectedsequenceid)) {\n"
      << indent() << "  throw new \\TProtocolException(\""
      << tfunction->get_name() << " failed: sequence id is out of order\");\n"
      << indent() << "}\n";

  scope_down(out);
  indent_down();
  indent(out) << "} catch (\\THandlerShortCircuitException $ex) {\n";
  indent_up();
  out << indent() << "switch ($ex->resultType) {\n"
      << indent() << "  case \\THandlerShortCircuitException::R_EXPECTED_EX:\n"
      << indent() << "    $this->eventHandler_->recvException('"
      << rpc_function_name << "', $expectedsequenceid, $ex->result);\n"
      << indent() << "    throw $ex->result;\n"
      << indent()
      << "  case \\THandlerShortCircuitException::R_UNEXPECTED_EX:\n"
      << indent() << "    $this->eventHandler_->recvError('"
      << rpc_function_name << "', $expectedsequenceid, $ex->result);\n"
      << indent() << "    throw $ex->result;\n"
      << indent() << "  case \\THandlerShortCircuitException::R_SUCCESS:\n"
      << indent() << "  default:\n"
      << indent() << "    $this->eventHandler_->postRecv('" << rpc_function_name
      << "', $expectedsequenceid, $ex->result);\n"
      << indent() << "    return";

  if (!tfunction->get_returntype()->is_void()) {
    out << " $ex->result";
  }
  out << ";\n" << indent() << "}\n";
  indent_down();
  out << indent() << "} catch (\\Exception $ex) {\n";
  indent_up();
  out << indent() << "$this->eventHandler_->recvError('" << rpc_function_name
      << "', $expectedsequenceid, $ex);\n"
      << indent() << "throw $ex;\n";
  indent_down();
  out << indent() << "}\n";

  // Careful, only return result if not a void function
  if (!tfunction->get_returntype()->is_void()) {
    out << indent() << "if ($result->success !== null) {\n"
        << indent() << "  $success = $result->success;\n"
        << indent() << "  $this->eventHandler_->postRecv('" << rpc_function_name
        << "', $expectedsequenceid, $success);"
        << "\n"
        << indent() << "  return $success;\n"
        << indent() << "}\n";
  }

  const t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;
  for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
    out << indent() << "if ($result->" << (*x_iter)->get_name()
        << " !== null) {\n"
        << indent() << "  $x = $result->" << (*x_iter)->get_name() << ";"
        << "\n"
        << indent() << "  $this->eventHandler_->recvException('"
        << rpc_function_name << "', $expectedsequenceid, $x);\n"
        << indent() << "  throw $x;\n"
        << indent() << "}\n";
  }

  // Careful, only return _result if not a void function
  if (tfunction->get_returntype()->is_void()) {
    out << indent() << "$this->eventHandler_->postRecv('" << rpc_function_name
        << "', $expectedsequenceid, null);\n"
        << indent() << "return;\n";
  } else {
    out << indent() << "$x = new \\TApplicationException(\""
        << tfunction->get_name() << " failed: unknown result\""
        << ", \\TApplicationException::MISSING_RESULT"
        << ");\n"
        << indent() << "$this->eventHandler_->recvError('" << rpc_function_name
        << "', $expectedsequenceid, $x);\n"
        << indent() << "throw $x;\n";
  }

  // Close function
  scope_down(out);
}

void t_hack_generator::_generate_sendImpl(
    ofstream& out,
    const t_service* tservice,
    const t_function* tfunction) {
  const t_struct* arg_struct = tfunction->get_paramlist();
  const vector<t_field*>& fields = arg_struct->get_members();
  string funname = tfunction->get_name();
  const string& rpc_function_name =
      generate_rpc_function_name(tservice, tfunction);

  if (nullable_everything_) {
    indent(out) << "protected function sendImpl_" << funname << "("
                << argument_list(tfunction->get_paramlist(), "", "", true, true)
                << "): int {\n";
  } else {
    indent(out) << "protected function sendImpl_"
                << function_signature(tfunction, "", "", "int") << " {\n";
  }
  indent_up();

  const string& argsname =
      generate_function_helper_name(tservice, tfunction, /*is_args*/ true);

  out << indent() << "$currentseqid = $this->getNextSequenceID();\n"
      << indent() << "$args = " << argsname;
  if (!fields.empty()) {
    out << "::fromShape(shape(\n";
    indent_up();
    // Loop through the fields and assign to the args struct
    for (const auto& field : fields) {
      indent(out);
      string name = "$" + field->get_name();
      out << "'" << field->get_name() << "' => ";
      if (nullable_everything_) {
        // just passthrough null
        out << name << " === null ? null : ";
      }
      t_name_generator namer;
      this->_generate_sendImpl_arg(out, namer, name, field->get_type());
      out << ",\n";
    }
    indent_down();
    indent(out) << "));\n";
  } else {
    out << "::withDefaultValues();\n";
  }
  out << indent() << "try {\n";
  indent_up();
  out << indent() << "$this->eventHandler_->preSend('" << rpc_function_name
      << "', $args, $currentseqid);\n";
  out << indent() << "if ($this->output_ is \\TBinaryProtocolAccelerated)\n";
  scope_up(out);

  out << indent() << "\\thrift_protocol_write_binary($this->output_, '"
      << rpc_function_name << "', "
      << "\\TMessageType::CALL, $args, $currentseqid, "
      << "$this->output_->isStrictWrite(), "
      << (tfunction->is_oneway() ? "true" : "false") << ");\n";

  scope_down(out);
  out << indent()
      << "else if ($this->output_ is \\TCompactProtocolAccelerated)\n";
  scope_up(out);

  out << indent() << "\\thrift_protocol_write_compact($this->output_, '"
      << rpc_function_name << "', "
      << "\\TMessageType::CALL, $args, $currentseqid, "
      << (tfunction->is_oneway() ? "true" : "false") << ");\n";

  scope_down(out);
  out << indent() << "else\n";
  scope_up(out);

  // Serialize the request header
  out << indent() << "$this->output_->writeMessageBegin('" << rpc_function_name
      << "', \\TMessageType::CALL, $currentseqid);\n";

  // Write to the stream
  out << indent() << "$args->write($this->output_);\n"
      << indent() << "$this->output_->writeMessageEnd();\n";
  if (tfunction->is_oneway()) {
    out << indent() << "$this->output_->getTransport()->onewayFlush();\n";
  } else {
    out << indent() << "$this->output_->getTransport()->flush();\n";
  }

  scope_down(out);

  indent_down();
  indent(out) << "} catch (\\THandlerShortCircuitException $ex) {\n";
  indent_up();
  out << indent() << "switch ($ex->resultType) {\n"
      << indent() << "  case \\THandlerShortCircuitException::R_EXPECTED_EX:\n"
      << indent()
      << "  case \\THandlerShortCircuitException::R_UNEXPECTED_EX:\n"
      << indent() << "    $this->eventHandler_->sendError('"
      << rpc_function_name << "', $args, $currentseqid, $ex->result);"
      << "\n"
      << indent() << "    throw $ex->result;\n"
      << indent() << "  case \\THandlerShortCircuitException::R_SUCCESS:\n"
      << indent() << "  default:\n"
      << indent() << "    $this->eventHandler_->postSend('" << rpc_function_name
      << "', $args, $currentseqid);\n"
      << indent() << "    return $currentseqid;\n"
      << indent() << "}\n";
  indent_down();
  indent(out) << "} catch (\\Exception $ex) {\n";
  indent_up();
  out << indent() << "$this->eventHandler_->sendError('" << rpc_function_name
      << "', $args, $currentseqid, $ex);\n"
      << indent() << "throw $ex;\n";
  indent_down();
  indent(out) << "}\n";

  out << indent() << "$this->eventHandler_->postSend('" << rpc_function_name
      << "', $args, $currentseqid);\n";

  indent(out) << "return $currentseqid;\n";

  scope_down(out);
}

// If !strict_types, containers are typehinted as KeyedContainer<Key, Value>
// to better support passing in arrays/dicts/maps/vecs/vectors and
// handle backwards compatibility. However, structs are typehinted as
// the actual container (ex: Map<Key, Val>), and we need to safely
// convert the typehints.
//
// This isn't as simple as dict($param) or new Map($param). If there is
// a nested container, that also needs to have its typehints converted.
// This iterates through the type object and generates the appropriate
// code to convert all the nested typehints.
void t_hack_generator::_generate_sendImpl_arg(
    ofstream& out,
    t_name_generator& namer,
    const string& var,
    const t_type* t) {
  const t_type* val_type;
  if (strict_types_ || !t->is_container()) {
    out << var;
    return;
  }

  if (t->is_map()) {
    val_type = ((t_map*)t)->get_val_type();
  } else if (t->is_list()) {
    val_type = ((t_list*)t)->get_elem_type();
  } else if (t->is_set()) {
    val_type = ((t_set*)t)->get_elem_type();
  } else {
    throw std::runtime_error("Unknown container type");
  }

  val_type = val_type->get_true_type();
  if (val_type->is_container() && !val_type->is_set()) {
    if (t->is_map()) {
      if (arrays_) {
        out << "Dict\\map(" << var << ", ";
      } else if (no_use_hack_collections_) {
        out << "darray(Dict\\map(" << var << ", ";
      } else {
        out << "(new Map(" << var << "))->map(";
      }
    } else if (t->is_list()) {
      if (arrays_) {
        out << "Vec\\map(" << var << ", ";
      } else if (no_use_hack_collections_) {
        out << "varray(Vec\\map(" << var << ", ";
      } else {
        out << "(new Vector(" << var << "))->map(";
      }
    } else if (t->is_set()) {
      throw std::runtime_error("Sets can't have nested containers");
    } else {
      throw std::runtime_error("Unknown container type");
    }
    indent_up();
    out << "\n" << indent();
    // update var to what it will be next, since we no longer need the old value
    string new_var = "$" + namer("_val");
    out << new_var << " ==> ";
    this->_generate_sendImpl_arg(out, namer, new_var, val_type);
    indent_down();
    out << "\n" << indent();
    if (no_use_hack_collections_) {
      out << ")";
    }
    out << ")";
  } else {
    // the parens around the collections are unnecessary but I'm leaving them
    // so that I don't end up changing literally all files
    if (t->is_map()) {
      if (arrays_) {
        out << "dict(" << var << ")";
      } else if (no_use_hack_collections_) {
        out << "darray(" << var << ")";
      } else {
        out << "new Map(" << var << ")";
      }
    } else if (t->is_list()) {
      if (arrays_) {
        out << "vec(" << var << ")";
      } else if (no_use_hack_collections_) {
        out << "varray(" << var << ")";
      } else {
        out << "new Vector(" << var << ")";
      }
    } else if (t->is_set()) {
      out << var;
    } else {
      throw std::runtime_error("Unknown container type");
    }
  }
}

void t_hack_generator::_generate_service_client_children(
    ofstream& out,
    const t_service* tservice,
    bool mangle,
    bool async,
    bool rpc_options) {
  string long_name = php_servicename_mangle(mangle, tservice);
  string class_suffix =
      string(async ? "Async" : "") + (rpc_options ? "RpcOptions" : "");
  if (rpc_options && !async) {
    throw std::runtime_error(
        "RpcOptions are currently supported for async clients only");
  }
  string interface_suffix =
      string(async ? "Async" : "Client") + (rpc_options ? "RpcOptions" : "");
  string extends = "\\ThriftClientBase";
  bool root = tservice->get_extends() == nullptr;
  if (!root) {
    extends = php_servicename_mangle(mangle, tservice->get_extends(), true) +
        class_suffix + "Client";
  }

  out << "class " << long_name << class_suffix << "Client extends " << extends
      << " implements " << long_name << interface_suffix << "If {\n"
      << "  use " << long_name << "ClientBase;\n\n";
  indent_up();

  // Generate client method implementations
  vector<t_function*> functions = get_supported_functions(tservice);
  vector<t_function*>::const_iterator f_iter;

  // generate functions as necessary
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    _generate_service_client_child_fn(out, tservice, *f_iter, rpc_options);
    if (no_use_hack_collections_) {
      _generate_service_client_child_fn(
          out, tservice, *f_iter, rpc_options, /*legacy_arrays*/ true);
    }
  }

  if (!async) {
    out << indent() << "/* send and recv functions */\n";

    for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
      const t_struct* arg_struct = (*f_iter)->get_paramlist();
      const vector<t_field*>& fields = arg_struct->get_members();
      vector<t_field*>::const_iterator fld_iter;
      string funname = (*f_iter)->get_name();
      string return_typehint = type_to_typehint((*f_iter)->get_returntype());

      out << indent() << "public function send_"
          << function_signature(*f_iter, "", "", "int") << " {\n"
          << indent() << "  return $this->sendImpl_" << funname << "(";
      bool first = true;
      for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
        if (first) {
          first = false;
        } else {
          out << ", ";
        }
        out << "$" << (*fld_iter)->get_name();
      }
      out << ");\n" << indent() << "}\n";
      if (!(*f_iter)->is_oneway()) {
        t_function recv_function(
            (*f_iter)->get_returntype(),
            string("recv_") + (*f_iter)->get_name(),
            std::make_unique<t_paramlist>(program_));
        // Open function
        if (arrprov_skip_frames_) {
          indent(out) << "<<__ProvenanceSkipFrame>>\n";
        }
        bool is_void = (*f_iter)->get_returntype()->is_void();
        out << indent() << "public function "
            << function_signature(
                   &recv_function,
                   "",
                   "?int $expectedsequenceid = null",
                   return_typehint)
            << " {\n"
            << indent() << "  " << (is_void ? "" : "return ")
            << "$this->recvImpl_" << funname << "($expectedsequenceid);\n"
            << indent() << "}\n";
      }
    }
  }

  indent_down();
  out << "}\n\n";
}

void t_hack_generator::_generate_service_client_child_fn(
    ofstream& out,
    const t_service* tservice,
    const t_function* tfunction,
    bool rpc_options,
    bool legacy_arrays) {
  const t_struct* arg_struct = tfunction->get_paramlist();
  const vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator fld_iter;
  string funname =
      tfunction->get_name() + (legacy_arrays ? "__LEGACY_ARRAYS" : "");
  const string& tservice_name =
      (tservice->is_interaction() ? service_name_ : tservice->get_name());
  string return_typehint = type_to_typehint(tfunction->get_returntype());
  string head_parameters = rpc_options ? "\\RpcOptions $rpc_options" : "";
  string rpc_options_param =
      rpc_options ? "$rpc_options" : "new \\RpcOptions()";

  generate_php_docstring(out, tfunction);
  if (arrprov_skip_frames_) {
    indent(out) << "<<__ProvenanceSkipFrame>>\n";
  }
  indent(out) << "public async function " << funname << "("
              << argument_list(
                     tfunction->get_paramlist(),
                     head_parameters,
                     "",
                     true,
                     nullable_everything_)
              << "): Awaitable<" + return_typehint + "> {\n";

  indent_up();

  if (tservice->is_interaction()) {
    if (!rpc_options) {
      throw std::runtime_error("Interaction methods require rpc_options");
    }
    indent(out) << rpc_options_param << " = " << rpc_options_param
                << "->setInteractionId($this->interactionId);\n";
  }

  indent(out) << "await $this->asyncHandler_->genBefore(\"" << tservice_name
              << "\", \"" << generate_rpc_function_name(tservice, tfunction)
              << "\");\n";
  indent(out) << "$currentseqid = $this->sendImpl_" << tfunction->get_name()
              << "(";

  bool first = true;
  for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
    if (first) {
      first = false;
    } else {
      out << ", ";
    }
    out << "$" << (*fld_iter)->get_name();
  }
  out << ");\n";

  if (!tfunction->is_oneway()) {
    out << indent() << "$channel = $this->channel_;\n"
        << indent() << "$out_transport = $this->output_->getTransport();\n"
        << indent() << "$in_transport = $this->input_->getTransport();\n"
        << indent()
        << "if ($channel !== null && $out_transport is \\TMemoryBuffer && $in_transport is \\TMemoryBuffer) {\n";
    indent_up();
    out << indent() << "$msg = $out_transport->getBuffer();\n"
        << indent() << "$out_transport->resetBuffer();\n"
        << indent()
        << "list($result_msg, $_read_headers) = await $channel->genSendRequestResponse("
        << rpc_options_param << ", $msg);\n"
        << indent() << "$in_transport->resetBuffer();\n"
        << indent() << "$in_transport->write($result_msg);\n";
    indent_down();
    indent(out) << "} else {\n";
    indent_up();
    indent(out) << "await $this->asyncHandler_->genWait($currentseqid);\n";
    scope_down(out);
    out << indent();
    if (!tfunction->get_returntype()->is_void()) {
      out << "return ";
    }
    out << "$this->recvImpl_" << tfunction->get_name() << "("
        << "$currentseqid";
    if (legacy_arrays) {
      out << ", shape('read_options' => THRIFT_MARK_LEGACY_ARRAYS)";
    }
    out << ");\n";
  } else {
    out << indent() << "$channel = $this->channel_;\n"
        << indent() << "$out_transport = $this->output_->getTransport();\n"
        << indent()
        << "if ($channel !== null && $out_transport is \\TMemoryBuffer) {\n";
    indent_up();
    out << indent() << "$msg = $out_transport->getBuffer();\n"
        << indent() << "$out_transport->resetBuffer();\n"
        << indent() << "await $channel->genSendRequestNoResponse("
        << rpc_options_param << ", $msg);\n";
    scope_down(out);
  }
  scope_down(out);
  out << "\n";
}
/**
 * Declares a field, which may include initialization as necessary.
 *
 * @param ttype The type
 * @param init iff initialize the field
 * @param obj iff the field is an object
 * @param thrift iff the object is a thrift object
 */
string t_hack_generator::declare_field(
    const t_field* tfield,
    bool init,
    bool obj,
    bool /*thrift*/) {
  string result = "$" + tfield->get_name();
  if (init) {
    const t_type* type = tfield->get_type()->get_true_type();
    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
        case t_base_type::TYPE_VOID:
          break;
        case t_base_type::TYPE_STRING:
        case t_base_type::TYPE_BINARY:
          result += " = ''";
          break;
        case t_base_type::TYPE_BOOL:
          result += " = false";
          break;
        case t_base_type::TYPE_BYTE:
        case t_base_type::TYPE_I16:
        case t_base_type::TYPE_I32:
        case t_base_type::TYPE_I64:
          result += " = 0";
          break;
        case t_base_type::TYPE_DOUBLE:
        case t_base_type::TYPE_FLOAT:
          result += " = 0.0";
          break;
        default:
          throw std::runtime_error(
              "compiler error: no Hack initializer for base type " +
              t_base_type::t_base_name(tbase));
      }
    } else if (type->is_enum()) {
      result += " = null";
    } else if (type->is_map()) {
      if (arrays_) {
        result += " = dict[]";
      } else if (no_use_hack_collections_) {
        result += " = darray[]";
      } else {
        result += " = Map {}";
      }
    } else if (type->is_list()) {
      if (arrays_) {
        result += " = vec[]";
      } else if (no_use_hack_collections_) {
        result += " = varray[]";
      } else {
        result += " = Vector {}";
      }
    } else if (type->is_set()) {
      if (arrays_) {
        result += " = keyset[]";
      } else if (arraysets_) {
        result += " = " + array_keyword_ + "[]";
      } else {
        result += " = Set {}";
      }
    } else if (type->is_struct() || type->is_xception()) {
      if (obj) {
        result += " = " + hack_name(type) + "::withDefaultValues()";
      } else {
        result += " = null";
      }
    }
  }
  return result + ";";
}

/**
 * Renders a function signature of the form 'type name(args)'
 *
 * @param tfunction Function definition
 * @return String of rendered function definition
 */
string t_hack_generator::function_signature(
    const t_function* tfunction,
    string more_head_parameters,
    string more_tail_parameters,
    string typehint) {
  if (typehint.empty()) {
    typehint = type_to_typehint(tfunction->get_returntype());
  }

  return tfunction->get_name() + "(" +
      argument_list(
             tfunction->get_paramlist(),
             more_head_parameters,
             more_tail_parameters) +
      "): " + typehint;
}

/**
 * Renders a field list
 */
string t_hack_generator::argument_list(
    const t_struct* tstruct,
    string more_head_parameters,
    string more_tail_parameters,
    bool typehints,
    bool force_nullable) {
  string result = "";

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = true;

  if (more_head_parameters.length() > 0) {
    result += more_head_parameters;
    first = false;
  }

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }
    if (typehints) {
      // If a field is not sent to a thrift server, the value is null :(
      const t_type* ftype = (*f_iter)->get_type()->get_true_type();
      bool nullable = !no_nullables_ ||
          (ftype->is_enum() &&
           ((*f_iter)->get_value() == nullptr ||
            (*f_iter)->get_req() != t_field::e_req::required));
      if (force_nullable &&
          !field_is_nullable(tstruct, *f_iter, render_default_value(ftype))) {
        result += "?";
      }
      result += type_to_param_typehint((*f_iter)->get_type(), nullable) + " ";
    }
    result += "$" + (*f_iter)->get_name();
  }

  if (more_tail_parameters.length() > 0) {
    if (!first) {
      result += ", ";
    }
    result += more_tail_parameters;
  }
  return result;
}

/**
 * Renders the function name to be used in RPC
 */
string t_hack_generator::generate_rpc_function_name(
    const t_service* tservice,
    const t_function* tfunction) const {
  const string& prefix =
      tservice->is_interaction() ? tservice->get_name() + "." : "";
  return prefix + tfunction->get_name();
}

/**
 * Generate function's helper structures name
 * @param is_args defines the suffix
 *    true  : _args
 *    false : _result
 */
string t_hack_generator::generate_function_helper_name(
    const t_service* tservice,
    const t_function* tfunction,
    bool is_args) {
  string prefix = "";
  if (tservice->is_interaction()) {
    prefix = hack_name(service_name_, program_) + "_" + tservice->get_name();
  } else {
    prefix = hack_name(tservice);
  }
  const string& suffix = is_args ? "args" : "result";
  return prefix + "_" + tfunction->get_name() + "_" + suffix;
}

/**
 * Gets a typecast string for a particular type.
 */
string t_hack_generator::type_to_cast(const t_type* type) {
  if (type->is_base_type()) {
    t_base_type* btype = (t_base_type*)type;
    switch (btype->get_base()) {
      case t_base_type::TYPE_BOOL:
        return "(bool)";
      case t_base_type::TYPE_BYTE:
      case t_base_type::TYPE_I16:
      case t_base_type::TYPE_I32:
      case t_base_type::TYPE_I64:
        return "(int)";
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        return "(float)";
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_BINARY:
        return "(string)";
      default:
        return "";
    }
  } else if (type->is_enum()) {
    return "";
  }
  return "";
}

/**
 * Converts the parse type to a C++ enum string for the given type.
 */
string t_hack_generator::type_to_enum(const t_type* type) {
  type = type->get_true_type();

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw std::runtime_error("NO T_VOID CONSTRUCT");
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_BINARY:
        return "\\TType::STRING";
      case t_base_type::TYPE_BOOL:
        return "\\TType::BOOL";
      case t_base_type::TYPE_BYTE:
        return "\\TType::BYTE";
      case t_base_type::TYPE_I16:
        return "\\TType::I16";
      case t_base_type::TYPE_I32:
        return "\\TType::I32";
      case t_base_type::TYPE_I64:
        return "\\TType::I64";
      case t_base_type::TYPE_DOUBLE:
        return "\\TType::DOUBLE";
      case t_base_type::TYPE_FLOAT:
        return "\\TType::FLOAT";
    }
  } else if (type->is_enum()) {
    return "\\TType::I32";
  } else if (type->is_struct() || type->is_xception()) {
    return "\\TType::STRUCT";
  } else if (type->is_map()) {
    return "\\TType::MAP";
  } else if (type->is_set()) {
    return "\\TType::SET";
  } else if (type->is_list()) {
    return "\\TType::LST";
  }

  throw std::runtime_error("INVALID TYPE IN type_to_enum: " + type->get_name());
}

THRIFT_REGISTER_GENERATOR(
    hack,
    "HACK",
    "    server:          Generate Hack server stubs.\n"
    "    rest:            Generate Hack REST processors.\n"
    "    json:            Generate functions to parse JSON into thrift struct.\n"
    "    mangledsvcs      Generate services with namespace mangling.\n"
    "    stricttypes      Use Collection classes everywhere rather than KeyedContainer.\n"
    "    strict           Generate strict hack header.\n"
    "    arraysets        Use legacy arrays for sets rather than objects.\n"
    "    nonullables      Instantiate struct fields within structs, rather than nullable\n"
    "    structtrait      Add 'use [StructName]Trait;' to generated classes\n"
    "    shapes           Generate Shape definitions for structs\n"
    "    protected_unions Generate protected members for thrift unions\n"
    "    shape_arraykeys  When generating Shape definition for structs:\n"
    "                        replace array<string, TValue> with array<arraykey, TValue>\n"
    "    shapes_allow_unknown_fields Allow unknown fields and implicit subtyping for shapes \n"
    "    frommap_construct Generate fromMap_DEPRECATED method.\n"
    "    arrays           Use Hack arrays for maps/lists/sets instead of objects.\n"
    "    const_collections Use ConstCollection objects rather than their mutable counterparts.\n"
    "    enum_transparenttype Use transparent typing for Hack enums: 'enum FooBar: int as int'.\n");

} // namespace compiler
} // namespace thrift
} // namespace apache
