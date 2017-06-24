/*
 * Copyright 2014-present Facebook, Inc.
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
#include <string>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include <stdlib.h>
#include <sys/stat.h>
#include <sstream>
#include <thrift/compiler/generate/t_oop_generator.h>
#include <thrift/compiler/platform.h>
using namespace std;


/**
 * Hack code generator.
 *
 */
class t_hack_generator : public t_oop_generator {
 public:
  t_hack_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/)
      : t_oop_generator(program) {
    std::map<std::string, std::string>::const_iterator iter;

    json_ = option_is_specified(parsed_options, "json");
    rest_ = option_is_specified(parsed_options, "rest");
    phps_ = option_is_specified(parsed_options, "server");
    oldenum_ = option_is_specified(parsed_options, "oldenum");
    strict_types_ = option_is_specified(parsed_options, "stricttypes");
    strict_ = option_is_specified(parsed_options, "strict");
    arraysets_ = option_is_specified(parsed_options, "arraysets");
    no_nullables_ = option_is_specified(parsed_options, "nonullables");
    map_construct_ = option_is_specified(parsed_options, "mapconstruct");
    struct_trait_ = option_is_specified(parsed_options, "structtrait");
    shapes_ = option_is_specified(parsed_options, "shapes");
    shape_arraykeys_ = option_is_specified(parsed_options, "shape_arraykeys");
    shape_unsafe_json_ = option_is_specified(parsed_options, "shape_unsafe_json");
    lazy_constants_ = option_is_specified(parsed_options, "lazy_constants");
    arrays_ = option_is_specified(parsed_options, "arrays");

    mangled_services_ = option_is_set(parsed_options, "mangledsvcs", false);

    out_dir_base_ = "gen-hack";
  }

  /**
   * Init and close methods
   */

  void init_generator() override;
  void close_generator() override;

  /**
   * Program-level generation functions
   */

  void generate_typedef(t_typedef* ttypedef) override;
  void generate_enum(t_enum* tenum) override;
  void generate_const(t_const* tconst) override;
  void generate_struct(t_struct* tstruct) override;
  void generate_xception(t_struct* txception) override;
  void generate_service(t_service* tservice) override;

  std::string render_const_value(t_type* type, const t_const_value* value);
  std::string render_default_value(t_type* type);

  /**
   * Structs!
   */

  void generate_php_struct(t_struct* tstruct, bool is_exception);
  void generate_php_struct_definition(
      std::ofstream& out,
      t_struct* tstruct,
      bool is_xception = false,
      bool is_result = false,
      bool is_args = false);
  void _generate_php_struct_definition(
      std::ofstream& out,
      t_struct* tstruct,
      bool is_xception = false,
      bool is_result = false,
      bool is_args = false);
  void generate_php_struct_reader(std::ofstream& out, t_struct* tstruct);
  void generate_php_struct_writer(std::ofstream& out, t_struct* tstruct);
  void generate_php_function_helpers(t_function* tfunction);

  void generate_php_union_enum(std::ofstream& out, t_struct* tstruct);
  void generate_php_union_methods(std::ofstream& out, t_struct* tstruct);

  void generate_php_struct_shape_spec(std::ofstream& out, t_struct* tstruct);
  void generate_php_struct_shape_collection_value_lambda(std::ostream& out,
                                                         t_name_generator& namer,
                                                         t_type* t);
  bool field_is_nullable(t_struct* tstruct, const t_field* field, string dval);
  void generate_php_struct_shape_json_conversion(std::ofstream& out,
                                                 bool nullable,
                                                 string value,
                                                 t_type *t,
                                                 t_name_generator& namer);
  void generate_php_struct_shape_methods(std::ofstream& out, t_struct* tstruct);

  void generate_php_type_spec(std::ofstream& out, t_type* t);
  void generate_php_struct_spec(std::ofstream& out, t_struct* tstruct);
  void generate_php_struct_struct_trait(std::ofstream& out, t_struct* tstruct);
  void generate_php_structural_id(
      std::ofstream& out,
      t_struct* tstruct,
      bool asFunction);

  /**
   * Service-level generation functions
   */

  void generate_service           (t_service* tservice, bool mangle);
  void generate_service_helpers   (t_service* tservice);
  void generate_service_interface (t_service* tservice, bool mangle, bool async);
  void generate_service_rest      (t_service* tservice, bool mangle);
  void generate_service_client    (t_service* tservice, bool mangle);
  void _generate_service_client   (std::ofstream& out,
                                   t_service* tservice,
                                   bool mangle);
  void _generate_service_client_children (std::ofstream& out,
                                          t_service* tservice,
                                          bool mangle,
                                          bool async);
  void generate_service_processor (t_service* tservice, bool mangle, bool async);
  void generate_process_function  (t_service* tservice, t_function* tfunction, bool async);
  void generate_processor_event_handler_functions (std::ofstream& out);
  void generate_client_event_handler_functions (std::ofstream& out);
  void generate_event_handler_functions (std::ofstream& out, string cl);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field(
      std::ofstream& out,
      t_name_generator& namer,
      t_field* tfield,
      std::string name = "",
      bool declared = false);

  void generate_deserialize_struct       (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_deserialize_container    (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_deserialize_set_element  (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_set*      tset,
                                          std::string size,
                                          std::string prefix="");

  void generate_deserialize_map_element  (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_map*      tmap,
                                          std::string size,
                                          std::string prefix="");

  void generate_deserialize_list_element (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_list*     tlist,
                                          std::string size,
                                          std::string prefix="");

  void generate_serialize_field          (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_field*    tfield,
                                          std::string name="");

  void generate_serialize_struct         (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_serialize_container      (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_type* ttype,
                                          std::string prefix = "");

  void generate_serialize_map_element    (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_map*      tmap,
                                          std::string kiter,
                                          std::string viter);

  void generate_serialize_set_element    (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_set*      tmap,
                                          std::string iter);

  void generate_serialize_list_element   (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_list*     tlist,
                                          std::string iter);
  /**
   * Read thrift object from JSON string, generated using the
   * TSimpleJSONProtocol.
   */

  void generate_json_enum            (std::ofstream& out,
                                      t_name_generator& namer,
                                      t_enum* tenum,
                                      const string& prefix_thrift,
                                      const string& prefix_json);

  void generate_json_struct          (std::ofstream& out,
                                      t_name_generator& namer,
                                      t_struct* tstruct,
                                      const string& prefix_thrift,
                                      const string& prefix_json);

  void generate_json_field           (std::ofstream& out,
                                      t_name_generator& namer,
                                      t_field* tfield,
                                      const string& prefix_thrift = "",
                                      const string& suffix_thrift = "",
                                      const string& prefix_json = "");

  void generate_json_container       (std::ofstream& out,
                                      t_name_generator& namer,
                                      t_type* ttype,
                                      const string& prefix_thrift = "",
                                      const string& prefix_json = "");

  void generate_json_set_element     (std::ofstream& out,
                                      t_name_generator& namer,
                                      t_set* tset,
                                      const string& value,
                                      const string& prefix_thrift);

  void generate_json_list_element    (ofstream& out,
                                      t_name_generator& namer,
                                      t_list* list,
                                      const string& value,
                                      const string& prefix_thrift);

  void generate_json_map_element     (std::ofstream& out,
                                      t_name_generator& namer,
                                      t_map* tmap,
                                      const string& key,
                                      const string& value,
                                      const string& prefix_thrift);

  void generate_json_reader          (std::ofstream& out,
                                      t_struct* tstruct);

  /**
   * Helper rendering functions
   */

  std::string declare_field(t_field* tfield, bool init=false,
                            bool obj=false, bool thrift=false);
  std::string function_signature(t_function* tfunction,
                                 std::string prefix="",
                                 std::string moreparameters="",
                                 std::string typehint="");
  std::string argument_list(t_struct* tstruct, std::string moreparameters="", bool typehints = true);
  std::string type_to_cast(t_type* ttype);
  std::string type_to_enum(t_type* ttype);
  void generate_php_docstring(ofstream& out, t_doc* tdoc);
  void generate_php_docstring(ofstream& out, t_enum* tenum);
  void generate_php_docstring(ofstream& out, t_service* tservice);
  void generate_php_docstring(ofstream& out, t_const* tconst);
  void generate_php_docstring(ofstream& out, t_function* tfunction);
  void generate_php_docstring(ofstream& out, t_field* tfield);
  void generate_php_docstring(
      ofstream& out,
      t_struct* tstruct,
      bool is_exception = false);
  void generate_php_docstring_args(ofstream& out,
                                   int start_pos,
                                   t_struct* arg_list);
  std::string render_string(std::string value);

  std::string type_to_typehint(t_type* ttype, bool nullable=false, bool shape=false);
  std::string type_to_param_typehint(t_type* ttype, bool nullable=false);

  std::string union_enum_name(t_struct* tstruct) {
    // <StructName>Type
    return hack_name(tstruct) + "Enum";
  }

  std::string union_field_to_enum(t_struct* tstruct, t_field* tfield) {
    // If null is passed,  it refer to empty;
    if (tfield) {
      return union_enum_name(tstruct) + "::" + tfield->get_name();
    } else {
      return union_enum_name(tstruct) + "::" + UNION_EMPTY;
    }
  }

  bool is_bitmask_enum(t_enum* tenum) {
    return tenum->annotations_.find("bitmask") != tenum->annotations_.end();
  }

  std::string hack_namespace(const t_program* p) {
    std::string ns;
    ns = p->get_namespace("hack");
    std::replace(ns.begin(), ns.end(), '.', '\\');
    return ns;
  }

  std::string php_namespace(const t_program* p) {
    std::string ns;
    ns = hack_namespace(p);
    p->get_namespace("hack");
    if (!ns.empty()) {
      return "";
    }
    ns = p->get_namespace("php");
    if (!ns.empty()) {
      ns.push_back('_');
      return ns;
    }
    return "";
  }

  string php_namespace(t_service* s) {
    return php_namespace(s->get_program());
  }

  string hack_name(string name, const t_program* prog, bool decl = false) {
    string ns;
    ns = prog->get_namespace("hack");
    if (!ns.empty()) {
      if (decl) {
        return name;
      }
      std::replace(ns.begin(), ns.end(), '.', '\\');
      return '\\' + ns + '\\' + name;
    }
    ns = prog->get_namespace("php");
    if (!ns.empty()) {
      return ns + "_" + name;
    }
    return name;
  }

  string hack_name(t_type* t, bool decl = false) {
    return hack_name(t->get_name(), t->get_program(), decl);
  }

  string hack_name(t_service* s, bool decl = false) {
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

  std::string php_path(t_service* s) {
    return php_path(s->get_program());
  }

  const char* UNION_EMPTY = "_EMPTY_";

  void generate_lazy_init_for_constant(ofstream& out,
                                       const std::string& name,
                                       const std::string& typehint,
                                       const std::string& rendered_value);

 private:

  /**
   * Generate the namespace mangled string, if necessary
   */
  std::string php_servicename_mangle(bool mangle, t_service* svc) {
    return (mangle ? php_namespace(svc) : "") + svc->get_name();
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
   * Generate a REST handler class
   */
  bool rest_;

  /**
   * Generate stubs for a PHP server
   */
  bool phps_;

  /**
   * * Whether to generate the old style php enums
   */
  bool oldenum_;

  /**
   * * Whether to use collection classes everywhere vs Indexish
   */
  bool strict_types_;

  /**
   * Whether to generate strict hack
   */
  bool strict_;

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
   * True if struct fields within structs should be instantiated rather than nullable typed
   */
  bool no_nullables_;

  /**
   * True if struct constructors should accept arrays/Maps rather than their fields
   */
  bool map_construct_;

  /**
   * True if we should add a "use StructNameTrait" to the generated class
   */
  bool struct_trait_;

  /**
   * True if we should generate Shape types for the generated structs
   */
  bool shapes_;

  /**
   * True if we should generate array<arraykey, TValue> instead of array<string, TValue>
   */
  bool shape_arraykeys_;

  /**
   * True if we should not validate json when converting to Shapes
   */
  bool shape_unsafe_json_;

  /**
   * True if we should generate lazy initialization code for constants
   */
  bool lazy_constants_;

  /**
   * True to use Hack arrays instead of collections
   */
  bool arrays_;
};

void t_hack_generator::generate_json_enum(
    std::ofstream& out,
    t_name_generator& /* namer */,
    t_enum* tenum,
    const string& prefix_thrift,
    const string& prefix_json) {
  indent(out) << prefix_thrift << " = " << hack_name(tenum) << "::coerce("
              << prefix_json << ");";
}

void t_hack_generator::generate_json_struct(ofstream& out,
                                            t_name_generator& namer,
                                            t_struct* tstruct,
                                            const string& prefix_thrift,
                                            const string& prefix_json) {

  string enc = namer("$_tmp");
  indent(out) << enc << " = " << "json_encode(" << prefix_json
              << ");" << endl;
  string tmp = namer("$_tmp");
  t_field felem(tstruct, tmp);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << endl;
  indent(out) << tmp << "->readFromJson(" << enc << ");" << endl;
  indent(out) << prefix_thrift << " = " << tmp << ";" << endl;
}

void t_hack_generator::generate_json_field(ofstream& out,
                                           t_name_generator& namer,
                                           t_field* tfield,
                                           const string& prefix_thrift,
                                           const string& suffix_thrift,
                                           const string& prefix_json) {
  t_type* type = get_true_type(tfield->get_type());

  if (type->is_void()) {
    throw "CANNOT READ JSON FIELD WITH void TYPE: " +
      prefix_thrift + tfield->get_name();
  }

  string name = prefix_thrift + tfield->get_name() + suffix_thrift;

  if (type->is_struct() || type->is_xception()) {
    generate_json_struct(out, namer, (t_struct*)type, name, prefix_json);
  } else if (type->is_container()) {
    generate_json_container(out, namer, (t_container*)type, name, prefix_json);
  } else if (type->is_enum()) {
    generate_json_enum(
        out, namer, static_cast<t_enum*>(type), name, prefix_json);
  } else if (type->is_base_type()) {
    string typeConversionString = "";
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    string number_limit = "";
    switch (tbase) {
      case t_base_type::TYPE_VOID:
        break;
      case t_base_type::TYPE_STRING:
        break;
      case t_base_type::TYPE_BOOL:
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
      case t_base_type::TYPE_I64:
        break;
      case t_base_type::TYPE_DOUBLE:
        break;
      case t_base_type::TYPE_FLOAT:
        break;
      default:
        throw "compiler error: no PHP reader for base type "
          + t_base_type::t_base_name(tbase) + name;
    }

    if (number_limit.empty()) {
        indent(out) << name << " = " << typeConversionString
                    << prefix_json << ";" << endl;
    } else {
      string temp = namer("$_tmp");
      indent(out) << temp << " = (int)" << prefix_json << ";" << endl;
      indent(out) << "if (" << temp << " > " << number_limit << ") {" <<endl;
      indent_up();
      indent(out) << "throw new \\TProtocolException(\"number exceeds "
                  << "limit in field\");" << endl;
      indent_down();
      indent(out) << "} else {" <<endl;
      indent_up();
      indent(out) << name << " = " << typeConversionString << temp <<
        ";" << endl;
      indent_down();
      indent(out) << "}" << endl;
    }
  }
}

void t_hack_generator::generate_json_container(std::ofstream& out,
                                               t_name_generator& namer,
                                               t_type* ttype,
                                               const string& prefix_thrift,
                                               const string& prefix_json) {
  t_container* tcontainer = (t_container*)ttype;
  string size = namer("$_size");
  string key = namer("$_key");
  string value = namer("$_value");
  string json = namer("$_json");
  string container = namer("$_container");

  indent(out) << json << " = " << prefix_json << ";" << endl;
  if (ttype->is_map()) {
    if (arrays_) {
      indent(out) << container << " = dict[];" << endl;
    } else {
      indent(out) << container << " = Map {};" << endl;
    }
  } else if (ttype->is_list()) {
    if (arrays_) {
      indent(out) << container << " = vec[];" << endl;
    } else {
      indent(out) << container << " = Vector {};" << endl;
    }
  } else if (ttype->is_set()) {
    if (arrays_) {
      indent(out) << container << " = keyset[];" << endl;
    } else if (arraysets_) {
      indent(out) << container << " = array();" << endl;
    } else {
      indent(out) << container << " = Set {};" << endl;
    }
  }
  indent(out) << "foreach(" << json << " as " << key << " => " << value
              << ") {" << endl;
  indent_up();

  if (ttype->is_list()) {
    generate_json_list_element(out, namer, (t_list*)ttype, value, container);
  } else if (ttype->is_set()) {
    generate_json_set_element(out, namer, (t_set*)ttype, value, container);
  } else if (ttype->is_map()) {
    generate_json_map_element(out, namer, (t_map*)ttype, key, value, container);
  } else {
    throw "compiler error: no PHP reader for this type.";
  }
  indent_down();
  indent(out) << "}" << endl;
  indent(out) << prefix_thrift << " = " << container << ";" << endl;
}

void t_hack_generator::generate_json_list_element(ofstream& out,
                                                  t_name_generator& namer,
                                                  t_list* tlist,
                                                  const string& value,
                                                  const string& prefix_thrift) {
  string elem = namer("$_elem");
  t_field felem(tlist->get_elem_type(), elem);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << endl;
  generate_json_field(out, namer, &felem, "", "", value);
  indent(out) << prefix_thrift << " []= " << elem << ";" << endl;
}

void t_hack_generator::generate_json_set_element(std::ofstream& out,
                                                 t_name_generator& namer,
                                                 t_set* tset,
                                                 const string& value,
                                                 const string& prefix_thrift) {
  string elem = namer("$_elem");
  t_field felem(tset->get_elem_type(), elem);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << endl;
  generate_json_field(out, namer, &felem, "", "", value);
  if (arrays_) {
    indent(out) << prefix_thrift << " []= " << elem << ";" << endl;
  } else if (arraysets_) {
    indent(out) << prefix_thrift << "[" << elem << "] = true;" << endl;
  } else {
    indent(out) << prefix_thrift << "->add(" << elem << ");" << endl;
  }
}

void t_hack_generator::generate_json_map_element(std::ofstream& out,
                                                 t_name_generator& namer,
                                                 t_map* tmap,
                                                 const string& key,
                                                 const string& value,
                                                 const string& prefix_thrift) {
  t_type* keytype = get_true_type(tmap->get_key_type());
  bool succ = true;
  string error_msg = "compiler error: Thrift Hack compiler"
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
  indent(out) << declare_field(&vfelem, true, true, true).substr(1) << endl;
  generate_json_field(out, namer, &vfelem, "", "", value);
  indent(out) << prefix_thrift << "[" << key << "] = " << _value << ";" << endl;
}

void t_hack_generator::generate_json_reader(ofstream& out, t_struct* tstruct) {
  if(!json_) {
    return;
  }
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  t_name_generator namer;

  string name = tstruct->get_name();
  indent(out) << "public function readFromJson(string $jsonText): void {" << endl;
  indent_up();
  indent(out) << "$parsed = json_decode($jsonText, true);" << endl << endl;

  indent(out) << "if ($parsed === null || !is_array($parsed)) {" << endl;
  indent_up();
  indent(out) << "throw new \\TProtocolException(\"Cannot parse the given json"
              << " string.\");" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field *tf = *f_iter;
    indent(out) << "if (isset($parsed['" << tf->get_name() << "'])) {" << endl;
    indent_up();
    generate_json_field(
        out, namer, tf, "$this->", "", "$parsed['" + tf->get_name() + "']");
    generate_json_field(out,
                        namer,
                        tf,
                        "$this->",
                        "",
                        "$parsed[" + render_string(tf->get_name()) + "]");
    indent_down();
    indent(out) << "}";
    if(tf->get_req() == t_field::T_REQUIRED) {
      out << " else {" << endl;
      indent_up();
      indent(out) << "throw new \\TProtocolException(\"Required field "
                  << tf->get_name() << " cannot be found.\");" << endl;
      indent_down();
      indent(out) << "}";
    }
    indent(out) << endl;
  }
  indent_down();
  indent(out) << "}" << endl << endl;
}


/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_hack_generator::init_generator() {
  // Make output directory
  make_dir(get_out_dir().c_str());

  // Make output file
  string f_types_name = get_out_dir()+program_name_+"_types.php";
  f_types_.open(f_types_name.c_str());
  record_genfile(f_types_name);

  // Print header
  f_types_ << "<?hh" << (strict_ ? " // strict" : "") << endl
           << autogen_comment() << endl;

  string hack_ns = hack_namespace(program_);
  if (!hack_ns.empty()) {
    f_types_ << "namespace " << hack_ns << ";" << endl << endl;
  }

  // Print header
  if (!program_->get_consts().empty()) {
    string f_consts_name = get_out_dir() + program_name_ + "_constants.php";
    f_consts_.open(f_consts_name.c_str());
    record_genfile(f_consts_name);
    f_consts_ << "<?hh" << (strict_ ? " // strict" : "") << endl
              << autogen_comment();
    constants_values_.clear();
    string const_namespace = php_namespace(program_);
    if (const_namespace != "") {
      f_consts_ << "class " << const_namespace << "CONSTANTS {" << endl;
    } else {
      if (!hack_ns.empty()) {
        f_consts_ << "namespace " << hack_ns << ";" << endl << endl;
      }
      f_consts_ << "class " << program_name_ << "_CONSTANTS {" << endl;
    }
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
    f_consts_ << endl;
    if (!lazy_constants_) {
      indent(f_consts_) << "public static array $__values = array(" << endl;
      std::copy(constants_values_.begin(),
                constants_values_.end(),
                std::ostream_iterator<string>(f_consts_, ",\n"));
      indent(f_consts_) << ");" << endl;
    } else {
      stringstream oss(stringstream::out);
      oss << "array(" << endl;
      std::copy(constants_values_.begin(),
                constants_values_.end(),
                std::ostream_iterator<string>(oss, ",\n"));
      indent(oss) << "    )";

      string rendered_value = oss.str();
      generate_lazy_init_for_constant(
          f_consts_, "__values", "array", rendered_value);
    }
    indent_down();
    // close constants class
    f_consts_ << "}" << endl <<
      endl;
    f_consts_.close();
  }
}

/**
 * Generates a typedef. This is not done in PHP, types are all implicit.
 *
 * @param ttypedef The type definition
 */
void t_hack_generator::generate_typedef(t_typedef* /*ttypedef*/) {}

/**
 * Generates code for an enumerated type. Since define is expensive to lookup
 * in PHP, we use a global array for this.
 *
 * @param tenum The enumeration
 */
void t_hack_generator::generate_enum(t_enum* tenum) {
  vector<t_enum_value*> constants = tenum->get_constants();
  vector<t_enum_value*>::iterator c_iter;

  std::string typehint;
  generate_php_docstring(f_types_, tenum);
  bool hack_enum = false;
  if (is_bitmask_enum(tenum)) {
    typehint = "int";
    f_types_ << "final class " << hack_name(tenum, true) << " extends Flags {"
             << endl;
  } else if (oldenum_) {
    typehint = hack_name(tenum, true) + "Type";
    f_types_ << "newtype " << typehint << " = int;" << endl
             << "final class " << hack_name(tenum, true) << " extends Enum<"
             << typehint << "> {" << endl;
  } else {
    hack_enum = true;
    typehint = hack_name(tenum, true);
    f_types_ << "enum " << hack_name(tenum, true) << " : int {" << endl;
  }

  indent_up();

  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int32_t value = (*c_iter)->get_value();

    generate_php_docstring(f_types_, *c_iter);
    if (!hack_enum) {
      indent(f_types_) <<
        "const " << typehint << " ";
    }
    indent(f_types_) <<
      (*c_iter)->get_name() << " = " << value << ";" << endl;
  }

  if (oldenum_) {
    // names
    indent(f_types_) <<
      "public static array $__names = array(" << endl;
    for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
      int32_t value = (*c_iter)->get_value();

      indent(f_types_) <<
        "  " << value << " => '" << (*c_iter)->get_name() << "'," << endl;
    }
    indent(f_types_) <<
      ");" << endl;
    // values
    indent(f_types_) <<
      "public static array $__values = array(" << endl;
    for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
      int32_t value = (*c_iter)->get_value();

      indent(f_types_) <<
        "  '" << (*c_iter)->get_name() << "' => " << value << "," << endl;
    }
    indent(f_types_) <<
      ");" << endl;
  }

  indent_down();
  f_types_ << "}" << endl;
  if (hack_enum) {
    f_types_ <<
      "type " << typehint << "Type = " << typehint << ";" << endl;
  }
  f_types_ << endl;
}

/**
 * Generate a constant value
 */
void t_hack_generator::generate_const(t_const* tconst) {
  t_type* type = tconst->get_type();
  string name = tconst->get_name();
  t_const_value* value = tconst->get_value();

  indent_up();
  generate_php_docstring(f_consts_, tconst);
  if (!lazy_constants_) {
    // for base php types, use const (guarantees optimization in hphp)
    if (type->is_base_type()) {
      indent(f_consts_) << "const " << type_to_typehint(type) << " " << name
                        << " = ";
      // cannot use const for objects (incl arrays). use static
    } else {
      indent(f_consts_) << "public static " << type_to_typehint(type) << " $"
                        << name << " = ";
    }
    indent_up();
    f_consts_ << render_const_value(type, value);
    indent_down();
    f_consts_ << ";" << endl;

    // add the definitions to a values array as well
    // indent up cause we're going to be in an array definition
    indent_up();
    stringstream oss(stringstream::out);
    indent(oss) << render_string(name) << " => ";
    indent_up();

    oss << render_const_value(type, value);
    indent_down();
    indent_down();
    constants_values_.push_back(oss.str());
  } else {
    // generate rendered value with right number of identations (2)
    indent_up();
    indent_up();
    stringstream val(stringstream::out);
    val << render_const_value(type, value);
    indent_down();
    indent_down();

    string rendered_value = val.str();
    generate_lazy_init_for_constant(
        f_consts_, name, type_to_typehint(type), rendered_value);

    // add the definitions to a values array as well
    // indent up 3 times cause we're going to be 2 levels deeper
    // than in non lazy case
    indent_up();
    indent_up();
    indent_up();
    stringstream oss(stringstream::out);
    indent(oss) << render_string(name) << " => ";
    oss << render_const_value(type, value);
    indent_down();
    indent_down();
    indent_down();

    constants_values_.push_back(oss.str());
  }

  indent_down();
}

void t_hack_generator::generate_lazy_init_for_constant(
    ofstream& out,
    const std::string& name,
    const std::string& typehint,
    const std::string& rendered_value) {
  string name_internal = "__" + name;
  indent(out) << "private static ?" << typehint << " $" << name_internal
              << " = null;" << endl;
  indent(out) << "public static function " << name << "(): " << typehint << " {"
              << endl;
  indent(out) << "  if (self::$" << name_internal << " == null) {" << endl;
  indent(out) << "    self::$" << name_internal << " = " << rendered_value
              << ";" << endl;
  indent(out) << "  }" << endl;
  indent(out) << "  return self::$" << name_internal << ";" << endl;
  indent(out) << "}" << endl << endl;
}

string t_hack_generator::render_string(string value) {
  std::ostringstream out;
  size_t pos = 0;
  while((pos = value.find('"', pos)) != string::npos) {
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
    t_type* type,
    const t_const_value* value) {
  std::ostringstream out;
  type = get_true_type(type);
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
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
      if (out.str().find(".") == string::npos &&
          out.str().find("e") == string::npos) {
        out << ".0";
      }
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    t_enum* tenum = (t_enum*) type;
    const t_enum_value* val = tenum->find_value(value->get_integer());
    if (val != nullptr) {
      out << hack_name(tenum) << "::" << val->get_name();
    } else {
      out << hack_name(tenum) << "::coerce(" << value->get_integer() << ")";
    }
  } else if (type->is_struct() || type->is_xception()) {
    out << "new " << hack_name(type) << "(" << endl;
    indent_up();
    if (map_construct_) {
      if (arrays_) {
        out << indent() << "dict[" << endl;
      } else {
        out << indent() << "Map {" << endl;
      }
      indent_up();
    }
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      t_type* field_type = nullptr;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == nullptr) {
        throw "type error: " + type->get_name() + " has no field " + v_iter->first->get_string();
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
      out << indent();
      if (map_construct_) {
        if (v != nullptr) {
          out << render_const_value(g_type_string, k);
          out << " => ";
          out << render_const_value((*f_iter)->get_type(), v);
          out << "," << endl;
        }
      } else {
        if (v == nullptr) {
          out << "null," << endl;
        } else {
          out << render_const_value((*f_iter)->get_type(), v);
          out << "," << endl;
        }
      }
    }
    if (map_construct_) {
      indent_down();
      if (arrays_) {
        out << indent() << "]" << endl;
      } else {
        out << indent() << "}" << endl;
      }
    }
    indent_down();
    indent(out) << ")";
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    if (arrays_) {
      out << "dict[" << endl;
    } else {
      out << "Map {" << endl;
    }
    indent_up();
    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(ktype, v_iter->first);
      out << " => ";
      out << render_const_value(vtype, v_iter->second);
      out << "," << endl;
    }
    indent_down();
    if (arrays_) {
      indent(out) << "]";
    } else {
      indent(out) << "}";
    }
  } else if (type->is_list()) {
    t_type* etype = ((t_list*)type)->get_elem_type();
    if (arrays_) {
      out << "vec[" << endl;
    } else {
      out << "Vector {" << endl;
    }
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(etype, *v_iter);
      out << "," << endl;
    }
    indent_down();
    if (arrays_) {
      indent(out) << "]";
    } else {
      indent(out) << "}";
    }
  } else if (type->is_set()) {
    t_type* etype = ((t_set*)type)->get_elem_type();
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    if (arrays_) {
      out << "keyset[" << endl;
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        out << indent();
        out << render_const_value(etype, *v_iter);
        out << "," << endl;
      }
      indent_down();
      indent(out) << "]";
    } else if (arraysets_) {
      out << "array(" << endl;
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        out << indent();
        out << render_const_value(etype, *v_iter);
        out << " => true";
        out << "," << endl;
      }
      indent_down();
      indent(out) << ")";
    } else {
      out << "Set {" << endl;
      for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
        out << indent();
        out << render_const_value(etype, *v_iter);
        out << "," << endl;
      }
      indent_down();
      indent(out) << "}";
    }
  }
  return out.str();
}

string t_hack_generator::render_default_value(t_type* type) {
  string dval;
  type = get_true_type(type);
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
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
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    t_enum* tenum = (t_enum*) type;
    dval = "null";
  } else if (type->is_struct() || type->is_xception()) {
    t_struct* tstruct = (t_struct*) type;
    if (no_nullables_) {
      dval = "new " + hack_name(tstruct) + "()";
    } else {
      dval = "null";
    }
  } else if (type->is_map()) {
    if (arrays_) {
      dval = "dict[]";
    } else {
      dval = "Map {}";
    }
  } else if (type->is_list()) {
    if (arrays_) {
      dval = "vec[]";
    } else {
      dval = "Vector {}";
    }
  } else if (type->is_set()) {
    if (arrays_) {
      dval = "keyset[]";
    } else if (arraysets_) {
      dval = "array()";
    } else {
      dval = "Set {}";
    }
  }
  return dval;
}

/**
 * Make a struct
 */
void t_hack_generator::generate_struct(t_struct* tstruct) {
  generate_php_struct(tstruct, false);
}

/**
 * Generates a struct definition for a thrift exception. Basically the same
 * as a struct but extends the Exception class.
 *
 * @param txception The struct definition
 */
void t_hack_generator::generate_xception(t_struct* txception) {
  generate_php_struct(txception, true);
}

/**
 * Structs can be normal or exceptions.
 */
void t_hack_generator::generate_php_struct(t_struct* tstruct,
                                          bool is_exception) {
  generate_php_struct_definition(f_types_, tstruct, is_exception, false);
}

void t_hack_generator::generate_php_type_spec(ofstream& out,
                                             t_type* t) {
  t = get_true_type(t);
  indent(out) << "'type' => " << type_to_enum(t) << "," << endl;

  if (t->is_base_type()) {
    // Noop, type is all we need
  } else if (t->is_enum()) {
    t_enum* tenum = (t_enum*) t;
    indent(out) << "'enum' => '" << hack_name(t) << "'," << endl;
  } else if (t->is_struct() || t->is_xception()) {
    indent(out) << "'class' => '" << hack_name(t) <<"'," << endl;
  } else if (t->is_map()) {
    t_type* ktype = get_true_type(((t_map*)t)->get_key_type());
    t_type* vtype = get_true_type(((t_map*)t)->get_val_type());
    indent(out) << "'ktype' => " << type_to_enum(ktype) << "," << endl;
    indent(out) << "'vtype' => " << type_to_enum(vtype) << "," << endl;
    indent(out) << "'key' => array(" << endl;
    indent_up();
    generate_php_type_spec(out, ktype);
    indent_down();
    indent(out) << ")," << endl;
    indent(out) << "'val' => array(" << endl;
    indent_up();
    generate_php_type_spec(out, vtype);
    indent(out) << ")," << endl;
    if (arrays_) {
      indent(out) << "'format' => 'harray'," << endl;
    } else {
      indent(out) << "'format' => 'collection'," << endl;
    }
    indent_down();
  } else if (t->is_list()) {
    t_type* etype = get_true_type(((t_list*)t)->get_elem_type());
    indent(out) << "'etype' => " << type_to_enum(etype) <<"," << endl;
    indent(out) << "'elem' => array(" << endl;
    indent_up();
    generate_php_type_spec(out, etype);
    indent(out) << ")," << endl;
    if (arrays_) {
      indent(out) << "'format' => 'harray'," << endl;
    } else {
      indent(out) << "'format' => 'collection'," << endl;
    }
    indent_down();
  } else if (t->is_set()) {
    t_type* etype = get_true_type(((t_set*)t)->get_elem_type());
    indent(out) << "'etype' => " << type_to_enum(etype) <<"," << endl;
    indent(out) << "'elem' => array(" << endl;
    indent_up();
    generate_php_type_spec(out, etype);
    indent(out) << ")," << endl;
    if (arrays_) {
      indent(out) << "'format' => 'harray'," << endl;
    } else if (arraysets_) {
      indent(out) << "'format' => 'array'," << endl;
    } else {
      indent(out) << "'format' => 'collection'," << endl;
    }
    indent_down();
  } else {
    throw "compiler error: no type for php struct spec field";
  }

}

/**
 * Generates the struct specification structure, which fully qualifies enough
 * type information to generalize serialization routines.
 */
void t_hack_generator::generate_php_struct_spec(ofstream& out,
                                               t_struct* tstruct) {
  indent(out) << "public static array $_TSPEC = array(" << endl;
  indent_up();

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());
    indent(out) << (*m_iter)->get_key() << " => array(" << endl;
    indent_up();
    out <<
      indent() << "'var' => '" << (*m_iter)->get_name() << "'," << endl;
    if (tstruct->is_union()) {
      // Optimally, we shouldn't set this per field but rather per struct.
      // However, the tspec is a field_id => data array, and if we set it at
      // the top level people might think the 'union' key is a field id, which
      // isn't cool. It's safer and more bc to instead set this key on all
      // fields.
      out << indent() << "'union' => true," << endl;
    }
    generate_php_type_spec(out, t);
    indent(out) << ")," << endl;
    indent_down();
  }

  indent_down();
  indent(out) << "  );" << endl;

  indent(out) << "public static Map<string, int> $_TFIELDMAP = Map {" << endl;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());
    indent(out) << "  '" << (*m_iter)->get_name() << "' => " << (*m_iter)->get_key() << "," << endl;
  }
  indent(out) << "};" << endl;
}

void t_hack_generator::generate_php_struct_struct_trait(std::ofstream& out,
                                                     t_struct* tstruct) {

  string traitName;
  auto it = tstruct->annotations_.find("php.structtrait");
  if (it != tstruct->annotations_.end()) {
    if (it->second.empty() || it->second == "1") {
      traitName = hack_name(tstruct) + "Trait";
    } else {
      traitName = hack_name(it->second, tstruct->get_program());
    }
  } else if (struct_trait_) {
    traitName = hack_name(tstruct) + "Trait";
  }

  if (!traitName.empty()) {
    indent(out) << "use " << traitName << ";" << endl << endl;
  }
}

void t_hack_generator::generate_php_struct_shape_spec(std::ofstream& out,
                                                     t_struct* tstruct) {
  indent(out) << "const type TShape = shape(" << endl;
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());

    string dval = "";
    if ((*m_iter)->get_value() != nullptr
        && !(t->is_struct()
          || t->is_xception()
        )
    ) {
      dval = render_const_value(t, (*m_iter)->get_value());
    } else {
      dval = render_default_value(t);
    }

    bool nullable = field_is_nullable(tstruct, *m_iter, dval);

    string typehint = nullable ? "?" : "";

    typehint += type_to_typehint(t, false, true);

    indent(out) << "  '" << (*m_iter)->get_name() << "' => " << typehint << "," << endl;
  }
  indent(out) << ");" << endl;
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
 *     $_val0 ==> $_val0->toArray(),
 *   )->toArray(),
 *
 * And this method here will get called with
 *
 *  generate_php_struct_shape_collection_value_lambda(..., list<i32>)
 *
 * And returns the string:
 *
 *   "  $_val0 ==> $_val0->toArray(),"
 *
 * This method operates via recursion on complex types.
 */
void t_hack_generator::generate_php_struct_shape_collection_value_lambda(std::ostream& out,
                                                                         t_name_generator& namer,
                                                                         t_type* t) {
  string tmp = namer("_val");
  indent(out) << "$" << tmp << " ==> ";
  if (t->is_struct()) {
    out << "$" << tmp << "->__toShape()," << endl;
  } else if (t->is_set()) {
    out << "array_fill_keys($" << tmp << ", true)," << endl;
  } else if (t->is_map() ||
             t->is_list()) {

    t_type* val_type;
    if (t->is_map()) {
      val_type = ((t_map*)t)->get_val_type();
    } else {
      val_type = ((t_list*)t)->get_elem_type();
    }
    val_type = get_true_type(val_type);

    if (!val_type->is_container() &&
        !val_type->is_struct()) {
      out << "$" << tmp << "->toArray()," << endl;
      return;
    }

    out << "$" << tmp << "->map(" << endl;
    indent_up();
    generate_php_struct_shape_collection_value_lambda(out, namer, val_type);
    indent_down();
    indent(out) << ")->toArray()," << endl;
  }
}

/**
 * Determine whether a field should be marked nullable.
 */
bool t_hack_generator::field_is_nullable(t_struct* tstruct,
                                         const t_field* field,
                                         string dval) {
  t_type* t = get_true_type(field->get_type());
  return
    (dval == "null") ||
    tstruct->is_union() ||
    (field->get_req() == t_field::T_OPTIONAL &&
     field->get_value() == nullptr) ||
    (t->is_enum() && field->get_req() != t_field::T_REQUIRED);
}


/**
 * Recursively convert a json field into a shape field by
 * -- performing type validation, and
 * -- converting Sets from array<int, X> to array<X, bool>
 *
 * Example output for a field 'set_of_i32' is:
 *
 *   if (!array_key_exists('set_of_i32', $shape_data)) {
 *     return null;
 *   }
 *   $the_set2 = array();
 *   foreach ($shape_data['set_of_i32'] as $key0 => $shape_data1) {
 *     if (!is_int($shape_data1)) {
 *       return null;
 *     }
 *     $the_set2[$shape_data1] = true;
 *   }
 *   $shape_data['set_of_i32'] = $the_set2;
 */
void t_hack_generator::generate_php_struct_shape_json_conversion(std::ofstream& out,
                                                                 bool nullable,
                                                                 string shape_data,
                                                                 t_type *t,
                                                                 t_name_generator& namer) {

  t = get_true_type(t);
  if (t->is_base_type()) {
    if (!shape_unsafe_json_) {
      switch (((t_base_type*)t)->get_base()) {
        case t_base_type::TYPE_STRING:
          indent(out) << "if (!is_string(" << shape_data << ")";
          if (nullable) {
            out << " && !is_null(" << shape_data << ")";
          }
          out << ") {" << endl;
          indent(out) << "  return null;" << endl;
          indent(out) << "}" << endl;
          return;
        case t_base_type::TYPE_BOOL:
          indent(out) << "if (!is_bool(" << shape_data << ")";
          if (nullable) {
            out << " && !is_null(" << shape_data << ")";
          }
          out << ") {" << endl;
          indent(out) << "  return null;" << endl;
          indent(out) << "}" << endl;
          return;
        case t_base_type::TYPE_BYTE:
        case t_base_type::TYPE_I16:
        case t_base_type::TYPE_I32:
        case t_base_type::TYPE_I64:
          indent(out) << "if (!is_int(" << shape_data << ")";
          if (nullable) {
            out << " && !is_null(" << shape_data << ")";
          }
          out << ") {" << endl;
          indent(out) << "  return null;" << endl;
          indent(out) << "}" << endl;
          return;
        case t_base_type::TYPE_DOUBLE:
        case t_base_type::TYPE_FLOAT:
          indent(out) << "if (!(is_float(" << shape_data
                      << ") || is_int(" << shape_data << "))";
          if (nullable) {
            out << " && !is_null(" << shape_data << ")";
          }
          out << ") {" << endl;
          indent(out) << "  return null;" << endl;
          indent(out) << "}" << endl;
          indent(out) << "if (is_int(" << shape_data << ")) {" << endl;
          indent(out) << "  " << shape_data
                      << " = (float)" << shape_data << ";" << endl;
          indent(out) << "}" << endl;
          return;
        default:
          indent(out) << "return null;" << endl;
      }
    }
  } else if (t->is_set()) {
    string k = "$" + namer("key");
    string v = "$" + namer("shape_data");
    if (nullable) {
      indent(out) << "if (!is_null(" << shape_data << ")) {" << endl;
      indent_up();
    }
    if (!shape_unsafe_json_) {
      indent(out) << "if (!is_array(" << shape_data << ")) {" << endl;
      indent(out) << "  return null;" << endl;
      indent(out) << "}" << endl;
    }
    t_type* val_type = get_true_type(((t_set*)t)->get_elem_type());
    string the_set = "$" + namer("the_set");
    indent(out) << the_set << " = array();" << endl;
    indent(out) << "foreach (/* HH_IGNORE_ERROR[4110] */ " << shape_data << " as "
                << k << " => " << v << ") {" << endl;
    if (!shape_unsafe_json_) {
      if (((t_base_type*)val_type)->get_base() == t_base_type::TYPE_STRING) {
        indent(out) << "  if (!is_string(" << v << ")) {" << endl;
        indent(out) << "    return null;" << endl;
        indent(out) << "  }" << endl;
      } else {
        indent(out) << "  if (!is_int(" << v << ")) {" << endl;
        indent(out) << "    return null;" << endl;
        indent(out) << "  }" << endl;
      }
    }
    indent(out) << "  " << the_set << "[" << v << "] = true;" << endl;
    indent(out) << "}" << endl;
    indent(out) << shape_data << " = " << the_set << ";" << endl;
    if (nullable) {
      indent_down();
      indent(out) << "}" << endl;
    }
  } else if (t->is_list()) {
    t_type* val_type = get_true_type(((t_list*)t)->get_elem_type());
    string k = "$" + namer("key");
    string v = "$" + namer("value");
    if (nullable) {
      indent(out) << "if (!is_null(" << shape_data << ")) {" << endl;
      indent_up();
    }
    if (!shape_unsafe_json_) {
      indent(out) << "if (!is_array(" << shape_data << ")) {" << endl;
      indent(out) << "  return null;" << endl;
      indent(out) << "}" << endl;
    }
    indent(out) << "foreach (/* HH_IGNORE_ERROR[4110] */" << shape_data << " as "
                << k << " => " << v << ") {" << endl;
    indent_up();
    if (!shape_unsafe_json_) {
      indent(out) << "if (!is_int(" << k << ")) {" << endl;
      indent(out) << "  return null;" << endl;
      indent(out) << "}" << endl;
    }
    generate_php_struct_shape_json_conversion(out,
                                              false,
                                              v,
                                              val_type,
                                              namer);
    indent(out) << "/* HH_IGNORE_ERROR[4005] */" << endl;
    indent(out) << shape_data << "[" << k << "] = " << v << ";" << endl;
    indent_down();
    indent(out) << "}" << endl;
    if (nullable) {
      indent_down();
      indent(out) << "}" << endl;
    }
  } else if (t->is_map()) {
    t_type* key_type = get_true_type(((t_map*)t)->get_key_type());
    t_type* val_type = get_true_type(((t_map*)t)->get_val_type());
    string k = "$" + namer("key");
    string v = "$" + namer("value");
    if (nullable) {
      indent(out) << "if (!is_null(" << shape_data << ")) {" << endl;
      indent_up();
    }
    if (!shape_unsafe_json_) {
      indent(out) << "if (!is_array(" << shape_data << ")) {" << endl;
      indent(out) << "  return null;" << endl;
      indent(out) << "}" << endl;
    }
    indent(out) << "foreach (/* HH_IGNORE_ERROR[4110] */" << shape_data << " as "
                << k << " => " << v << ") {" << endl;
    indent_up();

    bool key_is_string = false;
    if (((t_base_type*)key_type)->get_base() == t_base_type::TYPE_STRING) {
      key_is_string = true;
    }
    if (!shape_unsafe_json_) {
      if (shape_arraykeys_) {
        indent(out) << "if (!is_string(" << k << ") && " << endl;
        indent(out) << "    !is_int(" << k << ")) {" << endl;
        indent(out) << "  return null;" << endl;
        indent(out) << "}" << endl;
      } else {
        if (key_is_string) {
          indent(out) << "if (!is_string(" << k << ")) {" << endl;
          indent(out) << "  return null;" << endl;
          indent(out) << "}" << endl;
        } else {
          indent(out) << "if (!is_int(" << k << ")) {" << endl;
          indent(out) << "  return null;" << endl;
          indent(out) << "}" << endl;
        }
      }
    }
    generate_php_struct_shape_json_conversion(out,
                                              false,
                                              v,
                                              val_type,
                                              namer);
    indent(out) << "/* HH_IGNORE_ERROR[4005] */" << endl;
    indent(out) << shape_data << "[" << k << "] = " << v << ";" << endl;
    indent_down();
    indent(out) << "}" << endl;
    if (nullable) {
      indent_down();
      indent(out) << "}" << endl;
    }
  } else if (t->is_struct()) {
    string struct_type = hack_name(t);

    if (nullable && !shape_unsafe_json_) {
      indent(out) << "if (!is_null(" << shape_data << ")) {" << endl;
      indent_up();
    }
    indent(out) << shape_data << " = "
                << struct_type
                << "::__jsonArrayToShape(/* HH_IGNORE_ERROR[4110] */ "
                << shape_data << ");" << endl;
    if (!shape_unsafe_json_) {
      indent(out) << "if (is_null(" << shape_data << ")) {" << endl;
      indent(out) << "  return null;" << endl;
      indent(out) << "}" << endl;
    }
    if (nullable && !shape_unsafe_json_) {
      indent_down();
      indent(out) << "}" << endl;
    }
  } else if (t->is_enum()) {
    if (!shape_unsafe_json_) {
      indent(out) << "if (!is_int(" << shape_data << ")";
      if (nullable) {
        out << " && !is_null(" << shape_data << ")";
      }
      out << ") {" << endl;
      indent(out) << "  return null;" << endl;
      indent(out) << "}" << endl;
    }
  } else {
    indent(out) << "return null; // unknown type" <<endl;
  }
}

void t_hack_generator::generate_php_struct_shape_methods(std::ofstream& out,
                                                         t_struct* tstruct) {
  indent(out) << "public static function __jsonArrayToShape(" << endl;
  indent(out) << "  array<arraykey, mixed> $json_data," << endl;
  indent(out) << "): ?self::TShape {" << endl;
  indent_up();
  indent(out) << "$shape_data = $json_data;"<<endl;
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  t_name_generator namer;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());

    string dval = "";
    // If a value was specified for the field in the thrift file, and the field
    // is not a struct or exception, use the value as the default.
    if ((*m_iter)->get_value() != nullptr
        && !(t->is_struct() || t->is_xception())) {
      dval = render_const_value(t, (*m_iter)->get_value());
    // Otherwise, the default value is null (or equivalent).
    } else {
      dval = render_default_value(t);
    }

    bool nullable = field_is_nullable(tstruct, *m_iter, render_default_value(t));
    out << endl;

    // Note: default values are not currently working for non-POD types.
    // The logic below needs to be reworked so that dval is used for nullable
    // types (if available). See D4609418 for more information.
    if (!shape_unsafe_json_) {
      indent(out) << "if (!array_key_exists('"
                  << (*m_iter)->get_name() << "', $shape_data)) {" << endl;
      if (nullable) {
        indent(out) << "  $shape_data['" << (*m_iter)->get_name() << "'] = null;"
                    << endl;
      } else if (dval != "null" && t->is_base_type()) {
        // for "optional" fields with default values:
        indent(out) << "  $shape_data['" << (*m_iter)->get_name() << "'] = "
                    << dval << ";"
                    << endl;
      } else {
        indent(out) << "  return null;" << endl;
      }
      indent(out) << "}" << endl;
    }
    string value = "$shape_data['" + (*m_iter)->get_name() + "']";
    generate_php_struct_shape_json_conversion(out,
                                              nullable,
                                              value,
                                              t,
                                              namer);
  }
  out << endl;
  indent(out) << "return /* HH_IGNORE_ERROR[4110] */ $shape_data;" << endl;
  indent_down();
  indent(out) << "}" << endl;
  out << endl;

  if (shape_arraykeys_) {
    indent(out) << "public static function __stringifyMapKeys<T>("
                << "Map<arraykey, T> $m): Map<string, T> {" << endl;
    indent(out) << "  $new_map = Map {};" << endl;
    indent(out) << "  foreach ($m as $k => $v) {" << endl;
    indent(out) << "    $new_map[(string)$k] = $v;" << endl;
    indent(out) << "  }" << endl;
    indent(out) << "  return $new_map;" << endl;
    indent(out) << "}" << endl;
    out << endl;
  }

  indent(out) << "public static function __fromShape(self::TShape $shape): this {" << endl;
  indent_up();
  indent(out) << "$me = /* HH_IGNORE_ERROR[4060] */ new static();" << endl;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());

    string dval = "";
    if ((*m_iter)->get_value() != nullptr &&
        !(t->is_struct() || t->is_xception())) {
      dval = render_const_value(t, (*m_iter)->get_value());
    } else {
      dval = render_default_value(t);
    }

    bool nullable = field_is_nullable(tstruct, *m_iter, dval);

    stringstream source;
    if (nullable) {
      source << "Shapes::idx($shape, '" << (*m_iter)->get_name() << "')";
    } else {
      source << "$shape['" << (*m_iter)->get_name() << "']";
    }

    stringstream val;

    indent(val) << "$me->" << (*m_iter)->get_name() << " = ";
    if (t->is_set()) {
      if (nullable) {
        val << source.str() << " === null ? null : ";
      }
      if (arraysets_) {
        val << source.str() << ";" << endl;
      } else {
        val << "new Set(array_keys("
            << (nullable ? "nullthrows(" : "")
            << source.str()
            << (nullable ? ")" : "")
            << "));" << endl;
      }
    } else if (t->is_map() || t->is_list()) {
      if (nullable) {
        val << source.str() << " === null ? null : " << endl;
        indent_up();
        indent(val);
      }

      bool stringify_map_keys = false;
      if (t->is_map() && shape_arraykeys_) {
        t_type *key_type =((t_map*)t)->get_key_type();
        if (key_type->is_base_type() &&
            ((t_base_type*)key_type)->get_base() == t_base_type::TYPE_STRING) {
          stringify_map_keys = true;
        }
      }

      if (stringify_map_keys) {
        val << "self::__stringifyMapKeys(";
      } else {
        val << "(";
      }

      if (t->is_map()) {
        val << "new Map(";
      } else {
        val << "new Vector(";
      }

      val << source.str() << "))";

      int nest = 0;
      while (true) {
        t_type* val_type;
        if (t->is_map()) {
          val_type = ((t_map*)t)->get_val_type();
        } else {
          val_type = ((t_list*)t)->get_elem_type();
        }
        val_type = get_true_type(val_type);

        if ((val_type->is_set() && !arraysets_) ||
            val_type->is_map() ||
            val_type->is_list() ||
            val_type->is_struct()) {
          indent_up();
          nest++;
          val << "->map(" << endl;

          if (val_type->is_set()) {
            string tmp = namer("val");
            indent(val) << "$" << tmp << " ==> new Set(array_keys($" << tmp << "))," << endl;
            break;
          } else if (val_type->is_map() || val_type->is_list()) {
            string tmp = namer("val");

            stringify_map_keys = false;
            if (val_type->is_map() && shape_arraykeys_) {
              t_type *key_type =((t_map*)val_type)->get_key_type();
              if (key_type->is_base_type() &&
                  ((t_base_type*)key_type)->get_base() == t_base_type::TYPE_STRING) {
                stringify_map_keys = true;
              }
            }

            indent(val)  << "$" << tmp
                         << " ==> "
                         << (stringify_map_keys ? "self::__stringifyMapKeys" : "")
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
                        << "$" << tmp << ")," << endl;
            break;
          }
        } else {
          if (nest > 0) {
            val << "," <<  endl;
          }
          break;
        }
      }
      while (nest-- > 0) {
        indent_down();
        indent(val) << ")";
        if (nest > 0) {
          val << "," << endl;
        }
      }
      val << ";" << endl;
      if (nullable) {
        indent_down();
      }
    } else if (t->is_struct()) {
      string type = hack_name(t);
      if (nullable) {
        val <<source.str() << " === null ? null : ";
      }
      val
        << type << "::__fromShape("
        << (nullable ? "nullthrows(" : "")
        << source.str()
        << (nullable ? ")" : "")
        << ");" << endl;
    } else {
      val << source.str() << ";" << endl;
    }
    out << val.str();
  }
  indent(out) << "return $me;" << endl;
  indent_down();
  indent(out) << "}" << endl;
  out << endl;

  indent(out) << "public function __toShape(): self::TShape {" << endl;
  indent_up();
  indent(out) << "return shape(" << endl;
  indent_up();
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());

    indent(out) << "'" << (*m_iter)->get_name() << "' => ";

    stringstream val;

    bool nullable = field_is_nullable(tstruct, *m_iter, render_default_value(t));

    if (t->is_container()) {
      if (t->is_map() || t->is_list()) {
        val << "$this->" << (*m_iter)->get_name() << (nullable ? "?" :"");

        t_type* val_type;
        if (t->is_map()) {
          val_type = ((t_map*)t)->get_val_type();
        } else {
          val_type = ((t_list*)t)->get_elem_type();
        }
        val_type = get_true_type(val_type);

        if (val_type->is_container() ||
            val_type->is_struct()) {
          val  << "->map(" << endl;
          indent_up();
          t_name_generator ngen;
          generate_php_struct_shape_collection_value_lambda(val, ngen, val_type);
          indent_down();
          indent(val) << ")" << (nullable ? "?" :"") << "->toArray()," << endl;
        } else {
          val <<  "->toArray()," << endl;
        }
      } else {
        if (nullable) {
          val << "$this->" << (*m_iter)->get_name() << " === null ? null : ";
        }
        if (arraysets_) {
          val << "$this->" << (*m_iter)->get_name() << "," << endl;
        } else {
          val << "array_fill_keys("
              << (nullable ? "nullthrows(" : "")
              << "$this->" << (*m_iter)->get_name() << "->toValuesArray()"
              << (nullable ? ")" : "")
              << ", true)," << endl;
        }
      }
    } else if (t->is_struct()) {
      val << "$this->" << (*m_iter)->get_name();
      val << (nullable ? "?" : "") << "->__toShape()," << endl;
    } else {
      val << "$this->" << (*m_iter)->get_name() << "," << endl;
    }

    out << val.str();
  }
  indent_down();
  indent(out) << ");" << endl;
  indent_down();
  indent(out) << "}" << endl;
}

/**
 * Generates the structural ID definition, see generate_structural_id()
 * for information about the structural ID.
 */
void t_hack_generator::generate_php_structural_id(ofstream& out,
                                                  t_struct* tstruct,
                                                  bool asFunction) {
  if (asFunction) {
    indent(out) << "static function getStructuralID(): int {" << endl;
    indent_up();
    indent(out) << "return "
                << generate_structural_id(tstruct->get_members())
                << ";"
                << endl;
    indent_down();
    indent(out) << "}" << endl;
  } else {
    indent(out) << "const int STRUCTURAL_ID = "
                << generate_structural_id(tstruct->get_members()) << ";"
                << endl;
  }
}

void t_hack_generator::generate_php_struct_definition(
    ofstream& out,
    t_struct* tstruct,
    bool is_exception,
    bool is_result,
    bool is_args) {
  if (tstruct->is_union()) {
    // Generate enum for union before the actual class
    generate_php_union_enum(out, tstruct);
  }
  _generate_php_struct_definition(
      out, tstruct, is_exception, is_result, is_args);
}

void t_hack_generator::generate_php_union_methods(ofstream& out,
                                                  t_struct* tstruct) {
  vector<t_field*>::const_iterator m_iter;
  const vector<t_field*>& members = tstruct->get_members();
  auto enumName = union_enum_name(tstruct);

  // getType() : <UnionName>Enum {}
  indent(out) << "public function getType(): " << enumName << " {" << endl;
  indent(out) << indent() << "return $this->_type;"<< endl;
  indent(out) << "}" << endl << endl;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    auto fieldName = (*m_iter)->get_name();
    auto typehint = type_to_typehint((*m_iter)->get_type());
    // set_<fieldName>()
    indent(out) << "public function set_" << fieldName << "(" << typehint <<
        " $" << fieldName << "): this {" << endl;
    indent_up();
    indent(out) << "$this->_type = " << enumName << "::" << fieldName << ";" <<
        endl;
    indent(out) << "$this->" << fieldName << " = " << "$" << fieldName << ";" <<
        endl;
    indent(out) << "return $this;" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;

    // set_<fieldName>()
    indent(out) << "public function get_" << fieldName << "(): " << typehint <<
        " {" << endl;
    indent_up();
    indent(out) << "invariant(" << endl;
    indent_up();
    indent(out) << "$this->_type === " << enumName << "::" << fieldName <<
        "," << endl;
    indent(out) << "'get_" << fieldName << " called on an instance of " <<
        tstruct->get_name() << " whose current type is %s'," << endl;
    indent(out) << "$this->_type," << endl;
    indent_down();
    indent(out) << ");" << endl;
    indent(out) << "return nullthrows($this->" << fieldName << ");" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;
  }
}

void t_hack_generator::generate_php_union_enum(ofstream& out,
                                               t_struct* tstruct) {
  // Generate enum class with this pattern
  // enum <UnionName>Enum: int {
  //   __EMPTY__ = 0;
  //   field1 = 1;
  // }
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  out << "enum " << union_enum_name(tstruct) << ": int {"<< endl;

  indent_up();
  // If no member is set
  indent(out) <<  UNION_EMPTY << " = 0;" << endl;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out)
        << (*m_iter)->get_name() << " = "<< (*m_iter)->get_key() << ";" << endl;
  }
  indent_down();
  out << "}" << endl << endl;
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
    t_struct* tstruct,
    bool is_exception,
    bool is_result,
    bool is_args) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  bool generateAsTrait =
    tstruct->annotations_.find("php.trait") != tstruct->annotations_.end();

  if (!is_result && !is_args && (is_exception || !generateAsTrait)) {
    generate_php_docstring(out, tstruct, is_exception);
  }
  out << (generateAsTrait ? "trait " : "class ")
      << hack_name(tstruct, true);
  if (generateAsTrait) {
    out << "Trait";
  } else if (is_exception) {
    out << " extends \\TException";
  }
  out << " implements \\IThriftStruct";

  if (tstruct->is_union()) {
    out << ", \\IThriftUnion<" << union_enum_name(tstruct) << ">";
  }

  if (shapes_) {
    out << ", \\IThriftShapishStruct";
  }

  out << " {" << endl;
  indent_up();

  if (generateAsTrait && is_exception) {
    indent(out) << "require extends TException;" << endl;
  }

  generate_php_struct_struct_trait(out, tstruct);
  generate_php_struct_spec(out, tstruct);

  if (shapes_ && !is_exception && !is_result) {
    generate_php_struct_shape_spec(out, tstruct);
  }

  generate_php_structural_id(out, tstruct, generateAsTrait);

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());

    if (t->is_enum() && is_bitmask_enum((t_enum*) t)) {
      throw "Enum " + (((t_enum*) t)->get_name()) + "is actually a bitmask, cannot generate a field of this enum type";
    }

    string dval = "";
    if ((*m_iter)->get_value() != nullptr
        && !(t->is_struct()
          || t->is_xception()
        )
    ) {
      dval = render_const_value(t, (*m_iter)->get_value());
    } else {
      dval = render_default_value(t);
    }

    // result structs only contain fields: success and e.
    // success is whatever type the method returns, but must be nullable
    // regardless, since if there is an exception we expect it to be null
    // TODO(ckwalsh) Extract this logic into a helper function
    bool nullable = (dval == "null")
      || tstruct->is_union()
        || is_result
      || ((*m_iter)->get_req() == t_field::T_OPTIONAL
          && (*m_iter)->get_value() == nullptr)
      || (t->is_enum()
          && (*m_iter)->get_req() != t_field::T_REQUIRED);
    string typehint = nullable ? "?" : "";

    typehint += type_to_typehint(t);
    if (!is_result && !is_args) {
      generate_php_docstring(out, *m_iter);
    }
    indent(out) << "public " << typehint << " $" << (*m_iter)->get_name() << ";"
                << endl;
  }

  if (tstruct->is_union()) {
    // Generate _type to store which field is set and initialize it to _EMPTY_
    indent(out) <<
      "protected " << union_enum_name(tstruct) << " $_type = "
      << union_field_to_enum(tstruct, nullptr) << ";" << endl;
  }

  out << endl;

  if (map_construct_) {
    if (strict_types_) {
      // Generate constructor from Map
      out <<
        indent() << "public function __construct(Map<string, mixed> $vals = Map {}) {" << endl;
    } else {
      // Generate constructor from Indexish
      out <<
        indent() << "public function __construct(@\\Indexish<string, mixed> $vals = array()) {" << endl;
    }
    out <<
      indent() << "  // UNSAFE_BLOCK $vals is not type safe :(, and we don't cast structs (yet)" << endl;
  } else {
    out <<
      indent() << "public function __construct(";
    bool first = true;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if (!first) {
        out << ", ";
      }
      t_type* t = get_true_type((*m_iter)->get_type());
      out << "?" << type_to_typehint(t) << " $" << (*m_iter)->get_name() << " = null";
      first = false;
    }
    out <<
      indent() << ") {" << endl;
  }
  indent_up();

  if (is_exception) {
    out << indent() << "parent::__construct();" << endl;
  }
  if (tstruct->is_union()) {
    out << indent() << "$this->_type = "
        << union_field_to_enum(tstruct, nullptr) << ";" << endl;
  }
  for (m_iter = members.begin(); !is_result && m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());
    string dval = "";
    if ((*m_iter)->get_value() != nullptr && !(t->is_struct() || t->is_xception())) {
      dval = render_const_value(t, (*m_iter)->get_value());
    } else if (tstruct->is_union()) {
      dval = "null";
    } else {
      dval = render_default_value(t);
    }

    if (map_construct_) {
      string cast = type_to_cast(t);

      if (strict_types_) {
        out <<
          indent() << "$this->" << (*m_iter)->get_name() << " = " << cast << "($vals->get('" <<
            (*m_iter)->get_name() << "') ?: " << dval << ");" << endl;
      } else {
        if ((*m_iter)->get_req() == t_field::T_OPTIONAL && (*m_iter)->get_value() == nullptr) {
          out <<
            indent() << "if (array_key_exists('" << (*m_iter)->get_name() << "', $vals)) {" << endl;
          indent_up();
        }
        out <<
          indent() << "$this->" << (*m_iter)->get_name() << " = " << cast << "idx($vals, '" <<
            (*m_iter)->get_name() << "', " << dval << ");" << endl;
        if ((*m_iter)->get_req() == t_field::T_OPTIONAL && (*m_iter)->get_value() == nullptr) {
          indent_down();
          out << indent() << "}" << endl;
        }
      }
    } else {
      // result structs only contain fields: success and e.
      // success is whatever type the method returns, but must be nullable
      // regardless, since if there is an exception we expect it to be null
      // TODO(ckwalsh) Extract this logic into a helper function
      bool nullable = (dval == "null")
          || is_result
          || ((*m_iter)->get_req() == t_field::T_OPTIONAL
              && (*m_iter)->get_value() == nullptr);

      if (tstruct->is_union()) {
        // Capture value from constructor and update _type field
        out <<
          indent() << "if ($" << (*m_iter)->get_name() << " !== null) {" << endl <<
          indent() << "  $this->" << (*m_iter)->get_name() << " = $" << (*m_iter)->get_name() << ";" << endl <<
          indent() << "  $this->_type = " << union_field_to_enum(tstruct, *m_iter) << ";" << endl <<
          indent() << "}" << endl;
      } else if (nullable) {
        indent(out) << "$this->" << (*m_iter)->get_name() << " = $" << (*m_iter)->get_name() << ";" << endl;
      } else {
        out <<
          indent() << "if ($" << (*m_iter)->get_name() << " === null) {" << endl <<
          indent() << "  $this->" << (*m_iter)->get_name() << " = " << dval << ";" << endl <<
          indent() << "} else {" << endl <<
          indent() << "  $this->" << (*m_iter)->get_name() << " = $" << (*m_iter)->get_name() << ";" << endl <<
          indent() << "}" << endl;
      }
    }
  }

  scope_down(out);
  out << endl;

  out <<
    indent() << "public function getName(): string {" << endl <<
    indent() << "  return '" << tstruct->get_name() << "';" << endl <<
    indent() << "}" << endl <<
    endl;
  if (tstruct->is_union()) {
    generate_php_union_methods(out, tstruct);
  }
  if (shapes_ && !is_exception && !is_result) {
    generate_php_struct_shape_methods(out, tstruct);
  }
  generate_php_struct_reader(out, tstruct);
  generate_php_struct_writer(out, tstruct);
  generate_json_reader(out, tstruct);
  indent_down();
  out <<
    indent() << "}" << endl <<
    endl;
}

/**
 * Generates the read() method for a struct
 */
void t_hack_generator::generate_php_struct_reader(ofstream& out,
                                                 t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  indent(out) <<
    "public function read(\\TProtocol $input): int {" << endl;
  indent_up();
  t_name_generator namer;

  out <<
    indent() << "$xfer = 0;" << endl <<
    indent() << "$fname = '';" << endl <<
    indent() << "$ftype = 0;" << endl <<
    indent() << "$fid = 0;" << endl;

  if (tstruct->is_union()) {
    // init _type field
    out << indent() << "$this->_type = "
        << union_field_to_enum(tstruct, nullptr) << ";" << endl;
  }

  // create flags for required fields, we check then after reading
  for (const auto& f : fields) {
    if (f->get_req() == t_field::T_REQUIRED) {
      indent(out) << "$" << f->get_name() << "__isset = false;" << endl;
    }
  }
  // Declare stack tmp variables
  indent(out) <<
    "$xfer += $input->readStructBegin($fname);" << endl;

  // Loop over reading in fields
  indent(out) <<
    "while (true)" << endl;

    scope_up(out);

    // Read beginning field marker
    out <<
      indent() << "$xfer += $input->readFieldBegin($fname, $ftype, $fid);" << endl;
    // Check for field STOP marker and break
    indent(out) <<
      "if ($ftype == \\TType::STOP) {" << endl;
    indent_up();
    indent(out) <<
      "break;" << endl;
    indent_down();
    indent(out) <<
      "}" << endl;
    out <<
      indent() << "if (!$fid && $fname !== null) {" << endl <<
      indent() << "  $fid = (int) self::$_TFIELDMAP->get($fname);" << endl <<
      indent() << "  if ($fid !== 0) {" << endl <<
      indent() << "    $ftype = self::$_TSPEC[$fid]['type'];" << endl <<
      indent() << "  }" << endl <<
      indent() << "}" << endl;

    // Switch statement on the field we are reading
    indent(out) <<
      "switch ($fid)" << endl;

      scope_up(out);

      // Generate deserialization code for known cases
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        indent(out) <<
          "case " << (*f_iter)->get_key() << ":" << endl;
        indent_up();
        indent(out) << "if ($ftype == " << type_to_enum((*f_iter)->get_type()) << ") {" << endl;
        indent_up();
        generate_deserialize_field(
            out, namer, *f_iter, "this->" + (*f_iter)->get_name(), true);
        if (tstruct->is_union()) {
          // Update _type for union
          indent(out) << "$this->_type = "
                      << union_field_to_enum(tstruct, *f_iter) << ";" << endl;
        }
        if ((*f_iter)->get_req() == t_field::T_REQUIRED) {
          indent(out) << "$" << (*f_iter)->get_name() << "__isset = true;"
                      << endl;
        }
        indent_down();
        out <<
          indent() << "} else {" << endl;
        indent(out) <<  "  $xfer += $input->skip($ftype);" << endl;
        out <<
          indent() << "}" << endl <<
          indent() << "break;" << endl;
        indent_down();
      }

      // In the default case we skip the field
      indent(out) <<  "default:" << endl;
      indent(out) <<  "  $xfer += $input->skip($ftype);" << endl;
      indent(out) <<  "  break;" << endl;

      scope_down(out);

    indent(out) <<
      "$xfer += $input->readFieldEnd();" << endl;

    scope_down(out);

  indent(out) <<
    "$xfer += $input->readStructEnd();" << endl;

  if (tstruct->is_xception()) {
    const char* annotations[] = {"message", "code"};
    for (auto& annotation: annotations) {
      auto it = tstruct->annotations_.find(annotation);
      if (it != tstruct->annotations_.end()) {
        // if annotation is also used as field, ignore annotation
        if (tstruct->has_field_named(annotation)) {
          if (strcmp(annotation, it->second.c_str()) != 0) {
            fprintf(stderr, "Ignoring annotation '%s' in Hack generator because "
                    "it is already being used as a field in exception "
                    "'%s'\n", annotation, tstruct->get_name().c_str());
          }
          continue;
        }

        std::string msg = it->second;
        size_t idx = 0;

        // replace . with -> so annotation works in both C++ and php.
        while ( (idx = msg.find_first_of('.', idx)) != string::npos) {
          msg.replace(idx, 1, "->");
        }

        indent(out) << "$this->" << annotation << " = $this->" << msg << ";\n";
      }
    }
  }

  // The code that checks for the require field
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED) {
      indent(out) << "if (!$" << (*f_iter)->get_name() << "__isset) {"
        << endl;
      indent_up();
      indent(out) << "throw new \\TProtocolException(\"Required field '"
        << (*f_iter)->get_name()
        << "' was not found in serialized data! Struct: "
        << tstruct->get_name() << "\", "
        << "\\TProtocolException::MISSING_REQUIRED_FIELD);"
        << endl;
      indent_down();
      indent(out) << "}" << endl;
    }
  }

  indent(out) <<
    "return $xfer;" << endl;

  indent_down();
  out <<
    indent() << "}" << endl <<
    endl;
}

/**
 * Generates the write() method for a struct
 */
void t_hack_generator::generate_php_struct_writer(ofstream& out,
                                                 t_struct* tstruct) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  indent(out) <<
    "public function write(\\TProtocol $output): int {" << endl;
  indent_up();

  t_name_generator namer;

  indent(out) <<
    "$xfer = 0;" << endl;

  indent(out) <<
    "$xfer += $output->writeStructBegin('" << name << "');" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_type* type = get_true_type((*f_iter)->get_type());

    indent(out) << "if ($this->" << (*f_iter)->get_name() << " !== null";

    if ((*f_iter)->get_req() == t_field::T_OPTIONAL
        && (*f_iter)->get_value() != nullptr
        && !(type->is_struct() || type->is_xception())
    ) {
      out << " && $this->" << (*f_iter)->get_name() << " !== "
          << render_const_value(type, (*f_iter)->get_value());
    }

    out << ") {" << endl;

    indent_up();
    string val = namer("_val");

    if (type->is_enum()) {
      t_enum* tenum = (t_enum*) type;
      indent(out)<<
        "$" << val << " = " << hack_name(type) << "::assert($this->" <<
        (*f_iter)->get_name() << ");" << endl;
    } else {
      indent(out) << "$" << val << " = $this->" << (*f_iter)->get_name() << ";" << endl;
    }

    if (type->is_container() || type->is_struct()) {
      out <<
        indent() << "if (";
      if (strict_types_) {
        if (type->is_map()) {
          if (arrays_) {
            out << "!(is_dict($" << val << "))";
          } else {
            out << "!($" << val << " instanceof Map)";
          }
        } else if (type->is_list()) {
          if (arrays_) {
            out << "!(is_vec($" << val << "))";
          } else {
            out << "!($" << val << " instanceof Vector)";
          }
        } else if (type->is_set()) {
          if (arrays_) {
            out << "!(is_keyset($" << val << "))";
          } else if (arraysets_) {
            out <<
              "!($" << val << " instanceof \\Indexish) && " <<
              "!(($" << val << " instanceof \\Iterator || " <<
              "$" << val << " instanceof \\IteratorAggregate) " <<
              "&& $" << val << " instanceof \\Countable)";
          } else {
            out <<
              "!($" << val << " instanceof Set)";
          }
        } else {
          out <<
            "!($" << val << " instanceof " << type_to_typehint(type) << ")";
        }
      } else {
        if (type->is_set() && !arraysets_) {
          if (arrays_) {
            out << "!(is_keyset($" << val << "))";
          } else {
            out << "!($" << val << " instanceof Set)";
          }
        } else if (type->is_container()) {
          out <<
            "!($" << val << " instanceof \\Indexish) && " <<
            "!(($" << val << " instanceof \\Iterator || " <<
            "$" << val << " instanceof \\IteratorAggregate) " <<
            "&& $" << val << " instanceof \\Countable)";
        } else {
          out <<
            "!($" << val << " instanceof " << type_to_typehint(type) << ")";
        }
      }

      out <<
        ") {" << endl;
      indent_up();
      out <<
        indent() << "throw new \\TProtocolException('Bad type in structure.', \\TProtocolException::INVALID_DATA);" << endl;
      scope_down(out);
    }

    // Write field header
    indent(out) <<
      "$xfer += $output->writeFieldBegin(" <<
      "'" << (*f_iter)->get_name() << "', " <<
      type_to_enum((*f_iter)->get_type()) << ", " <<
      (*f_iter)->get_key() << ");" << endl;

    // Write field contents
    generate_serialize_field(out, namer, *f_iter, val);

    // Write field closer
    indent(out) <<
      "$xfer += $output->writeFieldEnd();" << endl;

    indent_down();
    indent(out) <<
      "}" << endl;
  }

  out <<
    indent() << "$xfer += $output->writeFieldStop();" << endl <<
    indent() << "$xfer += $output->writeStructEnd();" << endl;

  out <<
    indent() << "return $xfer;" << endl;

  indent_down();
  out <<
    indent() << "}" << endl <<
    endl;
}

/**
 * Generates a thrift service.
 *
 * @param tservice The service definition
 */
void t_hack_generator::generate_service(t_service* tservice) {
  if (mangled_services_) {
    if (php_namespace(tservice).empty()) {
      throw "cannot generate mangled services for " + tservice->get_name() +
          "; no php namespace found";
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

void t_hack_generator::generate_service(t_service* tservice, bool mangle) {
  string f_base_name = php_servicename_mangle(mangle, tservice);
  string f_service_name = get_out_dir()+f_base_name+".php";
  f_service_.open(f_service_name.c_str());
  record_genfile(f_service_name);

  f_service_ <<
    "<?hh" << (strict_ ? " // strict" : "") << endl <<
    autogen_comment() << endl;
  string hack_ns = hack_namespace(program_);
  if (!hack_ns.empty()) {
    f_service_ << "namespace " << hack_ns << ";" << endl << endl;
  }

  // Generate the main parts of the service
  generate_service_interface(tservice, mangle, true);
  generate_service_interface(tservice, mangle, false);
  if (rest_) {
    generate_service_rest(tservice, mangle);
  }
  generate_service_client(tservice, mangle);
  if (phps_) {
    generate_service_processor(tservice, mangle, true);
    generate_service_processor(tservice, mangle, false);
  }
  // Generate the structures passed around and helper functions
  generate_service_helpers(tservice);

  // Close service file
  f_service_.close();
}

/**
 * Generates event handler functions.
 */
void t_hack_generator::generate_processor_event_handler_functions(ofstream& out) {
  generate_event_handler_functions(out, "\\TProcessorEventHandler");
}
void t_hack_generator::generate_client_event_handler_functions(ofstream& out) {
  generate_event_handler_functions(out, "\\TClientEventHandler");
}
void t_hack_generator::generate_event_handler_functions(ofstream& /*out*/,
                                                        string cl) {
  f_service_ <<
    indent() << "public function setEventHandler(" << cl <<
                " $event_handler): this {" << endl <<
    indent() << "  $this->eventHandler_ = $event_handler;" << endl <<
    indent() << "  return $this;" << endl <<
    indent() << "}" << endl <<
    endl;

  indent(f_service_) <<
    "public function getEventHandler(): " << cl << " {" << endl <<
    indent() << "  return $this->eventHandler_;" << endl <<
    indent() << "}" << endl <<
    endl;
}

/**
 * Generates a service server definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_hack_generator::generate_service_processor(t_service* tservice,
        bool mangle, bool async) {
  // Generate the dispatch methods
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  string suffix = async ? "Async" : "Sync";
  string extends = "";
  string extends_processor = string("Thrift") + suffix + "Processor";
  if (tservice->get_extends() != nullptr) {
    extends = php_servicename_mangle(mangle, tservice->get_extends());
    extends_processor = extends + suffix + "ProcessorBase";
  }

  string long_name = php_servicename_mangle(mangle, tservice);

  // I hate to make this abstract, but Hack doesn't support overriding const
  // types. Thus, we will have an inheritance change that does not define the
  // const type, then branch off at each service with the processor that does
  // define the const type.

  f_service_ <<
    indent() << "abstract class " << long_name << suffix << "ProcessorBase extends " << extends_processor << " {" << endl <<
    indent() << "  abstract const type TThriftIf as " << long_name << (async ? "Async" : "") << "If;" << endl;

  indent_up();

  // Generate the process subfunctions
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    generate_process_function(tservice, *f_iter, async);
  }

  indent_down();
  f_service_ << "}" << endl;

  f_service_ <<
    indent() << "class " << long_name << suffix << "Processor extends " << long_name << suffix << "ProcessorBase {" << endl <<
    indent() << "  const type TThriftIf = " << long_name << (async ? "Async" : "") << "If;" << endl <<
    indent() << "}" << endl;

  if (!async) {
    f_service_ <<
      indent() << "// For backwards compatibility" << endl <<
      indent() << "class " << long_name << "Processor extends " << long_name << "SyncProcessor {}" << endl;
  }

  f_service_ << endl;
}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_hack_generator::generate_process_function(t_service* tservice,
                                                t_function* tfunction,
                                                bool async) {
  // Open function
  indent(f_service_) <<
    "protected" << (async ? " async" : "") << " function process_" << tfunction->get_name() <<
    "(int $seqid, \\TProtocol $input, \\TProtocol $output): " << (async ? "Awaitable<void>" : "void") << " {" << endl;
  indent_up();

  string service_name = hack_name(tservice);
  string argsname = service_name + "_" + tfunction->get_name() + "_args";
  string resultname = service_name + "_" + tfunction->get_name() + "_result";
  string fn_name = tfunction->get_name();

  f_service_ <<
    indent() << "$handler_ctx = $this->eventHandler_->getHandlerContext('"
             << fn_name << "');" << endl <<
    indent() << "$reply_type = \\TMessageType::REPLY;" << endl
             << endl <<
    indent() << "$this->eventHandler_->preRead($handler_ctx, '"
             << fn_name << "', array());" << endl
             << endl <<
    indent() << "if ($input instanceof \\TBinaryProtocolAccelerated) {" << endl <<
    indent() << "  $args = thrift_protocol_read_binary_struct("
             << "$input, '" << argsname << "');" << endl <<
    indent() << "} else if ($input instanceof \\TCompactProtocolAccelerated) {"
             << endl <<
    indent() << "  $args = thrift_protocol_read_compact_struct($input, '"
             << argsname << "');" << endl <<
    indent() << "} else {" << endl <<
    indent() << "  $args = new " << argsname << "();" << endl <<
    indent() << "  $args->read($input);" << endl <<
    indent() << "}" << endl;
  f_service_ <<
    indent() << "$input->readMessageEnd();" << endl;
  f_service_ <<
    indent() << "$this->eventHandler_->postRead($handler_ctx, '"
             << fn_name << "', $args);" << endl;

  t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;

  // Declare result for non oneway function
  if (!tfunction->is_oneway()) {
    f_service_ <<
      indent() << "$result = new " << resultname << "();" << endl;
  }

  // Try block for a function with exceptions
  f_service_ <<
    indent() << "try {" << endl;
  indent_up();

  // Generate the function call
  t_struct* arg_struct = tfunction->get_arglist();
  const std::vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator f_iter;

  indent(f_service_) << "$this->eventHandler_->preExec($handler_ctx, '"
                     << fn_name << "', $args);" << endl;

  f_service_ << indent();
  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
    f_service_ << "$result->success = ";
  }
  f_service_ << (async ? "await " : "") <<
    "$this->handler->" << tfunction->get_name() << "(";
  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      f_service_ << ", ";
    }
    f_service_ << "$args->" << (*f_iter)->get_name();
  }
  f_service_ << ");" << endl;

  if (!tfunction->is_oneway()) {
    indent(f_service_) << "$this->eventHandler_->postExec($handler_ctx, '"
                       << fn_name << "', $result);" << endl;
  }

  indent_down();
  int exc_num;
  for (exc_num = 0, x_iter = xceptions.begin();
        x_iter != xceptions.end(); ++x_iter, ++exc_num) {
    f_service_ <<
      indent() << "} catch ("
               << hack_name((*x_iter)->get_type()) << " $exc" << exc_num
               << ") {" << endl;
    if (!tfunction->is_oneway()) {
      indent_up();
      f_service_ <<
        indent() << "$this->eventHandler_->handlerException($handler_ctx, '"
                 << fn_name << "', $exc" << exc_num << ");" << endl <<
        indent() << "$result->" << (*x_iter)->get_name() << " = $exc"
                 << exc_num << ";" << endl;
      indent_down();
    }
  }
  f_service_ <<
    indent() << "} catch (Exception $ex) {" << endl <<
    indent() << "  $reply_type = \\TMessageType::EXCEPTION;" << endl <<
    indent() << "  $this->eventHandler_->handlerError($handler_ctx, '"
             << fn_name << "', $ex);" << endl <<
    indent() << "  $result = new \\TApplicationException($ex->getMessage().\"\\n\".$ex->getTraceAsString());"
             << endl <<
    indent() << "}" << endl;

  // Shortcut out here for oneway functions
  if (tfunction->is_oneway()) {
    f_service_ <<
      indent() << "return;" << endl;
    indent_down();
    f_service_ <<
      indent() << "}" << endl;
    return;
  }

  f_service_ <<
    indent() << "$this->eventHandler_->preWrite($handler_ctx, '"
             << fn_name << "', $result);" << endl;

  f_service_ <<
    indent() << "if ($output instanceof \\TBinaryProtocolAccelerated)" << endl;
  scope_up(f_service_);

  f_service_ <<
    indent() << "thrift_protocol_write_binary($output, '"
             << tfunction->get_name()
             << "', $reply_type, $result, $seqid, $output->isStrictWrite());"
             << endl;

  scope_down(f_service_);
  f_service_ <<
    indent() << "else if ($output instanceof \\TCompactProtocolAccelerated)" << endl;
  scope_up(f_service_);

  f_service_ <<
    indent() << "thrift_protocol_write_compact($output, '"
             << tfunction->get_name()
             << "', $reply_type, $result, $seqid);"
             << endl;

  scope_down(f_service_);
  f_service_ <<
    indent() << "else" << endl;
  scope_up(f_service_);

  // Serialize the request header
  f_service_ <<
    indent() << "$output->writeMessageBegin(\"" << tfunction->get_name()
             << "\", $reply_type, $seqid);" << endl <<
    indent() << "$result->write($output);" << endl <<
    indent() << "$output->writeMessageEnd();" << endl <<
    indent() << "$output->getTransport()->flush();" << endl;

  scope_down(f_service_);

  f_service_ <<
    indent() << "$this->eventHandler_->postWrite($handler_ctx, '"
             << fn_name << "', $result);" << endl;

  // Close function
  indent_down();
  f_service_ <<
    indent() << "}" << endl;
}

/**
 * Generates helper functions for a service.
 *
 * @param tservice The service to generate a header definition for
 */
void t_hack_generator::generate_service_helpers(t_service* tservice) {
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  f_service_ <<
    "// HELPER FUNCTIONS AND STRUCTURES" << endl << endl;

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* ts = (*f_iter)->get_arglist();
    string name = ts->get_name();
    ts->set_name(service_name_ + "_" + name);
    generate_php_struct_definition(f_service_, ts, false, false, true);
    generate_php_function_helpers(*f_iter);
    ts->set_name(name);
  }
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_hack_generator::generate_php_function_helpers(t_function* tfunction) {
  if (!tfunction->is_oneway()) {
    t_struct result(program_, service_name_ + "_" + tfunction->get_name() + "_result");
    t_field success(tfunction->get_returntype(), "success", 0);
    if (!tfunction->get_returntype()->is_void()) {
      result.append(&success);
    }

    t_struct* xs = tfunction->get_xceptions();
    const vector<t_field*>& fields = xs->get_members();
    vector<t_field*>::const_iterator f_iter;
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      result.append(*f_iter);
    }

    generate_php_struct_definition(f_service_, &result, false, true);
  }
}

/**
 * Generates the docstring for a generic object.
 */
void t_hack_generator::generate_php_docstring(ofstream& out,
                                             t_doc* tdoc) {
  if (tdoc->has_doc()) {
    generate_docstring_comment(out,              // out
                               "/**\n",          // comment_start
                               " * ",            // line_prefix
                               tdoc->get_doc(),  // contents
                               " */\n");         // comment_end
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
void t_hack_generator::generate_php_docstring(ofstream& out,
                                             t_function* tfunction) {
  indent(out) << "/**" << endl;
  // Copy the doc.
  if (tfunction->has_doc()) {
    generate_docstring_comment(out,                   // out
                               "",                    // comment_start
                               " * ",                 // line_prefix
                               tfunction->get_doc(),  // contents
                               "");                   // comment_end
  }

  // Also write the original thrift function definition.
  if (tfunction->has_doc()) {
    indent(out) << " * " << endl;
  }
  indent(out) << " * " << "Original thrift definition:-" << endl;
  // Return type.
  indent(out) << " * ";
  if (tfunction->is_oneway()) {
    out << "oneway ";
  }
  out << thrift_type_name(tfunction->get_returntype()) << endl;

  // Function name.
  indent(out) << " * " << indent(1) << tfunction->get_name() << "(";
  // Find the position after the " * " from where the function arguments should
  // be rendered.
  int start_pos = get_indent_size() + tfunction->get_name().size() + 1;

  // Parameters.
  generate_php_docstring_args(out, start_pos, tfunction->get_arglist());
  out << ")";

  // Exceptions.
  t_struct* xceptions = tfunction->get_xceptions();
  if (xceptions && xceptions->get_members().size()) {
    out << endl << indent() << " * " << indent(1) << "throws (";
    // Find the position after the " * " from where the exceptions should be
    // rendered.
    start_pos = get_indent_size() + strlen("throws (");
    generate_php_docstring_args(out, start_pos, xceptions);
    out << ")";
  }
  out << ";" << endl;
  indent(out) << " */" << endl;
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
void t_hack_generator::generate_php_docstring(ofstream& out, t_field* tfield) {
  indent(out) << "/**" << endl;
  // Copy the doc.
  if (tfield->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tfield->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * " << endl;
  }
  indent(out) << " * "
              << "Original thrift field:-" << endl;
  indent(out) << " * " << tfield->get_key() << ": "
              << tfield->get_type()->get_full_name() << " "
              << tfield->get_name() << endl;
  indent(out) << " */" << endl;
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
    t_struct* tstruct,
    bool is_exception) {
  indent(out) << "/**" << endl;
  // Copy the doc.
  if (tstruct->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tstruct->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * " << endl;
  }
  indent(out) << " * "
              << "Original thrift " << (is_exception ? "exception" : "struct")
              << ":-" << endl;
  indent(out) << " * " << tstruct->get_name() << endl;
  indent(out) << " */" << endl;
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
void t_hack_generator::generate_php_docstring(ofstream& out, t_enum* tenum) {
  indent(out) << "/**" << endl;
  // Copy the doc.
  if (tenum->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tenum->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * " << endl;
  }
  indent(out) << " * "
              << "Original thrift enum:-" << endl;
  indent(out) << " * " << tenum->get_name() << endl;
  indent(out) << " */" << endl;
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
    t_service* tservice) {
  indent(out) << "/**" << endl;
  // Copy the doc.
  if (tservice->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tservice->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * " << endl;
  }
  indent(out) << " * "
              << "Original thrift service:-" << endl;
  indent(out) << " * " << tservice->get_name() << endl;
  indent(out) << " */" << endl;
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
void t_hack_generator::generate_php_docstring(ofstream& out, t_const* tconst) {
  indent(out) << "/**" << endl;
  // Copy the doc.
  if (tconst->has_doc()) {
    generate_docstring_comment(
        out, // out
        "", // comment_start
        " * ", // line_prefix
        tconst->get_doc(), // contents
        ""); // comment_end
    indent(out) << " * " << endl;
  }
  indent(out) << " * "
              << "Original thrift constant:-" << endl;
  indent(out) << " * " << tconst->get_type()->get_full_name() << " "
              << tconst->get_name() << endl;
  // no value because it could have characters that mess up the comment
  indent(out) << " */" << endl;
}

/**
 * Generates the docstring for function arguments and exceptions.
 *
 * @param int start_pos the position (after " * ") from which the rendering of
 * arguments should start. In other words, we put that many space after " * "
 * and then render the argument.
 */
void t_hack_generator::generate_php_docstring_args(ofstream& out,
                                                  int start_pos,
                                                  t_struct* arg_list) {
  if (arg_list) {
    vector<t_field*> params = arg_list->get_members();
    if (params.size()) {
      vector<t_field*>::const_iterator p_iter;
      for (p_iter = params.begin();
           p_iter != params.end();
           ++p_iter) {
        t_field* p = *p_iter;
        if (p_iter != params.begin()) {  // If not first argument
          out << "," << endl
              << indent() << " * " << std::string(start_pos, ' ');
        }
        out << p->get_key() << ": " << thrift_type_name(p->get_type())
            << " " << p->get_name();
      }
    }
  }
}

/**
 * Generate an appropriate string for a php typehint
 */
string t_hack_generator::type_to_typehint(t_type* ttype, bool nullable, bool shape) {
  if (ttype->is_base_type()) {
    switch (((t_base_type*)ttype)->get_base()) {
      case t_base_type::TYPE_VOID:
        return "void";
      case t_base_type::TYPE_STRING:
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
  } else if (ttype->is_typedef()) {
    return type_to_typehint(((t_typedef*) ttype)->get_type(), nullable, shape);
  } else if (ttype->is_enum()) {
    if (is_bitmask_enum((t_enum*) ttype)) {
      return "int";
    } else {
      return (nullable ? "?" : "") + hack_name(ttype);
    }
  } else if (ttype->is_struct() || ttype->is_xception()) {
    return (nullable ? "?" : "") + hack_name(ttype) + (shape ? "::TShape" : "");
  } else if (ttype->is_list()) {
    string prefix = arrays_ ? "vec" : (shape ? "array" : "Vector");
    return prefix + "<" + type_to_typehint(((t_list*)ttype)->get_elem_type(), false, shape)  + ">";
  } else if (ttype->is_map()) {
    string prefix = arrays_ ? "dict" : (shape ? "array" : "Map");
    string key_type =  type_to_typehint(((t_map*)ttype)->get_key_type(), nullable, shape);
    if (shape &&
        shape_arraykeys_ &&
        key_type == "string") {
      key_type = "arraykey";
    }
    return prefix + "<" + key_type + ", "  + type_to_typehint(((t_map*)ttype)->get_val_type(), false, shape) + ">";
  } else if (ttype->is_set()) {
    string prefix = arraysets_ ? "array" : (arrays_ ? "keyset" : (shape ? "array" : "Set"));
    string suffix = (arraysets_ || (shape && !arrays_)) ? ", bool>" : ">";
    return prefix + "<" + type_to_typehint(((t_set*)ttype)->get_elem_type(), false, shape) + suffix;
  } else {
    return "mixed";
  }
}

/**
 * Generate an appropriate string for a parameter typehint.
 * The difference from type_to_typehint() is for parameters we should accept an
 * array or a collection type, so we return Indexish
 */
string t_hack_generator::type_to_param_typehint(t_type* ttype, bool nullable) {
  if (ttype->is_list()) {
    if (strict_types_) {
      return type_to_typehint(ttype, nullable);
    } else {
      return "\\Indexish<int, " + type_to_param_typehint(((t_list*)ttype)->get_elem_type()) + ">";
    }
  } else if (ttype->is_map()) {
    if (strict_types_) {
      return type_to_typehint(ttype, nullable);
    } else {
      return "\\Indexish<" + type_to_param_typehint(((t_map*)ttype)->get_key_type())
        + ", " + type_to_param_typehint(((t_map*)ttype)->get_val_type()) + ">";
    }
  } else {
    return type_to_typehint(ttype, nullable);
  }
}

/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 * @param mangle Generate mangled service classes
 */
void t_hack_generator::generate_service_interface(t_service* tservice,
        bool mangle, bool async) {
  generate_php_docstring(f_service_, tservice);
  string suffix = async ? "Async" : "";
  string extends_if = string("\\IThrift") + (async ? "Async" : "Sync") + "If";
  if (tservice->get_extends() != nullptr) {
    string ext_prefix = php_servicename_mangle(mangle, tservice->get_extends());
    extends_if = ext_prefix + suffix + "If";
  }
  string long_name = php_servicename_mangle(mangle, tservice);
  f_service_ <<
    "interface " << long_name << suffix << "If extends " << extends_if << " {" << endl;
  indent_up();
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    // Add a blank line before the start of a new function definition
    if (f_iter != functions.begin()) {
      f_service_ << endl;
    }

    // Add the doxygen style comments.
    generate_php_docstring(f_service_, *f_iter);

    // Finally, the function declaration.
    string return_typehint = type_to_typehint((*f_iter)->get_returntype());
    if (async) {
      return_typehint = "Awaitable<" + return_typehint + ">";
    }
    indent(f_service_) <<
      "public function " << function_signature(*f_iter, "", "", return_typehint) << ";" << endl;
  }
  indent_down();
  f_service_ <<
    "}" << endl << endl;
}

/**
 * Generates a REST interface
 */
void t_hack_generator::generate_service_rest(t_service* tservice, bool mangle) {
  string extends = "";
  string extends_if = "";
  if (tservice->get_extends() != nullptr) {
    string ext_prefix = php_servicename_mangle(mangle, tservice->get_extends());
    extends = " extends " + ext_prefix;
    extends_if = " extends " + ext_prefix + "Rest";
  }
  string long_name = php_servicename_mangle(mangle, tservice);
  f_service_ <<
    "class " << long_name << "Rest" << extends_if << " {" << endl;
  indent_up();

  if (extends.empty()) {
    f_service_ <<
      indent() << "protected " << long_name << "If $impl_;" << endl <<
      endl;
  }

  f_service_ <<
    indent() << "public function __construct(" << long_name
             << "If $impl) {" << endl;
  f_service_ <<
    indent() << "  $this->impl_ = $impl;" << endl <<
    indent() << "}" << endl <<
    endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(f_service_) <<
      "public function " << (*f_iter)->get_name() << "(\\Indexish<string, mixed> $request): " << type_to_typehint((*f_iter)->get_returntype()) << " {" << endl;
    indent_up();
    indent(f_service_) << "// UNSAFE_BLOCK $request is not type safe :(, and we don't cast structs (yet)" << endl;
    const vector<t_field*>& args = (*f_iter)->get_arglist()->get_members();
    vector<t_field*>::const_iterator a_iter;
    for (a_iter = args.begin(); a_iter != args.end(); ++a_iter) {
      t_type* atype = get_true_type((*a_iter)->get_type());
      string cast = type_to_cast(atype);
      string req = "$request['" + (*a_iter)->get_name() + "']";
      if (atype->is_bool()) {
        f_service_ <<
          indent() << "$" << (*a_iter)->get_name() << " = " << cast
                   << "(!empty(" << req << ") && (" << req
                   << " !== 'false'));" << endl;
      } else {
        f_service_ <<
          indent() << "$" << (*a_iter)->get_name() << " = " << cast << " idx($request, '" << (*a_iter)->get_name() << "');" << endl;
      }
      if (atype->is_string() &&
          ((t_base_type*)atype)->is_string_list()) {
        f_service_ <<
          indent() << "$" << (*a_iter)->get_name() << " = explode(',', $"
                   << (*a_iter)->get_name() << ");" << endl;
      } else if (atype->is_container()) {
        f_service_ <<
          indent() << "$" << (*a_iter)->get_name() << " = json_decode($"
                   << (*a_iter)->get_name() << ", true);" << endl;
      } else if (atype->is_struct() || atype->is_xception()) {
        f_service_ <<
          indent() << "if ($" << (*a_iter)->get_name() << " !== null) {"
                   << endl <<
          indent() << "  $" << (*a_iter)->get_name() << " = new "
                   << hack_name(atype)
                   << "(json_decode($" << (*a_iter)->get_name() << ", true));"
                   << endl <<
          indent() << "}" << endl;
      }
    }

    f_service_ << indent();
    if (!(*f_iter)->get_returntype()->is_void()) {
      f_service_ << "return ";
    }
    f_service_ <<
      "$this->impl_->" << (*f_iter)->get_name() <<
      "(" << argument_list((*f_iter)->get_arglist(), "", false) << ");" << endl;

    indent_down();
    indent(f_service_) <<
      "}" << endl <<
      endl;
  }
  indent_down();
  f_service_ <<
    "}" << endl << endl;
}

void t_hack_generator::generate_service_client(t_service* tservice,
        bool mangle) {
  _generate_service_client(f_service_, tservice, mangle);
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_hack_generator::_generate_service_client(
        ofstream& out, t_service* tservice, bool mangle) {
  generate_php_docstring(out, tservice);

  string long_name = php_servicename_mangle(mangle, tservice);
  out << "trait " << long_name << "ClientBase {" << endl
      << "  require extends ThriftClientBase;" << endl
      << endl;
  indent_up();

  // Generate client method implementations
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arg_struct = (*f_iter)->get_arglist();
    const vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator fld_iter;
    string funname = (*f_iter)->get_name();

    indent(out) <<
      "protected function sendImpl_" << function_signature(*f_iter, "", "", "int") << " {" << endl;
    indent_up();

    std::string argsname =
        hack_name(tservice) + "_" + (*f_iter)->get_name() + "_args";

    out << indent() << "$currentseqid = $this->getNextSequenceID();" << endl
        << indent() << "$args = new " << argsname << "();" << endl;

    // Loop through the fields and assign to the args struct
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      out << indent() << "$args->" << (*fld_iter)->get_name() << " = ";
      t_type* t = (*fld_iter)->get_type();
      if (!strict_types_ && t->is_container() && !t->is_set()) {
        // If !strict_types, containers are typehinted as Indexish<Key, Value>
        // to better support passing in arrays/dicts/maps/vecs/vectors and
        // handle backwards compatibility. However, structs are typehinted as
        // the actual container (ex: Map<Key, Val>), and we need to safely
        // convert the typehints.
        //
        // This isn't as simple as dict($param) or new Map($param). If there is
        // a nested container, that also needs to have its typehints converted.
        // This iterates through the type object and generates the appropriate
        // code to convert all the nested typehints.
        //
        // sets are treated somewhat specially in this code, since we are
        // guaranteed sets in Hack cannot contain complex structures, and are
        // not typed as Indexish at any point.
        if (arrays_) {
          t_type* val_type;
          std::string var = "$" + (*fld_iter)->get_name();
          t_name_generator namer;
          int close_parens = 0;
          while (t->is_container()) {
            if (t->is_map()) {
              val_type = ((t_map*)t)->get_val_type();
            } else if (t->is_list()) {
              val_type = ((t_list*)t)->get_elem_type();
            } else if (t->is_set()) {
              val_type = ((t_set*)t)->get_elem_type();
            } else {
              throw std::runtime_error("Unknown container type");
            }
            val_type = get_true_type(val_type);
            if (val_type->is_container()) {
              if (t->is_map()) {
                out << "ThriftUtil::mapDict(" << var << ", ";
              } else if (t->is_list()) {
                out << "ThriftUtil::mapVec(" << var << ", ";
              } else if (t->is_set()) {
                out << "ThriftUtil::mapKeyset(" << var << ", ";
              } else {
                throw std::runtime_error("Unknown container type");
              }
              var = "$"+namer("_val");
              out << var << " ==> ";
              close_parens++;
            } else {
              if (t->is_map()) {
                out << "dict(" << var << ")";
              } else if (t->is_list()) {
                out << "vec(" << var << ")";
              } else if (t->is_set()) {
                out << "keyset(" << var << ")";
              } else {
                throw std::runtime_error("Unknown container type");
              }
            }
            t = val_type;
          }
          for (int i = 0; i < close_parens; i++) {
            out << ")";
          }
          out << ";" << endl;
        } else {
          t_type* val_type;
          if (t->is_map()) {
            out << "(new Map($" << (*fld_iter)->get_name() << "))";
            val_type = ((t_map*)t)->get_val_type();
          } else {
            out << "(new Vector($" << (*fld_iter)->get_name() << "))";
            val_type = ((t_list*)t)->get_elem_type();
          }
          val_type = get_true_type(val_type);
          int nest = 0;
          t_name_generator namer;
          while (val_type->is_container() && !val_type->is_set()) {
            nest++;
            string val = namer("_val");
            indent_up();
            out << "->map(" << endl << indent() << "$" << val << " ==> ";
            if (val_type->is_map()) {
              out << "(new Map($" << val << "))";
              val_type = ((t_map*)val_type)->get_val_type();
            } else {
              out << "(new Vector($" << val << "))";
              val_type = ((t_list*)val_type)->get_elem_type();
            }
            val_type = get_true_type(val_type);
          }
          for (int i = nest; i > 0; i--) {
            indent_down();
            out << endl << indent() << ")";
          }
          out << ";" << endl;
        }
      } else {
        out << "$" << (*fld_iter)->get_name() << ";" << endl;
      }
    }

    out << indent() << "try {" << endl;
    indent_up();
    out <<
      indent() << "$this->eventHandler_->preSend('" << (*f_iter)->get_name() <<
                  "', $args, $currentseqid);" << endl;
    out <<
      indent() << "if ($this->output_ instanceof \\TBinaryProtocolAccelerated)" << endl;
    scope_up(out);

    out <<
      indent() << "thrift_protocol_write_binary($this->output_, '" <<
      (*f_iter)->get_name() << "', " <<
      "\\TMessageType::CALL, $args, $currentseqid, " <<
      "$this->output_->isStrictWrite(), " <<
      ((*f_iter)->is_oneway() ? "true" : "false") << ");" << endl;

    scope_down(out);
    out <<
      indent() << "else if ($this->output_ instanceof \\TCompactProtocolAccelerated)" << endl;
    scope_up(out);

    out <<
      indent() << "thrift_protocol_write_compact($this->output_, '" <<
      (*f_iter)->get_name() << "', " <<
      "\\TMessageType::CALL, $args, $currentseqid, " <<
      ((*f_iter)->is_oneway() ? "true" : "false") << ");" << endl;

    scope_down(out);
    out <<
      indent() << "else" << endl;
    scope_up(out);

    // Serialize the request header
    out <<
      indent() << "$this->output_->writeMessageBegin('" <<
      (*f_iter)->get_name() <<
      "', \\TMessageType::CALL, $currentseqid);" << endl;

    // Write to the stream
    out <<
      indent() << "$args->write($this->output_);" << endl <<
      indent() << "$this->output_->writeMessageEnd();" << endl;
    if ((*f_iter)->is_oneway()) {
      out <<
        indent() << "$this->output_->getTransport()->onewayFlush();" << endl;
    } else {
      out <<
        indent() << "$this->output_->getTransport()->flush();" << endl;
    }

    scope_down(out);

    indent_down();
    indent(out) << "} catch (\\THandlerShortCircuitException $ex) {" << endl;
    indent_up();
    out <<
      indent() << "switch ($ex->resultType) {" << endl <<
      indent() << "  case \\THandlerShortCircuitException::R_EXPECTED_EX:" << endl <<
      indent() << "  case \\THandlerShortCircuitException::R_UNEXPECTED_EX:" << endl <<
      indent() << "    $this->eventHandler_->sendError('" << (*f_iter)->get_name() <<
                  "', $args, $currentseqid, $ex->result);" << endl <<
      indent() << "    throw $ex->result;" << endl <<
      indent() << "  case \\THandlerShortCircuitException::R_SUCCESS:" << endl <<
      indent() << "  default:" << endl <<
      indent() << "    $this->eventHandler_->postSend('" << (*f_iter)->get_name() <<
                  "', $args, $currentseqid);" << endl <<
      indent() << "    return $currentseqid;" << endl <<
      indent() << "}" << endl;
    indent_down();
    indent(out) << "} catch (Exception $ex) {" << endl;
    indent_up();
    out <<
      indent() << "$this->eventHandler_->sendError('" << (*f_iter)->get_name() <<
                  "', $args, $currentseqid, $ex);" << endl <<
      indent() << "throw $ex;" << endl;
    indent_down();
    indent(out) << "}" << endl;

    out <<
      indent() << "$this->eventHandler_->postSend('" << (*f_iter)->get_name() <<
                  "', $args, $currentseqid);" << endl;

    indent(out) << "return $currentseqid;" << endl;

    scope_down(out);

    if (!(*f_iter)->is_oneway()) {
      std::string resultname =
          hack_name(tservice) + "_" + (*f_iter)->get_name() + "_result";
      t_struct noargs(program_);

      t_function recv_function((*f_iter)->get_returntype(),
                               string("recvImpl_") + (*f_iter)->get_name(),
                               &noargs);
      string return_typehint = type_to_typehint((*f_iter)->get_returntype());
      // Open function
      out <<
        endl <<
        indent() << "protected function " <<
        function_signature(&recv_function, "", "?int $expectedsequenceid = null", return_typehint) << " {" <<
        endl;
      indent_up();

      out <<
        indent() << "try {" << endl;
      indent_up();

      out <<
        indent() << "$this->eventHandler_->preRecv('" << (*f_iter)->get_name() <<
                    "', $expectedsequenceid);" << endl;

      out <<
        indent() << "if ($this->input_ instanceof \\TBinaryProtocolAccelerated) {" << endl;

      indent_up();

      out <<
        indent() << "$result = thrift_protocol_read_binary("
                 << "$this->input_, '" << resultname
                 << "', $this->input_->isStrictRead());" << endl;

      indent_down();

      out <<
        indent() << "} else if ($this->input_ instanceof \\TCompactProtocolAccelerated)" << endl;
      scope_up(out);
      out <<
        indent() << "$result = thrift_protocol_read_compact($this->input_, '"
                 << resultname << "');" << endl;
      scope_down(out);

      out <<
        indent() << "else" << endl;
      scope_up(out);

      out <<
        indent() << "$rseqid = 0;" << endl <<
        indent() << "$fname = '';" << endl <<
        indent() << "$mtype = 0;" << endl <<
        endl;

      out <<
        indent() << "$this->input_->readMessageBegin($fname, $mtype, "
                 << "$rseqid);" << endl <<
        indent() << "if ($mtype == \\TMessageType::EXCEPTION) {" << endl <<
        indent() << "  $x = new \\TApplicationException();" << endl <<
        indent() << "  $x->read($this->input_);" << endl <<
        indent() << "  $this->input_->readMessageEnd();" << endl <<
        indent() << "  throw $x;" << endl <<
        indent() << "}" << endl;

      out <<
        indent() << "$result = new " << resultname << "();" << endl <<
        indent() << "$result->read($this->input_);" << endl;

      out <<
        indent() << "$this->input_->readMessageEnd();" << endl;

      out <<
        indent() <<
        "if ($expectedsequenceid !== null && ($rseqid != $expectedsequenceid)) {"
        << endl <<
        indent() << "  throw new \\TProtocolException(\"" <<
        (*f_iter)->get_name() <<
        " failed: sequence id is out of order\");" << endl <<
        indent() << "}" << endl;

      scope_down(out);
      indent_down();
      indent(out) << "} catch (\\THandlerShortCircuitException $ex) {" << endl;
      indent_up();
      out <<
        indent() << "switch ($ex->resultType) {" << endl <<
        indent() << "  case \\THandlerShortCircuitException::R_EXPECTED_EX:" << endl <<
        indent() << "    $this->eventHandler_->recvException('" << (*f_iter)->get_name() <<
                    "', $expectedsequenceid, $ex->result);" << endl <<
        indent() << "    throw $ex->result;" << endl <<
        indent() << "  case \\THandlerShortCircuitException::R_UNEXPECTED_EX:" << endl <<
        indent() << "    $this->eventHandler_->recvError('" << (*f_iter)->get_name() <<
                    "', $expectedsequenceid, $ex->result);" << endl <<
        indent() << "    throw $ex->result;" << endl <<
        indent() << "  case \\THandlerShortCircuitException::R_SUCCESS:" << endl <<
        indent() << "  default:" << endl <<
        indent() << "    $this->eventHandler_->postRecv('" << (*f_iter)->get_name() <<
                    "', $expectedsequenceid, $ex->result);" << endl <<
        indent() << "    return";

      if (!(*f_iter)->get_returntype()->is_void()) {
        out << " $ex->result";
      }
      out <<
        ";" << endl <<
        indent() << "}" << endl;
      indent_down();
      out <<
        indent() << "} catch (Exception $ex) {" << endl;
      indent_up();
      out <<
        indent() << "$this->eventHandler_->recvError('" << (*f_iter)->get_name() <<
                    "', $expectedsequenceid, $ex);" << endl <<
        indent() << "throw $ex;" << endl;
      indent_down();
      out <<
        indent() << "}" << endl;

      // Careful, only return result if not a void function
      if (!(*f_iter)->get_returntype()->is_void()) {
        out <<
          indent() << "if ($result->success !== null) {" << endl <<
          indent() << "  $success = $result->success;" << endl <<
          indent() << "  $this->eventHandler_->postRecv('" << (*f_iter)->get_name() <<
                      "', $expectedsequenceid, $success);" << endl <<
          indent() << "  return $success;" << endl <<
          indent() << "}" << endl;
      }

      t_struct* xs = (*f_iter)->get_xceptions();
      const std::vector<t_field*>& xceptions = xs->get_members();
      vector<t_field*>::const_iterator x_iter;
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        out <<
          indent() << "if ($result->" << (*x_iter)->get_name() << " !== null) {"
                   << endl <<
          indent() << "  $x = $result->" << (*x_iter)->get_name() << ";" << endl <<
          indent() << "  $this->eventHandler_->recvException('" << (*f_iter)->get_name() <<
                      "', $expectedsequenceid, $x);" << endl <<
          indent() << "  throw $x;"
                   << endl <<
          indent() << "}" << endl;
      }

      // Careful, only return _result if not a void function
      if ((*f_iter)->get_returntype()->is_void()) {
        indent(out) <<
          indent() << "  $this->eventHandler_->postRecv('" << (*f_iter)->get_name() <<
                      "', $expectedsequenceid, null);" << endl <<
          "return;" << endl;
      } else {
        out <<
          indent()
            << "$x = new \\TApplicationException(\""
            << (*f_iter)->get_name() << " failed: unknown result\""
            << ", \\TApplicationException::MISSING_RESULT"
            << ");" << endl <<
          indent() << "$this->eventHandler_->recvError('" << (*f_iter)->get_name() <<
                      "', $expectedsequenceid, $x);" << endl <<
          indent() << "throw $x;" << endl;
      }

      // Close function
      scope_down(out);
    }

    out << endl;
  }

  scope_down(out);
  out << endl;

  _generate_service_client_children(out, tservice, mangle, /*async*/ true);
  _generate_service_client_children(out, tservice, mangle, /*async*/ false);
}

void t_hack_generator::_generate_service_client_children(
        ofstream& out, t_service* tservice, bool mangle, bool async) {
  string long_name = php_servicename_mangle(mangle, tservice);
  string suffix = (async ? "Async" : "");
  string extends = "ThriftClientBase";
  bool root = tservice->get_extends() == nullptr;
  bool first = true;
  if (!root) {
    extends = php_servicename_mangle(mangle, tservice->get_extends()) + suffix + "Client";
  }

  out << "class " << long_name << suffix << "Client extends " << extends << " implements " << long_name << suffix <<"If {" << endl
      << "  use " << long_name << "ClientBase;" << endl
      << endl;
  indent_up();

  t_struct noargs(program_);

  // Generate client method implementations
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arg_struct = (*f_iter)->get_arglist();
    const vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator fld_iter;
    string funname = (*f_iter)->get_name();
    string return_typehint = type_to_typehint((*f_iter)->get_returntype());

    if (!async) {
      // Non-Async function
      indent(out) << "<<__Deprecated('use gen_" << funname << "()')>>" << endl;
      indent(out) <<
        "public function " << function_signature(*f_iter) << " {" << endl;
      indent_up();
        indent(out) <<
          "$currentseqid = $this->sendImpl_" << funname << "(";

        first = true;
        for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
          if (first) {
            first = false;
          } else {
            out << ", ";
          }
          out << "$" << (*fld_iter)->get_name();
        }
        out << ");" << endl;

        if (!(*f_iter)->is_oneway()) {
          out << indent();
          if (!(*f_iter)->get_returntype()->is_void()) {
            out << "return ";
          }
          out <<
            "$this->recvImpl_" << funname << "($currentseqid);" << endl;
        }
      scope_down(out);
      out << endl;
    }

    // Async function
    generate_php_docstring(out, *f_iter);
    indent(out)
      << "public async function "
      << function_signature(
          *f_iter,
          async ? "" : "gen_",
          "",
          "Awaitable<" + return_typehint + ">")
      << " {" << endl;
    indent_up();
    indent(out) <<
      "$currentseqid = $this->sendImpl_" << funname << "(";

    first = true;
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      out << "$" << (*fld_iter)->get_name();
    }
    out << ");" << endl;

    if (!(*f_iter)->is_oneway()) {
      indent(out) << "await $this->asyncHandler_->genWait($currentseqid);" << endl;
      out << indent();
      if (!(*f_iter)->get_returntype()->is_void()) {
        out << "return ";
      }
      out <<
        "$this->recvImpl_" << funname << "($currentseqid);" << endl;
    }
    scope_down(out);
    out << endl;
  }

  if (!async) {
    out <<
      indent() << "/* send and recv functions */" << endl;

    for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arg_struct = (*f_iter)->get_arglist();
      const vector<t_field*>& fields = arg_struct->get_members();
      vector<t_field*>::const_iterator fld_iter;
      string funname = (*f_iter)->get_name();
      string return_typehint = type_to_typehint((*f_iter)->get_returntype());

      out <<
        indent() << "public function send_" << function_signature(*f_iter, "", "", "int") << " {" << endl <<
        indent() << "  return $this->sendImpl_" << funname << "(";
      first = true;
      for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
        if (first) {
          first = false;
        } else {
          out << ", ";
        }
        out << "$" << (*fld_iter)->get_name();
      }
      out <<
        ");" << endl <<
        indent() << "}" << endl;
      if (!(*f_iter)->is_oneway()) {
        t_function recv_function((*f_iter)->get_returntype(),
                                 string("recv_") + (*f_iter)->get_name(),
                                 &noargs);
        // Open function
        bool is_void = (*f_iter)->get_returntype()->is_void();
        out <<
          indent() << "public function " <<
            function_signature(&recv_function, "", "?int $expectedsequenceid = null", return_typehint) << " {" << endl <<
          indent() << "  " << (is_void ? "" : "return ") << "$this->recvImpl_" << funname << "($expectedsequenceid);" << endl <<
          indent() << "}" << endl;
      }
    }
  }

  indent_down();
  out <<
    "}" << endl << endl;
}

/**
 * Deserializes a field of any type.
 */
void t_hack_generator::generate_deserialize_field(ofstream& out,
                                                  t_name_generator& namer,
                                                  t_field* tfield,
                                                  string name,
                                                  bool declared) {
  t_type* type = get_true_type(tfield->get_type());
  if (name == "") {
    name = tfield->get_name();
  }

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " + name;
  }

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out, (t_struct*)type, name);
  } else {

    if (type->is_container()) {
      generate_deserialize_container(out, namer, type, name);
    } else if (type->is_base_type() || type->is_enum()) {

      if (type->is_base_type()) {
        if (!declared) {
          indent(out) << "$" << name << " = null;" << endl;
        }
        indent(out) << "$xfer += $input->";

        t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
        switch (tbase) {
        case t_base_type::TYPE_VOID:
          throw "compiler error: cannot serialize void field in a struct: " +
            name;
        case t_base_type::TYPE_STRING:
          out << "readString($" << name << ");";
          break;
        case t_base_type::TYPE_BOOL:
          out << "readBool($" << name << ");";
          break;
        case t_base_type::TYPE_BYTE:
          out << "readByte($" << name << ");";
          break;
        case t_base_type::TYPE_I16:
          out << "readI16($" << name << ");";
          break;
        case t_base_type::TYPE_I32:
          out << "readI32($" << name << ");";
          break;
        case t_base_type::TYPE_I64:
          out << "readI64($" << name << ");";
          break;
        case t_base_type::TYPE_DOUBLE:
          out << "readDouble($" << name << ");";
          break;
        case t_base_type::TYPE_FLOAT:
          out << "readFloat($" << name << ");";
          break;
        default:
          throw "compiler error: no PHP name for base type " + t_base_type::t_base_name(tbase);
        }
      } else if (type->is_enum()) {
        t_enum* tenum = (t_enum*) type;

        string val = namer("_val");
        out <<
          indent() << "$" << val << " = null;" << endl <<
          indent() << "$xfer += $input->readI32($" << val << ");" << endl <<
          indent() << "$" << name << " = " << hack_name(tenum);
        if (tfield->get_req() == t_field::T_REQUIRED) {
          out << "::assert(";
        } else {
          out << "::coerce(";
        }
        out << "$" << val << ");" << endl;
      }
      out << endl;
    } else {
      printf("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
             name.c_str(), type->get_name().c_str());
    }
  }
}

/**
 * Generates an unserializer for a variable. This makes two key assumptions,
 * first that there is a const char* variable named data that points to the
 * buffer for deserialization, and that there is a variable protocol which
 * is a reference to a TProtocol serialization object.
 */
void t_hack_generator::generate_deserialize_struct(ofstream& out,
                                                  t_struct* tstruct,
                                                  string prefix) {
  out << indent() << "$" << prefix << " = new " << hack_name(tstruct) << "();"
      << endl
      << indent() << "$xfer += $" << prefix << "->read($input);" << endl;
}

void t_hack_generator::generate_deserialize_container(ofstream& out,
                                                      t_name_generator& namer,
                                                      t_type* ttype,
                                                      string prefix) {
  string val = namer("_val");
  string size = namer("_size");
  string ktype = namer("_ktype");
  string vtype = namer("_vtype");
  string etype = namer("_etype");

  t_field fsize(g_type_i32, size);
  t_field fktype(g_type_byte, ktype);
  t_field fvtype(g_type_byte, vtype);
  t_field fetype(g_type_byte, etype);

  out <<
    indent() << "$" << size << " = 0;" << endl;

  // Declare variables, read header
  if (ttype->is_map()) {
    if (arrays_) {
      out << indent() << "$" << val << " = dict[];" << endl;
    } else {
      out << indent() << "$" << val << " = Map {};" << endl;
    }
    out <<
      indent() << "$" << ktype << " = 0;" << endl <<
      indent() << "$" << vtype << " = 0;" << endl;
    out <<
      indent() << "$xfer += $input->readMapBegin(" <<
      "$" << ktype << ", $" << vtype << ", $" << size << ");" << endl;
  } else if (ttype->is_set()) {
    out << indent() << "$" << etype << " = 0;" << endl;
    if (arrays_) {
      out << indent() << "$" << val << " = keyset[];" << endl;
    } else if (arraysets_) {
      out << indent() << "$" << val << " = array();" << endl;
    } else {
      out << indent() << "$" << val << " = Set{};" << endl;
    }
    out <<
      indent() << "$xfer += $input->readSetBegin(" <<
      "$" << etype << ", $" << size << ");" << endl;
  } else if (ttype->is_list()) {
    if (arrays_) {
      out << indent() << "$" << val << " = vec[];" << endl;
    } else {
      out << indent() << "$" << val << " = Vector {};" << endl;
    }
    out <<
      indent() << "$" << etype << " = 0;" << endl <<
      indent() << "$xfer += $input->readListBegin(" <<
      "$" << etype << ", $" << size << ");" << endl;
  }

  // For loop iterates over elements
  string i = namer("_i");
  indent(out) <<
    "for ($" <<
    i << " = 0; $" << size << " === null || $" << i << " < $" << size << "; ++$" << i << ")" << endl;

    scope_up(out);

    if (ttype->is_map()) {
      generate_deserialize_map_element(out, namer, (t_map*)ttype, size, val);

    } else if (ttype->is_set()) {
      generate_deserialize_set_element(out, namer, (t_set*)ttype, size, val);
    } else if (ttype->is_list()) {
      generate_deserialize_list_element(out, namer, (t_list*)ttype, size, val);
    }

    scope_down(out);

  // Read container end
  if (ttype->is_map()) {
    indent(out) << "$xfer += $input->readMapEnd();" << endl;
  } else if (ttype->is_set()) {
    indent(out) << "$xfer += $input->readSetEnd();" << endl;
  } else if (ttype->is_list()) {
    indent(out) << "$xfer += $input->readListEnd();" << endl;
  }

  out <<
    indent() << "$" << prefix << " = $" << val << ";" << endl;
}


/**
 * Generates code to deserialize a map
 */
void t_hack_generator::generate_deserialize_map_element(ofstream& out,
                                                        t_name_generator& namer,
                                                        t_map* tmap,
                                                        string size,
                                                        string prefix) {
  string key = namer("key");
  string val = namer("val");
  t_field fkey(tmap->get_key_type(), key);
  t_field fval(tmap->get_val_type(), val);

  out <<
    indent() << "if ($" << size << " === null && !$input->readMapHasNext()) {" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl;

  generate_deserialize_field(out, namer, &fkey);
  generate_deserialize_field(out, namer, &fval);

  indent(out) <<
    "if ($" << key << " !== null && $" << val << " !== null) {" << endl;
  indent_up();
  indent(out) <<
    "$" << prefix << "[$" << key << "] = $" << val << ";" << endl;
  indent_down();
  indent(out) <<
    "}" << endl;
}

void t_hack_generator::generate_deserialize_set_element(ofstream& out,
                                                        t_name_generator& namer,
                                                        t_set* tset,
                                                        string size,
                                                        string prefix) {
  string elem = namer("elem");
  t_field felem(tset->get_elem_type(), elem);

  out <<
    indent() << "if ($" << size << " === null && !$input->readSetHasNext()) {" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl;

  generate_deserialize_field(out, namer, &felem);

  indent(out) <<
    "if ($" << elem << " !== null) {" << endl;
  indent_up();
  if (arrays_) {
    indent(out) <<
      "$" << prefix << "[] = $" << elem << ";" << endl;
  } else if (arraysets_) {
    indent(out) <<
      "$" << prefix << "[$" << elem << "] = true;" << endl;
  } else {
    indent(out) <<
      "$" << prefix << "->add($" << elem << ");" << endl;
  }
  indent_down();
  indent(out) <<
    "}" << endl;
}

void t_hack_generator::generate_deserialize_list_element(
    ofstream& out,
    t_name_generator& namer,
    t_list* tlist,
    string size,
    string prefix) {
  string elem = namer("elem");
  t_field felem(tlist->get_elem_type(), elem);

  out <<
    indent() << "if ($" << size << " === null && !$input->readListHasNext()) {" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl;

  generate_deserialize_field(out, namer, &felem);

  indent(out) <<
    "if ($" << elem << " !== null) {" << endl;
  indent_up();
  indent(out) <<
    "$" << prefix << " []= $" << elem << ";" << endl;
  indent_down();
  indent(out) <<
    "}" << endl;
}


/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_hack_generator::generate_serialize_field(ofstream& out,
                                                t_name_generator& namer,
                                                t_field* tfield,
                                                string name) {
  t_type* type = get_true_type(tfield->get_type());
  if (name == "") {
    name = tfield->get_name();
  }

  // Do nothing for void types
  if (type->is_void()) {
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " + name;
  }

  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct(out, namer, (t_struct*)type, name);
  } else if (type->is_container()) {
    generate_serialize_container(out, namer, type, name);
  } else if (type->is_base_type() || type->is_enum()) {

    indent(out) <<
      "$xfer += $output->";

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw
          "compiler error: cannot serialize void field in a struct: " + name;
      case t_base_type::TYPE_STRING:
        out << "writeString($" << name << ");";
        break;
      case t_base_type::TYPE_BOOL:
        out << "writeBool($" << name << ");";
        break;
      case t_base_type::TYPE_BYTE:
        out << "writeByte($" << name << ");";
        break;
      case t_base_type::TYPE_I16:
        out << "writeI16($" << name << ");";
        break;
      case t_base_type::TYPE_I32:
        out << "writeI32($" << name << ");";
        break;
      case t_base_type::TYPE_I64:
        out << "writeI64($" << name << ");";
        break;
      case t_base_type::TYPE_DOUBLE:
        out << "writeDouble($" << name << ");";
        break;
      case t_base_type::TYPE_FLOAT:
        out << "writeFloat($" << name << ");";
        break;
      default:
        throw "compiler error: no PHP name for base type " + t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      out << "writeI32($" << name << ");";
    }
    out << endl;
  } else {
    printf("DO NOT KNOW HOW TO SERIALIZE FIELD '%s' TYPE '%s'\n",
           name.c_str(),
           type->get_name().c_str());
  }
}

/**
 * Serializes all the members of a struct.
 *
 * @param tstruct The struct to serialize
 * @param prefix  String prefix to attach to all fields
 */
void t_hack_generator::generate_serialize_struct(
    ofstream& out,
    t_name_generator& /* namer */,
    t_struct* /* tstruct */,
    string prefix) {
  indent(out) <<
    "$xfer += $" << prefix << "->write($output);" << endl;
}

/**
 * Writes out a container
 */
void t_hack_generator::generate_serialize_container(ofstream& out,
                                                    t_name_generator& namer,
                                                    t_type* ttype,
                                                    string prefix) {
  if (ttype->is_map()) {
    indent(out) <<
      "$output->writeMapBegin(" <<
      type_to_enum(((t_map*)ttype)->get_key_type()) << ", " <<
      type_to_enum(((t_map*)ttype)->get_val_type()) << ", " <<
      "count($" << prefix << "));" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "$output->writeSetBegin(" <<
      type_to_enum(((t_set*)ttype)->get_elem_type()) << ", " <<
      "count($" << prefix << "));" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "$output->writeListBegin(" <<
      type_to_enum(((t_list*)ttype)->get_elem_type()) << ", " <<
      "count($" << prefix << "));" << endl;
  }

  indent(out) <<
    "if ($" << prefix << " !== null)" << endl;

  scope_up(out);

  if (ttype->is_map()) {
    string kiter = namer("kiter");
    string viter = namer("viter");
    indent(out) <<
      "foreach ($" << prefix << " as " <<
      "$" << kiter << " => $" << viter << ")" << endl;
    scope_up(out);
    generate_serialize_map_element(out, namer, (t_map*)ttype, kiter, viter);
      scope_down(out);
    } else if (ttype->is_set()) {
      string iter = namer("iter");
      if (arraysets_) {
        indent(out) <<
          "foreach ($" << prefix << " as $" << iter << " => $true)" << endl;
      } else {
        indent(out) <<
          "foreach ($" << prefix << " as $" << iter << ")" << endl;
      }
      scope_up(out);
      generate_serialize_set_element(out, namer, (t_set*)ttype, iter);
    scope_down(out);
  } else if (ttype->is_list()) {
    string iter = namer("iter");
    indent(out) <<
      "foreach ($" << prefix << " as $" << iter << ")" << endl;
    scope_up(out);
    generate_serialize_list_element(out, namer, (t_list*)ttype, iter);
    scope_down(out);
  }

  scope_down(out);

  if (ttype->is_map()) {
    indent(out) <<
      "$output->writeMapEnd();" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "$output->writeSetEnd();" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "$output->writeListEnd();" << endl;
  }
}

/**
 * Serializes the members of a map.
 *
 */
void t_hack_generator::generate_serialize_map_element(ofstream& out,
                                                      t_name_generator& namer,
                                                      t_map* tmap,
                                                      string kiter,
                                                      string viter) {
  t_field kfield(tmap->get_key_type(), kiter);
  generate_serialize_field(out, namer, &kfield, "");

  t_field vfield(tmap->get_val_type(), viter);
  generate_serialize_field(out, namer, &vfield, "");
}

/**
 * Serializes the members of a set.
 */
void t_hack_generator::generate_serialize_set_element(ofstream& out,
                                                      t_name_generator& namer,
                                                      t_set* tset,
                                                      string iter) {
  t_field efield(tset->get_elem_type(), iter);
  generate_serialize_field(out, namer, &efield);
}

/**
 * Serializes the members of a list.
 */
void t_hack_generator::generate_serialize_list_element(ofstream& out,
                                                       t_name_generator& namer,
                                                       t_list* tlist,
                                                       string iter) {
  t_field efield(tlist->get_elem_type(), iter);
  generate_serialize_field(out, namer, &efield);
}

/**
 * Declares a field, which may include initialization as necessary.
 *
 * @param ttype The type
 * @param init iff initialize the field
 * @param obj iff the field is an object
 * @param thrift iff the object is a thrift object
 */
string t_hack_generator::declare_field(t_field* tfield, bool init,
                                       bool obj, bool /*thrift*/) {
  string result = "$" + tfield->get_name();
  if (init) {
    t_type* type = get_true_type(tfield->get_type());
    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        break;
      case t_base_type::TYPE_STRING:
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
        throw "compiler error: no Hack initializer for base type "
              + t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      result += " = null";
    } else if (type->is_map()) {
      if (arrays_) {
        result += " = dict[]";
      } else {
        result += " = Map {}";
      }
    } else if (type->is_list()) {
      if (arrays_) {
        result += " = vec[]";
      } else {
        result += " = Vector {}";
      }
    } else if (type->is_set()) {
      if (arrays_) {
        result += " = keyset[]";
      } else if (arraysets_) {
        result += " = array()";
      } else {
        result += " = Set {}";
      }
    } else if (type->is_struct() || type->is_xception()) {
      if (obj) {
        result += " = new " + hack_name(type) + "()";
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
string t_hack_generator::function_signature(t_function* tfunction,
                                           string prefix,
                                           string moreparameters,
                                           string typehint) {
  if (typehint.empty()) {
    typehint = type_to_typehint(tfunction->get_returntype());
  }

  return
    prefix + tfunction->get_name()
    + "(" + argument_list(tfunction->get_arglist(), moreparameters) + "): "
    + typehint;
}

/**
 * Renders a field list
 */
string t_hack_generator::argument_list(t_struct* tstruct,
                                      string moreparameters,
                                      bool typehints) {
  string result = "";

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }
    if (typehints) {
      // If a field is not sent to a thrift server, the value is null :(
      t_type* ftype = (*f_iter)->get_type();
      bool nullable = !no_nullables_
        || (ftype->is_enum() && (
              (*f_iter)->get_value() == nullptr
           || (*f_iter)->get_req() != t_field::T_REQUIRED
          )
        );
      result += type_to_param_typehint((*f_iter)->get_type(), nullable) + " ";
    }
    result += "$" + (*f_iter)->get_name();
  }

  if (moreparameters.length() > 0) {
    if (!first) {
      result += ", ";
    }
    result += moreparameters;
  }
  return result;
}

/**
 * Gets a typecast string for a particular type.
 */
string t_hack_generator::type_to_cast(t_type* type) {
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
string t_hack_generator ::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
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

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

THRIFT_REGISTER_GENERATOR(hack, "HACK",
"    server:          Generate Hack server stubs.\n"
"    rest:            Generate Hack REST processors.\n"
"    oldenum:         Generate enums with $__names and $__values fields.\n"
"    json:            Generate functions to parse JSON into thrift struct.\n"
"    mangledsvcs      Generate services with namespace mangling.\n"
"    stricttypes      Use Collection classes everywhere rather than Indexish.\n"
"    strict           Generate strict hack header.\n"
"    arraysets        Use legacy arrays for sets rather than objects.\n"
"    nonullables      Instantiate struct fields within structs, rather than nullable\n"
"    mapconstruct     Struct constructors accept arrays/Maps rather than their fields\n"
"    structtrait         Add 'use [StructName]Trait;' to generated classes\n"
"    shapes           Generate Shape definitions for structs\n"
"    shape_arraykeys  When generating Shape definition for structs:\n"
"                        replace array<string, TValue> with array<arraykey, TValue>\n"
"    shape_unsafe_json When converting json to Shapes, do not validate.\n"
"    lazy_constants   Generate lazy initialization code for global constants.\n"
"    arrays           Use Hack arrays for maps/lists/sets instead of objects.\n"
);
