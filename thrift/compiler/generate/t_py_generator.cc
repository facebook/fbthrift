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
#include <vector>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>
#include <algorithm>
#include <set>
#include "thrift/compiler/generate/t_generator.h"
#include "thrift/compiler/platform.h"
using namespace std;

// All other Python keywords (as of 2.7) are reserved by the Thrift
// compiler.
static const char* py_reserved_keywords[] = {
  "from",
};

/**
 * Python code generator.
 *
 */
class t_py_generator : public t_generator {
 public:
  t_py_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
    : t_generator(program)
  {
    std::map<std::string, std::string>::const_iterator iter;

    iter = parsed_options.find("json");
    gen_json_ = (iter != parsed_options.end());

    iter = parsed_options.find("new_style");
    gen_newstyle_ = (iter != parsed_options.end());

    iter = parsed_options.find("slots");
    gen_slots_ = (iter != parsed_options.end());

    iter = parsed_options.find("asyncio");
    gen_asyncio_ = (iter != parsed_options.end());

    iter = parsed_options.find("twisted");
    gen_twisted_ = (iter != parsed_options.end());

    iter = parsed_options.find("utf8strings");
    gen_utf8strings_ = (iter != parsed_options.end());

    iter = parsed_options.find("sort_keys");
    sort_keys_ = (iter != parsed_options.end());

    iter = parsed_options.find("thrift_port");
    if (iter != parsed_options.end()) {
      default_port_ = iter->second;
    } else {
      default_port_ = "9090";
    }

    out_dir_base_ = "gen-py";

    for (size_t i = 0;
         i < sizeof py_reserved_keywords / sizeof py_reserved_keywords[0];
         ++i) {
      reserved_keywords_.insert(py_reserved_keywords[i]);
    }
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
  void generate_forward_declaration   (t_struct*   tstruct);
  void generate_xception (t_struct*   txception);
  void generate_service  (t_service*  tservice);

  std::string render_const_value(t_type* type, t_const_value* value);

  /**
   * Struct generation code
   */

  void generate_py_struct(t_struct* tstruct, bool is_exception);
  void generate_py_thrift_spec(std::ofstream& out, t_struct* tstruct, bool is_exception);
  void generate_py_union(std::ofstream& out, t_struct* tstruct);
  void generate_py_struct_definition(std::ofstream& out, t_struct* tstruct,
      bool is_xception=false, bool is_result=false);
  void generate_py_struct_reader(std::ofstream& out, t_struct* tstruct);
  void generate_py_struct_writer(std::ofstream& out, t_struct* tstruct);
  void generate_py_function_helpers(t_function* tfunction);

  /**
   * Service-level generation functions
   */

  void generate_service_helpers   (t_service*  tservice);
  void generate_service_interface (t_service* tservice, bool with_context);
  void generate_service_client    (t_service* tservice);
  void generate_service_remote    (t_service* tservice);
  void generate_service_server    (t_service* tservice, bool with_context);
  void generate_process_function  (t_service* tservice, t_function* tfunction,
                                   bool with_context);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field        (std::ofstream &out,
                                          t_field*    tfield,
                                          std::string prefix="",
                                          bool inclass=false);

  void generate_deserialize_struct       (std::ofstream &out,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_deserialize_container    (std::ofstream &out,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_deserialize_set_element  (std::ofstream &out,
                                          t_set*      tset,
                                          std::string prefix="");

  void generate_deserialize_map_element  (std::ofstream &out,
                                          t_map*      tmap,
                                          std::string prefix="");

  void generate_deserialize_list_element (std::ofstream &out,
                                          t_list*     tlist,
                                          std::string prefix="");

  void generate_serialize_field          (std::ofstream &out,
                                          t_field*    tfield,
                                          std::string prefix="");

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

  void generate_python_docstring         (std::ofstream& out,
                                          t_struct* tstruct);

  void generate_python_docstring         (std::ofstream& out,
                                          t_function* tfunction);

  void generate_python_docstring         (std::ofstream& out,
                                          t_doc*    tdoc,
                                          t_struct* tstruct,
                                          const char* subheader);

  void generate_python_docstring         (std::ofstream& out,
                                          t_doc* tdoc);

  void generate_json_enum            (std::ofstream& out, t_enum* tenum,
                                      const string& prefix_thrift,
                                      const string& prefix_json);
  void generate_json_struct          (std::ofstream& out, t_struct* tstruct,
                                      const string& prefix_thrift,
                                      const string& prefix_json);
  void generate_json_field           (std::ofstream& out, t_field* tfield,
                                      const string& prefix_thrift = "",
                                      const string& suffix_thrift = "",
                                      const string& prefix_json = "",
                                      bool generate_assignment = true);
  void generate_json_container       (std::ofstream& out,
                                      t_type* ttype,
                                      const string& prefix_thrift = "",
                                      const string& prefix_json = "");

  void generate_json_collection_element   (ofstream& out,
                                           t_type* type,
                                           const string& collection,
                                           const string& elem,
                                           const string& action_prefix,
                                           const string& action_suffix,
                                           const string& prefix_json);

  void generate_json_map_key         (ofstream& out,
                                      t_type* type,
                                      const string& parsed_key,
                                      const string& string_key);

  void generate_json_reader          (std::ofstream& out,
                                      t_struct* tstruct);

  /**
   * Helper rendering functions
   */

  std::string py_autogen_comment();
  std::string py_remote_warning();
  std::string py_imports();
  std::string rename_reserved_keywords(const std::string& value);
  std::string render_includes();
  std::string render_fastbinary_includes();
  std::string declare_argument(std::string structname, t_field* tfield);
  std::string render_field_default_value(t_field* tfield);
  std::string type_name(t_type* ttype);
  std::string function_signature(t_function* tfunction, std::string prefix="");
  std::string function_signature_if(t_function* tfunction, bool with_context);
  std::string argument_list(t_struct* tstruct);
  std::string type_to_enum(t_type* ttype);
  std::string type_to_spec_args(t_type* ttype);
  std::string get_real_py_module(const t_program* program);

 private:

  /**
   * True iff we should generate a function parse json to thrift object.
   */
  bool gen_json_;

  /**
   * True iff we should generate new-style classes.
   */
  bool gen_newstyle_;

  /**
   * True iff we should generate __slots__ for thrift structs.
   */
  bool gen_slots_;

  /**
   * True iff we should generate code for asyncio server in Python 3.
   */
  bool gen_asyncio_;

  /**
   * True iff we should generate Twisted-friendly RPC services.
   */
  bool gen_twisted_;

  /**
   * True iff strings should be encoded using utf-8.
   */
  bool gen_utf8strings_;

  /**
   * True iff we serialize maps in the ascending order of ther keys
   */
  bool sort_keys_;

  /**
   * Default port to use.
   */
  std::string default_port_;

  /**
   * File streams
   */

  std::ofstream f_types_;
  std::ofstream f_consts_;
  std::ofstream f_service_;

  std::string package_dir_;

  set<string> reserved_keywords_;
};

std::string t_py_generator::get_real_py_module(const t_program* program) {
  if (gen_asyncio_) {
    std::string asyncio_module = program->get_namespace("py.asyncio");
    if (!asyncio_module.empty()){
      return asyncio_module;
    }
  }

  if (gen_twisted_) {
    std::string twisted_module = program->get_namespace("py.twisted");
    if (!twisted_module.empty()){
      return twisted_module;
    }
  }

  std::string real_module = program->get_namespace("py");
  if (real_module.empty()) {
    return program->get_name();
  }
  return real_module;
}

void t_py_generator::generate_json_field(ofstream& out,
                                          t_field* tfield,
                                          const string& prefix_thrift,
                                          const string& suffix_thrift,
                                          const string& prefix_json,
                                          bool generate_assignment) {
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
        (t_enum*)type,
        name,
        prefix_json);
  } else if (type->is_base_type()) {
    string conversion_function = "";
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    string number_limit = "";
    string number_negative_limit = "";
    switch (tbase) {
      case t_base_type::TYPE_VOID:
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_BOOL:
        break;
      case t_base_type::TYPE_BYTE:
        number_limit = "0x7f";
        number_negative_limit = "-0x80";
        break;
      case t_base_type::TYPE_I16:
        number_limit = "0x7fff";
        number_negative_limit = "-0x8000";
        break;
      case t_base_type::TYPE_I32:
        number_limit = "0x7fffffff";
        number_negative_limit = "-0x80000000";
        break;
      case t_base_type::TYPE_I64:
        conversion_function = "long";
        break;
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        conversion_function = "float";
        break;
      default:
        throw "compiler error: no python reader for base type "
          + t_base_type::t_base_name(tbase) + name;
    }

    string value = prefix_json;
    if (!conversion_function.empty()) {
      value = conversion_function + "(" + value + ")";
    }

    if (generate_assignment) {
      indent(out) << name << " = " << value << endl;
    }

    if (!number_limit.empty()) {
      indent(out) << "if " << name << " > " << number_limit << " or "
        << name << " < " << number_negative_limit << ":"
        << endl;
      indent_up();
      indent(out) << "raise TProtocolException(TProtocolException.INVALID_DATA,"
        << " 'number exceeds limit in field')"
        << endl;
      indent_down();
    }
  } else {
    throw "Compiler did not generate_json_field reader";
  }
}

void t_py_generator::generate_json_struct(ofstream& out,
    t_struct* tstruct,
    const string& prefix_thrift,
    const string& prefix_json) {

  indent(out) << prefix_thrift << " = " << type_name(tstruct) << "()" << endl;
  indent(out) << prefix_thrift << ".readFromJson(" <<
    prefix_json << ", is_text=False)" << endl;
}

void t_py_generator::generate_json_enum(ofstream& out,
    t_enum* tenum,
    const string& prefix_thrift,
    const string& prefix_json) {

  indent(out) << prefix_thrift << " = " << prefix_json << endl;
  indent(out) << "if not " << prefix_thrift << " in "
              << type_name(tenum) << "._VALUES_TO_NAMES:" << endl;
  indent_up();
  indent(out) << "raise TProtocolException(TProtocolException.INVALID_DATA,"
              << " 'enum exceeds limit')" << endl;
  indent_down();
}

void t_py_generator::generate_json_container(ofstream& out,
    t_type* ttype,
    const string& prefix_thrift,
    const string& prefix_json) {

  t_container* tcontainer = (t_container*)ttype;

  if (ttype->is_list()) {
    string e = tmp("_tmp_e");
    indent(out) << prefix_thrift << " = []" << endl;

    indent(out) << "for " << e << " in " << prefix_json << ":" << endl;
    indent_up();
    generate_json_collection_element(
        out,
        ((t_list*)ttype)->get_elem_type(),
        prefix_thrift,
        e,
        ".append(",
        ")",
        prefix_json);
    indent_down();
  } else if (ttype->is_set()) {
    string e = tmp("_tmp_e");
    indent(out) << prefix_thrift << " = set()" << endl;

    indent(out) << "for " << e << " in " << prefix_json << ":" << endl;
    indent_up();
    generate_json_collection_element(
        out,
        ((t_set*)ttype)->get_elem_type(),
        prefix_thrift,
        e,
        ".add(",
        ")",
        prefix_json);
    indent_down();
  } else if (ttype->is_map()) {
    string k = tmp("_tmp_k");
    string v = tmp("_tmp_v");
    string kp = tmp("_tmp_kp");
    indent(out) << prefix_thrift << " = {}" << endl;

    indent(out) << "for " << k << ", " << v << " in "
      << prefix_json << ".items():" << endl;
    indent_up();

    generate_json_map_key(out, ((t_map*)ttype)->get_key_type(), kp, k);

    generate_json_collection_element(
        out,
        ((t_map*)ttype)->get_val_type(),
        prefix_thrift,
        v,
        "[" + kp + "] = ",
        "",
        prefix_json + "[" + kp + "]");
    indent_down();
  }
}

void t_py_generator::generate_json_collection_element(ofstream& out,
                                                t_type* type,
                                                const string& collection,
                                                const string& elem,
                                                const string& action_prefix,
                                                const string& action_suffix,
                                                const string& prefix_json) {
  string to_act_on = elem;
  string to_parse = prefix_json;
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch(tbase) {
      // Explicitly cast into float because there is an asymetry
      // between serializing and deserializing NaN.
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        to_act_on = "float(" + to_act_on + ")";
        break;
      default:
        break;
    }
  } else if (type->is_enum()) {
    to_parse = elem;
    to_act_on = tmp("_enum");
  } else if (type->is_list()) {
    to_parse = elem;
    to_act_on = tmp("_list");
  } else if (type->is_map()) {
    to_parse = elem;
    to_act_on = tmp("_map");
  } else if (type->is_set()) {
    to_parse = elem;
    to_act_on = tmp("_set");
  } else if (type->is_struct()) {
    to_parse = elem;
    to_act_on = tmp("_struct");
  }

  t_field felem(type, to_act_on);
  generate_json_field(out, &felem, "", "", to_parse, false);
  indent(out) << collection << action_prefix << to_act_on << action_suffix
    << endl;
}

void t_py_generator::generate_json_map_key(ofstream& out,
                                           t_type* type,
                                           const string& parsed_key,
                                           const string& raw_key) {
  type = get_true_type(type);
  if (type->is_enum()) {
    indent(out) << parsed_key << " = int(" << raw_key << ")" << endl;
  } else if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    string conversion_function = "";
    string number_limit = "";
    string number_negative_limit = "";
    bool generate_assignment = true;
    switch (tbase) {
      case t_base_type::TYPE_STRING:
        break;
      case t_base_type::TYPE_BOOL:
        indent(out) << "if " << raw_key << " == 'true':" << endl;
        indent_up();
        indent(out) << parsed_key << " = True" << endl;
        indent_down();
        indent(out) << "elif " << raw_key << " == 'false':" << endl;
        indent_up();
        indent(out) << parsed_key << " = False" << endl;
        indent_down();
        indent(out) << "else:" << endl;
        indent_up();
        indent(out) << "raise TProtocolException(TProtocolException."
                    << "INVALID_DATA, 'invalid boolean value' + " << raw_key
                    << ")" << endl;
        indent_down();
        generate_assignment = false;
        break;
      case t_base_type::TYPE_BYTE:
        conversion_function = "int";
        number_limit = "0x7f";
        number_negative_limit = "-0x80";
        break;
      case t_base_type::TYPE_I16:
        conversion_function = "int";
        number_limit = "0x7fff";
        number_negative_limit = "-0x8000";
        break;
      case t_base_type::TYPE_I32:
        conversion_function = "int";
        number_limit = "0x7fffffff";
        number_negative_limit = "-0x80000000";
        break;
      case t_base_type::TYPE_I64:
        conversion_function = "long";
        break;
      case t_base_type::TYPE_DOUBLE:
      case t_base_type::TYPE_FLOAT:
        conversion_function = "float";
        break;
      default:
        throw "compiler error: no C++ reader for base type "
          + t_base_type::t_base_name(tbase);
    }

    string value = raw_key;
    if (!conversion_function.empty()) {
      value = conversion_function + "(" + value + ")";
    }

    if (generate_assignment) {
      indent(out) << parsed_key << " = " << value << endl;
    }

    if (!number_limit.empty()) {
      indent(out) << "if " << parsed_key << " > " << number_limit << " or "
                  << parsed_key << " < " << number_negative_limit << ":"
                  << endl;
      indent_up();
      indent(out) << "raise TProtocolException(TProtocolException.INVALID_DATA,"
                  << " 'number exceeds the limit in key ' + " << raw_key << ")"
                  << endl;
      indent_down();
    }
  } else {
    throw string("compiler error: invalid key type");
  }
}

void t_py_generator::generate_json_reader(ofstream& out,
    t_struct* tstruct) {
  if (!gen_json_) {
    return;
  }

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  indent(out) << "def readFromJson(self, json, is_text=True):"
    << endl;
  indent_up();
  indent(out) << "json_obj = json" << endl;
  indent(out) << "if is_text:" << endl;
  indent_up();
  indent(out) << "json_obj = loads(json)" << endl;
  indent_down();

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    string field = (*f_iter)->get_name();
    indent(out) << "if '" << field << "' in json_obj "
                << "and json_obj['" << field << "'] is not None:"
                << endl;
    indent_up();
    generate_json_field(out, *f_iter, "self.", "",
                        "json_obj['" + (*f_iter)->get_name() + "']");

    indent_down();
    if ((*f_iter)->get_req() == t_field::T_REQUIRED) {
      indent(out) << "else:" << endl;
      indent_up();
      indent(out) << "raise TProtocolException('Required field "
        << (*f_iter)->get_name()
        << " was not found!')"
        << endl;
      indent_down();
    }
  }
  indent_down();
  out << endl;
}

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_py_generator::init_generator() {
  // Make output directory
  string module = get_real_py_module(program_);
  package_dir_ = get_out_dir();
  while (true) {
    // TODO: Do better error checking here.
    MKDIR(package_dir_.c_str());
    std::ofstream init_py((package_dir_+"/__init__.py").c_str());
    init_py << py_autogen_comment();
    init_py.close();
    if (module.empty()) {
      break;
    }
    string::size_type pos = module.find('.');
    if (pos == string::npos) {
      package_dir_ += "/";
      package_dir_ += module;
      module.clear();
    } else {
      package_dir_ += "/";
      package_dir_ += module.substr(0, pos);
      module.erase(0, pos+1);
    }
  }

  // Make output file
  string f_types_name = package_dir_+"/"+"ttypes.py";
  f_types_.open(f_types_name.c_str());
  record_genfile(f_types_name);

  string f_consts_name = package_dir_+"/"+"constants.py";
  f_consts_.open(f_consts_name.c_str());
  record_genfile(f_consts_name);

  string f_init_name = package_dir_+"/__init__.py";
  ofstream f_init;
  f_init.open(f_init_name.c_str());
  record_genfile(f_init_name);
  f_init << py_autogen_comment() <<
    "__all__ = ['ttypes', 'constants'";
  vector<t_service*> services = program_->get_services();
  vector<t_service*>::iterator sv_iter;
  for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
    f_init << ", '" << (*sv_iter)->get_name() << "'";
  }
  f_init << "]" << endl;
  f_init.close();

  // Print header
  f_types_ <<
    py_autogen_comment() << endl <<
    py_imports() << endl <<
    render_includes() << endl <<
    render_fastbinary_includes() <<
    "all_structs = []" << endl <<
    "UTF8STRINGS = bool(" << gen_utf8strings_ << ") or " <<
    "sys.version_info.major >= 3" << endl << endl;

  f_consts_ <<
    py_autogen_comment() << endl <<
    py_imports() << endl << "from .ttypes import *" << endl << endl;
}

/**
 * Ensures the string is not a reserved Python keyword.
 */
string t_py_generator::rename_reserved_keywords(const string& value) {
  if (reserved_keywords_.find(value) != reserved_keywords_.end()) {
    return value + "_PY_RESERVED_KEYWORD";
  } else {
    return value;
  }
}

/**
 * Renders all the imports necessary for including another Thrift program
 */
string t_py_generator::render_includes() {
  const vector<t_program*>& includes = program_->get_includes();
  string result = "";
  for (size_t i = 0; i < includes.size(); ++i) {
    result += "import " + get_real_py_module(includes[i]) +
      ".ttypes\n";
  }
  if (includes.size() > 0) {
    result += "\n";
  }
  return result;
}

/**
 * Renders all the imports necessary to use the accelerated TBinaryProtocol
 */
string t_py_generator::render_fastbinary_includes() {
  // In the try block below, we disable fastbinary if either
  // 'version' is not defined (causing an exception), or if it is defined
  // and has an old value.  Warn if it is an old value.
  return
    "import pprint\n"
    "import warnings\n"
    "from thrift import Thrift\n"
    "from thrift.transport import TTransport\n"
    "from thrift.protocol import TBinaryProtocol\n"
    "from thrift.protocol import TCompactProtocol\n"
    "from thrift.protocol import THeaderProtocol\n"
    "try:\n"
    "  from thrift.protocol import fastbinary\n"
    "  if fastbinary.version < 2:\n"
    "     fastbinary = None\n"
    "     warnings.warn(\"Disabling fastbinary, need at least version 2\")\n"
    "except:\n"
    "  fastbinary = None\n";
}

/**
 * Autogen'd comment
 */
string t_py_generator::py_autogen_comment() {
  return
    std::string("#\n") +
    "# Autogenerated by Thrift\n" +
    "#\n" +
    "# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING\n" +
    "#  @""generated\n" +
    "#\n";
}

/**
 * Print out warning message in the case that *-remote.py is ran instead of
 * running *-remote.par
 */

string t_py_generator::py_remote_warning() {
  return
    "if (not sys.argv[0].endswith(\"par\") and\n"
    "    os.getenv('PAR_UNPACK_TMP') == None):\n"
    "\n"
    "    f = open(sys.argv[0], \"r\")\n"
    "\n"
    "    f.readline() # This will be #!/bin/bash\n"
    "    line = f.readline()\n"
    "    f.close()\n"
    "\n"
    "    # The par generator tool always has '# This par was made' as the\n"
    "    # second line. See fbcode/tools/make_par/make_par.py\n"
    "    if (not line.startswith('# This par was made')):\n"
    "        print(\"\"\"WARNING\n"
    "        You are trying to run *-remote.py which is incorrect as the\n"
    "        paths are not set up correctly. Instead, you should generate\n"
    "        your thrift files with thrift_library and then run the\n"
    "        resulting *-remote.par.\n"
    "        For more information, please read\n"
    "        http://fburl.com/python-remotes\"\"\")\n"
    "        exit()\n";
}

/**
 * Prints standard thrift imports
 */
string t_py_generator::py_imports() {
  string imports = "from __future__ import absolute_import\n";

  imports += "import six\n";
  imports += "from thrift.util.Recursive import fix_spec\n";
  imports += "from thrift.Thrift import *\n";
  imports += "from thrift.protocol.TProtocol import TProtocolException\n\n";

  if (gen_json_) {
    imports += "from json import loads\n";
    imports += "import sys\n";
    imports += "if sys.version_info[0] >= 3:\n";
    imports += "  long = int\n";
  }

  return imports;
}

/**
 * Closes the type files
 */
void t_py_generator::close_generator() {
  // Close types file
  f_types_ << "fix_spec(all_structs)" << endl
           << "del all_structs" << endl;
  f_types_.close();
  f_consts_.close();
}

/**
 * Generates a typedef. This is not done in Python, types are all implicit.
 *
 * @param ttypedef The type definition
 */
void t_py_generator::generate_typedef(t_typedef* ttypedef) {}

/**
 * Generates code for an enumerated type. Done using a class to scope
 * the values.
 *
 * @param tenum The enumeration
 */
void t_py_generator::generate_enum(t_enum* tenum) {
  std::ostringstream to_string_mapping, from_string_mapping;

  f_types_ <<
    "class " << rename_reserved_keywords(tenum->get_name()) <<
    (gen_newstyle_ ? "(object)" : "") <<
    ":" << endl;

  indent_up();
  generate_python_docstring(f_types_, tenum);

  to_string_mapping << indent() << "_VALUES_TO_NAMES = {" << endl;
  from_string_mapping << indent() << "_NAMES_TO_VALUES = {" << endl;

  vector<t_enum_value*> constants = tenum->get_constants();
  vector<t_enum_value*>::iterator c_iter;
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int32_t value = (*c_iter)->get_value();

    f_types_ <<
      indent() << rename_reserved_keywords((*c_iter)->get_name()) << " = "
               << value << endl;

    // Dictionaries to/from string names of enums
    to_string_mapping <<
      indent() << indent() << value << ": \"" <<
      (*c_iter)->get_name() << "\"," << endl;
    from_string_mapping <<
      indent() << indent() << '"' << (*c_iter)->get_name() <<
      "\": " << value << ',' << endl;
  }
  to_string_mapping << indent() << "}" << endl;
  from_string_mapping << indent() << "}" << endl;

  indent_down();
  f_types_ << endl;
  f_types_ << to_string_mapping.str() << endl << from_string_mapping.str() << endl;
}

/**
 * Generate a constant value
 */
void t_py_generator::generate_const(t_const* tconst) {
  t_type* type = tconst->get_type();
  string name = rename_reserved_keywords(tconst->get_name());
  t_const_value* value = tconst->get_value();

  indent(f_consts_) << name << " = " << render_const_value(type, value);
  f_consts_ << endl << endl;
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
string t_py_generator::render_const_value(t_type* type, t_const_value* value) {
  type = get_true_type(type);
  std::ostringstream out;

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      out << "'" << value->get_string() << "'";
      break;
    case t_base_type::TYPE_BOOL:
      out << (value->get_integer() > 0 ? "True" : "False");
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
      throw "compiler error: no const of base type " +
        t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    indent(out) << value->get_integer();
  } else if (type->is_struct() || type->is_xception()) {
    out << rename_reserved_keywords(type->get_name()) << "(**{" << endl;
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
        throw "type error: " + type->get_name() +
          " has no field " + v_iter->first->get_string();
      }
      out << indent();
      out << render_const_value(g_type_string, v_iter->first);
      out << " : ";
      out << render_const_value(field_type, v_iter->second);
      out << "," << endl;
    }
    indent_down();
    indent(out) << "})";
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    out << "{" << endl;
    indent_up();
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(ktype, v_iter->first);
      out << " : ";
      out << render_const_value(vtype, v_iter->second);
      out << "," << endl;
    }
    indent_down();
    indent(out) << "}";
  } else if (type->is_list() || type->is_set()) {
    t_type* etype;
    if (type->is_list()) {
      etype = ((t_list*)type)->get_elem_type();
    } else {
      etype = ((t_set*)type)->get_elem_type();
    }
    if (type->is_set()) {
      out << "set(";
    }
    out << "[" << endl;
    indent_up();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << indent();
      out << render_const_value(etype, *v_iter);
      out << "," << endl;
    }
    indent_down();
    indent(out) << "]";
    if (type->is_set()) {
      out << ")";
    }
  } else {
    throw "CANNOT GENERATE CONSTANT FOR TYPE: " + type->get_name();
  }

  return out.str();
}

void t_py_generator::generate_forward_declaration(t_struct* tstruct) {
  if (!tstruct->is_union()) {
    generate_py_struct(tstruct, tstruct->is_xception());
  }
}

/**
 * Generates a python struct
 */
void t_py_generator::generate_struct(t_struct* tstruct) {
  if (tstruct->is_union()) {
    generate_py_union(f_types_, tstruct);
  }

  generate_py_thrift_spec(f_types_, tstruct, false);
}

/**
 * Generates a struct definition for a thrift exception. Basically the same
 * as a struct but extends the Exception class.
 *
 * @param txception The struct definition
 */
void t_py_generator::generate_xception(t_struct* txception) {
  generate_py_thrift_spec(f_types_, txception, true);
}

/**
 * Generates a python struct
 */
void t_py_generator::generate_py_struct(t_struct* tstruct,
                                        bool is_exception) {
  generate_py_struct_definition(f_types_, tstruct, is_exception);
}

/**
 * Generates a python union definition. We just keep a variable `value`
 * which holds the current value and to know type we use instanceof.
 */
void t_py_generator::generate_py_union(ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();
  const vector<t_field*>& sorted_members = tstruct->get_sorted_members();

  out << "class " << rename_reserved_keywords(tstruct->get_name())
      << "(object):" << endl;

  indent_up();
  generate_python_docstring(out, tstruct);

  out << endl;

  // TODO: make union work with fastbinary
  indent(out) << "thrift_spec = None" << endl;
  if (members.size() != 0) {
    indent(out) << "__init__ = None" << endl << endl;
  }

  // Generate some class level identifiers (similar to enum)
  indent(out) << "__EMPTY__ = 0" << endl;
  for (auto& member: sorted_members) {
    indent(out) << uppercase(member->get_name()) << " = "
                << member->get_key() << endl;
  }
  indent(out) << endl;

  // Generate `isUnion` method
  indent(out) << "@staticmethod" << endl;
  indent(out) << "def isUnion():" << endl;
  indent(out) << "  return True" << endl << endl;

  // Generate `get_` methods
  for (auto& member: members) {
    indent(out) << "def get_" << member->get_name() << "(self):" << endl;
    indent(out) << "  assert self.field == " << member->get_key() << endl;
    indent(out) << "  return self.value" << endl << endl;
  }

  // Generate `set_` methods
  for (auto& member: members) {
    indent(out) << "def set_" << member->get_name() << "(self, value):" << endl;
    indent(out) << "  self.field = " << member->get_key() << endl;
    indent(out) << "  self.value = value" << endl << endl;
  }

  // Method to get the stored type
  indent(out) << "def getType(self):" << endl;
  indent(out) << "  return self.field" << endl;

  // Printing utilities so that on the command line thrift
  // structs look somewhat human readable
  out <<
    indent() << "def __repr__(self):" << endl <<
    indent() << "  L = []" << endl <<
    indent() << "  for key, value in six.iteritems(self.__dict__):" << endl <<
    indent() << "    padding = ' ' * (len(key) + 1)" << endl <<
    indent() << "    value = pprint.pformat(value)" << endl <<
    indent() << "    value = padding.join(value.splitlines(True))" << endl <<
    indent() << "    L.append('    %s=%s' % (key, value))" << endl <<
    indent() << "  return \"%s(\\n%s)\" % (self.__class__.__name__, "
             << "\",\\n\".join(L))" << endl << endl;

  // Generate `read` method
  indent(out) << "def read(self, iprot):" << endl;
  indent_up();

  indent(out) << "self.field = 0" << endl;
  indent(out) << "self.value = None" << endl;
  indent(out) << "iprot.readStructBegin()" << endl;
  indent(out) << "while True:" << endl;
  indent_up();
  indent(out) << "(fname, ftype, fid) = iprot.readFieldBegin()" << endl;
  indent(out) << "if ftype == TType.STOP:" << endl;
  indent_up();
  indent(out) << "break" << endl << endl;
  indent_down();

  bool first = true;
  for (auto& member: sorted_members) {
    auto t = type_to_enum(member->get_type());
    auto n = member->get_name();
    auto k = member->get_key();
    indent(out) << (first ? "" : "el") << "if fid == " << k << ":" << endl;
    indent_up();
    indent(out) << "if ftype == " << t << ":" << endl;
    indent_up();
    generate_deserialize_field(out, member, "");
    indent(out) << "assert self.field == 0 and self.value is None" << endl;
    indent(out) << "self.set_" << n << "(" << rename_reserved_keywords(n)
                << ")" << endl;
    indent_down();
    indent(out) << "else:" << endl;
    indent(out) << "  iprot.skip(ftype)" << endl;
    indent_down();

    first = false;
  }

  indent(out) << "else:" << endl;
  indent(out) << "  iprot.skip(ftype)" << endl;
  indent(out) << "iprot.readFieldEnd()" << endl;
  indent_down();

  indent(out) << "iprot.readStructEnd()" << endl << endl;
  indent_down();

  // Generate `write` method
  indent(out) << "def write(self, oprot):" << endl;
  indent_up();

  indent(out) << "oprot.writeUnionBegin('" << tstruct->get_name()
              << "')" << endl;

  first = true;
  for (auto& member: sorted_members) {
    auto t = type_to_enum(member->get_type());
    auto n = member->get_name();
    auto k = member->get_key();

    indent(out) << (first ? "" : "el") << "if self.field == " << k << ":"
                << endl;
    indent_up();
    indent(out) << "oprot.writeFieldBegin('" << n << "', " << t << ", " << k
                << ")" << endl;

    indent(out) << rename_reserved_keywords(n) << " = self.value" << endl;
    generate_serialize_field(out, member, "");
    indent(out) << "oprot.writeFieldEnd()" << endl;
    indent_down();
  }
  indent(out) << "oprot.writeFieldStop()" << endl;
  indent(out) << "oprot.writeUnionEnd()" << endl;
  indent_down();
  indent(out) << endl;

  // Generate json reader
  if (gen_json_) {
    indent(out) << "def readFromJson(self, json, is_text=True):" << endl;
    indent_up();
    indent(out) << "self.field = 0" << endl;
    indent(out) << "self.value = None" << endl;
    indent(out) << "obj = json" << endl;
    indent(out) << "if is_text:" << endl;
    indent_up();
    indent(out) << "obj = loads(json)" << endl;
    indent_down();

    indent(out) << "if not isinstance(obj, dict) or len(obj) > 1:" << endl;
    indent(out) << "  raise TProtocolException('Can not parse')" << endl;
    indent(out) << endl;

    for (auto& member: members) {
      auto n = member->get_name();
      indent(out) << "if '" << n << "' in obj:" << endl;
      indent_up();
      generate_json_field(out, member, "", "", "obj['" + n + "']");
      indent(out) << "self.set_" << n << "(" << rename_reserved_keywords(n)
                  << ")" << endl;
      indent_down();
    }
    indent_down();
    out << endl;
  }

  // Equality and inequality methods that compare by value
  out <<
    indent() << "def __eq__(self, other):" << endl;
  indent_up();
  out <<
    indent() << "return isinstance(other, self.__class__) and "
                "self.__dict__ == other.__dict__" << endl;
  indent_down();
  out << endl;

  out <<
    indent() << "def __ne__(self, other):" << endl;
  indent_up();
  out <<
    indent() << "return not (self == other)" << endl;
  indent_down();
  out << endl;

  indent_down();
}

void t_py_generator::generate_py_thrift_spec(ofstream& out,
                                             t_struct* tstruct,
                                             bool is_exception) {

  const vector<t_field*>& members = tstruct->get_members();
  const vector<t_field*>& sorted_members = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator m_iter;

  indent(out) << "all_structs.append("
              << rename_reserved_keywords(tstruct->get_name())
              << ")" << endl
              << rename_reserved_keywords(tstruct->get_name())
              << ".thrift_spec = (" << endl;

  indent_up();

  int sorted_keys_pos = 0;
  for (m_iter = sorted_members.begin(); m_iter != sorted_members.end();
      ++m_iter) {

    if (sorted_keys_pos >= 0 && (*m_iter)->get_key() < 0) {
      sorted_keys_pos = (*m_iter)->get_key();
    }

    for (; sorted_keys_pos != (*m_iter)->get_key(); sorted_keys_pos++) {
      indent(out) << "None, # " << sorted_keys_pos << endl;
    }

    indent(out) << "(" << (*m_iter)->get_key() << ", "
          << type_to_enum((*m_iter)->get_type()) << ", "
          << "'" << rename_reserved_keywords((*m_iter)->get_name()) << "'"
          << ", "
          << type_to_spec_args((*m_iter)->get_type()) << ", "
          << render_field_default_value(*m_iter) << ", "
          << (*m_iter)->get_req() << ", "
          << "),"
          << " # " << sorted_keys_pos
          << endl;

    sorted_keys_pos ++;
  }

  indent_down();
  indent(out) << ")" << endl << endl;

  if (members.size() > 0) {
    out <<
      indent() << "def " << rename_reserved_keywords(tstruct->get_name())
               << "__init__(self,";
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      // This fills in default values, as opposed to nulls
      out << " " << declare_argument(
          rename_reserved_keywords(tstruct->get_name()), *m_iter) << ",";
    }
    out << "):" << endl;

    indent_up();
    if (tstruct->is_union()) {
      indent(out) << "self.field = 0" << endl;
      indent(out) << "self.value = None" << endl;

      for (auto& member: sorted_members) {
        indent(out) << "if " << rename_reserved_keywords(member->get_name())
                    << " is not None:" << endl;
        indent(out) << "  assert self.field == 0 and self.value is None"
                    << endl;
        indent(out) << "  self.field = " << member->get_key() << endl;
        indent(out) << "  self.value = "
                    << rename_reserved_keywords(member->get_name()) << endl;
      }
    } else {
      for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        // Initialize fields
        t_type* type = (*m_iter)->get_type();
        if (!type->is_base_type() && !type->is_enum() &&
            (*m_iter)->get_value() != nullptr) {
          indent(out)
            << "if " << rename_reserved_keywords((*m_iter)->get_name())
            << " is self.thrift_spec[" << (*m_iter)->get_key() << "][4]:"
            << endl;
          indent(out)
            << "  " << rename_reserved_keywords((*m_iter)->get_name())
            << " = " << render_field_default_value(*m_iter) << endl;
        }
        indent(out)
          << "self." << rename_reserved_keywords((*m_iter)->get_name())
          << " = " << rename_reserved_keywords((*m_iter)->get_name())
          << endl;
      }
    }
    indent_down();

    out << endl;
    out <<
      indent() << rename_reserved_keywords(tstruct->get_name())
               << ".__init__ = "
               << rename_reserved_keywords(tstruct->get_name())
               << "__init__" << endl << endl;
  }

}

/**
 * Generates a struct definition for a thrift data type.
 *
 * @param tstruct The struct definition
 */
void t_py_generator::generate_py_struct_definition(ofstream& out,
                                                   t_struct* tstruct,
                                                   bool is_exception,
                                                   bool is_result) {

  const vector<t_field*>& members = tstruct->get_members();
  const vector<t_field*>& sorted_members = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator m_iter;

  out <<
    "class " << rename_reserved_keywords(tstruct->get_name());
  if (is_exception) {
    out << "(TException)";
  } else if (gen_newstyle_) {
    out << "(object)";
  }
  out <<
    ":" << endl;
  indent_up();
  generate_python_docstring(out, tstruct);

  out << endl;

  /*
     Here we generate the structure specification for the fastbinary codec.
     These specifications have the following structure:
     thrift_spec -> tuple of item_spec
     item_spec -> None | (tag, type_enum, name, spec_args, default)
     tag -> integer
     type_enum -> TType.I32 | TType.STRING | TType.STRUCT | ...
     name -> string_literal
     default -> None  # Handled by __init__
     spec_args -> None  # For simple types
                | True/False for Text/Binary Strings
                | (type_enum, spec_args)  # Value type for list/set
                | (type_enum, spec_args, type_enum, spec_args)
                  # Key and value for map
                | [class_name, spec_args_ptr, is_union] # For struct/exception
     class_name -> identifier  # Basically a pointer to the class
     spec_args_ptr -> expression  # just class_name.spec_args
  */

  if (gen_slots_) {
    indent(out) << "__slots__ = [ " << endl;
    indent_up();
    for (m_iter = sorted_members.begin(); m_iter != sorted_members.end();
         ++m_iter) {
      indent(out) <<  "'" << rename_reserved_keywords((*m_iter)->get_name())
                  << "'," << endl;
    }
    indent_down();
    indent(out) << " ]" << endl << endl;

  }

  // TODO(dreiss): Test encoding of structs where some inner structs
  // don't have thrift_spec.
  indent(out) << "thrift_spec = None" << endl;
  if (members.size() != 0) {
    indent(out) << "__init__ = None" << endl;
  }

  // Generate `isUnion` method to distinguish union
  indent(out) << "@staticmethod" << endl;
  indent(out) << "def isUnion():" << endl;
  indent(out) << "  return False" << endl << endl;

  generate_py_struct_reader(out, tstruct);
  generate_py_struct_writer(out, tstruct);
  generate_json_reader(out, tstruct);

  // For exceptions only, generate a __str__ method. This is
  // because when raised exceptions are printed to the console, __repr__
  // isn't used. See python bug #5882
  if (is_exception) {
    out <<
      indent() << "def __str__(self):" << endl <<
      indent() << "  return repr(self)" << endl <<
      endl;
  }

  if (!gen_slots_) {
    // Printing utilities so that on the command line thrift
    // structs look somewhat human readable
    out <<
      indent() << "def __repr__(self):" << endl <<
      indent() << "  L = []" << endl <<
      indent() << "  for key, value in six.iteritems(self.__dict__):" << endl <<
      indent() << "    padding = ' ' * (len(key) + 1)" << endl <<
      indent() << "    value = pprint.pformat(value)" << endl <<
      indent() << "    value = padding.join(value.splitlines(True))" << endl <<
      indent() << "    L.append('    %s=%s' % (key, value))" << endl;

    // For exceptions only, force message attribute to be included in
    // __repr__(). This is because BaseException.message has been deprecated as
    // of Python 2.6 so python refuses to include the message attribute in
    // __dict__ of an Exception object which is used for generating return
    // value of __repr__.
    if (is_exception) {
      out <<
        indent() << "  if 'message' not in self.__dict__:" << endl <<
        indent() << "    message = getattr(self, 'message', None)" << endl <<
        indent() << "    if message:" << endl <<
        indent() << "      L.append('message=%r' % message)" << endl;
    }

    out <<
      indent() << "  return \"%s(\\n%s)\" % (self.__class__.__name__, "
               << "\",\\n\".join(L))" << endl << endl;

    // Equality and inequality methods that compare by value
    out <<
      indent() << "def __eq__(self, other):" << endl;
    indent_up();
    out <<
      indent() << "return isinstance(other, self.__class__) and "
      "self.__dict__ == other.__dict__" << endl;
    indent_down();
    out << endl;

    out <<
      indent() << "def __ne__(self, other):" << endl;
    indent_up();

    out <<
      indent() << "return not (self == other)" << endl;
    indent_down();
    out << endl;
  } else {
    // Use __slots__ instead of __dict__ for implementing
    // __eq__, __repr__, __ne__
    out <<
      indent() << "def __repr__(self):" << endl <<
      indent() << "  L = []" << endl <<
      indent() << "  for key in self.__slots__:" << endl <<
      indent() << "    padding = ' ' * (len(key) + 1)" << endl <<
      indent() << "    value = pprint.pformat(getattr(self, key))" << endl <<
      indent() << "    value = padding.join(value.splitlines(True))" << endl <<
      indent() << "    L.append('    %s=%s' % (key, value))" << endl <<
      indent() << "  return \"%s(\\n%s)\" % (self.__class__.__name__, "
               << "\",\\n\".join(L))" << endl << endl;

    // Equality method that compares each attribute by value and type,
    // walking __slots__
    out <<
      indent() << "def __eq__(self, other):" << endl <<
      indent() << "  if not isinstance(other, self.__class__):" << endl <<
      indent() << "    return False" << endl <<
      indent() << "  for attr in self.__slots__:" << endl <<
      indent() << "    my_val = getattr(self, attr)" << endl <<
      indent() << "    other_val = getattr(other, attr)" << endl <<
      indent() << "    if my_val != other_val:" << endl <<
      indent() << "      return False" << endl <<
      indent() << "  return True" << endl <<
      endl;

    out <<
      indent() << "def __ne__(self, other):" << endl <<
      indent() << "  return not (self == other)" << endl <<
      endl;
  }

  indent_down();
}

/**
 * Generates the read method for a struct
 */
void t_py_generator::generate_py_struct_reader(ofstream& out,
                                                t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  indent(out) <<
    "def read(self, iprot):" << endl;
  indent_up();

  indent(out) <<
    "if (iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated "
    "or (iprot.__class__ == THeaderProtocol.THeaderProtocol and "
    "iprot.get_protocol_id() == "
    "THeaderProtocol.THeaderProtocol.T_BINARY_PROTOCOL)) "
    "and isinstance(iprot.trans, TTransport.CReadableTransport) "
    "and self.thrift_spec is not None "
    "and fastbinary is not None:" << endl;
  indent_up();

  indent(out) <<
    "fastbinary.decode_binary(self, iprot.trans, " <<
    "[self.__class__, self.thrift_spec, False], " <<
    "utf8strings=UTF8STRINGS)" << endl;

  indent(out) <<
    "return" << endl;
  indent_down();

  indent(out) <<
    "iprot.readStructBegin()" << endl;

  // Loop over reading in fields
  indent(out) <<
    "while True:" << endl;
    indent_up();

    // Read beginning field marker
    indent(out) <<
      "(fname, ftype, fid) = iprot.readFieldBegin()" << endl;

    // Check for field STOP marker and break
    indent(out) <<
      "if ftype == TType.STOP:" << endl;
    indent_up();
    indent(out) <<
      "break" << endl;
    indent_down();

    // Switch statement on the field we are reading
    bool first = true;

    // Generate deserialization code for known cases
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      if (first) {
        first = false;
        out <<
          indent() << "if ";
      } else {
        out <<
          indent() << "elif ";
      }
      out << "fid == " << (*f_iter)->get_key() << ":" << endl;
      indent_up();
      indent(out) << "if ftype == " << type_to_enum((*f_iter)->get_type()) <<
        ":" << endl;
      indent_up();
      generate_deserialize_field(out, *f_iter, "self.");
      indent_down();
      out <<
        indent() << "else:" << endl <<
        indent() << "  iprot.skip(ftype)" << endl;
      indent_down();
    }

    // In the default case we skip the field
    out <<
      indent() <<  "else:" << endl <<
      indent() <<  "  iprot.skip(ftype)" << endl;

    // Read field end marker
    indent(out) <<
      "iprot.readFieldEnd()" << endl;

    indent_down();

    indent(out) <<
      "iprot.readStructEnd()" << endl;


  // The code that checks for the require field
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED) {
      indent(out) << "if self."
        << rename_reserved_keywords((*f_iter)->get_name())
        << " == None:" << endl;
      indent_up();
      indent(out) << "raise TProtocolException("
        << "TProtocolException.MISSING_REQUIRED_FIELD"
        << ", \"Required field '"
        << (*f_iter)->get_name()
        << "' was not found in serialized data! Struct: "
        << tstruct->get_name() << "\")" << endl;
      indent_down();
      out << endl;
    }
  }

    indent_down();
  out << endl;
}

void t_py_generator::generate_py_struct_writer(ofstream& out,
                                               t_struct* tstruct) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  indent(out) <<
    "def write(self, oprot):" << endl;
  indent_up();

  indent(out) <<
    "if (oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated "
    "or (oprot.__class__ == THeaderProtocol.THeaderProtocol and "
    "oprot.get_protocol_id() == "
    "THeaderProtocol.THeaderProtocol.T_BINARY_PROTOCOL)) "
    "and self.thrift_spec is not None "
    "and fastbinary is not None:" << endl;
  indent_up();

  indent(out) <<
    "oprot.trans.write(fastbinary.encode_binary(self, " <<
    "[self.__class__, self.thrift_spec, False], "
    "utf8strings=UTF8STRINGS))" << endl;
  indent(out) <<
    "return" << endl;
  indent_down();

  indent(out) <<
    "oprot.writeStructBegin('" << name << "')" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Write field header
    indent(out) <<
      "if self." << rename_reserved_keywords((*f_iter)->get_name()) <<
      " != None";
    if ((*f_iter)->get_req() == t_field::T_OPTIONAL &&
        (*f_iter)->get_value() != nullptr) {
      // An optional field with a value set should not be serialized if
      // the value equals the default value
      out << " and self." <<
        rename_reserved_keywords((*f_iter)->get_name()) << " != " <<
        "self.thrift_spec[" << (*f_iter)->get_key() << "][4]";
    }
    out << ":" << endl;
    indent_up();
    indent(out) <<
      "oprot.writeFieldBegin(" <<
      "'" << (*f_iter)->get_name() << "', " <<
      type_to_enum((*f_iter)->get_type()) << ", " <<
      (*f_iter)->get_key() << ")" << endl;

    // Write field contents
    generate_serialize_field(out, *f_iter, "self.");

    // Write field closer
    indent(out) <<
      "oprot.writeFieldEnd()" << endl;

    indent_down();
  }

  // Write the struct map
  out <<
    indent() << "oprot.writeFieldStop()" << endl <<
    indent() << "oprot.writeStructEnd()" << endl;

  indent_down();
  out <<
    endl;
}

/**
 * Generates a thrift service.
 *
 * @param tservice The service definition
 */
void t_py_generator::generate_service(t_service* tservice) {
  string f_service_name = package_dir_+"/"+
    rename_reserved_keywords(service_name_)+".py";
  f_service_.open(f_service_name.c_str());
  record_genfile(f_service_name);

  f_service_ <<
    py_autogen_comment() << endl <<
    py_imports() << endl;

  if (tservice->get_extends() != nullptr) {
    f_service_ <<
      "import " << get_real_py_module(tservice->get_extends()->get_program())
                << "."
                << rename_reserved_keywords(tservice->get_extends()->get_name())
                << endl;
  }

  f_service_ <<
    "from .ttypes import *" << endl <<
    "from thrift.Thrift import TProcessor" << endl <<
    render_fastbinary_includes() << endl <<
    "all_structs = []" << endl <<
    "UTF8STRINGS = bool(" << gen_utf8strings_ << ") or " <<
    "sys.version_info.major >= 3" << endl;

  if (gen_twisted_) {
    f_service_ <<
      "from zope.interface import Interface, implements" << endl <<
      "from twisted.internet import defer" << endl <<
      "from thrift.transport import TTwisted" << endl;
  }

  if (gen_asyncio_) {
    f_service_ <<
      "import asyncio" << endl <<
      "from thrift.util.asyncio import *" << endl;
  } else {
    f_service_ <<
      "from thrift.util.Decorators import *" << endl;
  }

  f_service_ << endl;

  // Generate the three main parts of the service (well, two for now in PHP)
  generate_service_interface(tservice, false);
  generate_service_interface(tservice, true);
  generate_service_client(tservice);
  generate_service_server(tservice, false);
  generate_service_server(tservice, true);
  generate_service_helpers(tservice);
  generate_service_remote(tservice);

  // Close service file
  f_service_ << "fix_spec(all_structs)" << endl
             << "del all_structs" << endl << endl;
  f_service_.close();
}

/**
 * Generates helper functions for a service.
 *
 * @param tservice The service to generate a header definition for
 */
void t_py_generator::generate_service_helpers(t_service* tservice) {
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  f_service_ <<
    "# HELPER FUNCTIONS AND STRUCTURES" << endl << endl;

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* ts = (*f_iter)->get_arglist();
    generate_py_struct_definition(f_service_, ts, false);
    generate_py_thrift_spec(f_service_, ts, false);
    generate_py_function_helpers(*f_iter);
  }
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_py_generator::generate_py_function_helpers(t_function* tfunction) {
  if (!tfunction->is_oneway()) {
    t_struct result(program_, rename_reserved_keywords(tfunction->get_name()) +
                    "_result");
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
    generate_py_struct_definition(f_service_, &result, false, true);
    generate_py_thrift_spec(f_service_, &result, false);
  }
}

/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 */
void t_py_generator::generate_service_interface(t_service* tservice,
                                                bool with_context) {
  string iface_prefix = with_context ? "Context" : "";
  string extends = "";
  string extends_if = "";
  if (tservice->get_extends() != nullptr) {
    extends = type_name(tservice->get_extends());
    extends_if = "(" + extends + "." + iface_prefix + "Iface)";
  } else {
    if (gen_twisted_) {
      extends_if = "(Interface)";
    }
  }

  f_service_ <<
    "class " << iface_prefix << "Iface" << extends_if << ":" << endl;
  indent_up();
  generate_python_docstring(f_service_, tservice);
  vector<t_function*> functions = tservice->get_functions();
  if (functions.empty()) {
    f_service_ <<
      indent() << "pass" << endl;
  } else {
    vector<t_function*>::iterator f_iter;
    for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
      f_service_ <<
        indent() << "def " << function_signature_if(*f_iter, with_context) <<
        ":" << endl;
      indent_up();
      generate_python_docstring(f_service_, (*f_iter));
      f_service_ <<
        indent() << "pass" << endl << endl;
      indent_down();
    }
  }

  indent_down();
  f_service_ <<
    endl;
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_py_generator::generate_service_client(t_service* tservice) {
  string extends = "";
  string extends_client = "";
  if (tservice->get_extends() != nullptr) {
    extends = type_name(tservice->get_extends());
    if (gen_twisted_) {
      extends_client = "(" + extends + ".Client)";
    } else {
      extends_client = extends + ".Client, ";
    }
  } else {
    if (gen_twisted_ && gen_newstyle_) {
        extends_client = "(object)";
    }
  }

  if (gen_twisted_) {
    f_service_ <<
      "class Client" << extends_client << ":" << endl <<
      "  implements(Iface)" << endl << endl;
  } else {
    f_service_ <<
      "class Client(" << extends_client << "Iface):" << endl;
  }
  indent_up();
  generate_python_docstring(f_service_, tservice);

  // Constructor function
  if (gen_twisted_) {
    f_service_ <<
      indent() << "def __init__(self, transport, oprot_factory):" << endl;
  } else if (gen_asyncio_) {
    f_service_ <<
      indent() <<
      "def __init__(self, transport, otrans_factory, oprot_factory):" << endl;
  } else {
    f_service_ <<
      indent() << "def __init__(self, iprot, oprot=None):" << endl;
  }
  if (extends.empty()) {
    if (gen_twisted_) {
      f_service_ <<
        indent() << "  self._transport = transport" << endl <<
        indent() << "  self._oprot_factory = oprot_factory" << endl <<
        indent() << "  self._seqid = 0" << endl <<
        indent() << "  self._reqs = {}" << endl <<
        endl;
    } else if (gen_asyncio_) {
      f_service_ <<
        indent() << "  self._transport = transport" << endl <<
        indent() << "  self._otrans_factory = otrans_factory" << endl <<
        indent() << "  self._oprot_factory = oprot_factory" << endl <<
        indent() << "  self._seqid = 0" << endl <<
        indent() << "  self._futures = {}" << endl <<
        endl;
    } else {
      f_service_ <<
        indent() << "  self._iprot = self._oprot = iprot" << endl <<
        indent() << "  if oprot != None:" << endl <<
        indent() << "    self._oprot = oprot" << endl <<
        indent() << "  self._seqid = 0" << endl <<
        endl;
    }
  } else {
    if (gen_twisted_) {
      f_service_ <<
        indent() << "  " << extends <<
        ".Client.__init__(self, transport, oprot_factory)" << endl << endl;
    } else if (gen_asyncio_) {
      f_service_ <<
        indent() << "  " << extends <<
        ".Client.__init__(self, transport, otrans_factory, oprot_factory)" <<
        endl << endl;
    } else {
      f_service_ <<
        indent() << "  " << extends <<
        ".Client.__init__(self, iprot, oprot)" << endl << endl;
    }
  }

  // Generate client method implementations
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arg_struct = (*f_iter)->get_arglist();
    const vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator fld_iter;
    string funname = rename_reserved_keywords((*f_iter)->get_name());

    // Open function
    indent(f_service_) <<
      "def " << function_signature(*f_iter) << ":" << endl;
    indent_up();
    generate_python_docstring(f_service_, (*f_iter));
    if (gen_twisted_) {
      indent(f_service_) << "self._seqid += 1" << endl;
      if (!(*f_iter)->is_oneway()) {
        indent(f_service_) <<
          "d = self._reqs[self._seqid] = defer.Deferred()" << endl;
      }
    } else if (gen_asyncio_) {
      indent(f_service_) << "self._seqid += 1" << endl;
      indent(f_service_) <<
        "fut = self._futures[self._seqid] = asyncio.Future()" << endl;
    }

    indent(f_service_) <<
      "self.send_" << funname << "(";

    bool first = true;
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      if (first) {
        first = false;
      } else {
        f_service_ << ", ";
      }
      f_service_ << rename_reserved_keywords((*fld_iter)->get_name());
    }
    f_service_ << ")" << endl;

    if (!(*f_iter)->is_oneway()) {
      f_service_ << indent();
      if (gen_twisted_) {
        f_service_ << "return d" << endl;
      } else if (gen_asyncio_) {
        f_service_ << "return fut" << endl;
      } else {
        if (!(*f_iter)->get_returntype()->is_void()) {
          f_service_ << "return ";
        }
        f_service_ <<
          "self.recv_" << funname << "()" << endl;
      }
    } else {
      if (gen_twisted_) {
        f_service_ <<
          indent() << "return defer.succeed(None)" << endl;
      } else if (gen_asyncio_) {
        f_service_ <<
          indent() << "fut.set_result(None)" << endl <<
          indent() << "return fut" << endl;
      }
    }
    indent_down();
    f_service_ << endl;

    indent(f_service_) <<
      "def send_" << function_signature(*f_iter) << ":" << endl;

    indent_up();

    std::string argsname = (*f_iter)->get_name() + "_args";

    // Serialize the request header
    if (gen_twisted_) {
      f_service_ <<
        indent() <<
          "oprot = self._oprot_factory.getProtocol(self._transport)" <<
          endl <<
        indent() <<
          "oprot.writeMessageBegin('" << (*f_iter)->get_name() <<
          "', TMessageType.CALL, self._seqid)" << endl;
    } else if (gen_asyncio_) {
      f_service_ <<
        indent() <<
          "otrans = self._otrans_factory.getTransport(self._transport)" <<
          endl <<
        indent() <<
          "oprot = self._oprot_factory.getProtocol(otrans)" << endl <<
        indent() <<
          "oprot.writeMessageBegin('" << (*f_iter)->get_name() <<
          "', TMessageType.CALL, self._seqid)" << endl;
    } else {
      f_service_ <<
        indent() << "self._oprot.writeMessageBegin('" <<
        (*f_iter)->get_name() << "', TMessageType.CALL, self._seqid)" <<
        endl;
    }

    f_service_ <<
      indent() << "args = " << argsname << "()" << endl;

    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      f_service_ <<
        indent() << "args." << rename_reserved_keywords((*fld_iter)->get_name())
                 << " = " <<
        rename_reserved_keywords((*fld_iter)->get_name()) << endl;
    }

    std::string flush = (*f_iter)->is_oneway() ? "onewayFlush" : "flush";
    // Write to the stream
    if (gen_twisted_ || gen_asyncio_) {
      f_service_ <<
        indent() << "args.write(oprot)" << endl <<
        indent() << "oprot.writeMessageEnd()" << endl <<
        indent() << "oprot.trans." << flush << "()" << endl;
    } else {
      f_service_ <<
        indent() << "args.write(self._oprot)" << endl <<
        indent() << "self._oprot.writeMessageEnd()" << endl <<
        indent() << "self._oprot.trans." << flush << "()" << endl;
    }

    indent_down();

    if (!(*f_iter)->is_oneway()) {
      std::string resultname = (*f_iter)->get_name() + "_result";
      // Open function
      f_service_ <<
        endl;
      if (gen_twisted_ || gen_asyncio_) {
        f_service_ <<
          indent() << "def recv_" << (*f_iter)->get_name() <<
              "(self, iprot, mtype, rseqid):" << endl;
      } else {
        t_struct noargs(program_);
        t_function recv_function((*f_iter)->get_returntype(),
                               string("recv_") + (*f_iter)->get_name(),
                               &noargs);
        f_service_ <<
          indent() << "def " << function_signature(&recv_function) <<
          ":" << endl;
      }
      indent_up();

      // TODO(mcslee): Validate message reply here, seq ids etc.

      if (gen_twisted_) {
        f_service_ <<
          indent() << "d = self._reqs.pop(rseqid)" << endl;
      } else if (gen_asyncio_) {
        f_service_ <<
          indent() << "fut = self._futures.pop(rseqid)" << endl;
      } else {
        f_service_ <<
          indent() << "(fname, mtype, rseqid) = " <<
          "self._iprot.readMessageBegin()" << endl;
      }

      f_service_ <<
        indent() << "if mtype == TMessageType.EXCEPTION:" << endl <<
        indent() << "  x = TApplicationException()" << endl;

      if (gen_twisted_) {
        f_service_ <<
          indent() << "  x.read(iprot)" << endl <<
          indent() << "  iprot.readMessageEnd()" << endl <<
          indent() << "  return d.errback(x)" << endl <<
          indent() << "result = " << resultname << "()" << endl <<
          indent() << "result.read(iprot)" << endl <<
          indent() << "iprot.readMessageEnd()" << endl;
      } else if (gen_asyncio_) {
        f_service_ <<
          indent() << "  x.read(iprot)" << endl <<
          indent() << "  iprot.readMessageEnd()" << endl <<
          indent() << "  fut.set_exception(x)" << endl <<
          indent() << "  return" << endl <<
          indent() << "result = " << resultname << "()" << endl <<
          indent() << "result.read(iprot)" << endl <<
          indent() << "iprot.readMessageEnd()" << endl;
      } else {
        f_service_ <<
          indent() << "  x.read(self._iprot)" << endl <<
          indent() << "  self._iprot.readMessageEnd()" << endl <<
          indent() << "  raise x" << endl <<
          indent() << "result = " << resultname << "()" << endl <<
          indent() << "result.read(self._iprot)" << endl <<
          indent() << "self._iprot.readMessageEnd()" << endl;
      }

      // Careful, only return _result if not a void function
      if (!(*f_iter)->get_returntype()->is_void()) {
        f_service_ <<
          indent() << "if result.success != None:" << endl;
          if (gen_twisted_) {
            f_service_ <<
              indent() << "  return d.callback(result.success)" << endl;
          } else if (gen_asyncio_) {
            f_service_ <<
              indent() << "  fut.set_result(result.success)" << endl <<
              indent() << "  return" << endl;
          } else {
            f_service_ <<
              indent() << "  return result.success" << endl;
          }
      }

      t_struct* xs = (*f_iter)->get_xceptions();
      const std::vector<t_field*>& xceptions = xs->get_members();
      vector<t_field*>::const_iterator x_iter;
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        f_service_ <<
          indent() << "if result." <<
          rename_reserved_keywords((*x_iter)->get_name()) <<
          " != None:" << endl;
          if (gen_twisted_) {
            f_service_ <<
              indent() << "  return d.errback(result." <<
              rename_reserved_keywords((*x_iter)->get_name()) << ")" << endl;
          } else if (gen_asyncio_) {
            f_service_ <<
              indent() << "  fut.set_exception(result." <<
              rename_reserved_keywords((*x_iter)->get_name()) <<
              ")" << endl <<
              indent() << "  return" << endl;
          } else {
            f_service_ <<
              indent() << "  raise result." <<
              rename_reserved_keywords((*x_iter)->get_name()) << endl;
          }
      }

      // Careful, only return _result if not a void function
      if ((*f_iter)->get_returntype()->is_void()) {
        if (gen_twisted_) {
          indent(f_service_) <<
            "return d.callback(None)" << endl;
        } else if (gen_asyncio_) {
          f_service_ <<
            indent() << "fut.set_result(None)" << endl <<
            indent() << "return" << endl;
        } else {
          indent(f_service_) <<
            "return" << endl;
        }
      } else {
        if (gen_twisted_) {
          f_service_ <<
            indent() << "return d.errback(TApplicationException(" <<
            "TApplicationException.MISSING_RESULT, \"" <<
            (*f_iter)->get_name() << " failed: unknown result\"))" << endl;
        } else if (gen_asyncio_) {
          f_service_ <<
            indent() << "fut.set_exception(TApplicationException(" <<
            "TApplicationException.MISSING_RESULT, \"" <<
            (*f_iter)->get_name() << " failed: unknown result\"))" << endl <<
            indent() << "return" << endl;
        } else {
          f_service_ <<
            indent() << "raise TApplicationException(" <<
            "TApplicationException.MISSING_RESULT, \"" <<
            (*f_iter)->get_name() << " failed: unknown result\");" << endl;
        }
      }

      // Close function
      indent_down();
      f_service_ << endl;
    }
  }

  indent_down();
  f_service_ <<
    endl;
}

/**
 * Generates a command line tool for making remote requests
 *
 * @param tservice The service to generate a remote for.
 */
void t_py_generator::generate_service_remote(t_service* tservice) {
  string f_remote_name = package_dir_+"/"+service_name_+"-remote";
  ofstream f_remote;
  f_remote.open(f_remote_name.c_str());
  record_genfile(f_remote_name);

  f_remote <<
    "#!/usr/bin/env python\n" <<
    py_autogen_comment() << "\n"
    "from __future__ import print_function\n"
    "from __future__ import absolute_import\n"
    "import optparse\n"
    "import os\n"
    "import pprint\n"
    "import sys\n"
    "import traceback\n"
    "from six.moves.urllib.parse import urlparse\n"
    "\n" <<
    // This has to be before thrift definitions
    // in case the environment is not correct.
    py_remote_warning() <<
    "\n"
    "from thrift.transport import TTransport\n"
    "from thrift.transport import TSocket\n"
    "from thrift.transport import TSSLSocket\n"
    "from thrift.transport import THttpClient\n"
    "from thrift.protocol import TBinaryProtocol\n"
    "from thrift.protocol import TJSONProtocol\n"
    "from thrift.protocol import THeaderProtocol\n"
    "\n"
    << "from . import "
    << rename_reserved_keywords(service_name_) << "\n"
    << "from .ttypes import *\n\n\n";

  // Emit a list of objects describing the service functions.
  // The rest of the code will use this to print the usage message and
  // perform function argument processing
  f_remote <<
    "class Function(object):\n"
    "    def __init__(self, name, return_type, args):\n"
    "        self.name = name\n"
    "        self.return_type = return_type\n"
    "        self.args = args\n"
    "\n"
    "\n"
    "FUNCTIONS = {\n";

  set<string> processed_fns;
  for (t_service* cur_service = tservice;
       cur_service != nullptr;
       cur_service = cur_service->get_extends()) {
    const vector<t_function*>& functions = cur_service->get_functions();
    for (vector<t_function*>::const_iterator it = functions.begin();
         it != functions.end();
         ++it) {
      const t_function* fn = *it;
      const string& fn_name = fn->get_name();
      pair< set<string>::iterator, bool > ret = processed_fns.insert(fn_name);
      if (!ret.second) {
        // A child class has overridden this function, so we've listed it
        // already.
        continue;
      }

      f_remote <<
        "    '" << fn_name << "': Function('" << fn_name << "', ";
      if (fn->is_oneway()) {
        f_remote << "None, ";
      } else {
        f_remote << "'" << thrift_type_name(fn->get_returntype()) << "', ";
      }

      f_remote << "[";
      const vector<t_field*>& args = fn->get_arglist()->get_members();
      bool first = true;
      for (vector<t_field*>::const_iterator it = args.begin();
           it != args.end();
           ++it) {
        if (first) {
          first = false;
        } else {
          f_remote << ", ";
        }
        f_remote <<
          "('" << thrift_type_name((*it)->get_type()) << "', '" <<
          (*it)->get_name() << "', '" <<
          thrift_type_name(get_true_type((*it)->get_type())) << "')";
      }
      f_remote << "]),\n";
    }
  }
  f_remote <<
    "}\n"
    "\n";

  f_remote <<
    "def print_functions(out=sys.stdout):\n"
    "    out.write('Functions:\\n')\n"
    "    for fn_name in sorted(FUNCTIONS):\n"
    "        fn = FUNCTIONS[fn_name]\n"
    "        if fn.return_type is None:\n"
    "            out.write('  oneway void ')\n"
    "        else:\n"
    "            out.write('  %s ' % (fn.return_type,))\n"
    "        out.write(fn_name + '(')\n"
    "        out.write(', '.join('%s %s' % (type, name)\n"
    "                            for type, name, true_type in fn.args))\n"
    "        out.write(')\\n')\n"
    "\n"
    "\n"
    "def parse_host_port(value, default_port):\n"
    "    parts = value.rsplit(':', 1)\n"
    "    if len(parts) == 1:\n"
    "        return (parts[0], default_port)\n"
    "    try:\n"
    "        port = int(parts[1])\n"
    "    except ValueError:\n"
    "        raise ValueError('invalid port: ' + parts[1])\n"
    "    return (parts[0], port)\n"
    "\n"
    "\n";

  // We emit optparse code for now.
  // In the future it would be nice to emit argparse code for python 2.7 and
  // above.
  f_remote <<
    "def main(argv):\n"
    "    usage = '%prog [OPTIONS] FUNCTION [ARGS ...]'\n"
    "    op = optparse.OptionParser(usage=usage, add_help_option=False)\n"
    "    op.disable_interspersed_args()\n"
    "    op.add_option('-h', '--host',\n"
    "                  action='store', metavar='HOST[:PORT]',\n"
    "                  help='The host and port to connect to')\n"
    "    op.add_option('-u', '--url',\n"
    "                  action='store',\n"
    "                  help='The URL to connect to, for HTTP transport')\n"
    "    op.add_option('-f', '--framed',\n"
    "                  action='store_true', default=False,\n"
    "                  help='Use framed transport')\n"
    "    op.add_option('-s', '--ssl',\n"
    "                  action='store_true', default=False,\n"
    "                  help='Use SSL socket')\n"
    "    op.add_option('-U', '--unframed',\n"
    "                  action='store_true', default=False,\n"
    "                  help='Use unframed transport')\n"
    "    op.add_option('-j', '--json',\n"
    "                  action='store_true', default=False,\n"
    "                  help='Use TJSONProtocol')\n"
    "    op.add_option('-c', '--compact',\n"
    "                  action='store_true', default=False,\n"
    "                  help='Use TCompactProtocol')\n"
    "    op.add_option('-?', '--help',\n"
    "                  action='help',\n"
    "                  help='Show this help message and exit')\n"
    "\n"
    "    (options, args) = op.parse_args(argv[1:])\n"
    "\n"
    "    if not args:\n"
    "        op.print_help(sys.stderr)\n"
    "        print_functions(sys.stderr)\n"
    "        return os.EX_USAGE\n"
    "\n"
    "    # Before we try to connect, make sure\n"
    "    # the function and arguments are valid\n"
    "    fn_name = args[0]\n"
    "    try:\n"
    "        fn = FUNCTIONS[fn_name]\n"
    "    except KeyError:\n"
    "        print_functions(sys.stderr)\n"
    "        print('\\nerror: unknown function \"%s\"' % fn_name, \n"
    "                 file=sys.stderr)\n"
    "        return os.EX_USAGE\n"
    "\n"
    "    if len(args) != len(fn.args) + 1:\n"
    "        print('error: %s requires exactly %d arguments'% \n"
    "                 (fn_name, len(fn.args)), file=sys.stderr)\n"
    "        return os.EX_USAGE\n"
    "    fn_args = []\n"
    "    for arg, arg_info in zip(args[1:], fn.args):\n"
    "        if arg_info[2] == 'string':\n"
    "            # For ease-of-use, we don't eval string arguments, simply so\n"
    "            # users don't have to wrap the arguments in quotes\n"
    "            fn_args.append(arg)\n"
    "            continue\n"
    "\n"
    "        try:\n"
    "            value = eval(arg)\n"
    "        except:\n"
    "            traceback.print_exc(file=sys.stderr)\n"
    "            print('\\nerror parsing argument \"%s\"' % (arg,), \n"
    "                     file=sys.stderr)\n"
    "            return os.EX_DATAERR\n"
    "        fn_args.append(value)\n"
    "\n"
    "    # Create the transport\n"
    "    if options.framed and options.unframed:\n"
    "        op.error('cannot specify both --framed and --unframed')\n"
    "    if options.url is not None:\n"
    "        if options.host is not None:\n"
    "            op.error('cannot specify both --url and --host')\n"
    "        if not any([options.unframed, options.json]):\n"
    "            op.error('can only specify --url with --unframed or --json')\n"
    "        url = urlparse(options.url)\n"
    "        parse_host_port(url[1], 80)\n"
    "        transport = THttpClient.THttpClient(options.url)\n"
    "    elif options.host is not None:\n"
    "        host, port = parse_host_port(options.host, "
           << default_port_ << ")\n"
    "        socket = TSSLSocket.TSSLSocket(host, port) if options.ssl \\\n"
    "                else TSocket.TSocket(host, port)\n"
    "        if options.framed:\n"
    "            transport = TTransport.TFramedTransport(socket)\n"
    "        else:\n"
    "            transport = TTransport.TBufferedTransport(socket)\n"
    "    else:\n"
    "        print('error: no --host or --url specified', file=sys.stderr)\n"
    "        return os.EX_USAGE\n"
    "\n"
    "    # Create the protocol and client\n"
    "    if options.json:\n"
    "        protocol = TJSONProtocol.TJSONProtocol(transport)\n"
    "    elif options.compact:\n"
    "        protocol = TCompactProtocol.TCompactProtocol(transport)\n"
    "    # No explicit option about protocol is specified. Try to infer.\n"
    "    elif options.framed or options.unframed:\n"
    "        protocol = TBinaryProtocol.TBinaryProtocolAccelerated(transport)\n"
    "    else:\n"
    "        protocol = THeaderProtocol.THeaderProtocol(socket)\n"
    "        transport = protocol.trans\n"
    "    transport.open()\n"
    "    client = " << rename_reserved_keywords(service_name_) <<
    ".Client(protocol)\n"
    "\n"
    "    # Call the function\n"
    "    method = getattr(client, fn_name)\n"
    "    ret = method(*fn_args)\n"
    "\n"
    "    # Print the result\n"
    "    pprint.pprint(ret, indent=2)\n"
    "\n"
    "\n"
    "if __name__ == '__main__':\n"
    "    rc = main(sys.argv)\n"
    "    sys.exit(rc)\n";

  // Close the remote file
  f_remote.close();

  // Make file executable, love that bitwise OR action
  chmod(f_remote_name.c_str(),
          S_IRUSR
        | S_IWUSR
        | S_IXUSR
#ifndef MINGW
        | S_IRGRP
        | S_IXGRP
        | S_IROTH
        | S_IXOTH
#endif
  );
}

/**
 * Generates a service server definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_py_generator::generate_service_server(t_service* tservice,
                                             bool with_context) {
  string class_prefix = with_context ? "Context" : "";

  // Generate the dispatch methods
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  string extends = "";
  string extends_processor = "";
  if (tservice->get_extends() != nullptr) {
    extends = type_name(tservice->get_extends());
    extends_processor = extends + "." + class_prefix + "Processor, ";
  }

  // Generate the header portion
  if (gen_twisted_) {
    f_service_ <<
      "class " << class_prefix << "Processor(" << extends_processor <<
      "TProcessor):" << endl <<
      "  implements(" << class_prefix << "Iface)" << endl << endl;
  } else {
    f_service_ <<
      "class " << class_prefix << "Processor(" << extends_processor <<
      class_prefix << "Iface, TProcessor):" <<
      endl;
  }

  indent_up();

  indent(f_service_) <<
    "def __init__(self, handler):" << endl;
  indent_up();
  if (extends.empty()) {
    f_service_ << indent() << "TProcessor.__init__(self)" << endl;
    if (gen_twisted_) {
      f_service_ <<
        indent() << "self._handler = " << class_prefix << "Iface(handler)" <<
        endl;
    } else {
      f_service_ <<
        indent() << "self._handler = handler" << endl;
    }

    f_service_ <<
      indent() << "self._processMap = {}" << endl;
  } else {
    if (gen_twisted_) {
      f_service_ <<
        indent() << extends << "." << class_prefix <<
        "Processor.__init__(self, " << class_prefix << "Iface(handler))" <<
        endl;
    } else {
      f_service_ <<
        indent() << extends << "." << class_prefix <<
        "Processor.__init__(self, handler)" << endl;
    }
  }
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_service_ <<
      indent() << "self._processMap[\"" << (*f_iter)->get_name() <<
      "\"] = " << class_prefix << "Processor.process_" <<
      (*f_iter)->get_name() << endl;
  }
  indent_down();
  f_service_ << endl;

  // Generate the server implementation
  if (gen_asyncio_) {
    indent(f_service_) << "@process_main" << endl;
  } else {
    indent(f_service_) << "@process_main" <<
      (gen_twisted_ ? "(twisted=True)" : "()") << endl;
  }
  indent(f_service_) <<
    "def process(self,): pass" << endl << endl;

  // Generate the process subfunctions
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    generate_process_function(tservice, *f_iter, with_context);
  }

  indent_down();

  f_service_ << indent() << class_prefix << "Iface._processor_type = " <<
    class_prefix << "Processor" << endl;

  f_service_ << endl;
}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_py_generator::generate_process_function(t_service* tservice,
                                               t_function* tfunction,
                                               bool with_context) {
  string fn_name = tfunction->get_name();

  // Open function
  indent(f_service_)
    << "@process_method(oneway="
    << (tfunction->is_oneway() ? "True" : "False")
    << (gen_twisted_ ? ", twisted=True)" : ")") << endl;

  f_service_
    << indent() << "def process_" << fn_name << "(self, args, handler_ctx"
    << (gen_twisted_ ? ", seqid, oprot):" : "):") << endl;
  indent_up();

  t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;

  // Declare result for non oneway function
  if (!tfunction->is_oneway()) {
    f_service_ <<
      indent() << "result = " << fn_name + "_result()" << endl;
  }

  if (gen_twisted_) {
    // Generate the function call
    t_struct* arg_struct = tfunction->get_arglist();
    const std::vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator f_iter;

    f_service_ <<
      indent() << "d = defer.maybeDeferred(self._handler." <<
      rename_reserved_keywords(fn_name) << ", ";
    bool first = true;
    if (with_context) {
      f_service_ << "handler_ctx";
      first = false;
    }
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      if (first) {
        first = false;
      } else {
        f_service_ << ", ";
      }
      f_service_ << "args." << rename_reserved_keywords((*f_iter)->get_name());
    }
    f_service_ << ")" << endl;

    // Shortcut out here for oneway functions
    if (tfunction->is_oneway()) {
      f_service_ <<
        indent() << "return d" << endl;
      indent_down();
      f_service_ << endl;
      return;
    }

    f_service_ <<
      indent() <<
        "d.addCallback(self.write_results_success_" <<
          fn_name << ", result, seqid, oprot, handler_ctx)" << endl;

    f_service_ <<
      indent() <<
        "d.addErrback(self.write_results_exception_" <<
          fn_name << ", result, seqid, oprot, handler_ctx)" << endl;

    f_service_ <<
      indent() << "return d" << endl;

    indent_down();
    f_service_ << endl;

    indent(f_service_) << "@write_results_success_callback" << endl;
    f_service_
      << indent() << "def write_results_success_" << fn_name <<
      "(self,): pass" << endl << endl;

    indent(f_service_) << "@write_results_exception_callback" << endl;
    f_service_
      << indent() << "def write_results_exception_" << fn_name <<
      "(self, error, result, handler_ctx):" << endl;
    indent_up();
    f_service_ <<
      indent() << "try:" << endl;

    // Kinda absurd
    f_service_ <<
      indent() << "  error.raiseException()" << endl;
    int exc_num;
    for (exc_num = 0, x_iter = xceptions.begin();
         x_iter != xceptions.end(); ++x_iter, ++exc_num) {
      f_service_ <<
        indent() << "except " << type_name((*x_iter)->get_type())
                 << " as exc" << exc_num << ":" << endl;
      indent_up();
      f_service_ <<
        indent() << "self._event_handler.handlerException(handler_ctx, '"
                 << fn_name << "', exc" << exc_num << ")" << endl;
      f_service_ <<
        indent() << "reply_type = TMessageType.REPLY" << endl;
      f_service_ <<
        indent() << "result." <<
        rename_reserved_keywords((*x_iter)->get_name())
                 << " = exc" << exc_num << endl;
      indent_down();
    }
    f_service_ <<
      indent() << "except:" << endl <<
      indent() << "  reply_type = TMessageType.EXCEPTION" << endl <<
      indent() << "  ex = sys.exc_info()[1]" << endl <<
      indent() << "  self._event_handler.handlerError(handler_ctx, '"
               << fn_name << "', ex)" << endl <<
      indent() << "  result = Thrift.TApplicationException(message="
               << "str(ex))" << endl <<
      indent() << "return reply_type, result" << endl;
    indent_down();
    f_service_ << endl;
  } else {

    // Try block to wrap call to handler
    f_service_ <<
      indent() << "try:" << endl;
    indent_up();

    // Generate the function call
    t_struct* arg_struct = tfunction->get_arglist();
    const std::vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator f_iter;

    string handler = "self._handler." +
      rename_reserved_keywords(tfunction->get_name());

    f_service_ << indent();

    if (gen_asyncio_) {
      string args_list = "";
      bool first = true;
      if (with_context) {
        args_list += "handler_ctx";
        first = false;
      }
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if (first) {
          first = false;
        } else {
          args_list += ", ";
        }
        args_list += "args.";
        args_list += rename_reserved_keywords((*f_iter)->get_name());
      }

      f_service_ << "if asyncio.iscoroutinefunction(" << handler << "):" <<
        endl <<
        indent() << "  ret = yield from " << handler << "(" << args_list <<
        ")" << endl <<
        indent() << "elif should_run_on_thread(" << handler << "):" << endl <<
        indent() << "  loop = asyncio.get_event_loop()" << endl <<
        indent() << "  ret = yield from loop.run_in_executor(None, " <<
        handler << ", " << args_list << ")" << endl <<
        indent() << "else: " << endl <<
        indent() << "  ret = " << handler << "(" << args_list << ")" << endl;

      if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
        f_service_ << indent() << "result.success = ret" << endl;
      }
    } else {
      if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
        f_service_ << "result.success = ";
      }
      f_service_ << handler << "(";
      bool  first = true;
      if (with_context) {
        f_service_ << "handler_ctx";
        first = false;
      }
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if (first) {
          first = false;
        } else {
          f_service_ << ", ";
        }
        f_service_ << "args." <<
          rename_reserved_keywords((*f_iter)->get_name());
      }
      f_service_ << ")" << endl;
    }

    indent_down();
    int exc_num;
    for (exc_num = 0, x_iter = xceptions.begin();
          x_iter != xceptions.end(); ++x_iter, ++exc_num) {
      f_service_ <<
        indent() << "except " << type_name((*x_iter)->get_type())
                 << " as exc" << exc_num << ":" << endl;
      if (!tfunction->is_oneway()) {
        indent_up();
        f_service_ <<
          indent() << "self._event_handler.handlerException(handler_ctx, '"
                   << fn_name << "', exc" << exc_num << ")" << endl <<
          indent() << "result." <<
          rename_reserved_keywords((*x_iter)->get_name())
                   << " = exc" << exc_num << endl;
        indent_down();
      } else {
        f_service_ <<
          indent() << "pass" << endl;
      }
    }
    f_service_ <<
      indent() << "except:" << endl <<
      indent() << "  ex = sys.exc_info()[1]" << endl <<
      indent() << "  self._event_handler.handlerError(handler_ctx, '"
               << fn_name << "', ex)" << endl <<
      indent() << "  result = Thrift.TApplicationException(message="
               << "str(ex))" << endl;
    if (!tfunction->is_oneway()) {
      f_service_ << indent() << "return result" << endl;
    }

    // Close function
    indent_down();
    f_service_ << endl;
  }
}

/**
 * Deserializes a field of any type.
 */
void t_py_generator::generate_deserialize_field(ofstream &out,
                                                t_field* tfield,
                                                string prefix,
                                                bool inclass) {
  t_type* type = get_true_type(tfield->get_type());

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " +
      prefix + tfield->get_name();
  }

  string name = prefix + rename_reserved_keywords(tfield->get_name());

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out,
                                (t_struct*)type,
                                 name);
  } else if (type->is_container()) {
    generate_deserialize_container(out, type, name);
  } else if (type->is_base_type() || type->is_enum()) {
    indent(out) <<
      name << " = iprot.";

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw "compiler error: cannot serialize void field in a struct: " +
          name;
        break;
      case t_base_type::TYPE_STRING:
        if (((t_base_type*)type)->is_binary()) {
          out << "readString()";
        } else {
          out << "readString().decode('utf-8') " <<
              "if UTF8STRINGS else iprot.readString()";
        }
        break;
      case t_base_type::TYPE_BOOL:
        out << "readBool()";
        break;
      case t_base_type::TYPE_BYTE:
        out << "readByte()";
        break;
      case t_base_type::TYPE_I16:
        out << "readI16()";
        break;
      case t_base_type::TYPE_I32:
        out << "readI32()";
        break;
      case t_base_type::TYPE_I64:
        out << "readI64()";
        break;
      case t_base_type::TYPE_DOUBLE:
        out << "readDouble()";
        break;
      case t_base_type::TYPE_FLOAT:
        out << "readFloat()";
        break;
      default:
        throw "compiler error: no Python name for base type " +
          t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      out << "readI32()";
    }
    out << endl;

  } else {
    printf("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
           tfield->get_name().c_str(), type->get_name().c_str());
  }
}

/**
 * Generates an unserializer for a struct, calling read()
 */
void t_py_generator::generate_deserialize_struct(ofstream &out,
                                                  t_struct* tstruct,
                                                  string prefix) {
  out <<
    indent() << prefix << " = " << type_name(tstruct) << "()" << endl <<
    indent() << prefix << ".read(iprot)" << endl;
}

/**
 * Serialize a container by writing out the header followed by
 * data and then a footer.
 */
void t_py_generator::generate_deserialize_container(ofstream &out,
                                                    t_type* ttype,
                                                    string prefix) {
  string size = tmp("_size");
  string ktype = tmp("_ktype");
  string vtype = tmp("_vtype");
  string etype = tmp("_etype");

  t_field fsize(g_type_i32, size);
  t_field fktype(g_type_byte, ktype);
  t_field fvtype(g_type_byte, vtype);
  t_field fetype(g_type_byte, etype);

  // Declare variables, read header
  if (ttype->is_map()) {
    out <<
      indent() << prefix << " = {}" << endl <<
      indent() << "(" << ktype << ", " << vtype << ", " << size
               << " ) = iprot.readMapBegin() " << endl;
  } else if (ttype->is_set()) {
    out <<
      indent() << prefix << " = set()" << endl <<
      indent() << "(" << etype << ", " << size
               << ") = iprot.readSetBegin()" << endl;
  } else if (ttype->is_list()) {
    out <<
      indent() << prefix << " = []" << endl <<
      indent() << "(" << etype << ", " << size
               << ") = iprot.readListBegin()" << endl;
  }


  // For loop iterates over elements
  string i = tmp("_i");
  indent(out) <<
    "if " << size << " >= 0:" << endl << indent() <<
    "  for " << i << " in six.moves.range(" << size << "):" << endl;

  indent_up();
  indent_up();

  if (ttype->is_map()) {
    generate_deserialize_map_element(out, (t_map*)ttype, prefix);
  } else if (ttype->is_set()) {
    generate_deserialize_set_element(out, (t_set*)ttype, prefix);
  } else if (ttype->is_list()) {
    generate_deserialize_list_element(out, (t_list*)ttype, prefix);
  }

  indent_down();
  indent_down();

  indent(out) << "else: " << endl;
  if (ttype->is_map()) {
    out << indent() << "  while iprot.peekMap():" << endl;
  } else if (ttype->is_set()) {
    out << indent() << "  while iprot.peekSet():" << endl;
  } else if (ttype->is_list()) {
    out << indent() << "  while iprot.peekList():" << endl;
  }

  indent_up();
  indent_up();

  if (ttype->is_map()) {
    generate_deserialize_map_element(out, (t_map*)ttype, prefix);
  } else if (ttype->is_set()) {
    generate_deserialize_set_element(out, (t_set*)ttype, prefix);
  } else if (ttype->is_list()) {
    generate_deserialize_list_element(out, (t_list*)ttype, prefix);
  }

  indent_down();
  indent_down();

  // Read container end
  if (ttype->is_map()) {
    indent(out) << "iprot.readMapEnd()" << endl;
  } else if (ttype->is_set()) {
    indent(out) << "iprot.readSetEnd()" << endl;
  } else if (ttype->is_list()) {
    indent(out) << "iprot.readListEnd()" << endl;
  }
}


/**
 * Generates code to deserialize a map
 */
void t_py_generator::generate_deserialize_map_element(ofstream &out,
                                                       t_map* tmap,
                                                       string prefix) {
  string key = tmp("_key");
  string val = tmp("_val");
  t_field fkey(tmap->get_key_type(), key);
  t_field fval(tmap->get_val_type(), val);

  generate_deserialize_field(out, &fkey);
  generate_deserialize_field(out, &fval);

  indent(out) <<
    prefix << "[" << key << "] = " << val << endl;
}

/**
 * Write a set element
 */
void t_py_generator::generate_deserialize_set_element(ofstream &out,
                                                       t_set* tset,
                                                       string prefix) {
  string elem = tmp("_elem");
  t_field felem(tset->get_elem_type(), elem);

  generate_deserialize_field(out, &felem);

  indent(out) <<
    prefix << ".add(" << elem << ")" << endl;
}

/**
 * Write a list element
 */
void t_py_generator::generate_deserialize_list_element(ofstream &out,
                                                        t_list* tlist,
                                                        string prefix) {
  string elem = tmp("_elem");
  t_field felem(tlist->get_elem_type(), elem);

  generate_deserialize_field(out, &felem);

  indent(out) <<
    prefix << ".append(" << elem << ")" << endl;
}


/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_py_generator::generate_serialize_field(ofstream &out,
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
                              (t_struct*)type,
                              prefix +
                              rename_reserved_keywords(tfield->get_name()));
  } else if (type->is_container()) {
    generate_serialize_container(out,
                                 type,
                                 prefix +
                                 rename_reserved_keywords(tfield->get_name()));
  } else if (type->is_base_type() || type->is_enum()) {

    string name = prefix + rename_reserved_keywords(tfield->get_name());

    indent(out) <<
      "oprot.";

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw
          "compiler error: cannot serialize void field in a struct: " + name;
        break;
      case t_base_type::TYPE_STRING:
        if (((t_base_type*)type)->is_binary()) {
          out << "writeString(" << name << ")";
        } else {
          out << "writeString(" << name << ".encode('utf-8')) " <<
              "if UTF8STRINGS and not isinstance(" << name << ", bytes) " <<
              "else oprot.writeString(" << name << ")";
        }
        break;
      case t_base_type::TYPE_BOOL:
        out << "writeBool(" << name << ")";
        break;
      case t_base_type::TYPE_BYTE:
        out << "writeByte(" << name << ")";
        break;
      case t_base_type::TYPE_I16:
        out << "writeI16(" << name << ")";
        break;
      case t_base_type::TYPE_I32:
        out << "writeI32(" << name << ")";
        break;
      case t_base_type::TYPE_I64:
        out << "writeI64(" << name << ")";
        break;
      case t_base_type::TYPE_DOUBLE:
        out << "writeDouble(" << name << ")";
        break;
      case t_base_type::TYPE_FLOAT:
        out << "writeFloat(" << name << ")";
          break;
      default:
        throw "compiler error: no Python name for base type " +
          t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      out << "writeI32(" << name << ")";
    }
    out << endl;
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
void t_py_generator::generate_serialize_struct(ofstream &out,
                                               t_struct* tstruct,
                                               string prefix) {
  indent(out) <<
    prefix << ".write(oprot)" << endl;
}

void t_py_generator::generate_serialize_container(ofstream &out,
                                                  t_type* ttype,
                                                  string prefix) {
  if (ttype->is_map()) {
    indent(out) <<
      "oprot.writeMapBegin(" <<
      type_to_enum(((t_map*)ttype)->get_key_type()) << ", " <<
      type_to_enum(((t_map*)ttype)->get_val_type()) << ", " <<
      "len(" << prefix << "))" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "oprot.writeSetBegin(" <<
      type_to_enum(((t_set*)ttype)->get_elem_type()) << ", " <<
      "len(" << prefix << "))" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "oprot.writeListBegin(" <<
      type_to_enum(((t_list*)ttype)->get_elem_type()) << ", " <<
      "len(" << prefix << "))" << endl;
  }

  if (ttype->is_map()) {
    string kiter = tmp("kiter");
    string viter = tmp("viter");
    if (sort_keys_) {
      string sorted = tmp("sorted");
      indent(out) <<
        sorted << " = " << prefix << ".items()" << endl;
      string tuple = tmp("tuple");
      indent(out) <<
        sorted << ".sort(key=lambda " << tuple <<
        ": " << tuple << "[0])" << endl;
      indent(out) <<
        "for " << kiter << "," << viter << " in " << sorted << ":" <<
        endl;
    } else {
      indent(out) <<
        "for " << kiter << "," << viter << " in " << prefix << ".items():" <<
        endl;
    }
    indent_up();
    generate_serialize_map_element(out, (t_map*)ttype, kiter, viter);
    indent_down();
  } else if (ttype->is_set()) {
    string iter = tmp("iter");
    indent(out) <<
      "for " << iter << " in " << prefix << ":" << endl;
    indent_up();
    generate_serialize_set_element(out, (t_set*)ttype, iter);
    indent_down();
  } else if (ttype->is_list()) {
    string iter = tmp("iter");
    indent(out) <<
      "for " << iter << " in " << prefix << ":" << endl;
    indent_up();
    generate_serialize_list_element(out, (t_list*)ttype, iter);
    indent_down();
  }

  if (ttype->is_map()) {
    indent(out) <<
      "oprot.writeMapEnd()" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "oprot.writeSetEnd()" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "oprot.writeListEnd()" << endl;
  }
}

/**
 * Serializes the members of a map.
 *
 */
void t_py_generator::generate_serialize_map_element(ofstream &out,
                                                     t_map* tmap,
                                                     string kiter,
                                                     string viter) {
  t_field kfield(tmap->get_key_type(), kiter);
  generate_serialize_field(out, &kfield, "");

  t_field vfield(tmap->get_val_type(), viter);
  generate_serialize_field(out, &vfield, "");
}

/**
 * Serializes the members of a set.
 */
void t_py_generator::generate_serialize_set_element(ofstream &out,
                                                     t_set* tset,
                                                     string iter) {
  t_field efield(tset->get_elem_type(), iter);
  generate_serialize_field(out, &efield, "");
}

/**
 * Serializes the members of a list.
 */
void t_py_generator::generate_serialize_list_element(ofstream &out,
                                                      t_list* tlist,
                                                      string iter) {
  t_field efield(tlist->get_elem_type(), iter);
  generate_serialize_field(out, &efield, "");
}

/**
 * Generates the docstring for a given struct.
 */
void t_py_generator::generate_python_docstring(ofstream& out,
                                               t_struct* tstruct) {
  generate_python_docstring(out, tstruct, tstruct, "Attributes");
}

/**
 * Generates the docstring for a given function.
 */
void t_py_generator::generate_python_docstring(ofstream& out,
                                               t_function* tfunction) {
  generate_python_docstring(out, tfunction, tfunction->get_arglist(),
      "Parameters");
}

/**
 * Generates the docstring for a struct or function.
 */
void t_py_generator::generate_python_docstring(ofstream& out,
                                               t_doc*    tdoc,
                                               t_struct* tstruct,
                                               const char* subheader) {
  bool has_doc = false;
  stringstream ss;
  if (tdoc->has_doc()) {
    has_doc = true;
    ss << tdoc->get_doc();
  }

  const vector<t_field*>& fields = tstruct->get_members();
  if (fields.size() > 0) {
    if (has_doc) {
      ss << endl;
    }
    has_doc = true;
    ss << subheader << ":\n";
    vector<t_field*>::const_iterator p_iter;
    for (p_iter = fields.begin(); p_iter != fields.end(); ++p_iter) {
      t_field* p = *p_iter;
      ss << " - " << rename_reserved_keywords(p->get_name());
      if (p->has_doc()) {
        ss << ": " << p->get_doc();
      } else {
        ss << endl;
      }
    }
  }

  if (has_doc) {
    generate_docstring_comment(out,
      "\"\"\"\n",
      "", ss.str(),
      "\"\"\"\n");
  }
}

/**
 * Generates the docstring for a generic object.
 */
void t_py_generator::generate_python_docstring(ofstream& out,
                                               t_doc* tdoc) {
  if (tdoc->has_doc()) {
    generate_docstring_comment(out,
      "\"\"\"\n",
      "", tdoc->get_doc(),
      "\"\"\"\n");
  }
}

/**
 * Declares an argument, which may include initialization as necessary.
 *
 * @param tfield The field
 */
string t_py_generator::declare_argument(std::string structname, t_field* tfield) {
  std::ostringstream result;
  result << rename_reserved_keywords(tfield->get_name()) << "=";
  if (tfield->get_value() != nullptr) {
    result << structname << ".thrift_spec[" <<
      tfield->get_key() << "][4]";
  } else {
    result << "None";
  }
  return result.str();
}

/**
 * Renders a field default value, returns None otherwise.
 *
 * @param tfield The field
 */
string t_py_generator::render_field_default_value(t_field* tfield) {
  t_type* type = get_true_type(tfield->get_type());
  if (tfield->get_value() != nullptr) {
    return render_const_value(type, tfield->get_value());
  } else {
    return "None";
  }
}

/**
 * Renders a function signature of the form 'type name(args)'
 *
 * @param tfunction Function definition
 * @return String of rendered function definition
 */
string t_py_generator::function_signature(t_function* tfunction,
                                           string prefix) {
  // TODO(mcslee): Nitpicky, no ',' if argument_list is empty
  return
    prefix + rename_reserved_keywords(tfunction->get_name()) +
    "(self, " + argument_list(tfunction->get_arglist()) + ")";
}

/**
 * Renders an interface function signature of the form 'type name(args)'
 *
 * @param tfunction Function definition
 * @return String of rendered function definition
 */
string t_py_generator::function_signature_if(t_function* tfunction,
                                             bool with_context) {
  // TODO(mcslee): Nitpicky, no ',' if argument_list is empty
  string signature = rename_reserved_keywords(tfunction->get_name()) + "(";
  if (!gen_twisted_) {
    signature += "self, ";
  }
  if (with_context) {
    signature += "handler_ctx, ";
  }
  signature += argument_list(tfunction->get_arglist()) + ")";
  return signature;
}


/**
 * Renders a field list
 */
string t_py_generator::argument_list(t_struct* tstruct) {
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
    result += rename_reserved_keywords((*f_iter)->get_name());
    result += "=" + render_field_default_value(*f_iter);
  }
  return result;
}

string t_py_generator::type_name(t_type* ttype) {
  t_program* program = ttype->get_program();
  if (ttype->is_service()) {
    return get_real_py_module(program) + "." +
      rename_reserved_keywords(ttype->get_name());
  }
  if (program != nullptr && program != program_) {
    return get_real_py_module(program) + ".ttypes." +
      rename_reserved_keywords(ttype->get_name());
  }
  return rename_reserved_keywords(ttype->get_name());
}

/**
 * Converts the parse type to a Python tyoe
 */
string t_py_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
      return "TType.STRING";
    case t_base_type::TYPE_BOOL:
      return "TType.BOOL";
    case t_base_type::TYPE_BYTE:
      return "TType.BYTE";
    case t_base_type::TYPE_I16:
      return "TType.I16";
    case t_base_type::TYPE_I32:
      return "TType.I32";
    case t_base_type::TYPE_I64:
      return "TType.I64";
    case t_base_type::TYPE_DOUBLE:
      return "TType.DOUBLE";
      case t_base_type::TYPE_FLOAT:
      return "TType.FLOAT";
    }
  } else if (type->is_enum()) {
    return "TType.I32";
  } else if (type->is_struct() || type->is_xception()) {
    return "TType.STRUCT";
  } else if (type->is_map()) {
    return "TType.MAP";
  } else if (type->is_set()) {
    return "TType.SET";
  } else if (type->is_list()) {
    return "TType.LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

/** See the comment inside generate_py_struct_definition for what this is. */
string t_py_generator::type_to_spec_args(t_type* ttype) {
  ttype = get_true_type(ttype);

  if (ttype->is_base_type() || ttype->is_enum()) {
    t_base_type::t_base tbase = ((t_base_type*)ttype)->get_base();
    if (tbase == t_base_type::TYPE_STRING) {
      if (((t_base_type*)ttype)->is_binary()) {
        return "False";
      }
      return "True";
    }
    return "None";
  } else if (ttype->is_struct()){
    return "[" + type_name(ttype) + ", " + type_name(ttype) + ".thrift_spec, " +
      (((t_struct*)ttype)->is_union() ? "True]" : "False]");
  } else if (ttype->is_xception()) {
    return "[" + type_name(ttype) + ", " + type_name(ttype) +
      ".thrift_spec, False]";
  } else if (ttype->is_map()) {
    return "(" +
      type_to_enum(((t_map*)ttype)->get_key_type()) + "," +
      type_to_spec_args(((t_map*)ttype)->get_key_type()) + "," +
      type_to_enum(((t_map*)ttype)->get_val_type()) + "," +
      type_to_spec_args(((t_map*)ttype)->get_val_type()) +
      ")";

  } else if (ttype->is_set()) {
    return "(" +
      type_to_enum(((t_set*)ttype)->get_elem_type()) + "," +
      type_to_spec_args(((t_set*)ttype)->get_elem_type()) +
      ")";

  } else if (ttype->is_list()) {
    return "(" +
      type_to_enum(((t_list*)ttype)->get_elem_type()) + "," +
      type_to_spec_args(((t_list*)ttype)->get_elem_type()) +
      ")";
  }

  throw "INVALID TYPE IN type_to_spec_args: " + ttype->get_name();
}

THRIFT_REGISTER_GENERATOR(py, "Python",
"    json:            Generate function to parse entity from json\n"
"    new_style:       Generate new-style classes.\n"
"    slots:           Generate code using slots for instance members.\n"
"    sort_keys:       Serialize maps in the ascending order of their keys.\n"
"    thrift_port=NNN: Default port to use in remote client (default 9090).\n"
"    twisted:         Generate Twisted-friendly RPC services.\n"
"    asyncio:         Generate asyncio-friendly RPC services.\n"
"    utf8strings:     Encode/decode strings using utf8 in the generated code.\n"
);
