/*
 * Copyright 2014 Facebook, Inc.
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
#include "thrift/compiler/generate/t_oop_generator.h"
#include "thrift/compiler/platform.h"
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
      const std::string& option_string)
    : t_oop_generator(program)
  {
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

    mangled_services_ = option_is_set(parsed_options, "mangledsvcs", false);

    out_dir_base_ = "gen-hack";
  }

  /**
   * Init and close methods
   */

  void init_generator();
  void close_generator();

  /**
   * Program-level generation functions
   */

  void generate_typedef  (t_typedef*  ttypedef);
  void generate_enum     (t_enum*     tenum);
  void generate_const    (t_const*    tconst);
  void generate_struct   (t_struct*   tstruct);
  void generate_xception (t_struct*   txception);
  void generate_service  (t_service*  tservice);

  std::string render_const_value(t_type* type, t_const_value* value);
  std::string render_default_value(t_type* type);

  /**
   * Structs!
   */

  void generate_php_struct(t_struct* tstruct, bool is_exception);
  void generate_php_struct_definition(std::ofstream& out, t_struct* tstruct,
                                        bool is_xception=false,
                                        bool is_result = false);
  void _generate_php_struct_definition(std::ofstream& out, t_struct* tstruct,
                                        bool is_xception=false,
                                        bool is_result = false);
  void generate_php_struct_reader(std::ofstream& out, t_struct* tstruct);
  void generate_php_struct_writer(std::ofstream& out, t_struct* tstruct);
  void generate_php_function_helpers(t_function* tfunction);

  void generate_php_union_enum(std::ofstream& out, t_struct* tstruct);
  void generate_php_union_methods(std::ofstream& out, t_struct* tstruct);

  void generate_php_type_spec(std::ofstream &out, t_type* t);
  void generate_php_struct_spec(std::ofstream &out, t_struct* tstruct);
  void generate_php_struct_struct_trait(std::ofstream &out, t_struct* tstruct);
  void generate_php_structural_id(std::ofstream &out, t_struct* tstruct);

  /**
   * Service-level generation functions
   */

  void generate_service           (t_service* tservice, bool mangle);
  void generate_service_helpers   (t_service* tservice);
  void generate_service_interface (t_service* tservice, bool mangle);
  void generate_service_rest      (t_service* tservice, bool mangle);
  void generate_service_client    (t_service* tservice, bool mangle);
  void _generate_service_client   (std::ofstream &out, t_service* tservice,
                                        bool mangle);
  void generate_service_processor (t_service* tservice, bool mangle);
  void generate_process_function  (t_service* tservice, t_function* tfunction);
  void generate_processor_event_handler_functions (std::ofstream& out);
  void generate_client_event_handler_functions (std::ofstream& out);
  void generate_event_handler_functions (std::ofstream& out, string cl);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field        (std::ofstream &out,
                                          t_field*    tfield,
                                          std::string name="",
                                          bool inclass=false);

  void generate_deserialize_struct       (std::ofstream &out,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_deserialize_container    (std::ofstream &out,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_deserialize_set_element  (std::ofstream &out,
                                          t_set*      tset,
                                          std::string size,
                                          std::string prefix="");

  void generate_deserialize_map_element  (std::ofstream &out,
                                          t_map*      tmap,
                                          std::string size,
                                          std::string prefix="");

  void generate_deserialize_list_element (std::ofstream &out,
                                          t_list*     tlist,
                                          std::string size,
                                          std::string prefix="");

  void generate_serialize_field          (std::ofstream &out,
                                          t_field*    tfield,
                                          std::string name="");

  void generate_serialize_struct         (std::ofstream &out,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_serialize_container      (std::ofstream &out,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_serialize_map_element    (std::ofstream &out,
                                          t_map*      tmap,
                                          std::string kiter,
                                          std::string viter);

  void generate_serialize_set_element    (std::ofstream &out,
                                          t_set*      tmap,
                                          std::string iter);

  void generate_serialize_list_element   (std::ofstream &out,
                                          t_list*     tlist,
                                          std::string iter);
  /**
   * Read thrift object from JSON string, generated using the
   * TSimpleJSONProtocol.
   */

  void generate_json_enum            (std::ofstream& out, t_enum* tenum,
                                      const string& prefix_thrift,
                                      const string& prefix_json);

  void generate_json_struct          (std::ofstream& out, t_struct* tstruct,
                                      const string& prefix_thrift,
                                      const string& prefix_json);

  void generate_json_field           (std::ofstream& out, t_field* tfield,
                                      const string& prefix_thrift = "",
                                      const string& suffix_thrift = "",
                                      const string& prefix_json = "");

  void generate_json_container       (std::ofstream& out,
                                      t_type* ttype,
                                      const string& prefix_thrift = "",
                                      const string& prefix_json = "");

  void generate_json_set_element     (std::ofstream& out,
                                      t_set* tset,
                                      const string& value,
                                      const string& prefix_thrift);

  void generate_json_list_element    (ofstream& out,
                                      t_list* list,
                                      const string& value,
                                      const string& prefix_thrift);

  void generate_json_map_element     (std::ofstream& out,
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
  void generate_php_docstring(ofstream& out, t_function* tfunction);
  void generate_php_docstring_args(ofstream& out,
                                   int start_pos,
                                   t_struct* arg_list);

  std::string type_to_typehint(t_type* ttype, bool nullable=false);
  std::string type_to_param_typehint(t_type* ttype, bool nullable=false);

  std::string union_enum_name(t_struct* tstruct) {
    // <StructName>Type
    return php_namespace(tstruct->get_program()) +
      tstruct->get_name() + "Enum";
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

  std::string php_namespace(t_program* p) {
    std::string ns = p->get_namespace("php");
    return ns.size() ? (ns + "_") : "";
  }

  std::string php_namespace(t_service* s) {
    return php_namespace(s->get_program());
  }

  std::string php_path(t_program* p) {
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

 private:

  /**
   * Generate a tmp php variable name started by '$'
   */
  std::string php_tmp(std::string pre) {
    return "$" + tmp(pre);
  }

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
};


void t_hack_generator::generate_json_enum(std::ofstream& out, t_enum* tenum,
                                         const string& prefix_thrift,
                                         const string& prefix_json) {
  indent(out) << prefix_thrift << " = " << php_namespace(tenum->get_program())
              << tenum->get_name() << "::coerce(" << prefix_json << ");";
}

void t_hack_generator::generate_json_struct(ofstream& out, t_struct* tstruct,
                                           const string& prefix_thrift,
                                           const string& prefix_json) {

  string enc = php_tmp("_tmp");
  indent(out) << enc << " = " << "json_encode(" << prefix_json
              << ");" << endl;
  string tmp = php_tmp("_tmp");
  t_field felem(tstruct, tmp);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << endl;
  indent(out) << tmp << "->readFromJson(" << enc << ");" << endl;
  indent(out) << prefix_thrift << " = " << tmp << ";" << endl;
}

void t_hack_generator::generate_json_field(ofstream& out,
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
    generate_json_struct(out,
        (t_struct*)type,
        name,
        prefix_json);
  } else if (type->is_container()) {
    generate_json_container(out,
        (t_container*)type,
        name,
        prefix_json);
  } else if (type->is_enum()) {
    generate_json_enum(out,
        static_cast<t_enum*>(type),
        name,
        prefix_json);
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
      string temp = php_tmp("_tmp");
      indent(out) << temp << " = (int)" << prefix_json << ";" << endl;
      indent(out) << "if (" << temp << " > " << number_limit << ") {" <<endl;
      indent_up();
      indent(out) << "throw new TProtocolException(\"number exceeds "
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
                                              t_type* ttype,
                                              const string& prefix_thrift,
                                              const string& prefix_json) {
  t_container* tcontainer = (t_container*)ttype;
  string size = php_tmp("_size");
  string key = php_tmp("_key");
  string value = php_tmp("_value");
  string json = php_tmp("_json");
  string container = php_tmp("_container");

  indent(out) << json << " = " << prefix_json << ";" << endl;
  if (ttype->is_map()) {
    indent(out) << container << " = Map {};" << endl;
  } else if (ttype->is_list()) {
    indent(out) << container << " = Vector {};" << endl;
  } else if (ttype->is_set()) {
    if (arraysets_) {
      indent(out) << container << " = array();" << endl;
    } else {
      indent(out) << container << " = Set {};" << endl;
    }
  }
  indent(out) << "foreach(" << json << " as " << key << " => " << value
              << ") {" << endl;
  indent_up();

  if (ttype->is_list()) {
    generate_json_list_element(out, (t_list*)ttype, value, container);
  } else if (ttype->is_set()) {
    generate_json_set_element(out, (t_set*)ttype, value, container);
  } else if (ttype->is_map()) {
    generate_json_map_element(out, (t_map*)ttype, key, value, container);
  } else {
    throw "compiler error: no PHP reader for this type.";
  }
  indent_down();
  indent(out) << "}" << endl;
  indent(out) << prefix_thrift << " = " << container << ";" << endl;
}

void t_hack_generator::generate_json_list_element(ofstream& out,
                                                 t_list* tlist,
                                                 const string& value,
                                                 const string& prefix_thrift) {
  string elem = php_tmp("_elem");
  t_field felem(tlist->get_elem_type(), elem);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << endl;
  generate_json_field(out, &felem, "", "", value);
  indent(out) << prefix_thrift << " []= " << elem << ";" << endl;

}
void t_hack_generator::generate_json_set_element(std::ofstream& out,
                                               t_set* tset,
                                               const string& value,
                                               const string& prefix_thrift) {
  string elem = php_tmp("_elem");
  t_field felem(tset->get_elem_type(), elem);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << endl;
  generate_json_field(out, &felem, "", "", value);
  if (arraysets_) {
    indent(out) << prefix_thrift << "[" << elem << "] = true;" << endl;
  } else {
    indent(out) << prefix_thrift << "->add(" << elem << ");" << endl;
  }
}
void t_hack_generator::generate_json_map_element(std::ofstream& out,
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
  string _value = php_tmp("_value");
  t_field vfelem(tmap->get_val_type(), _value);
  indent(out) << declare_field(&vfelem, true, true, true).substr(1) << endl;
  generate_json_field(out, &vfelem, "", "", value);
  indent(out) << prefix_thrift << "[" << key << "] = " << _value << ";" << endl;
}

void t_hack_generator::generate_json_reader(ofstream &out,
                                           t_struct* tstruct) {
  if(!json_) {
    return;
  }
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  string name = tstruct->get_name();
  indent(out) << "public function readFromJson(string $jsonText): void {" << endl;
  indent_up();
  indent(out) << "$parsed = json_decode($jsonText, true);" << endl << endl;

  indent(out) << "if ($parsed === null || !is_array($parsed)) {" << endl;
  indent_up();
  indent(out) << "throw new TProtocolException(\"Cannot parse the given json"
              << " string.\");" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field *tf = *f_iter;
    indent(out) << "if (isset($parsed['" << tf->get_name() << "'])) {" << endl;
    indent_up();
    generate_json_field(out, tf, "$this->", "",
                        "$parsed['" + tf->get_name() + "']");
    indent_down();
    indent(out) << "}";
    if(tf->get_req() == t_field::T_REQUIRED) {
      out << " else {" << endl;
      indent_up();
      indent(out) << "throw new TProtocolException(\"Required field "
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
  MKDIR(get_out_dir().c_str());

  // Make output file
  string f_types_name = get_out_dir()+program_name_+"_types.php";
  f_types_.open(f_types_name.c_str());
  record_genfile(f_types_name);

  // Print header
  f_types_ <<
    "<?hh" << (strict_ ? " // strict" : "") << endl <<
    autogen_comment() << endl;

  // Print header
  if (!program_->get_consts().empty()) {
    string f_consts_name = get_out_dir()+program_name_+"_constants.php";
    f_consts_.open(f_consts_name.c_str());
    record_genfile(f_consts_name);
    f_consts_ <<
      "<?hh" << (strict_ ? " // strict" : "") << endl <<
      autogen_comment();
    constants_values_.clear();
    string const_namespace = php_namespace(program_);
    if (const_namespace != "") {
      f_consts_ <<
        "class " << const_namespace << "CONSTANTS {" << endl;
    } else {
      f_consts_ <<
        "class " << program_name_ << "_CONSTANTS {" << endl;
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
    indent(f_consts_) << "public static array $__values = array(" << endl;
    std::copy(constants_values_.begin(), constants_values_.end(),
        std::ostream_iterator<string>(f_consts_, ",\n"));
    indent(f_consts_) << ");" << endl;
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
void t_hack_generator::generate_typedef(t_typedef* ttypedef) {}

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
  if (is_bitmask_enum(tenum)) {
    typehint = "int";
    f_types_ <<
      "final class " <<
        php_namespace(tenum->get_program()) <<
        tenum->get_name() << " extends Flags {" <<
        endl;
  } else {
    typehint = php_namespace(tenum->get_program()) + tenum->get_name() + "Type";
    f_types_ <<
      "newtype " << typehint << " = int;" << endl <<
      "final class " <<
        php_namespace(tenum->get_program()) <<
        tenum->get_name() << " extends Enum<" <<
        typehint << "> {" << endl;
  }

  indent_up();

  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int32_t value = (*c_iter)->get_value();

    generate_php_docstring(f_types_, *c_iter);
    indent(f_types_) <<
      "const " << typehint << " " << (*c_iter)->get_name() << " = " << value << ";" << endl;
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
  f_types_ << "}" << endl << endl;
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
  // for base php types, use const (guarantees optimization in hphp)
  if (type->is_base_type()) {
    indent(f_consts_) << "const " << name << " = ";
  // cannot use const for objects (incl arrays). use static
  } else {
    indent(f_consts_) << "public static " << type_to_typehint(type) << " $" << name << " = ";
  }
  indent_up();
  f_consts_ << render_const_value(type, value);
  indent_down();
  f_consts_ << ";" << endl;
  // add the definitions to a values array as well
  // indent up cause we're going to be in an array definition
  indent_up();
  stringstream oss(stringstream::out);
  indent(oss) << "'" << name << "' => ";
  indent_up();
  oss << render_const_value(type, value);
  indent_down();
  indent_down();
  constants_values_.push_back(oss.str());
  indent_down();
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
string t_hack_generator::render_const_value(t_type* type, t_const_value* value) {
  std::ostringstream out;
  type = get_true_type(type);
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      out << "'" << value->get_string() << "'";
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
      if (out.str().find(".") == string::npos) {
        out << ".0";
      }
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    t_enum* tenum = (t_enum*) type;
    const t_enum_value* val = tenum->find_value(value->get_integer());

    out << php_namespace(tenum->get_program()) << tenum->get_name()
      << "::" << val->get_name();
  } else if (type->is_struct() || type->is_xception()) {
    out << "new " << php_namespace(type->get_program()) << type->get_name() << "(Map {" << endl;
    indent_up();
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
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
      out << indent();
      out << render_const_value(g_type_string, v_iter->first);
      out << " => ";
      out << render_const_value(field_type, v_iter->second);
      out << "," << endl;
    }
    indent_down();
    indent(out) << "})";
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    out << "Map {" << endl;
    indent_up();
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(ktype, v_iter->first);
      out << " => ";
      out << render_const_value(vtype, v_iter->second);
      out << "," << endl;
    }
    indent_down();
    indent(out) << "}";
  } else if (type->is_list()) {
    t_type* etype = ((t_list*)type)->get_elem_type();
    out << "Vector {" << endl;
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(etype, *v_iter);
      out << "," << endl;
    }
    indent_down();
    indent(out) << "}";
  } else if (type->is_set()) {
    t_type* etype = ((t_set*)type)->get_elem_type();
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    if (arraysets_) {
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
      dval = "new " + php_namespace(tstruct->get_program())
              + tstruct->get_name() + "()";
    } else {
      dval = "null";
    }
  } else if (type->is_map()) {
    dval = "Map {}";
  } else if (type->is_list()) {
    dval = "Vector {}";
  } else if (type->is_set()) {
    if (arraysets_) {
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
    indent(out) << "'enum' => '" << php_namespace(tenum->get_program())
                << tenum->get_name() << "'," << endl;
  } else if (t->is_struct() || t->is_xception()) {
    indent(out) << "'class' => '" << php_namespace(t->get_program()) << t->get_name() <<"'," << endl;
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
    indent(out) << "'format' => 'collection'," << endl;
    indent_down();
  } else if (t->is_list()) {
    t_type* etype = get_true_type(((t_list*)t)->get_elem_type());
    indent(out) << "'etype' => " << type_to_enum(etype) <<"," << endl;
    indent(out) << "'elem' => array(" << endl;
    indent_up();
    generate_php_type_spec(out, etype);
    indent(out) << ")," << endl;
    indent(out) << "'format' => 'collection'," << endl;
    indent_down();
  } else if (t->is_set()) {
    t_type* etype = get_true_type(((t_set*)t)->get_elem_type());
    indent(out) << "'etype' => " << type_to_enum(etype) <<"," << endl;
    indent(out) << "'elem' => array(" << endl;
    indent_up();
    generate_php_type_spec(out, etype);
    indent(out) << ")," << endl;
    if (arraysets_) {
      indent(out) << "'format' => 'array'," << endl;
    } else {
      // TODO(ckwalsh) Re-add this flag
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
      traitName = tstruct->get_name() + "Trait";
    } else {
      traitName = it->second;
    }
  } else if (struct_trait_) {
    traitName = tstruct->get_name() + "Trait";
  }

  if (!traitName.empty()) {
    indent(out) << "use " << php_namespace(tstruct->get_program())
                << traitName << ";" << endl << endl;
  }
}

/**
 * Generates the structural ID definition, see generate_structural_id()
 * for information about the structural ID.
 */
void t_hack_generator::generate_php_structural_id(ofstream& out,
                                                  t_struct* tstruct) {
  indent(out) << "const int STRUCTURAL_ID = "
              << generate_structural_id(tstruct->get_members()) << ";"
              << endl;
}

void t_hack_generator::generate_php_struct_definition(ofstream& out,
                                                     t_struct* tstruct,
                                                     bool is_exception,
                                                     bool is_result) {
  if (tstruct->is_union()) {
    // Generate enum for union before the actual class
    generate_php_union_enum(out, tstruct);
  }
  _generate_php_struct_definition(out, tstruct, is_exception, is_result);
}

void t_hack_generator::generate_php_union_methods(ofstream& out,
                                                  t_struct* tstruct) {
  vector<t_field*>::const_iterator m_iter;

  // getType() : <UnionName>Enum {}
  indent(out) << "public function getType(): "
      << union_enum_name(tstruct) << " {" << endl;
  indent(out) << indent() << "return $this->_type;"<< endl;
  indent(out) << "}" << endl << endl;

  // TODO: Generate missing methods such as {set|get}_<field_name>(), clear()
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
  int enum_index = 1;

  out << "enum " << union_enum_name(tstruct) << ": int {"<< endl;

  indent_up();
  // If no member is set
  indent(out) <<  UNION_EMPTY << " = 0;" << endl;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out)
        << (*m_iter)->get_name() << " = "<< enum_index++ << ";" << endl;
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
void t_hack_generator::_generate_php_struct_definition(ofstream& out,
                                                     t_struct* tstruct,
                                                     bool is_exception,
                                                     bool is_result) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  generate_php_docstring(out, tstruct);
  out <<
    "class " << php_namespace(tstruct->get_program()) << tstruct->get_name();
  if (is_exception) {
    out << " extends TException";
  }
  out <<
    " implements IThriftStruct {" << endl;
  indent_up();

  generate_php_struct_struct_trait(out, tstruct);

  generate_php_struct_spec(out, tstruct);

  generate_php_structural_id(out, tstruct);

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
    bool nullable = (dval == "null")
        || tstruct->is_union()
        || is_result
        || ((*m_iter)->get_req() == t_field::T_OPTIONAL
            && (*m_iter)->get_value() == nullptr)
        || (t->is_enum()
            && (*m_iter)->get_req() != t_field::T_REQUIRED);
    string typehint = nullable ? "?" : "";

    typehint += type_to_typehint(t);

    indent(out) <<
      "public " << typehint << " $" << (*m_iter)->get_name() << ";" << endl;
  }

  if (tstruct->is_union()) {
    // Generate _type to store which field is set and initialize it to _EMPTY_
    indent(out) <<
      "private " << union_enum_name(tstruct) << " $_type = "
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
        indent() << "public function __construct(@Indexish<string, mixed> $vals = array()) {" << endl;
    }
    out <<
      indent() << "  // UNSAFE $vals is not type safe :(, and we don't cast structs (yet)" << endl;
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
    "public function read(TProtocol $input): int {" << endl;
  indent_up();

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
      "if ($ftype == TType::STOP) {" << endl;
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
        generate_deserialize_field(out, *f_iter, "this->" + (*f_iter)->get_name());
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
      indent(out) << "throw new TProtocolException(\"Required field '"
        << (*f_iter)->get_name()
        << "' was not found in serialized data! Struct: "
        << tstruct->get_name() << "\", "
        << "TProtocolException::MISSING_REQUIRED_FIELD);"
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
    "public function write(TProtocol $output): int {" << endl;
  indent_up();

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
    string val = tmp("_val");

    if (type->is_enum()) {
      t_enum* tenum = (t_enum*) type;
      indent(out)<<
        "$" << val << " = " << php_namespace(tenum->get_program()) <<
        tenum->get_name() << "::assert($this->" <<
        (*f_iter)->get_name() << ");" << endl;
    } else {
      indent(out) << "$" << val << " = $this->" << (*f_iter)->get_name() << ";" << endl;
    }

    if (type->is_container() || type->is_struct()) {
      out <<
        indent() << "if (";
      if (strict_types_) {
        if (type->is_map()) {
          out <<
            "!($" << val << " instanceof Map)";
        } else if (type->is_list()) {
          out <<
            "!($" << val << " instanceof Vector)";
        } else if (type->is_set()) {
          if (arraysets_) {
            out <<
              "!($" << val << " instanceof Indexish) && " <<
              "!(($" << val << " instanceof Iterator || " <<
              "$" << val << " instanceof IteratorAggregate) " <<
              "&& $" << val << " instanceof Countable)";
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
          out <<
            "!($" << val << " instanceof Set)";
        } else if (type->is_container()) {
          out <<
            "!($" << val << " instanceof Indexish) && " <<
            "!(($" << val << " instanceof Iterator || " <<
            "$" << val << " instanceof IteratorAggregate) " <<
            "&& $" << val << " instanceof Countable)";
        } else {
          out <<
            "!($" << val << " instanceof " << type_to_typehint(type) << ")";
        }
      }

      out <<
        ") {" << endl;
      indent_up();
      out <<
        indent() << "throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);" << endl;
      scope_down(out);
    }

    // Write field header
    indent(out) <<
      "$xfer += $output->writeFieldBegin(" <<
      "'" << (*f_iter)->get_name() << "', " <<
      type_to_enum((*f_iter)->get_type()) << ", " <<
      (*f_iter)->get_key() << ");" << endl;

    // Write field contents
    generate_serialize_field(out, *f_iter, val);

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

  // Generate the main parts of the service
  generate_service_interface(tservice, mangle);
  if (rest_) {
    generate_service_rest(tservice, mangle);
  }
  generate_service_client(tservice, mangle);
  if (phps_) {
    generate_service_processor(tservice, mangle);
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
  generate_event_handler_functions(out, "TProcessorEventHandler");
}
void t_hack_generator::generate_client_event_handler_functions(ofstream& out) {
  generate_event_handler_functions(out, "TClientEventHandler");
}
void t_hack_generator::generate_event_handler_functions(ofstream& out, string cl) {
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
        bool mangle) {
  // Generate the dispatch methods
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  string extends = "";
  string extends_processor = "";
  if (tservice->get_extends() != nullptr) {
    extends = php_servicename_mangle(mangle, tservice->get_extends());
    extends_processor = " extends " + extends + "Processor";
  }

  string long_name = php_servicename_mangle(mangle, tservice);
  // Generate the header portion
  f_service_ <<
    "class " << long_name << "Processor" << extends_processor << " implements IThriftProcessor {" << endl;
  indent_up();

  if (extends.empty()) {
    f_service_ <<
      indent() << "protected TProcessorEventHandler $eventHandler_;" << endl <<
      endl <<
      indent() << "// This exists so subclasses still using php can still access the handler" << endl <<
      indent() << "// Once the migration to hack is complete, this field can be removed safely" << endl <<
      indent() << "protected $handler_;" << endl <<
      endl;
  }

  f_service_ <<
    indent() << "private " << long_name << "If $_handler;" << endl;

  f_service_ <<
    indent() << "public function __construct(" << long_name
             << "If $handler) {" << endl;

  indent_up();

  if (extends.empty()) {
    f_service_ <<
      indent() << "$this->eventHandler_ = new TProcessorEventHandler();" << endl <<
      indent() << "$this->handler_ = $handler;" << endl;
  } else {
    f_service_ <<
      indent() << "parent::__construct($handler);" << endl;
  }
  f_service_ <<
    indent() << "$this->_handler = $handler;" << endl;

  indent_down();

  f_service_ <<
    indent() << "}" << endl <<
    endl;

  // Generate processor event handler functions
  generate_processor_event_handler_functions(f_service_);

  // Generate the server implementation
  indent(f_service_) <<
    "public function process(TProtocol $input, TProtocol $output): bool {" << endl;
  indent_up();

  f_service_ <<
    indent() << "$rseqid = 0;" << endl <<
    indent() << "$fname = '';" << endl <<
    indent() << "$mtype = 0;" << endl <<
    endl;

  f_service_ <<
    indent() << "$input->readMessageBegin($fname, $mtype, $rseqid);" << endl;

  // HOT: check for method implementation
  f_service_ <<
    indent() << "$methodname = 'process_'.$fname;" << endl <<
    indent() << "if (!method_exists($this, $methodname)) {" << endl;
  f_service_ <<
    indent() << "  $input->skip(TType::STRUCT);" << endl <<
    indent() << "  $input->readMessageEnd();" << endl <<
    indent() << "  $x = new TApplicationException('Function '.$fname.' not implemented.', TApplicationException::UNKNOWN_METHOD);" << endl <<
    indent() << "  $output->writeMessageBegin($fname, TMessageType::EXCEPTION, $rseqid);" << endl <<
    indent() << "  $x->write($output);" << endl <<
    indent() << "  $output->writeMessageEnd();" << endl <<
    indent() << "  $output->getTransport()->flush();" << endl <<
    indent() << "  return true;" << endl;
  f_service_ <<
    indent() << "}" << endl <<
    indent() << "$this->$methodname($rseqid, $input, $output);" << endl <<
    indent() << "return true;" << endl;
  indent_down();
  f_service_ <<
    indent() << "}" << endl <<
    endl;

  // Generate the process subfunctions
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    generate_process_function(tservice, *f_iter);
  }

  indent_down();
  f_service_ << "}" << endl;
}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_hack_generator::generate_process_function(t_service* tservice,
                                                t_function* tfunction) {
  // Open function
  indent(f_service_) <<
    "protected function process_" << tfunction->get_name() <<
    "(int $seqid, TProtocol $input, TProtocol $output): void {" << endl;
  indent_up();

  string argsname = php_namespace(tservice->get_program()) + service_name_ + "_" + tfunction->get_name() + "_args";
  string resultname = php_namespace(tservice->get_program()) + service_name_ + "_" + tfunction->get_name() + "_result";
  string fn_name = tfunction->get_name();

  f_service_ <<
    indent() << "$handler_ctx = $this->eventHandler_->getHandlerContext('"
             << fn_name << "');" << endl <<
    indent() << "$reply_type = TMessageType::REPLY;" << endl
             << endl <<
    indent() << "$this->eventHandler_->preRead($handler_ctx, '"
             << fn_name << "', array());" << endl
             << endl <<
    indent() << "if ($input instanceof TBinaryProtocolAccelerated) {" << endl <<
    indent() << "  $args = thrift_protocol_read_binary_struct("
             << "$input, '" << argsname << "');" << endl <<
    indent() << "} else if ($input instanceof TCompactProtocolAccelerated) {"
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
  f_service_ <<
    "$this->_handler->" << tfunction->get_name() << "(";
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
               << php_namespace((*x_iter)->get_type()->get_program())
               << (*x_iter)->get_type()->get_name() << " $exc" << exc_num
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
    indent() << "  $reply_type = TMessageType::EXCEPTION;" << endl <<
    indent() << "  $this->eventHandler_->handlerError($handler_ctx, '"
             << fn_name << "', $ex);" << endl <<
    indent() << "  $result = new TApplicationException($ex->getMessage());"
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
    indent() << "if ($output instanceof TBinaryProtocolAccelerated)" << endl;
  scope_up(f_service_);

  f_service_ <<
    indent() << "thrift_protocol_write_binary($output, '"
             << tfunction->get_name()
             << "', $reply_type, $result, $seqid, $output->isStrictWrite());"
             << endl;

  scope_down(f_service_);
  f_service_ <<
    indent() << "else if ($output instanceof TCompactProtocolAccelerated)" << endl;
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
    generate_php_struct_definition(f_service_, ts, false);
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

  // Also write the original thrift function defintion.
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
string t_hack_generator::type_to_typehint(t_type* ttype, bool nullable) {
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
    return type_to_typehint(((t_typedef*) ttype)->get_type());
  } else if (ttype->is_enum()) {
    if (is_bitmask_enum((t_enum*) ttype)) {
      return "int";
    } else {
      return (nullable ? "?" : "") + php_namespace(ttype->get_program()) + ttype->get_name() + "Type";
    }
  } else if (ttype->is_struct() || ttype->is_xception()) {
    return (nullable ? "?" : "") + php_namespace(ttype->get_program()) + ttype->get_name();
  } else if (ttype->is_list()) {
    return "Vector<" + type_to_typehint(((t_list*)ttype)->get_elem_type())  + ">";
  } else if (ttype->is_map()) {
    return "Map<" + type_to_typehint(((t_map*)ttype)->get_key_type()) + ", "  + type_to_typehint(((t_map*)ttype)->get_val_type()) + ">";
  } else if (ttype->is_set()) {
    if (arraysets_) {
      return "array<" + type_to_typehint(((t_set*)ttype)->get_elem_type()) + ", bool>";
    } else {
      return "Set<" + type_to_typehint(((t_set*)ttype)->get_elem_type()) + ">";
    }
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
      return "Indexish<int, " + type_to_param_typehint(((t_list*)ttype)->get_elem_type()) + ">";
    }
  } else if (ttype->is_map()) {
    if (strict_types_) {
      return type_to_typehint(ttype, nullable);
    } else {
      return "Indexish<" + type_to_param_typehint(((t_map*)ttype)->get_key_type())
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
        bool mangle) {
  generate_php_docstring(f_service_, tservice);
  string extends = "";
  string extends_if = "";
  if (tservice->get_extends() != nullptr) {
    string ext_prefix = php_servicename_mangle(mangle, tservice->get_extends());
    extends = " extends " + ext_prefix;
    extends_if = " extends " + ext_prefix + "If";
  }
  string long_name = php_servicename_mangle(mangle, tservice);
  f_service_ <<
    "interface " << long_name << "If" << extends_if << " {" << endl;
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
    indent(f_service_) <<
      "public function " << function_signature(*f_iter) << ";" << endl;
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
      "public function " << (*f_iter)->get_name() << "(Indexish<string, mixed> $request): " << type_to_typehint((*f_iter)->get_returntype()) << " {" << endl;
    indent_up();
    indent(f_service_) << "// UNSAFE $request is not type safe :(, and we don't cast structs (yet)" << endl;
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
                   << php_namespace(atype->get_program()) << atype->get_name()
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
  string extends = "";
  string extends_client = "";
  if (tservice->get_extends() != nullptr) {
    extends = php_servicename_mangle(mangle, tservice->get_extends());
    extends_client = " extends " + extends + "Client";
  }

  string long_name = php_servicename_mangle(mangle, tservice);
  out << "class " << long_name << "Client" << extends_client << " implements "
      << long_name << "If, IThriftClient {" << endl;
  indent_up();

  // Private members
  if (extends.empty()) {
    out <<
      indent() << "protected TProtocol $input_;" << endl <<
      indent() << "protected TProtocol $output_;" << endl <<
      indent() << "protected TClientAsyncHandler $asyncHandler_;" << endl <<
      indent() << "protected TClientEventHandler $eventHandler_;" << endl <<
      endl;
    out <<
      indent() << "protected int $seqid_ = 0;" << endl <<
      endl;
  }

  // Factory
  indent(out) << "public static function factory(): Pair<string, (function (TProtocol, ?TProtocol): " << long_name << "Client)> {" << endl;
  indent_up();
  indent(out) << "return Pair {__CLASS__, function(TProtocol $input, ?TProtocol $output) {" << endl;
  indent_up();
  indent(out) << "return new self($input, $output);" << endl;
  indent_down();
  indent(out) << "}};" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;

  // Constructor function
  out << indent() << "public function __construct("
      << "TProtocol $input, ?TProtocol $output = null) {" << endl;
  if (!extends.empty()) {
    out <<
      indent() << "  parent::__construct($input, $output);" << endl;
  } else {
    indent_up();
    out <<
      indent() << "$this->input_ = $input;" << endl <<
      indent() << "$this->output_ = $output ?: $input;" << endl <<
      indent() << "$this->asyncHandler_ = new TClientAsyncHandler();" << endl <<
      indent() << "$this->eventHandler_ = new TClientEventHandler();" << endl <<
      indent() << "$this->eventHandler_->setClient($this);" << endl;
    indent_down();
  }
  out <<
    indent() << "}" << endl << endl;

  generate_client_event_handler_functions(out);

  out <<
    indent() << "public function setAsyncHandler(TClientAsyncHandler $async_handler): this {" << endl <<
    indent() << "  $this->asyncHandler_ = $async_handler;" << endl <<
    indent() << "  return $this;" << endl <<
    indent() << "}" << endl <<
    endl;

  out <<
    indent() << "public function getAsyncHandler(): TClientAsyncHandler {" << endl <<
    indent() << "  return $this->asyncHandler_;" << endl <<
    indent() << "}" << endl <<
    endl;


  // Generate the function to get the next sequence number
  out <<
    indent() << "private function getsequenceid(): int {" << endl <<
    indent() << "  $currentseqid = $this->seqid_;" << endl <<
    indent() << "  if ($this->seqid_ >= 0x7fffffff) {" << endl <<
    indent() << "     $this->seqid_ = 0;" << endl <<
    indent() << "  } else {" << endl <<
    indent() << "     $this->seqid_++;" << endl <<
    indent() << "  }" << endl <<
    indent() << "  return $currentseqid;" << endl <<
    indent() << "}" << endl << endl;

  // Generate client method implementations
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arg_struct = (*f_iter)->get_arglist();
    const vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator fld_iter;
    string funname = (*f_iter)->get_name();

    // Open function
    generate_php_docstring(out, *f_iter);
    indent(out) <<
      "public function " << function_signature(*f_iter) << " {" << endl;
    indent_up();
      indent(out) <<
        "$currentseqid = $this->send_" << funname << "(";

      bool first = true;
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
          "$this->recv_" << funname << "($currentseqid);" << endl;
      }
    scope_down(out);
    out << endl;

    // Gen function
    indent(out)
      << "public async function "
      << function_signature(
          *f_iter,
          "gen_",
          "",
          "Awaitable<" + type_to_typehint((*f_iter)->get_returntype()) + ">")
      << " {" << endl;
    indent_up();
      indent(out) <<
        "$currentseqid = $this->send_" << funname << "(";

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
          "$this->recv_" << funname << "($currentseqid);" << endl;
      }
    scope_down(out);
    out << endl;

    indent(out) <<
      "public function send_" << function_signature(*f_iter, "", "", "int") << " {" << endl;
    indent_up();

      std::string argsname = php_namespace(tservice->get_program()) +
            service_name_ + "_" + (*f_iter)->get_name() + "_args";

      out <<
        indent() << "$currentseqid = $this->getsequenceid();" << endl <<
        indent() << "$args = new " << argsname << "();" << endl;

      for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
        out <<
          indent() << "$args->" << (*fld_iter)->get_name() << " = ";
        t_type* t = (*fld_iter)->get_type();
        if (!strict_types_ && t->is_map()) {
          out <<
            "new Map($" << (*fld_iter)->get_name() << ");" << endl;
        } else if (!strict_types_ && t->is_list()) {
          out <<
            "new Vector($" << (*fld_iter)->get_name() << ");" << endl;
        } else {
          out <<
            "$" << (*fld_iter)->get_name() << ";" << endl;
        }
      }

      out << indent() << "try {" << endl;
      indent_up();
      out <<
        indent() << "$this->eventHandler_->preSend('" << (*f_iter)->get_name() <<
                    "', $args, $currentseqid);" << endl;
      out <<
        indent() << "if ($this->output_ instanceof TBinaryProtocolAccelerated)" << endl;
      scope_up(out);

      out <<
        indent() << "thrift_protocol_write_binary($this->output_, '" <<
        (*f_iter)->get_name() << "', " <<
        "TMessageType::CALL, $args, $currentseqid, " <<
        "$this->output_->isStrictWrite(), " <<
        ((*f_iter)->is_oneway() ? "true" : "false") << ");" << endl;

      scope_down(out);
      out <<
        indent() << "else if ($this->output_ instanceof TCompactProtocolAccelerated)" << endl;
      scope_up(out);

      out <<
        indent() << "thrift_protocol_write_compact($this->output_, '" <<
        (*f_iter)->get_name() << "', " <<
        "TMessageType::CALL, $args, $currentseqid, " <<
        ((*f_iter)->is_oneway() ? "true" : "false") << ");" << endl;

      scope_down(out);
      out <<
        indent() << "else" << endl;
      scope_up(out);

      // Serialize the request header
      out <<
        indent() << "$this->output_->writeMessageBegin('" <<
        (*f_iter)->get_name() <<
        "', TMessageType::CALL, $currentseqid);" << endl;

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
    indent(out) << "} catch (THandlerShortCircuitException $ex) {" << endl;
    indent_up();
    out <<
      indent() << "switch ($ex->resultType) {" << endl <<
      indent() << "  case THandlerShortCircuitException::R_EXPECTED_EX:" << endl <<
      indent() << "  case THandlerShortCircuitException::R_UNEXPECTED_EX:" << endl <<
      indent() << "    $this->eventHandler_->sendError('" << (*f_iter)->get_name() <<
                  "', $args, $currentseqid, $ex->result);" << endl <<
      indent() << "    throw $ex->result;" << endl <<
      indent() << "  case THandlerShortCircuitException::R_SUCCESS:" << endl <<
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
      std::string resultname = php_namespace(tservice->get_program()) +
            service_name_ + "_" + (*f_iter)->get_name() + "_result";
      t_struct noargs(program_);

      t_function recv_function((*f_iter)->get_returntype(),
                               string("recv_") + (*f_iter)->get_name(),
                               &noargs);
      string return_typehint = type_to_typehint((*f_iter)->get_returntype());
      // Open function
      out <<
        endl <<
        indent() << "public function " <<
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
        indent() << "if ($this->input_ instanceof TBinaryProtocolAccelerated) {" << endl;

      indent_up();

      out <<
        indent() << "$result = thrift_protocol_read_binary("
                 << "$this->input_, '" << resultname
                 << "', $this->input_->isStrictRead());" << endl;

      indent_down();

      out <<
        indent() << "} else if ($this->input_ instanceof TCompactProtocolAccelerated)" << endl;
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
        indent() << "if ($mtype == TMessageType::EXCEPTION) {" << endl <<
        indent() << "  $x = new TApplicationException();" << endl <<
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
        indent() << "  throw new TProtocolException(\"" <<
        (*f_iter)->get_name() <<
        " failed: sequence id is out of order\");" << endl <<
        indent() << "}" << endl;

      scope_down(out);
      indent_down();
      indent(out) << "} catch (THandlerShortCircuitException $ex) {" << endl;
      indent_up();
      out <<
        indent() << "switch ($ex->resultType) {" << endl <<
        indent() << "  case THandlerShortCircuitException::R_EXPECTED_EX:" << endl <<
        indent() << "    $this->eventHandler_->recvException('" << (*f_iter)->get_name() <<
                    "', $expectedsequenceid, $ex->result);" << endl <<
        indent() << "    throw $ex->result;" << endl <<
        indent() << "  case THandlerShortCircuitException::R_UNEXPECTED_EX:" << endl <<
        indent() << "    $this->eventHandler_->recvError('" << (*f_iter)->get_name() <<
                    "', $expectedsequenceid, $ex->result);" << endl <<
        indent() << "    throw $ex->result;" << endl <<
        indent() << "  case THandlerShortCircuitException::R_SUCCESS:" << endl <<
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
            << "$x = new TApplicationException(\""
            << (*f_iter)->get_name() << " failed: unknown result\""
            << ", TApplicationException::MISSING_RESULT"
            << ");" << endl <<
          indent() << "$this->eventHandler_->recvError('" << (*f_iter)->get_name() <<
                      "', $expectedsequenceid, $x);" << endl <<
          indent() << "throw $x;" << endl;
      }

    // Close function
    scope_down(out);
    out << endl;

    }
  }

  indent_down();
  out <<
    "}" << endl << endl;
}

/**
 * Deserializes a field of any type.
 */
void t_hack_generator::generate_deserialize_field(ofstream &out,
                                                 t_field* tfield,
                                                 string name,
                                                 bool inclass) {
  t_type* type = get_true_type(tfield->get_type());
  if (name == "") {
    name = tfield->get_name();
  }

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " + name;
  }

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out,
                                (t_struct*)type,
                                 name);
  } else {

    if (type->is_container()) {
      generate_deserialize_container(out, type, name);
    } else if (type->is_base_type() || type->is_enum()) {

      if (type->is_base_type()) {
        indent(out) <<
          "$xfer += $input->";

        t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
        switch (tbase) {
        case t_base_type::TYPE_VOID:
          throw "compiler error: cannot serialize void field in a struct: " +
            name;
          break;
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

        string val = tmp("_val");
        out <<
          indent() << "$" << val << " = null;" << endl <<
          indent() << "$xfer += $input->readI32($" << val << ");" << endl <<
          indent() << "$" << name << " = " << php_namespace(tenum->get_program())
                   << tenum->get_name();
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
void t_hack_generator::generate_deserialize_struct(ofstream &out,
                                                  t_struct* tstruct,
                                                  string prefix) {
  out <<
    indent() << "$" << prefix << " = new " << php_namespace(tstruct->get_program()) << tstruct->get_name() << "();" << endl <<
    indent() << "$xfer += $" << prefix << "->read($input);" << endl;
}

void t_hack_generator::generate_deserialize_container(ofstream &out,
                                                     t_type* ttype,
                                                     string prefix) {
  string val = tmp("_val");
  string size = tmp("_size");
  string ktype = tmp("_ktype");
  string vtype = tmp("_vtype");
  string etype = tmp("_etype");

  t_field fsize(g_type_i32, size);
  t_field fktype(g_type_byte, ktype);
  t_field fvtype(g_type_byte, vtype);
  t_field fetype(g_type_byte, etype);

  out <<
    indent() << "$" << size << " = 0;" << endl;

  // Declare variables, read header
  if (ttype->is_map()) {
    out <<
      indent() << "$" << val << " = Map {};" << endl <<
      indent() << "$" << ktype << " = 0;" << endl <<
      indent() << "$" << vtype << " = 0;" << endl;
    out <<
      indent() << "$xfer += $input->readMapBegin(" <<
      "$" << ktype << ", $" << vtype << ", $" << size << ");" << endl;
  } else if (ttype->is_set()) {
    out <<
      indent() << "$" << etype << " = 0;" << endl;
    if (arraysets_) {
      out <<
        indent() << "$" << val << " = array();" << endl;
    } else {
      out <<
        indent() << "$" << val << " = Set{};" << endl;
    }
    out <<
      indent() << "$xfer += $input->readSetBegin(" <<
      "$" << etype << ", $" << size << ");" << endl;
  } else if (ttype->is_list()) {
    out <<
      indent() << "$" << val << " = Vector {};" << endl <<
      indent() << "$" << etype << " = 0;" << endl <<
      indent() << "$xfer += $input->readListBegin(" <<
      "$" << etype << ", $" << size << ");" << endl;
  }

  // For loop iterates over elements
  string i = tmp("_i");
  indent(out) <<
    "for ($" <<
    i << " = 0; $" << size << " === null || $" << i << " < $" << size << "; ++$" << i << ")" << endl;

    scope_up(out);

    if (ttype->is_map()) {
      generate_deserialize_map_element(out, (t_map*)ttype, size, val);
    } else if (ttype->is_set()) {
      generate_deserialize_set_element(out, (t_set*)ttype, size, val);
    } else if (ttype->is_list()) {
      generate_deserialize_list_element(out, (t_list*)ttype, size, val);
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
void t_hack_generator::generate_deserialize_map_element(ofstream &out,
                                                       t_map* tmap,
                                                       string size,
                                                       string prefix) {
  string key = tmp("key");
  string val = tmp("val");
  t_field fkey(tmap->get_key_type(), key);
  t_field fval(tmap->get_val_type(), val);

  out <<
    indent() << "if ($" << size << " === null && !$input->readMapHasNext()) {" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl;

  indent(out) <<
    declare_field(&fkey, true, true) << endl;
  indent(out) <<
    declare_field(&fval, true, true) << endl;

  generate_deserialize_field(out, &fkey);
  generate_deserialize_field(out, &fval);

  indent(out) <<
    "if ($" << key << " !== null && $" << val << " !== null) {" << endl;
  indent_up();
  indent(out) <<
    "$" << prefix << "[$" << key << "] = $" << val << ";" << endl;
  indent_down();
  indent(out) <<
    "}" << endl;
}

void t_hack_generator::generate_deserialize_set_element(ofstream &out,
                                                       t_set* tset,
                                                       string size,
                                                       string prefix) {
  string elem = tmp("elem");
  t_field felem(tset->get_elem_type(), elem);

  out <<
    indent() << "if ($" << size << " === null && !$input->readSetHasNext()) {" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl;

  indent(out) <<
    "$" << elem << " = null;" << endl;

  generate_deserialize_field(out, &felem);

  indent(out) <<
    "if ($" << elem << " !== null) {" << endl;
  indent_up();
  if (arraysets_) {
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

void t_hack_generator::generate_deserialize_list_element(ofstream &out,
                                                        t_list* tlist,
                                                        string size,
                                                        string prefix) {
  string elem = tmp("elem");
  t_field felem(tlist->get_elem_type(), elem);

  out <<
    indent() << "if ($" << size << " === null && !$input->readListHasNext()) {" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl;

  indent(out) <<
    "$" << elem << " = null;" << endl;

  generate_deserialize_field(out, &felem);

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
void t_hack_generator::generate_serialize_field(ofstream &out,
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
    generate_serialize_struct(out,
                              (t_struct*)type,
                              name);
  } else if (type->is_container()) {
    generate_serialize_container(out,
                                 type,
                                 name);
  } else if (type->is_base_type() || type->is_enum()) {

    indent(out) <<
      "$xfer += $output->";

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw
          "compiler error: cannot serialize void field in a struct: " + name;
        break;
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
void t_hack_generator::generate_serialize_struct(ofstream &out,
                                                t_struct* tstruct,
                                                string prefix) {
  indent(out) <<
    "$xfer += $" << prefix << "->write($output);" << endl;
}

/**
 * Writes out a container
 */
void t_hack_generator::generate_serialize_container(ofstream &out,
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
    string kiter = tmp("kiter");
    string viter = tmp("viter");
    indent(out) <<
      "foreach ($" << prefix << " as " <<
      "$" << kiter << " => $" << viter << ")" << endl;
    scope_up(out);
    generate_serialize_map_element(out, (t_map*)ttype, kiter, viter);
    scope_down(out);
  } else if (ttype->is_set()) {
    string iter = tmp("iter");
    if (arraysets_) {
      indent(out) <<
        "foreach ($" << prefix << " as $" << iter << " => $true)" << endl;
    } else {
      indent(out) <<
        "foreach ($" << prefix << " as $" << iter << ")" << endl;
    }
    scope_up(out);
    generate_serialize_set_element(out, (t_set*)ttype, iter);
    scope_down(out);
  } else if (ttype->is_list()) {
    string iter = tmp("iter");
    indent(out) <<
      "foreach ($" << prefix << " as $" << iter << ")" << endl;
    scope_up(out);
    generate_serialize_list_element(out, (t_list*)ttype, iter);
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
void t_hack_generator::generate_serialize_map_element(ofstream &out,
                                                     t_map* tmap,
                                                     string kiter,
                                                     string viter) {
  t_field kfield(tmap->get_key_type(), kiter);
  generate_serialize_field(out, &kfield);

  t_field vfield(tmap->get_val_type(), viter);
  generate_serialize_field(out, &vfield);
}

/**
 * Serializes the members of a set.
 */
void t_hack_generator::generate_serialize_set_element(ofstream &out,
                                                     t_set* tset,
                                                     string iter) {
  t_field efield(tset->get_elem_type(), iter);
  generate_serialize_field(out, &efield);
}

/**
 * Serializes the members of a list.
 */
void t_hack_generator::generate_serialize_list_element(ofstream &out,
                                                      t_list* tlist,
                                                      string iter) {
  t_field efield(tlist->get_elem_type(), iter);
  generate_serialize_field(out, &efield);
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
                                      bool obj, bool thrift) {
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
      result += " = Map {}";
    } else if (type->is_list()) {
      result += " = Vector {}";
    } else if (type->is_set()) {
      if (arraysets_) {
        result += " = array()";
      } else {
        result += " = Set {}";
      }
    } else if (type->is_struct() || type->is_xception()) {
      if (obj) {
        result += " = new " + php_namespace(type->get_program())
               + type->get_name() + "()";
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
      return "TType::STRING";
    case t_base_type::TYPE_BOOL:
      return "TType::BOOL";
    case t_base_type::TYPE_BYTE:
      return "TType::BYTE";
    case t_base_type::TYPE_I16:
      return "TType::I16";
    case t_base_type::TYPE_I32:
      return "TType::I32";
    case t_base_type::TYPE_I64:
      return "TType::I64";
    case t_base_type::TYPE_DOUBLE:
      return "TType::DOUBLE";
    case t_base_type::TYPE_FLOAT:
      return "TType::FLOAT";
    }
  } else if (type->is_enum()) {
    return "TType::I32";
  } else if (type->is_struct() || type->is_xception()) {
    return "TType::STRUCT";
  } else if (type->is_map()) {
    return "TType::MAP";
  } else if (type->is_set()) {
    return "TType::SET";
  } else if (type->is_list()) {
    return "TType::LST";
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
);
