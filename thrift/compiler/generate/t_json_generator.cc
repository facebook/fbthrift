/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>

#include <sstream>
#include <thrift/compiler/generate/t_generator.h>
#include <thrift/compiler/generate/t_concat_generator.h>
#include <thrift/compiler/platform.h>
using namespace std;


/**
 * JSON code generator
 */
class t_json_generator : public t_concat_generator {
 public:
  t_json_generator(
      t_program* program,
      const std::map<std::string, std::string>& /*parsed_options*/,
      const std::string& /*option_string*/)
    : t_concat_generator(program)
  {
    out_dir_base_ = "gen-json";
  }

  void generate_program() override;

  /**
   * Program-level generation functions
   */

  void generate_typedef(t_typedef* ttypedef) override;
  void generate_enum(t_enum* tenum) override;
  void generate_const(t_const* tconst) override;
  void generate_consts(vector<t_const*> consts) override;
  void generate_struct(t_struct* tstruct) override;
  void generate_service(t_service* tservice) override;
  void generate_xception(t_struct* txception) override;

  void   print_type       (t_type* ttype);
  void   print_const_value(const t_const_value* tvalue);
  void   print_const_key  (t_const_value* tvalue);
  string type_to_string   (t_type* type);
  string type_to_spec_args(t_type* ttype);

  std::ofstream f_out_;
};

/**
 * Prepares for file generation by opening up the necessary file output
 * stream.
 */
void t_json_generator::generate_program() {
  // Make output directory
  make_dir(get_out_dir().c_str());
  string module_name = program_->get_namespace("json");
  string fname = get_out_dir();
  if (module_name.empty()) {
    module_name = program_->get_name();
  }
  string mangled_module_name = module_name;
  make_dir(fname.c_str());
  for (string::size_type pos = mangled_module_name.find('.');
       pos != string::npos;
       pos = mangled_module_name.find('.')) {
    fname += '/';
    fname += mangled_module_name.substr(0, pos);
    mangled_module_name.erase(0, pos+1);
    make_dir(fname.c_str());
  }

  fname += '/';
  fname += mangled_module_name;
  fname += ".json";
  f_out_.open(fname.c_str());
  indent(f_out_) << "{" << endl;
  indent_up();
  indent(f_out_) << "\"thrift_module\" : \"" << module_name << "\"";

  if (!program_->get_consts().empty()) {
    f_out_ << "," << endl << indent() << "\"constants\" : {" << endl;
    indent_up();
    vector<t_const*> consts = program_->get_consts();
    generate_consts(consts);
    f_out_ << endl;
    indent_down();
    indent(f_out_) << "}";
  }

  if (!program_->get_enums().empty()) {
    f_out_ << "," << endl << indent() << "\"enumerations\" : {" << endl;
    indent_up();
    // Generate enums
    vector<t_enum*> enums = program_->get_enums();
    vector<t_enum*>::iterator en_iter;
    for (en_iter = enums.begin(); en_iter != enums.end(); ++en_iter) {
      if (en_iter != enums.begin()) {
        f_out_ << "," << endl;
      }
      generate_enum(*en_iter);
    }
    f_out_ << endl;
    indent_down();
    indent(f_out_) << "}";
  }

  if (!program_->get_typedefs().empty()) {
    f_out_ << "," << endl << indent() << "\"typedefs\" : {" << endl;
    indent_up();
    // Generate typedefs
    vector<t_typedef*> typedefs = program_->get_typedefs();
    vector<t_typedef*>::iterator td_iter;
    for (td_iter = typedefs.begin(); td_iter != typedefs.end(); ++td_iter) {
      if (td_iter != typedefs.begin()) {
        f_out_ << "," << endl;
      }
      generate_typedef(*td_iter);
    }
    f_out_ << endl;
    indent_down();
    indent(f_out_) << "}";
  }

  if (!program_->get_objects().empty()) {
    f_out_ << "," << endl << indent() << "\"structs\" : {" << endl;
    indent_up();
    // Generate structs and exceptions in declared order
    vector<t_struct*> objects = program_->get_objects();
    vector<t_struct*>::iterator o_iter;
    for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
      if (o_iter != objects.begin()) {
        f_out_ << "," << endl;
      }
      if ((*o_iter)->is_xception()) {
        generate_xception(*o_iter);
      } else {
        generate_struct(*o_iter);
      }
    }
    f_out_ << endl;
    indent_down();
    indent(f_out_) << "}";
  }

  if (!program_->get_services().empty()) {
    f_out_ << "," << endl << indent() << "\"services\" : {" << endl;
    indent_up();
    // Generate services
    vector<t_service*> services = program_->get_services();
    vector<t_service*>::iterator sv_iter;
    for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
      service_name_ = get_service_name(*sv_iter);
      if (sv_iter != services.begin()) {
        f_out_ << "," << endl;
      }
      generate_service(*sv_iter);
    }
    f_out_ << endl;
    indent_down();
    indent(f_out_) << "}";
  }

  f_out_ << endl;
  indent_down();
  indent(f_out_) << "}" << endl;
  f_out_.close();
}

/**
 * Converts the parse type to a string
 */
string t_json_generator::type_to_string(t_type* type) {
  type = get_true_type(type);
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      return "VOID";
    case t_base_type::TYPE_STRING:
      return "STRING";
    case t_base_type::TYPE_BOOL:
      return "BOOL";
    case t_base_type::TYPE_BYTE:
      return "BYTE";
    case t_base_type::TYPE_I16:
      return "I16";
    case t_base_type::TYPE_I32:
      return "I32";
    case t_base_type::TYPE_I64:
      return "I64";
    case t_base_type::TYPE_DOUBLE:
      return "DOUBLE";
    case t_base_type::TYPE_FLOAT:
      return "FLOAT";
    }
  } else if (type->is_enum()) {
    return "ENUM";
  } else if (type->is_struct() || type->is_xception()) {
    return "STRUCT";
  } else if (type->is_map()) {
    return "MAP";
  } else if (type->is_set()) {
    return "SET";
  } else if (type->is_list()) {
    return "LIST";
  } else if (type->is_service()) {
    return "SERVICE";
  }

  throw "INVALID TYPE IN type_to_string: " + type->get_name();
}

/**
 * Returns a string containing a type spec for the provided type.
 * The specification has the following structure:
 *   type_enum -> STRING | BOOL | BYTE | ...
 *   tuple_spec -> { "type_enum" : type_enum, "spec_args" : spec_args }
 *   spec_args -> null  // (for base types)
 *              | tuple_spec  // (for lists and sets)
 *              | { "key_type" : tuple_spec, "val_type" : tuple_spec}  // (maps)
 */
string t_json_generator::type_to_spec_args(t_type* ttype) {
  ttype = get_true_type(ttype);

  if (ttype->is_base_type()) {
    return "null";
  } else if (ttype->is_struct() || ttype->is_xception() ||
             ttype->is_service() || ttype->is_enum()) {
    string module = "";
    if (ttype->get_program() != program_) {
      module = ttype->get_program()->get_name() + ".";
    }
    return "\"" + module + ttype->get_name() + "\"";
  } else if (ttype->is_map()) {
    return "{ \"key_type\" : { \"type_enum\" : \""
      + type_to_string(((t_map*)ttype)->get_key_type())
      + "\", \"spec_args\" : "
      + type_to_spec_args(((t_map*)ttype)->get_key_type())
      + " }, \"val_type\" : { \"type_enum\" : \""
      + type_to_string(((t_map*)ttype)->get_val_type())
      + "\", \"spec_args\" : "
      + type_to_spec_args(((t_map*)ttype)->get_val_type())
      + "} } ";
  } else if (ttype->is_set() || ttype->is_list()) {
    return "{ \"type_enum\" : \""
      + type_to_string(((t_set*)ttype)->get_elem_type())
      + "\", \"spec_args\" : "
      + type_to_spec_args(((t_set*)ttype)->get_elem_type())
      + "} ";
  }

  throw "INVALID TYPE IN type_to_spec_args: " + ttype->get_name();
}

/**
 * Prints out the provided type spec
 */
void t_json_generator::print_type(t_type* ttype) {
  indent(f_out_) << "\"type_enum\" : \""
    << type_to_string(ttype) << "\"," << endl;
  indent(f_out_) << "\"spec_args\" : " << type_to_spec_args(ttype);
}

/**
 * Prints out a JSON representation of the provided constant map key.
 * The JSON spec allows for strings, and nothing else.
 * TODO - support serialization of complex keys...
 */
void t_json_generator::print_const_key(t_const_value* tvalue) {
  switch (tvalue->get_type()) {
    case t_const_value::CV_INTEGER:
      f_out_ << "\"" << tvalue->get_integer() << "\"";
      break;
    case t_const_value::CV_DOUBLE:
      f_out_ << "\"" << tvalue->get_double() << "\"";
      break;
    case t_const_value::CV_STRING:
      f_out_ << "\"" << tvalue->get_string() << "\"";
      break;
    case t_const_value::CV_MAP:
    case t_const_value::CV_LIST:
    default:
      {
        std::ostringstream msg;
        msg << "INVALID TYPE IN print_const_key: " << tvalue->get_type();
        throw msg.str();
      }
  }
}

/**
 * Prints out a JSON representation of the provided constant value
 */
void t_json_generator::print_const_value(const t_const_value* tvalue) {
  bool first = true;
  switch (tvalue->get_type()) {
  case t_const_value::CV_INTEGER:
    f_out_ << tvalue->get_integer();
    break;
  case t_const_value::CV_DOUBLE:
    f_out_ << tvalue->get_double();
    break;
  case t_const_value::CV_STRING:
    f_out_ << "\"" << tvalue->get_string() << "\"";
    break;
  case t_const_value::CV_MAP:
    {
      f_out_ << "{ ";
      const vector<pair<t_const_value*, t_const_value*>>& map_elems =
        tvalue->get_map();
      vector<pair<t_const_value*, t_const_value*>>::const_iterator map_iter;
      for (map_iter = map_elems.begin(); map_iter != map_elems.end();
           map_iter++) {
        if (!first) {
          f_out_ << ", ";
        }
        first = false;
        print_const_key(map_iter->first);
        f_out_ << " : ";
        print_const_value(map_iter->second);
      }
      f_out_ << " }";
    }
    break;
  case t_const_value::CV_LIST:
    {
      f_out_ << "[ ";
      vector<t_const_value*> list_elems = tvalue->get_list();;
      vector<t_const_value*>::iterator list_iter;
      for (list_iter = list_elems.begin(); list_iter != list_elems.end();
           list_iter++) {
        if (!first) {
          f_out_ << ", ";
        }
        first = false;
        print_const_value(*list_iter);
      }
      f_out_ << " ]";
    }
    break;
  default:
    f_out_ << "UNKNOWN";
    break;
  }
}

/**
 * Generates a typedef.
 *
 * @param ttypedef The type definition
 */
void t_json_generator::generate_typedef(t_typedef* ttypedef) {
  indent(f_out_) << "\"" << ttypedef->get_name() << "\" : {" << endl;
  indent_up();
  print_type(ttypedef->get_type());
  f_out_ << endl;
  indent_down();
  indent(f_out_) << "}";
}

/**
 * Generates code for an enumerated type.
 *
 * @param tenum The enumeration
 */
void t_json_generator::generate_enum(t_enum* tenum) {
  indent(f_out_) << "\"" << tenum->get_name() << "\" : {" << endl;
  indent_up();
  vector<t_enum_value*> values = tenum->get_constants();
  vector<t_enum_value*>::iterator val_iter;
  for (val_iter = values.begin(); val_iter != values.end(); ++val_iter) {
    if (val_iter != values.begin()) {
      f_out_ << "," << endl;
    }
    indent(f_out_) << "\"" << (*val_iter)->get_name() << "\"" << " : "
      << (*val_iter)->get_value();
  }
  f_out_ << endl;
  indent_down();
  indent(f_out_) << "}";
}

/**
 * Generate constants
 */
void t_json_generator::generate_consts(vector<t_const*> consts) {
  vector<t_const*>::iterator c_iter;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    if (c_iter != consts.begin()) {
      f_out_ << "," << endl;
    }
    generate_const(*c_iter);
  }
}


/**
 * Generates a constant value
 */
void t_json_generator::generate_const(t_const* tconst) {
  string name = tconst->get_name();
  indent(f_out_) << "\"" << name << "\" : {" << endl;
  indent_up();
  indent(f_out_) << "\"value\" : ";
  print_const_value(tconst->get_value());
  f_out_ << "," << endl;
  print_type(tconst->get_type());
  f_out_ << endl;
  indent_down();
  indent(f_out_) << "}";
}

/**
 * Generates a struct definition for a thrift data type.
 *
 * @param tstruct The struct definition
 */
void t_json_generator::generate_struct(t_struct* tstruct) {
  string name = tstruct->get_name();
  indent(f_out_) << "\"" << name << "\" : {" << endl;
  indent_up();
  indent(f_out_) << "\"is_exception\" : "
    << (tstruct->is_xception() ? "true" : "false") << "," << endl;
  indent(f_out_) << "\"is_union\" : "
    << (tstruct->is_union() ? "true" : "false") << "," << endl;

  vector<t_field*> members = tstruct->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();
  indent(f_out_) << "\"fields\" : {" << endl;
  indent_up();
  for ( ; mem_iter != members.end(); mem_iter++) {
    if (mem_iter != members.begin()) {
      f_out_ << "," << endl;
    }
    indent(f_out_) << "\"" << (*mem_iter)->get_name() << "\" : {" << endl;
    indent_up();
    print_type((*mem_iter)->get_type());
    f_out_ << "," << endl << indent() <<  "\"required\" : "
      << ((*mem_iter)->get_req() != t_field::T_OPTIONAL ? "true" : "false");
    const t_const_value* default_val = (*mem_iter)->get_value();
    if (default_val != nullptr) {
      f_out_ << "," << endl << indent() << "\"default_value\" : ";
      print_const_value(default_val);
    }
    f_out_ << endl;
    indent_down();
    indent(f_out_) << "}";
  }
  f_out_ << endl;
  indent_down();
  indent(f_out_) << "}" << endl;

  indent_down();
  indent(f_out_) << "}";
}

/**
 * Exceptions are special structs
 *
 * @param tstruct The struct definition
 */
void t_json_generator::generate_xception(t_struct* txception) {
  generate_struct(txception);
}

/**
 * Generates the JSON object for a Thrift service.
 *
 * @param tservice The service definition
 */
void t_json_generator::generate_service(t_service* tservice) {
  indent(f_out_) << "\"" << service_name_ << "\" : {" << endl;
  indent_up();

  bool first = true;

  if (tservice->get_extends()) {
    indent(f_out_) << "\"extends\" : {" << endl;
    indent_up();
    print_type(tservice->get_extends());
    f_out_ << endl;
    indent_down();
    indent(f_out_) << "}";
    first = false;
  }
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator fn_iter = functions.begin();
  if (!first) {
    f_out_ << "," << endl;
  }
  f_out_ << indent() << "\"functions\" : {" << endl;
  indent_up();
  for ( ; fn_iter != functions.end(); fn_iter++) {
    if (fn_iter != functions.begin()) {
      f_out_ << "," << endl;
    }
    string fn_name = (*fn_iter)->get_name();
    indent(f_out_) << "\"" << service_name_ << "." << fn_name
      << "\" : {" << endl;
    indent_up();
    indent(f_out_) << "\"return_type\" : {" << endl;
    indent_up();
    print_type((*fn_iter)->get_returntype());
    f_out_ << endl;
    indent_down();
    indent(f_out_) << "}," << endl;

    indent(f_out_) << "\"args\" : [";
    vector<t_field*> args = (*fn_iter)->get_arglist()->get_members();
    vector<t_field*>::iterator arg_iter = args.begin();
    if (arg_iter != args.end()) {
      f_out_ << endl;
      indent_up();
      for ( ; arg_iter != args.end(); arg_iter++) {
        if (arg_iter != args.begin()) {
          f_out_ << "," << endl;
        }
        indent(f_out_) << "{" << endl;
        indent_up();
        print_type((*arg_iter)->get_type());
        if ((*arg_iter)->get_value() != nullptr) {
          f_out_ << "," << endl << indent() << "\"value\" : ";
          print_const_value((*arg_iter)->get_value());
        }
        f_out_ << endl;
        indent_down();
        indent(f_out_) << "}";
      }
      f_out_ << endl;
      indent_down();
      indent(f_out_);
    }
    f_out_ << "]," << endl;

    indent(f_out_) << "\"throws\" : [";
    vector<t_field*> excepts = (*fn_iter)->get_xceptions()->get_members();
    vector<t_field*>::iterator ex_iter = excepts.begin();
    if (ex_iter != excepts.end()) {
      f_out_ << endl;
      indent_up();
      for ( ; ex_iter != excepts.end(); ex_iter++) {
        if (ex_iter != excepts.begin()) {
          f_out_ << "," << endl;
        }
        indent(f_out_) << type_to_spec_args((*ex_iter)->get_type());
      }
      f_out_ << endl;
      indent_down();
      indent(f_out_);
    }
    f_out_ << "]" << endl;
    indent_down();
    indent(f_out_) << "}";
  }
  f_out_ << endl;
  indent_down();
  indent(f_out_) << "}";

  f_out_ << endl;
  indent_down();
  indent(f_out_) << "}";
}

THRIFT_REGISTER_GENERATOR(json, "JSON", "");
