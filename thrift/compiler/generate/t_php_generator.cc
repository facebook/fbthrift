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

#include <sstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <thrift/compiler/generate/t_oop_generator.h>
#include <thrift/compiler/platform.h>

using namespace std;
/**
 * PHP code generator.
 *
 */
class t_php_generator : public t_oop_generator {
 public:
  t_php_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/)
    : t_oop_generator(program)
  {
    std::map<std::string, std::string>::const_iterator iter;

    json_ = option_is_specified(parsed_options, "json");
    binary_inline_ = option_is_specified(parsed_options, "inlined");
    rest_ = option_is_specified(parsed_options, "rest");
    phps_ = option_is_specified(parsed_options, "server");
    autoload_ = option_is_specified(parsed_options, "autoload");
    norequires_ = option_is_specified(parsed_options, "norequires");
    oop_ = option_is_specified(parsed_options, "oop");
    ducktyping_ = option_is_specified(parsed_options, "ducktyping");
    hphpenum_ = option_is_specified(parsed_options, "hphpenum");
    async_ = option_is_specified(parsed_options, "async");

    mangled_services_ = option_is_set(parsed_options, "mangledsvcs", false);
    unmangled_services_ = option_is_set(parsed_options, "unmangledsvcs", true);

    // default behaviour for the moment is to save constant values in $GLOBALS
    // TODO: deprecate
    save_constants_in_global_ = !option_is_specified(parsed_options,
            "no-global-constants");

    if (oop_ && binary_inline_) {
      throw "oop and inlined are mutually exclusive.";
    }

    out_dir_base_ = (binary_inline_ ? "gen-phpi" : "gen-php");
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

  std::string render_const_value(t_type* type, t_const_value* value);

  /**
   * Structs!
   */

  void generate_php_struct(t_struct* tstruct, bool is_exception);
  void generate_php_struct_definition(std::ofstream& out, t_struct* tstruct,
                                        bool is_xception=false);
  void _generate_php_struct_definition(std::ofstream& out, t_struct* tstruct,
                                        bool is_xception=false);
  void generate_php_struct_reader(std::ofstream& out, t_struct* tstruct);
  void generate_php_struct_writer(std::ofstream& out, t_struct* tstruct);
  void generate_php_function_helpers(t_function* tfunction);

  void generate_php_type_spec(std::ofstream& out, t_type* t);
  void generate_php_struct_spec(std::ofstream& out, t_struct* tstruct);
  void generate_php_structural_id(std::ofstream& out, t_struct* tstruct);

  /**
   * Service-level generation functions
   */

  void generate_service           (t_service* tservice, bool mangle);
  void generate_service_helpers   (t_service* tservice);
  void generate_service_interface (t_service* tservice, bool mangle);
  void generate_service_rest      (t_service* tservice, bool mangle);
  void generate_service_client    (t_service* tservice, bool mangle);
  void _generate_service_client   (std::ofstream& out, t_service* tservice,
                                        bool mangle);
  void generate_service_processor (t_service* tservice, bool mangle);
  void generate_process_function  (t_service* tservice, t_function* tfunction);
  void generate_processor_event_handler_functions(std::ofstream& out);
  void generate_client_event_handler_functions(std::ofstream& out);
  void generate_event_handler_functions (std::ofstream& out, string cl);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field        (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_field*    tfield,
                                          std::string prefix="",
                                          bool inclass=false);

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
                                          t_map* tmap,
                                          std::string size,
                                          std::string prefix = "");

  void generate_deserialize_list_element (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_list* tlist,
                                          std::string size,
                                          std::string prefix = "");

  void generate_serialize_field          (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_field* tfield,
                                          std::string prefix = "");

  void generate_serialize_struct         (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_struct* tstruct,
                                          std::string prefix = "");

  void generate_serialize_container      (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_type* ttype,
                                          std::string prefix = "");

  void generate_serialize_map_element    (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_map* tmap,
                                          std::string kiter,
                                          std::string viter);

  void generate_serialize_set_element    (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_set* tmap,
                                          std::string iter);

  void generate_serialize_list_element   (std::ofstream& out,
                                          t_name_generator& namer,
                                          t_list* tlist,
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

  std::string php_includes();
  std::string include_types();
  std::string declare_field(t_field* tfield, bool init=false,
                            bool obj=false, bool thrift=false);
  std::string function_signature(t_function* tfunction,
                                 std::string prefix="",
                                 std::string moreparameters="");
  std::string argument_list(t_struct* tstruct, std::string moreparameters="");
  std::string type_to_cast(t_type* ttype);
  std::string type_to_enum(t_type* ttype);
  void generate_php_docstring(ofstream& out, t_doc* tdoc);
  void generate_php_docstring(ofstream& out, t_function* tfunction);
  void generate_php_docstring_args(ofstream& out,
                                   int start_pos,
                                   t_struct* arg_list);
  std::string render_string(std::string value);

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
   * Generate protocol-independent template? Or Binary inline code?
   */
  bool binary_inline_;

  /**
   * Generate a REST handler class
   */
  bool rest_;

  /**
   * Generate stubs for a PHP server
   */
  bool phps_;

  /**
   * Generate PHP code that uses autoload
   */
  bool autoload_;

  /**
   * Generate PHP code with no require_once/include_once
   */
  bool norequires_;

  /**
   * Whether to use OOP base class TBase
   */
  bool oop_;

  /**
   * Whether to allow duck-typing in processor and REST handler constructors
   */
  bool ducktyping_;

  /**
   * * Whether to enable HPHP Enum generation
   */
  bool hphpenum_;

  /**
   * * Whether to enable Hack async method generation
   */
  bool async_;

  /**
   * memory of the values of the constants in array initialisation form
   * for use with generate_const
   */
  std::vector<string> constants_values_;

  /**
   * (deprecated)
   * Whether to save the definitions of the constants in GLOBALS
   */
  bool save_constants_in_global_;

  /**
   * True iff unmangled service classes should be emitted
   */
  bool unmangled_services_;

  /**
   * True iff mangled service classes should be emitted
   */
  bool mangled_services_;
};

void t_php_generator::generate_json_enum(std::ofstream& out,
                                         t_name_generator& namer,
                                         t_enum* /*tenum*/,
                                         const string& prefix_thrift,
                                         const string& prefix_json) {
  indent(out) << prefix_thrift << " = " << "(int)"
              << prefix_json << ";" << endl;
}

void t_php_generator::generate_json_struct(ofstream& out,
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

void t_php_generator::generate_json_field(ofstream& out,
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

void t_php_generator::generate_json_container(std::ofstream& out,
                                              t_name_generator& namer,
                                              t_type* ttype,
                                              const string& prefix_thrift,
                                              const string& prefix_json) {
  t_container* tcontainer = (t_container*)ttype;
  string size = namer("$_size");
  string key = namer("$_key");
  string value = namer("$_value");
  string json = namer("$_json");

  indent(out) << json << " = " << prefix_json << ";" << endl;
  indent(out) << prefix_thrift << " = array();" << endl;
  indent(out) << "foreach(" << json << " as " << key << " => " << value
              << ") {" << endl;
  indent_up();

  if (ttype->is_list()) {
    generate_json_list_element(
        out, namer, (t_list*)ttype, value, prefix_thrift);
  } else if (ttype->is_set()) {
    generate_json_set_element(out, namer, (t_set*)ttype, value, prefix_thrift);
  } else if (ttype->is_map()) {
    generate_json_map_element(
        out, namer, (t_map*)ttype, key, value, prefix_thrift);
  } else {
    throw "compiler error: no PHP reader for this type.";
  }
  indent_down();
  indent(out) << "}" << endl;
}

void t_php_generator::generate_json_list_element(ofstream& out,
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

void t_php_generator::generate_json_set_element(std::ofstream& out,
                                                t_name_generator& namer,
                                                t_set* tset,
                                                const string& value,
                                                const string& prefix_thrift) {
  string elem = namer("$_elem");
  t_field felem(tset->get_elem_type(), elem);
  indent(out) << declare_field(&felem, true, true, true).substr(1) << endl;
  generate_json_field(out, namer, &felem, "", "", value);
  indent(out) << prefix_thrift << "[" << elem << "] = true;" << endl;
}
void t_php_generator::generate_json_map_element(std::ofstream& out,
                                                t_name_generator& namer,
                                                t_map* tmap,
                                                const string& key,
                                                const string& value,
                                                const string& prefix_thrift) {
  t_type* keytype = get_true_type(tmap->get_key_type());
  bool succ = true;
  string error_msg = "compiler error: Thrift PHP compiler"
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

void t_php_generator::generate_json_reader(ofstream& out, t_struct* tstruct) {
  if(!json_) {
    return;
  }
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  string name = tstruct->get_name();
  indent(out) << "public function " << "readFromJson($jsonText) {" << endl;

  t_name_generator namer;

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
void t_php_generator::init_generator() {
  // Make output directory
  MKDIR(get_out_dir().c_str());

  // Make output file
  string f_types_name = get_out_dir()+program_name_+"_types.php";
  f_types_.open(f_types_name.c_str());
  record_genfile(f_types_name);

  // Print header
  f_types_ <<
    "<?php" << endl <<
    autogen_comment() <<
    php_includes();

  if (!norequires_) {
    // Include other Thrift includes
    const vector<t_program*>& includes = program_->get_includes();
    for (size_t i = 0; i < includes.size(); ++i) {
      string package = includes[i]->get_name();
      string prefix = php_path(includes[i]);
      f_types_ <<
        "require_once $GLOBALS['THRIFT_ROOT'].'/packages/" << prefix << "/" << package << "_types.php';" << endl;
    }
  }
  f_types_ << endl;

  // Print header
  if (!program_->get_consts().empty()) {
    string f_consts_name = get_out_dir()+program_name_+"_constants.php";
    f_consts_.open(f_consts_name.c_str());
    record_genfile(f_consts_name);
    f_consts_ <<
      "<?php" << endl <<
      autogen_comment();
    if (!norequires_) {
      f_consts_ <<
        "require_once $GLOBALS['THRIFT_ROOT'].'/packages/" + php_path(program_) + "/" + program_name_ + "_types.php';" << endl <<
        endl;
    }
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
 * Prints standard php includes
 */
string t_php_generator::php_includes() {
  if (norequires_) {
    return "";
  }

  std::ostringstream includes;
  includes << "require_once $GLOBALS['THRIFT_ROOT'].'/Thrift.php';\n";
  if (autoload_) {
    includes << "require_once $GLOBALS['THRIFT_ROOT'].'/autoload.php';\n";
  }
  includes << "\n";
  return includes.str();
}

string t_php_generator::include_types() {
  if (norequires_) {
    return "";
  }

  std::ostringstream str;
  str <<
    "require_once $GLOBALS['THRIFT_ROOT'].'/packages/" << php_path(program_) <<
    "/" << program_name_ << "_types.php';\n\n";
  return str.str();
}

/**
 * Close up (or down) some filez.
 */
void t_php_generator::close_generator() {
  // Close types file
  f_types_ << "?>" << endl;
  f_types_.close();

  if (!program_->get_consts().empty()) {
    // write out the values array
    indent_up();
    f_consts_ << endl;
    indent(f_consts_) << "public static $__values = array(" << endl;
    std::copy(constants_values_.begin(), constants_values_.end(),
        std::ostream_iterator<string>(f_consts_, ",\n"));
    indent(f_consts_) << ");" << endl;
    indent_down();
    // close constants class
    f_consts_ << "}" << endl <<
      endl;
    // if save_constants_in_global_, write all the global definitions of the
    // constants as well
    if (save_constants_in_global_) {
      f_consts_ << "$GLOBALS['" << program_name_ << "_CONSTANTS'] = " <<
        program_name_ << "_CONSTANTS::$__values;"<< endl <<
        endl;
    }
    f_consts_ << "?>" << endl;
    f_consts_.close();
  }
}

/**
 * Generates a typedef. This is not done in PHP, types are all implicit.
 *
 * @param ttypedef The type definition
 */
void t_php_generator::generate_typedef(t_typedef* /*ttypedef*/) {}

/**
 * Generates code for an enumerated type. Since define is expensive to lookup
 * in PHP, we use a global array for this.
 *
 * @param tenum The enumeration
 */
void t_php_generator::generate_enum(t_enum* tenum) {
  vector<t_enum_value*> constants = tenum->get_constants();
  vector<t_enum_value*>::iterator c_iter;

  generate_php_docstring(f_types_, tenum);
  // We're also doing it this way to see how it performs. It's more legible
  // code but you can't do things like an 'extract' on it, which is a bit of
  // a downer.
  if (hphpenum_) {
    if (is_bitmask_enum(tenum)) {
      f_types_ <<
        "final class " <<
        php_namespace(tenum->get_program()) <<
        tenum->get_name() << " extends Flags {" << endl;
    } else {
      f_types_ <<
        "final class " <<
        php_namespace(tenum->get_program()) <<
        tenum->get_name() << " extends Enum {" << endl;
    }
  }
  else {
    f_types_ <<
      "final class " <<
      php_namespace(tenum->get_program()) <<
      tenum->get_name() << " {" << endl;
  }

  indent_up();

  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int32_t value = (*c_iter)->get_value();

    generate_php_docstring(f_types_, *c_iter);
    indent(f_types_) <<
      "const " << (*c_iter)->get_name() << " = " << value << ";" << endl;
  }

  if (!hphpenum_) {
    // names
    indent(f_types_) <<
      "static public $__names = array(" << endl;
    for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
      int32_t value = (*c_iter)->get_value();

      indent(f_types_) <<
        "  " << value << " => '" << (*c_iter)->get_name() << "'," << endl;
    }
    indent(f_types_) <<
      ");" << endl;
    // values
    indent(f_types_) <<
      "static public $__values = array(" << endl;
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

  if (save_constants_in_global_) {
    // generate global array of enum values
    if (!hphpenum_) {
      f_types_ <<
        "$GLOBALS['" << php_namespace(tenum->get_program()) << "E_" <<
        tenum->get_name() << "'] = " << php_namespace(tenum->get_program()) <<
        tenum->get_name() << "::$__values;" << endl << endl;
    }
  }
}

/**
 * Generate a constant value
 */
void t_php_generator::generate_const(t_const* tconst) {
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
    indent(f_consts_) << "static $" << name << " = ";
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
  indent_down();
}

string t_php_generator::render_string(string value) {
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
string t_php_generator::render_const_value(t_type* type, t_const_value* value) {
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
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    indent(out) << value->get_integer();
  } else if (type->is_struct() || type->is_xception()) {
    out << "new " << php_namespace(type->get_program()) << type->get_name() << "(array(" << endl;
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
    indent(out) << "))";
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    out << "array(" << endl;
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
    indent(out) << ")";
  } else if (type->is_list() || type->is_set()) {
    t_type* etype;
    if (type->is_list()) {
      etype = ((t_list*)type)->get_elem_type();
    } else {
      etype = ((t_set*)type)->get_elem_type();
    }
    out << "array(" << endl;
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(etype, *v_iter);
      if (type->is_set()) {
        out << " => true";
      }
      out << "," << endl;
    }
    indent_down();
    indent(out) << ")";
  }
  return out.str();
}

/**
 * Make a struct
 */
void t_php_generator::generate_struct(t_struct* tstruct) {
  generate_php_struct(tstruct, false);
}

/**
 * Generates a struct definition for a thrift exception. Basically the same
 * as a struct but extends the Exception class.
 *
 * @param txception The struct definition
 */
void t_php_generator::generate_xception(t_struct* txception) {
  generate_php_struct(txception, true);
}

/**
 * Structs can be normal or exceptions.
 */
void t_php_generator::generate_php_struct(t_struct* tstruct,
                                          bool is_exception) {
  generate_php_struct_definition(f_types_, tstruct, is_exception);
}

void t_php_generator::generate_php_type_spec(ofstream& out,
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
    indent(out) << "'format' => 'array'," << endl;
    indent_down();
  } else if (t->is_list() || t->is_set()) {
    t_type* etype;
    if (t->is_list()) {
      etype = get_true_type(((t_list*)t)->get_elem_type());
    } else {
      etype = get_true_type(((t_set*)t)->get_elem_type());
    }
    indent(out) << "'etype' => " << type_to_enum(etype) <<"," << endl;
    indent(out) << "'elem' => array(" << endl;
    indent_up();
    generate_php_type_spec(out, etype);
    indent(out) << ")," << endl;
    indent(out) << "'format' => 'array'," << endl;
    indent_down();
  } else {
    throw "compiler error: no type for php struct spec field";
  }

}

/**
 * Generates the struct specification structure, which fully qualifies enough
 * type information to generalize serialization routines.
 */
void t_php_generator::generate_php_struct_spec(ofstream& out,
                                               t_struct* tstruct) {
  indent(out) << "static $_TSPEC = array(" << endl;
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

  indent(out) << "public static $_TFIELDMAP = array(" << endl;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());
    indent(out) << "  '" << (*m_iter)->get_name() << "' => " << (*m_iter)->get_key() << "," << endl;
  }
  indent(out) << ");" << endl;
}

/**
 * Generates the structural ID definition, see generate_structural_id()
 * for information about the structural ID.
 */
void t_php_generator::generate_php_structural_id(ofstream& out,
                                                  t_struct* tstruct) {
  indent(out) << "const STRUCTURAL_ID = "
              << generate_structural_id(tstruct->get_members()) << ";"
              << endl;
}

void t_php_generator::generate_php_struct_definition(ofstream& out,
                                                     t_struct* tstruct,
                                                     bool is_exception) {
  if (autoload_) {
    // Make output file
    ofstream autoload_out;
    string f_struct = program_name_+"."+(tstruct->get_name())+".php";
    string f_struct_name = get_out_dir()+f_struct;
    autoload_out.open(f_struct_name.c_str());
    record_genfile(f_struct_name);
    autoload_out << "<?php" << endl
      << autogen_comment();
    _generate_php_struct_definition(autoload_out, tstruct, is_exception);
    autoload_out << endl << "?>" << endl;
    autoload_out.close();

    f_types_ <<
      "$GLOBALS['THRIFT_AUTOLOAD']['" << lowercase(php_namespace(tstruct->get_program()) + tstruct->get_name()) << "'] = '" << php_path(program_) << "/" << f_struct << "';" << endl;

  } else {
    _generate_php_struct_definition(out, tstruct, is_exception);
  }
}

/**
 * Generates a struct definition for a thrift data type. This is nothing in PHP
 * where the objects are all just associative arrays (unless of course we
 * decide to start using objects for them...)
 *
 * @param tstruct The struct definition
 */
void t_php_generator::_generate_php_struct_definition(ofstream& out,
                                                     t_struct* tstruct,
                                                     bool is_exception) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  generate_php_docstring(out, tstruct);
  out <<
    "class " << php_namespace(tstruct->get_program()) << tstruct->get_name();
  if (is_exception) {
    out << " extends TException";
  } else if (oop_) {
    out << " extends TBase";
  }
  out <<
    " implements IThriftStruct {" << endl;
  indent_up();

  generate_php_struct_spec(out, tstruct);

  generate_php_structural_id(out, tstruct);

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    string dval = "null";
    t_type* t = get_true_type((*m_iter)->get_type());

    if (t->is_enum() && is_bitmask_enum((t_enum*) t)) {
      throw "Enum " + (((t_enum*) t)->get_name()) + "is actually a bitmask, cannot generate a field of this enum type";
    }

    if ((*m_iter)->get_value() != nullptr && !(t->is_struct() || t->is_xception())) {
      dval = render_const_value((*m_iter)->get_type(), (*m_iter)->get_value());
    }
    indent(out) <<
      "public $" << (*m_iter)->get_name() << " = " << dval << ";" << endl;
  }

  out << endl;

  // Generate constructor from array
  out <<
    indent() << "public function __construct($vals=null) {" << endl;
  indent_up();

  if (members.size() > 0) {
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      t_type* t = get_true_type((*m_iter)->get_type());
      if ((*m_iter)->get_value() != nullptr
          && (t->is_struct() || t->is_xception())) {
        indent(out) << "$this->" << (*m_iter)->get_name() << " = "
                    << render_const_value(t, (*m_iter)->get_value())
                    << ";" << endl;
      }
    }
    out <<
      indent() << "if (is_array($vals)) {" << endl;
    indent_up();
    if (oop_) {
      out << indent() << "parent::__construct(self::$_TSPEC, $vals);" << endl;
    } else {
      for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        out <<
          indent() << "if (isset($vals['" << (*m_iter)->get_name() << "'])) {" << endl <<
          indent() << "  $this->" << (*m_iter)->get_name() << " = $vals['" << (*m_iter)->get_name() << "'];" << endl <<
          indent() << "}" << endl;
      }
    }
    indent_down();
    out <<
      indent() << "} else if ($vals) {" << endl;
    indent_up();
    out << indent() << "throw new TProtocolException(" << endl;
    indent_up();
    out << indent() << "'"
        << php_namespace(tstruct->get_program()) << tstruct->get_name()
        << " constructor must be passed array or null'" << endl;
    indent_down();
    out << indent() << ");" << endl;
    indent_down();
    out << indent() << "}" << endl;
  }
  scope_down(out);
  out << endl;

  out <<
    indent() << "public function getName() {" << endl <<
    indent() << "  return '" << tstruct->get_name() << "';" << endl <<
    indent() << "}" << endl <<
    endl;

  string param = (members.size() > 0) ? "($vals);" : "();";
  out <<
    indent() << "public static function __set_state($vals) {" << endl <<
    indent() << "  return new " << php_namespace(tstruct->get_program()) <<
      tstruct->get_name() << param << endl <<
    indent() << "}" << endl <<
    endl;

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
void t_php_generator::generate_php_struct_reader(ofstream& out,
                                                 t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  if (oop_) {
    indent(out) <<
      "public function read(TProtocol $input)" << endl;
    scope_up(out);
    indent(out) << "return $this->_read('" << tstruct->get_name()
                << "', self::$_TSPEC, $input);" << endl;
    scope_down(out);
    return;
  }

  indent(out) <<
    "public function read(TProtocol $input)" << endl;
  scope_up(out);

  t_name_generator namer;

  out <<
    indent() << "$xfer = 0;" << endl <<
    indent() << "$fname = null;" << endl <<
    indent() << "$ftype = 0;" << endl <<
    indent() << "$fid = 0;" << endl;

  // Declare stack tmp variables
  if (!binary_inline_) {
    indent(out) <<
      "$xfer += $input->readStructBegin($fname);" << endl;
  }

  // Loop over reading in fields
  indent(out) <<
    "while (true)" << endl;

    scope_up(out);

    // Read beginning field marker
    if (binary_inline_) {
      t_field fftype(g_type_byte, "ftype");
      t_field ffid(g_type_i16, "fid");
      generate_deserialize_field(out, namer, &fftype);
      out <<
        indent() << "if ($ftype == TType::STOP) {" << endl <<
        indent() << "  break;" << endl <<
        indent() << "}" << endl;
      generate_deserialize_field(out, namer, &ffid);
    } else {
      indent(out) <<
        "$xfer += $input->readFieldBegin($fname, $ftype, $fid);" << endl;
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
        indent() << "  if (isset(self::$_TFIELDMAP[$fname])) {" << endl <<
        indent() << "    $fid = self::$_TFIELDMAP[$fname];" << endl <<
        indent() << "    $ftype = self::$_TSPEC[$fid]['type'];" << endl <<
        indent() << "  }" << endl <<
        indent() << "}" << endl;
    }

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
        generate_deserialize_field(out, namer, *f_iter, "this->");
        indent_down();
        out <<
          indent() << "} else {" << endl;
        if (binary_inline_) {
          indent(out) <<  "  $xfer += TProtocol::skipBinary($input, $ftype);" << endl;
        } else {
          indent(out) <<  "  $xfer += $input->skip($ftype);" << endl;
        }
        out <<
          indent() << "}" << endl <<
          indent() << "break;" << endl;
        indent_down();
      }

      // In the default case we skip the field
      indent(out) <<  "default:" << endl;
      if (binary_inline_) {
        indent(out) <<  "  $xfer += TProtocol::skipBinary($input, $ftype);" << endl;
      } else {
        indent(out) <<  "  $xfer += $input->skip($ftype);" << endl;
      }
      indent(out) <<  "  break;" << endl;

      scope_down(out);

    if (!binary_inline_) {
      // Read field end marker
      indent(out) <<
        "$xfer += $input->readFieldEnd();" << endl;
    }

    scope_down(out);

  if (!binary_inline_) {
    indent(out) <<
      "$xfer += $input->readStructEnd();" << endl;
  }

  if (tstruct->is_xception()) {
    const char* annotations[] = {"message", "code"};
    for (auto& annotation: annotations) {
      auto it = tstruct->annotations_.find(annotation);
      if (it != tstruct->annotations_.end()) {
        // if annotation is also used as field, ignore annotation
        if (tstruct->has_field_named(annotation)) {
          if (strcmp(annotation, it->second.c_str()) != 0) {
            fprintf(stderr, "Ignoring annotation '%s' in PHP generator because "
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
      indent(out) << "if (!isset($this->" << (*f_iter)->get_name() << ")) {"
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
void t_php_generator::generate_php_struct_writer(ofstream& out,
                                                 t_struct* tstruct) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  if (oop_) {
    if (binary_inline_) {
      indent(out) << "public function write(TProtocol &$output)" << endl;
    } else {
      indent(out) << "public function write(TProtocol $output)" << endl;
    }
    scope_up(out);
    indent(out) << "return $this->_write('" << name << "', self::$_TSPEC, $output);" << endl;
    scope_down(out);
    return;
  }

  if (binary_inline_) {
    indent(out) <<
      "public function write(TProtocol &$output) {" << endl;
  } else {
    indent(out) <<
      "public function write(TProtocol $output) {" << endl;
  }
  indent_up();

  t_name_generator namer;

  indent(out) <<
    "$xfer = 0;" << endl;

  if (!binary_inline_) {
    indent(out) <<
      "$xfer += $output->writeStructBegin('" << name << "');" << endl;
  }

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Optional check
    bool optional = (*f_iter)->get_req() == t_field::T_OPTIONAL;
    if (optional) {
      indent(out) << "if (isset($this->" << (*f_iter)->get_name() << ") && "
        << "$this->" << (*f_iter)->get_name() << " !== null) {";
    } else {
      indent(out) << "if ($this->" << (*f_iter)->get_name() << " !== null) {";
    }

    out << endl;
    indent_up();

    t_type* type = get_true_type((*f_iter)->get_type());
    if (type->is_container() || type->is_struct()) {
      out <<
        indent() << "if (";
      if (type->is_container()) {
        out <<
          "!is_array($this->" + (*f_iter)->get_name() + ") && " <<
          "!(($this->" + (*f_iter)->get_name() + " instanceof Iterator || "<<
          "$this->" + (*f_iter)->get_name() + " instanceof IteratorAggregate) " <<
          "&& $this->" + (*f_iter)->get_name() + " instanceof Countable)";
      } else {
        out <<
          "!is_object($this->" << (*f_iter)->get_name() << ")";
      }

      out <<
        ") {" << endl;
      indent_up();
      out <<
        indent() << "throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);" << endl;
      scope_down(out);
    }

    // Write field header
    if (binary_inline_) {
      out <<
        indent() << "$output .= pack('c', " << type_to_enum((*f_iter)->get_type()) << ");" << endl <<
        indent() << "$output .= pack('n', " << (*f_iter)->get_key() << ");" << endl;
    } else {
      indent(out) <<
        "$xfer += $output->writeFieldBegin(" <<
        "'" << (*f_iter)->get_name() << "', " <<
        type_to_enum((*f_iter)->get_type()) << ", " <<
        (*f_iter)->get_key() << ");" << endl;
    }

    // Write field contents
    generate_serialize_field(out, namer, *f_iter, "this->");

    // Write field closer
    if (!binary_inline_) {
      indent(out) <<
        "$xfer += $output->writeFieldEnd();" << endl;
    }

    indent_down();
    indent(out) <<
      "}" << endl;
  }

  if (binary_inline_) {
    out <<
      indent() << "$output .= pack('c', TType::STOP);" << endl;
  } else {
    out <<
      indent() << "$xfer += $output->writeFieldStop();" << endl <<
      indent() << "$xfer += $output->writeStructEnd();" << endl;
  }

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
void t_php_generator::generate_service(t_service* tservice) {
  if (unmangled_services_) {
    generate_service(tservice, false);
  }

  if (mangled_services_) {
    if (php_namespace(tservice).empty()) {
      throw "cannot generate mangled services for " + tservice->get_name() +
          "; no php namespace found";
    }
    // generate new files for mangled services, if requested
    generate_service(tservice, true);
  }
}

void t_php_generator::generate_service(t_service* tservice, bool mangle) {
  string f_base_name = php_servicename_mangle(mangle, tservice);
  string f_service_name = get_out_dir()+f_base_name+".php";
  f_service_.open(f_service_name.c_str());
  record_genfile(f_service_name);

  f_service_ <<
    "<?php" << endl <<
    autogen_comment() <<
    php_includes();

  f_service_ << include_types();

  if (tservice->get_extends() != nullptr && !norequires_) {
    t_service* ext = tservice->get_extends();
    f_service_ <<
      "require_once $GLOBALS['THRIFT_ROOT'].'/packages/" << php_path(ext) <<
      "/" << php_servicename_mangle(mangle, ext) << ".php';" << endl;
  }

  f_service_ <<
    endl;

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
  f_service_ << "?>" << endl;
  f_service_.close();
}

/**
 * Generates process event handler functions.
 */
void t_php_generator::generate_processor_event_handler_functions(ofstream& out) {
  generate_event_handler_functions(out, "TProcessorEventHandler");
}
void t_php_generator::generate_client_event_handler_functions(ofstream& out) {
  generate_event_handler_functions(out, "TClientEventHandler");
}
void t_php_generator::generate_event_handler_functions(ofstream& out, string cl) {
  indent(out) <<
    "public function setEventHandler(" << cl << " $event_handler) {" << endl <<
    indent() << "  $this->eventHandler_ = $event_handler;" << endl <<
    indent() << "}" << endl <<
    endl;

  indent(out) <<
    "public function getEventHandler() {" << endl <<
    indent() << "  return $this->eventHandler_;" << endl <<
    indent() << "}" << endl <<
    endl;
}

/**
 * Generates a service server definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_php_generator::generate_service_processor(t_service* tservice,
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
      indent() << "protected $handler_ = null;" << endl;
  }

  f_service_ <<
    indent() << "protected $eventHandler_ = null;"
             << endl;

  if (ducktyping_) {
    f_service_ <<
      indent() << "public function __construct($handler) {" << endl;
  } else {
    f_service_ <<
      indent() << "public function __construct(" << long_name
               << "If $handler) {" << endl;
  }
  if (extends.empty()) {
    f_service_ <<
      indent() << "  $this->handler_ = $handler;" << endl;
  } else {
    f_service_ <<
      indent() << "  parent::__construct($handler);" << endl;
  }
  f_service_ <<
    indent() << "  $this->eventHandler_ = new TProcessorEventHandler();"
             << endl;
  f_service_ <<
    indent() << "}" << endl <<
    endl;

  // Generate processor event handler functions
  generate_processor_event_handler_functions(f_service_);

  // Generate the server implementation
  indent(f_service_) <<
    "public function process(TProtocol $input, TProtocol $output) {" << endl;
  indent_up();

  t_name_generator namer;

  f_service_ <<
    indent() << "$rseqid = 0;" << endl <<
    indent() << "$fname = null;" << endl <<
    indent() << "$mtype = 0;" << endl <<
    endl;

  if (binary_inline_) {
    t_field ffname(g_type_string, "fname");
    t_field fmtype(g_type_byte, "mtype");
    t_field fseqid(g_type_i32, "rseqid");
    generate_deserialize_field(f_service_, namer, &ffname, "", true);
    generate_deserialize_field(f_service_, namer, &fmtype, "", true);
    generate_deserialize_field(f_service_, namer, &fseqid, "", true);
  } else {
    f_service_ <<
      indent() << "$input->readMessageBegin($fname, $mtype, $rseqid);" << endl;
  }

  // HOT: check for method implementation
  f_service_ <<
    indent() << "$methodname = 'process_'.$fname;" << endl <<
    indent() << "if (!method_exists($this, $methodname)) {" << endl;
  if (binary_inline_) {
    f_service_ <<
      indent() << "  throw new Exception('Function '.$fname.' not implemented.');" << endl;
  } else {
    f_service_ <<
      indent() << "  $handler_ctx = $this->eventHandler_->getHandlerContext($methodname);" << endl <<
      indent() << "  $this->eventHandler_->preRead($handler_ctx, $methodname, array());" << endl <<
      indent() << "  $input->skip(TType::STRUCT);" << endl <<
      indent() << "  $input->readMessageEnd();" << endl <<
      indent() << "  $this->eventHandler_->postRead($handler_ctx, $methodname, array());" << endl <<
      indent() << "  $x = new TApplicationException('Function '.$fname.' not implemented.', TApplicationException::UNKNOWN_METHOD);" << endl <<
      indent() << "  $this->eventHandler_->handlerError($handler_ctx, $methodname, $x);" << endl <<
      indent() << "  $output->writeMessageBegin($fname, TMessageType::EXCEPTION, $rseqid);" << endl <<
      indent() << "  $x->write($output);" << endl <<
      indent() << "  $output->writeMessageEnd();" << endl <<
      indent() << "  $output->getTransport()->flush();" << endl <<
      indent() << "  return;" << endl;
  }
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
void t_php_generator::generate_process_function(t_service* tservice,
                                                t_function* tfunction) {
  // Open function
  indent(f_service_) <<
    "protected function process_" << tfunction->get_name() <<
    "($seqid, TProtocol $input, TProtocol $output) {" << endl;
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
    indent() << "$bin_accel = ($input instanceof "
             << "TProtocol::$TBINARYPROTOCOLACCELERATED)"
             << " && function_exists('thrift_protocol_read_binary_struct');" << endl <<
    indent() << "$compact_accel = ($input instanceof "
             << "TProtocol::$TCOMPACTPROTOCOLACCELERATED)"
             << " && function_exists('thrift_protocol_read_compact_struct')"
             << " && TCompactProtocolAccelerated::checkVersion(1);" << endl
             << endl <<
    indent() << "if ($bin_accel) $args = thrift_protocol_read_binary_struct("
             << "$input, '" << argsname << "');" << endl <<
    indent() << "else if ($compact_accel) "
             << "$args = thrift_protocol_read_compact_struct($input, '"
             << argsname << "');" << endl <<
    indent() << "else {" << endl <<
    indent() << "  $args = new " << argsname << "();" << endl <<
    indent() << "  $args->read($input);" << endl <<
    indent() << "}" << endl;
  if (!binary_inline_) {
    f_service_ <<
      indent() << "$input->readMessageEnd();" << endl;
  }
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
    "$this->handler_->" << tfunction->get_name() << "(";
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
    indent() << "$bin_accel = ($output instanceof TProtocol::$TBINARYPROTOCOLACCELERATED) && function_exists('thrift_protocol_write_binary');" << endl <<
    indent() << "$compact_accel = ($output instanceof TProtocol::$TCOMPACTPROTOCOLACCELERATED) && function_exists('thrift_protocol_write_compact') && TCompactProtocolAccelerated::checkVersion(1);" << endl;

  f_service_ <<
    indent() << "$this->eventHandler_->preWrite($handler_ctx, '"
             << fn_name << "', $result);" << endl;

  f_service_ <<
    indent() << "if ($bin_accel)" << endl;
  scope_up(f_service_);

  f_service_ <<
    indent() << "thrift_protocol_write_binary($output, '"
             << tfunction->get_name()
             << "', $reply_type, $result, $seqid, $output->isStrictWrite());"
             << endl;

  scope_down(f_service_);
  f_service_ <<
    indent() << "else if ($compact_accel)" << endl;
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
  if (binary_inline_) {
    f_service_ <<
      indent() << "$buff = pack('N', (0x80010000 | $reply_type)); " << endl <<
      indent() << "$buff .= pack('N', strlen('" << tfunction->get_name() << "'));" << endl <<
      indent() << "$buff .= '" << tfunction->get_name() << "';" << endl <<
      indent() << "$buff .= pack('N', $seqid);" << endl <<
      indent() << "$result->write($buff);" << endl <<
      indent() << "$output->write($buff);" << endl <<
      indent() << "$output->flush();" << endl;
  } else {
    f_service_ <<
      indent() << "$output->writeMessageBegin(\"" << tfunction->get_name()
               << "\", $reply_type, $seqid);" << endl <<
      indent() << "$result->write($output);" << endl <<
      indent() << "$output->writeMessageEnd();" << endl <<
      indent() << "$output->getTransport()->flush();" << endl;
  }

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
void t_php_generator::generate_service_helpers(t_service* tservice) {
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
void t_php_generator::generate_php_function_helpers(t_function* tfunction) {
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

    generate_php_struct_definition(f_service_, &result, false);
  }
}

/**
 * Generates the docstring for a generic object.
 */
void t_php_generator::generate_php_docstring(ofstream& out,
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
void t_php_generator::generate_php_docstring(ofstream& out,
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
void t_php_generator::generate_php_docstring_args(ofstream& out,
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
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 * @param mangle Generate mangled service classes
 */
void t_php_generator::generate_service_interface(t_service* tservice,
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
void t_php_generator::generate_service_rest(t_service* tservice, bool mangle) {
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
      indent() << "protected $impl_;" << endl <<
      endl;
  }

  if (ducktyping_) {
    f_service_ <<
      indent() << "public function __construct($impl) {" << endl;
  } else {
    f_service_ <<
      indent() << "public function __construct(" << long_name
               << "If $impl) {" << endl;
  }
  f_service_ <<
    indent() << "  $this->impl_ = $impl;" << endl <<
    indent() << "}" << endl <<
    endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(f_service_) <<
      "public function " << (*f_iter)->get_name() << "($request) {" << endl;
    indent_up();
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
          indent() << "$" << (*a_iter)->get_name() << " = isset(" << req
                   << ") ? " << cast << req << " : null;" << endl;
      }
      if (atype->is_string() &&
          ((t_base_type*)atype)->is_string_list()) {
        f_service_ <<
          indent() << "$" << (*a_iter)->get_name() << " = explode(',', $"
                   << (*a_iter)->get_name() << ");" << endl;
      } else if (atype->is_map() || atype->is_list()) {
        f_service_ <<
          indent() << "$" << (*a_iter)->get_name() << " = json_decode($"
                   << (*a_iter)->get_name() << ", true);" << endl;
      } else if (atype->is_set()) {
        f_service_ <<
          indent() << "$" << (*a_iter)->get_name()
                   << " = array_fill_keys(json_decode($"
                   << (*a_iter)->get_name() << ", true), 1);" << endl;
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
    f_service_ <<
      indent() << "return $this->impl_->" << (*f_iter)->get_name() <<
      "(" << argument_list((*f_iter)->get_arglist()) << ");" << endl;
    indent_down();
    indent(f_service_) <<
      "}" << endl <<
      endl;
  }
  indent_down();
  f_service_ <<
    "}" << endl << endl;
}

void t_php_generator::generate_service_client(t_service* tservice,
        bool mangle) {
  if (autoload_) {
    // Make output file
    ofstream autoload_out;
    string long_name = php_servicename_mangle(mangle, tservice);
    string f_struct = program_name_+"."+long_name+".client.php";
    string f_struct_name = get_out_dir()+f_struct;
    autoload_out.open(f_struct_name.c_str());
    record_genfile(f_struct_name);
    autoload_out << "<?php" << endl
      << autogen_comment() << php_includes() << include_types();
    _generate_service_client(autoload_out, tservice, mangle);
    autoload_out << endl << "?>" << endl;
    autoload_out.close();

    f_types_ << "$GLOBALS['THRIFT_AUTOLOAD']['"
             << lowercase(long_name + "Client") << "'] = '"
             << php_path(program_) << "/" << f_struct << "';" << endl;
  } else {
    _generate_service_client(f_service_, tservice, mangle);
  }
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_php_generator::_generate_service_client(
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
      indent() << "protected $input_ = null;" << endl <<
      indent() << "protected $output_ = null;" << endl;
    if (async_) {
      out <<
        indent() << "protected $asyncHandler_ = null;" << endl <<
        indent() << "protected $eventHandler_ = null;" << endl;
    }
    out << endl <<
      indent() << "protected $seqid_ = 0;" << endl <<
      endl;
  }

  // Constructor function
  indent(out) << "public function __construct("
      << "TProtocol $input, /*?TProtocol*/ $output=null) {" << endl;
  indent_up();
  if (!extends.empty()) {
    out <<
      indent() << "parent::__construct($input, $output);" << endl;
  } else {
    out <<
      indent() << "$this->input_ = $input;" << endl <<
      indent() << "$this->output_ = $output ? $output : $input;" << endl;
    if (async_) {
      out <<
        indent() << "$this->asyncHandler_ = new TClientAsyncHandler();" << endl;
    }
    out <<
      indent() << "$this->eventHandler_ = new TClientEventHandler();" << endl;
  }
  indent_down();
  out <<
    indent() << "}" << endl << endl;

  generate_client_event_handler_functions(out);

  if (async_) {
    out <<
      indent() << "public function setAsyncHandler(TClientAsyncHandler $async_handler) {" << endl <<
      indent() << "  $this->asyncHandler_ = $async_handler;" << endl <<
      indent() << "  return $this;" << endl <<
      indent() << "}" << endl <<
      endl;

    out <<
      indent() << "public function getAsyncHandler() {" << endl <<
      indent() << "  return $this->asyncHandler_;" << endl <<
      indent() << "}" << endl <<
      endl;
  }

  // Generate the function to get the next sequence number
  out <<
    indent() << "private function getsequenceid() {" << endl <<
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
      "public function " << function_signature(*f_iter) << endl;
    scope_up(out);
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

    if (async_) {
      // Gen function
      indent(out) <<
        "public async function " << function_signature(*f_iter, "gen_") << endl;
      scope_up(out);
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
    }

    indent(out) <<
      "public function send_" << function_signature(*f_iter) << endl;
    scope_up(out);

      std::string argsname = php_namespace(tservice->get_program()) +
            service_name_ + "_" + (*f_iter)->get_name() + "_args";

      out <<
        indent() << "$currentseqid = $this->getsequenceid();" << endl <<
        indent() << "$args = new " << argsname << "();" << endl;

      for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
        out <<
          indent() << "$args->" << (*fld_iter)->get_name() << " = $"
                   << (*fld_iter)->get_name() << ";" << endl;
      }

      out << indent() << "try {" << endl;
      indent_up();
      out <<
        indent() << "$this->eventHandler_->preSend('" << (*f_iter)->get_name()
                 << "', $args, $currentseqid);" << endl <<
        indent() << "$bin_accel = ($this->output_ instanceof "
                 << "TProtocol::$TBINARYPROTOCOLACCELERATED) && "
                 << "function_exists('thrift_protocol_write_binary');"
                 << endl <<
        indent() << "$compact_accel = ($this->output_ instanceof "
                 << "TProtocol::$TCOMPACTPROTOCOLACCELERATED) && "
                 << "function_exists('thrift_protocol_write_compact') && "
                 << "TCompactProtocolAccelerated::checkVersion(1);" << endl;

      out <<
        indent() << "if ($bin_accel)" << endl;
      scope_up(out);

      out <<
        indent() << "thrift_protocol_write_binary($this->output_, '" <<
        (*f_iter)->get_name() << "', " <<
        "TMessageType::CALL, $args, $currentseqid, " <<
        "$this->output_->isStrictWrite(), " <<
        ((*f_iter)->is_oneway() ? "true" : "false") << ");" << endl;

      scope_down(out);
      out <<
        indent() << "else if ($compact_accel)" << endl;
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

      // Serialize the request header and write to the stream
      if (binary_inline_) {
        out <<
          indent() << "$buff = pack('N', (0x80010000 | TMessageType::CALL));"
                   << endl <<
          indent() << "$buff .= pack('N', strlen('" << funname << "'));"
                   << endl <<
          indent() << "$buff .= '" << funname << "';" << endl <<
          indent() << "$buff .= pack('N', $currentseqid);" << endl <<
          indent() << "$args->write($buff);" << endl <<
          indent() << "$this->output_->write($buff);" << endl <<
          indent() << "$this->output_->flush();" << endl;
      } else {
        out <<
          indent() << "$this->output_->writeMessageBegin('" <<
          (*f_iter)->get_name() <<
          "', TMessageType::CALL, $currentseqid);" << endl <<
          indent() << "$args->write($this->output_);" << endl <<
          indent() << "$this->output_->writeMessageEnd();" << endl;
        if ((*f_iter)->is_oneway()) {
          out << indent() << "$this->output_->getTransport()->onewayFlush();" << endl;
        } else {
          out << indent() << "$this->output_->getTransport()->flush();" << endl;
        }
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

    out <<
      indent() << "return $currentseqid;" << endl;

    scope_down(out);


    if (!(*f_iter)->is_oneway()) {
      std::string resultname = php_namespace(tservice->get_program()) +
            service_name_ + "_" + (*f_iter)->get_name() + "_result";
      t_struct noargs(program_);

      t_function recv_function((*f_iter)->get_returntype(),
                               string("recv_") + (*f_iter)->get_name(),
                               &noargs);
      // Open function
      out <<
        endl <<
        indent() << "public function " <<
        function_signature(&recv_function, "", "$expectedsequenceid = null") <<
        endl;
      scope_up(out);

      t_name_generator namer;

      out <<
        indent() << "try {" << endl;
      indent_up();

      out <<
        indent() << "$this->eventHandler_->preRecv('" << (*f_iter)->get_name()
                 << "', $expectedsequenceid);" << endl <<
        indent() << "$bin_accel = ($this->input_ instanceof "
                 << "TProtocol::$TBINARYPROTOCOLACCELERATED)"
                 << " && function_exists('thrift_protocol_read_binary');" << endl <<
        indent() << "$compact_accel = ($this->input_ instanceof "
                 << "TProtocol::$TCOMPACTPROTOCOLACCELERATED)"
                 << " && function_exists('thrift_protocol_read_compact')"
                 << " && TCompactProtocolAccelerated::checkVersion(1);" << endl;

      out <<
        indent() << "if ($bin_accel) $result = thrift_protocol_read_binary("
                 << "$this->input_, '" << resultname
                 << "', $this->input_->isStrictRead());" << endl;
      out <<
        indent() << "else if ($compact_accel) "
                 << "$result = thrift_protocol_read_compact($this->input_, '"
                 << resultname << "');" << endl;

      out <<
        indent() << "else" << endl;
      scope_up(out);

      out <<
        indent() << "$rseqid = 0;" << endl <<
        indent() << "$fname = null;" << endl <<
        indent() << "$mtype = 0;" << endl <<
        endl;

      if (binary_inline_) {
        t_field ffname(g_type_string, "fname");
        t_field fseqid(g_type_i32, "rseqid");
        out <<
          indent() << "$ver = unpack('N', $this->input_->readAll(4));" << endl <<
          indent() << "$ver = $ver[1];" << endl <<
          indent() << "$mtype = $ver & 0xff;" << endl <<
          indent() << "$ver = $ver & 0xffff0000;" << endl <<
          indent() << "if ($ver != 0x80010000) "
                   << "throw new TProtocolException('Bad version identifier: "
                   << "'.$ver, TProtocolException::BAD_VERSION);" << endl;
        generate_deserialize_field(out, namer, &ffname, "", true);
        generate_deserialize_field(out, namer, &fseqid, "", true);
      } else {
        out <<
          indent() << "$this->input_->readMessageBegin($fname, $mtype, "
                   << "$rseqid);" << endl <<
          indent() << "if ($mtype == TMessageType::EXCEPTION) {" << endl <<
          indent() << "  $x = new TApplicationException();" << endl <<
          indent() << "  $x->read($this->input_);" << endl <<
          indent() << "  $this->input_->readMessageEnd();" << endl <<
          indent() << "  throw $x;" << endl <<
          indent() << "}" << endl;
      }

      out <<
        indent() << "$result = new " << resultname << "();" << endl <<
        indent() << "$result->read($this->input_);" << endl;

      if (!binary_inline_) {
        out <<
          indent() << "$this->input_->readMessageEnd();" << endl;
      }

      out <<
        indent() <<
        "if (isset($expectedsequenceid) && ($rseqid != $expectedsequenceid)) {"
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
        indent() << "    return $ex->result;" << endl <<
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
      out << indent() << "}" << endl;

      // Careful, only return result if not a void function
      if (!(*f_iter)->get_returntype()->is_void()) {
        out <<
          indent() << "if ($result->success !== null) {" << endl <<
          indent() << "  $this->eventHandler_->postRecv('" << (*f_iter)->get_name()
                   << "', $expectedsequenceid, $result->success);" << endl <<
          indent() << "  return $result->success;" << endl <<
          indent() << "}" << endl;
      }

      t_struct* xs = (*f_iter)->get_xceptions();
      const std::vector<t_field*>& xceptions = xs->get_members();
      vector<t_field*>::const_iterator x_iter;
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        out <<
          indent() << "if ($result->" << (*x_iter)->get_name() << " !== null) {"
                   << endl <<
          indent() << "  $this->eventHandler_->recvException('" << (*f_iter)->get_name()
                   << "', $expectedsequenceid, $result->" << (*x_iter)->get_name()
                   << ");" << endl <<
          indent() << "  throw $result->" << (*x_iter)->get_name() << ";"
                   << endl <<
          indent() << "}" << endl;
      }

      // Careful, only return _result if not a void function
      if ((*f_iter)->get_returntype()->is_void()) {
        indent(out) << "$this->eventHandler_->postRecv('" << (*f_iter)->get_name()
                    << "', $expectedsequenceid, null);" << endl <<
        indent() <<
          "return;" << endl;
      } else {
        out <<
          indent()
            << "$x = new TApplicationException(\""
            << (*f_iter)->get_name() << " failed: unknown result\""
            << ", TApplicationException::MISSING_RESULT"
            << ");" << endl <<
          indent() << "$this->eventHandler_->recvError('" << (*f_iter)->get_name()
                   << "', $expectedsequenceid, $x);" << endl <<
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
void t_php_generator::generate_deserialize_field(ofstream& out,
                                                 t_name_generator& namer,
                                                 t_field* tfield,
                                                 string prefix,
                                                 bool inclass) {
  t_type* type = get_true_type(tfield->get_type());

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " +
      prefix + tfield->get_name();
  }

  string name = prefix + tfield->get_name();

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out,
                                (t_struct*)type,
                                 name);
  } else {

    if (type->is_container()) {
      generate_deserialize_container(out, namer, type, name);
    } else if (type->is_base_type() || type->is_enum()) {

      if (binary_inline_) {
        std::string itrans = (inclass ? "$this->input_" : "$input");

        if (type->is_base_type()) {
          t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
          switch (tbase) {
          case t_base_type::TYPE_VOID:
            throw "compiler error: cannot serialize void field in a struct: " +
              name;
            break;
          case t_base_type::TYPE_STRING:
            out <<
              indent() << "$len = unpack('N', " << itrans << "->readAll(4));" << endl <<
              indent() << "$len = $len[1];" << endl <<
              indent() << "if ($len > 0x7fffffff) {" << endl <<
              indent() << "  $len = 0 - (($len - 1) ^ 0xffffffff);" << endl <<
              indent() << "}" << endl <<
              indent() << "$" << name << " = " << itrans << "->readAll($len);" << endl;
            break;
          case t_base_type::TYPE_BOOL:
            out <<
              indent() << "$" << name << " = unpack('c', " << itrans << "->readAll(1));" << endl <<
              indent() << "$" << name << " = (bool)$" << name << "[1];" << endl;
            break;
          case t_base_type::TYPE_BYTE:
            out <<
              indent() << "$" << name << " = unpack('c', " << itrans << "->readAll(1));" << endl <<
              indent() << "$" << name << " = $" << name << "[1];" << endl;
            break;
          case t_base_type::TYPE_I16:
            out <<
              indent() << "$val = unpack('n', " << itrans << "->readAll(2));" << endl <<
              indent() << "$val = $val[1];" << endl <<
              indent() << "if ($val > 0x7fff) {" << endl <<
              indent() << "  $val = 0 - (($val - 1) ^ 0xffff);" << endl <<
              indent() << "}" << endl <<
              indent() << "$" << name << " = $val;" << endl;
            break;
          case t_base_type::TYPE_I32:
            out <<
              indent() << "$val = unpack('N', " << itrans << "->readAll(4));" << endl <<
              indent() << "$val = $val[1];" << endl <<
              indent() << "if ($val > 0x7fffffff) {" << endl <<
              indent() << "  $val = 0 - (($val - 1) ^ 0xffffffff);" << endl <<
              indent() << "}" << endl <<
              indent() << "$" << name << " = $val;" << endl;
            break;
          case t_base_type::TYPE_I64:
            out <<
              indent() << "$arr = unpack('N2', " << itrans << "->readAll(8));" << endl <<
              indent() << "if ($arr[1] & 0x80000000) {" << endl <<
              indent() << "  $arr[1] = $arr[1] ^ 0xFFFFFFFF;" << endl <<
              indent() << "  $arr[2] = $arr[2] ^ 0xFFFFFFFF;" << endl <<
              indent() << "  $" << name << " = 0 - $arr[1]*4294967296 - $arr[2] - 1;" << endl <<
              indent() << "} else {" << endl <<
              indent() << "  $" << name << " = $arr[1]*4294967296 + $arr[2];" << endl <<
              indent() << "}" << endl;
            break;
          case t_base_type::TYPE_DOUBLE:
            out <<
              indent() << "$arr = unpack('d', strrev(" << itrans << "->readAll(8)));" << endl <<
              indent() << "$" << name << " = $arr[1];" << endl;
            break;
          case t_base_type::TYPE_FLOAT:
            out <<
              indent() << "$arr = unpack('f', strrev(" << itrans << "->readAll(4)));" << endl <<
              indent() << "$" << name << " = $arr[1];" << endl;
            break;
          default:
            throw "compiler error: no PHP name for base type " + t_base_type::t_base_name(tbase) + tfield->get_name();
          }
        } else if (type->is_enum()) {
            out <<
              indent() << "$val = unpack('N', " << itrans << "->readAll(4));" << endl <<
              indent() << "$val = $val[1];" << endl <<
              indent() << "if ($val > 0x7fffffff) {" << endl <<
              indent() << "  $val = 0 - (($val - 1) ^ 0xffffffff);" << endl <<
              indent() << "}" << endl <<
              indent() << "$" << name << " = $val;" << endl;
        }
      } else {

        indent(out) <<
          "$xfer += $input->";

        if (type->is_base_type()) {
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
          out << "readI32($" << name << ");";
        }
        out << endl;
      }
    } else {
      printf("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
             tfield->get_name().c_str(), type->get_name().c_str());
    }
  }
}

/**
 * Generates an unserializer for a variable. This makes two key assumptions,
 * first that there is a const char* variable named data that points to the
 * buffer for deserialization, and that there is a variable protocol which
 * is a reference to a TProtocol serialization object.
 */
void t_php_generator::generate_deserialize_struct(ofstream& out,
                                                  t_struct* tstruct,
                                                  string prefix) {
  out <<
    indent() << "$" << prefix << " = new " << php_namespace(tstruct->get_program()) << tstruct->get_name() << "();" << endl <<
    indent() << "$xfer += $" << prefix << "->read($input);" << endl;
}

void t_php_generator::generate_deserialize_container(ofstream& out,
                                                     t_name_generator& namer,
                                                     t_type* ttype,
                                                     string prefix) {
  string size = namer("_size");
  string ktype = namer("_ktype");
  string vtype = namer("_vtype");
  string etype = namer("_etype");

  t_field fsize(g_type_i32, size);
  t_field fktype(g_type_byte, ktype);
  t_field fvtype(g_type_byte, vtype);
  t_field fetype(g_type_byte, etype);

  out <<
    indent() << "$" << prefix << " = array();" << endl <<
    indent() << "$" << size << " = 0;" << endl;

  // Declare variables, read header
  if (ttype->is_map()) {
    out <<
      indent() << "$" << ktype << " = 0;" << endl <<
      indent() << "$" << vtype << " = 0;" << endl;
    if (binary_inline_) {
      generate_deserialize_field(out, namer, &fktype);
      generate_deserialize_field(out, namer, &fvtype);
      generate_deserialize_field(out, namer, &fsize);
    } else {
      out <<
        indent() << "$xfer += $input->readMapBegin(" <<
        "$" << ktype << ", $" << vtype << ", $" << size << ");" << endl;
    }
  } else if (ttype->is_set()) {
    if (binary_inline_) {
      generate_deserialize_field(out, namer, &fetype);
      generate_deserialize_field(out, namer, &fsize);
    } else {
      out <<
        indent() << "$" << etype << " = 0;" << endl <<
        indent() << "$xfer += $input->readSetBegin(" <<
        "$" << etype << ", $" << size << ");" << endl;
    }
  } else if (ttype->is_list()) {
    if (binary_inline_) {
      generate_deserialize_field(out, namer, &fetype);
      generate_deserialize_field(out, namer, &fsize);
    } else {
      out <<
        indent() << "$" << etype << " = 0;" << endl <<
        indent() << "$xfer += $input->readListBegin(" <<
        "$" << etype << ", $" << size << ");" << endl;
    }
  }

  // For loop iterates over elements
  string i = namer("_i");
  indent(out) <<
    "for ($" <<
    i << " = 0; $" << size << " === null || $" << i << " < $" << size << "; ++$" << i << ")" << endl;

    scope_up(out);

    if (ttype->is_map()) {
      generate_deserialize_map_element(
          out, namer, (t_map*)ttype, size, prefix);
    } else if (ttype->is_set()) {
      generate_deserialize_set_element(
          out, namer, (t_set*)ttype, size, prefix);
    } else if (ttype->is_list()) {
      generate_deserialize_list_element(
          out, namer, (t_list*)ttype, size, prefix);
    }

    scope_down(out);

  if (!binary_inline_) {
    // Read container end
    if (ttype->is_map()) {
      indent(out) << "$xfer += $input->readMapEnd();" << endl;
    } else if (ttype->is_set()) {
      indent(out) << "$xfer += $input->readSetEnd();" << endl;
    } else if (ttype->is_list()) {
      indent(out) << "$xfer += $input->readListEnd();" << endl;
    }
  }
}


/**
 * Generates code to deserialize a map
 */
void t_php_generator::generate_deserialize_map_element(
    ofstream& out,
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

  indent(out) <<
    declare_field(&fkey, true, true) << endl;
  indent(out) <<
    declare_field(&fval, true, true) << endl;

  generate_deserialize_field(out, namer, &fkey);
  generate_deserialize_field(out, namer, &fval);

  indent(out) <<
    "$" << prefix << "[$" << key << "] = $" << val << ";" << endl;
}

void t_php_generator::generate_deserialize_set_element(
    ofstream& out,
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

  indent(out) <<
    "$" << elem << " = null;" << endl;

  generate_deserialize_field(out, namer, &felem);

  indent(out) <<
    "$" << prefix << "[$" << elem << "] = true;" << endl;
}

void t_php_generator::generate_deserialize_list_element(
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

  indent(out) <<
    elem << " = null;" << endl;

  generate_deserialize_field(out, namer, &felem);

  indent(out) <<
    "$" << prefix << " []= $" << elem << ";" << endl;
}


/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_php_generator::generate_serialize_field(ofstream& out,
                                               t_name_generator& namer,
                                               t_field* tfield,
                                               string prefix) {
  t_type* type = get_true_type(tfield->get_type());

  // Do nothing for void types
  if (type->is_void()) {
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " +
      prefix + tfield->get_name();
  }

  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct(out,
                              namer,
                              (t_struct*)type,
                              prefix + tfield->get_name());
  } else if (type->is_container()) {
    generate_serialize_container(out,
                                 namer,
                                 type,
                                 prefix + tfield->get_name());
  } else if (type->is_base_type() || type->is_enum()) {

    string name = prefix + tfield->get_name();

    if (binary_inline_) {
      if (type->is_base_type()) {
        t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
        switch (tbase) {
        case t_base_type::TYPE_VOID:
          throw
            "compiler error: cannot serialize void field in a struct: " + name;
          break;
        case t_base_type::TYPE_STRING:
          out <<
            indent() << "$output .= pack('N', strlen($" << name << "));" << endl <<
            indent() << "$output .= $" << name << ";" << endl;
          break;
        case t_base_type::TYPE_BOOL:
          out <<
            indent() << "$output .= pack('c', $" << name << " ? 1 : 0);" << endl;
          break;
        case t_base_type::TYPE_BYTE:
          out <<
            indent() << "$output .= pack('c', $" << name << ");" << endl;
          break;
        case t_base_type::TYPE_I16:
          out <<
            indent() << "$output .= pack('n', $" << name << ");" << endl;
          break;
        case t_base_type::TYPE_I32:
          out <<
            indent() << "$output .= pack('N', $" << name << ");" << endl;
          break;
        case t_base_type::TYPE_I64:
          out <<
            indent() << "$output .= pack('N2', $" << name << " >> 32, $" << name << " & 0xFFFFFFFF);" << endl;
          break;
        case t_base_type::TYPE_DOUBLE:
          out <<
            indent() << "$output .= strrev(pack('d', $" << name << "));" << endl;
          break;
        case t_base_type::TYPE_FLOAT:
          out <<
            indent() << "$output .= strrev(pack('f', $" << name << "));" << endl;
          break;
        default:
          throw "compiler error: no PHP name for base type " + t_base_type::t_base_name(tbase);
        }
      } else if (type->is_enum()) {
        out <<
          indent() << "$output .= pack('N', $" << name << ");" << endl;
      }
    } else {

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
    }
  } else {
    printf("DO NOT KNOW HOW TO SERIALIZE FIELD '%s%s' TYPE '%s'\n",
           prefix.c_str(),
           tfield->get_name().c_str(),
           type->get_name().c_str());
  }
}

/**
 * Serializes all the members of a struct.
 *
 * @param tstruct The struct to serialize
 * @param prefix  String prefix to attach to all fields
 */
void t_php_generator::generate_serialize_struct(ofstream& out,
                                                t_name_generator& namer,
                                                t_struct* /*tstruct*/,
                                                string prefix) {
  indent(out) <<
    "$xfer += $" << prefix << "->write($output);" << endl;
}

/**
 * Writes out a container
 */
void t_php_generator::generate_serialize_container(ofstream& out,
                                                   t_name_generator& namer,
                                                   t_type* ttype,
                                                   string prefix) {
  scope_up(out);

  if (ttype->is_map()) {
    if (binary_inline_) {
      out <<
        indent() << "$output .= pack('c', " << type_to_enum(((t_map*)ttype)->get_key_type()) << ");" << endl <<
        indent() << "$output .= pack('c', " << type_to_enum(((t_map*)ttype)->get_val_type()) << ");" << endl <<
        indent() << "$output .= strrev(pack('l', count($" << prefix << ")));" << endl;
    } else {
      indent(out) <<
        "$output->writeMapBegin(" <<
        type_to_enum(((t_map*)ttype)->get_key_type()) << ", " <<
        type_to_enum(((t_map*)ttype)->get_val_type()) << ", " <<
        "count($" << prefix << "));" << endl;
    }
  } else if (ttype->is_set()) {
    if (binary_inline_) {
      out <<
        indent() << "$output .= pack('c', " << type_to_enum(((t_set*)ttype)->get_elem_type()) << ");" << endl <<
        indent() << "$output .= strrev(pack('l', count($" << prefix << ")));" << endl;

    } else {
      indent(out) <<
        "$output->writeSetBegin(" <<
        type_to_enum(((t_set*)ttype)->get_elem_type()) << ", " <<
        "count($" << prefix << "));" << endl;
    }
  } else if (ttype->is_list()) {
    if (binary_inline_) {
      out <<
        indent() << "$output .= pack('c', " << type_to_enum(((t_list*)ttype)->get_elem_type()) << ");" << endl <<
        indent() << "$output .= strrev(pack('l', count($" << prefix << ")));" << endl;

    } else {
      indent(out) <<
        "$output->writeListBegin(" <<
        type_to_enum(((t_list*)ttype)->get_elem_type()) << ", " <<
        "count($" << prefix << "));" << endl;
    }
  }

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
    indent(out) <<
      "foreach ($" << prefix << " as $" << iter << " => $true)" << endl;
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

  if (!binary_inline_) {
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

  scope_down(out);
}

/**
 * Serializes the members of a map.
 *
 */
void t_php_generator::generate_serialize_map_element(ofstream& out,
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
void t_php_generator::generate_serialize_set_element(ofstream& out,
                                                     t_name_generator& namer,
                                                     t_set* tset,
                                                     string iter) {
  t_field efield(tset->get_elem_type(), iter);
  generate_serialize_field(out, namer, &efield, "");
}

/**
 * Serializes the members of a list.
 */
void t_php_generator::generate_serialize_list_element(ofstream& out,
                                                      t_name_generator& namer,
                                                      t_list* tlist,
                                                      string iter) {
  t_field efield(tlist->get_elem_type(), iter);
  generate_serialize_field(out, namer, &efield, "");
}

/**
 * Declares a field, which may include initialization as necessary.
 *
 * @param ttype The type
 * @param init iff initialize the field
 * @param obj iff the field is an object
 * @param thrift iff the object is a thrift object
 */
string t_php_generator::declare_field(t_field* tfield, bool init,
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
        throw "compiler error: no PHP initializer for base type "
              + t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      result += " = 0";
    } else if (type->is_container()) {
      result += " = array()";
    } else if (type->is_struct() || type->is_xception()) {
      if (obj) {
        if (thrift) {
          //A thrift object requires an array in __construct__
          result += " = new " + php_namespace(type->get_program())
                 + type->get_name() + "(array())";
        } else {
          result += " = new " + php_namespace(type->get_program())
                 + type->get_name() + "()";
        }
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
string t_php_generator::function_signature(t_function* tfunction,
                                           string prefix,
                                           string moreparameters) {
  return
    prefix + tfunction->get_name() +
    "(" + argument_list(tfunction->get_arglist(), moreparameters) + ")";
}

/**
 * Renders a field list
 */
string t_php_generator::argument_list(t_struct* tstruct,
                                      string moreparameters) {
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
string t_php_generator::type_to_cast(t_type* type) {
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
      return "(double)";
    case t_base_type::TYPE_STRING:
      return "(string)";
    default:
      return "";
    }
  } else if (type->is_enum()) {
    return "(int)";
  }
  return "";
}

/**
 * Converts the parse type to a C++ enum string for the given type.
 */
string t_php_generator ::type_to_enum(t_type* type) {
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

THRIFT_REGISTER_GENERATOR(php, "PHP",
"    inlined:         Generate PHP inlined files.\n"
"    server:          Generate PHP server stubs.\n"
"    autoload:        Generate PHP with autoload.\n"
"    norequires:      Generate PHP with no require_once/include_once calls.\n"
"    oop:             Generate PHP with object oriented subclasses.\n"
"    rest:            Generate PHP REST processors.\n"
"    ducktyping:      Generate processor constructors without explicit types.\n"
"    hphpenum:        Generate enums that extend HPHP Enum.\n"
"    async:           Generate async methods for Hack.\n"
"    json:            Generate functions to parse JSON into thrift struct.\n"
"    mangledsvcs      Generate services with namespace mangling.\n"
"    unmangledsvcs    Generate services without namespace mangling.\n"
);
