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

#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <cctype>

#include <sys/stat.h>
#include <stdexcept>

#include "thrift/compiler/platform.h"
#include "thrift/compiler/generate/t_oop_generator.h"
using namespace std;


/**
 * Java code generator.
 *
 */
class t_java_generator : public t_oop_generator {
 public:
  t_java_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
    : t_oop_generator(program)
  {
    std::map<std::string, std::string>::const_iterator iter;

    iter = parsed_options.find("beans");
    bean_style_ = (iter != parsed_options.end());

    iter = parsed_options.find("nocamel");
    nocamel_style_ = (iter != parsed_options.end());

    iter = parsed_options.find("hashcode");
    gen_hash_code_ = (iter != parsed_options.end());

    out_dir_base_ = (bean_style_ ? "gen-javabean" : "gen-java");
  }

  /**
   * Init and close methods
   */

  void init_generator();
  void close_generator();

  void generate_consts(std::vector<t_const*> consts);

  /**
   * Program-level generation functions
   */

  void generate_typedef (t_typedef*  ttypedef);
  void generate_enum    (t_enum*     tenum);
  void generate_struct  (t_struct*   tstruct);
  void generate_union   (t_struct*   tunion);
  void generate_xception(t_struct*   txception);
  void generate_service (t_service*  tservice);
  void generate_default_toString(ofstream&, t_struct*);
  void generate_toString_prettyprint(std::ofstream&);

  void print_const_value(std::ofstream& out, std::string name, t_type* type, t_const_value* value, bool in_static, bool defval=false);
  std::string render_const_value(std::ofstream& out, std::string name, t_type* type, t_const_value* value);

  /**
   * Service-level generation functions
   */

  void generate_java_struct(t_struct* tstruct, bool is_exception);

  void generate_java_constructor(ofstream &out, t_struct* tstruct, const vector<t_field*>& fields);
  void generate_java_struct_definition(std::ofstream& out, t_struct* tstruct, bool is_xception=false, bool in_class=false, bool is_result=false);
  void generate_java_struct_equality(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_compare_to(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_reader(std::ofstream& out, t_struct* tstruct);
  void generate_java_validator(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_result_writer(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_writer(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_tostring(std::ofstream& out, t_struct* tstruct);
  void generate_java_meta_data_map(std::ofstream& out, t_struct* tstruct);
  void generate_field_value_meta_data(std::ofstream& out, t_type* type);
  std::string get_java_type_string(t_type* type);
  void generate_reflection_setters(std::ostringstream& out, t_type* type, std::string field_name, std::string cap_name);
  void generate_reflection_getters(std::ostringstream& out, t_type* type, std::string field_name, std::string cap_name);
  void generate_generic_field_getters_setters(std::ofstream& out, t_struct* tstruct);
  void generate_generic_isset_method(std::ofstream& out, t_struct* tstruct);
  void generate_java_bean_boilerplate(std::ofstream& out, t_struct* tstruct);
  std::string get_simple_getter_name(t_field* field);

  void generate_function_helpers(t_function* tfunction);
  std::string get_cap_name(std::string name);
  std::string generate_isset_check(t_field* field);
  std::string generate_isset_check(std::string field);
  std::string generate_setfield_check(t_field* field);
  std::string generate_setfield_check(std::string field);
  void generate_isset_set(ofstream& out, t_field* field);
  std::string isset_field_id(t_field* field);

  void generate_service_interface (t_service* tservice);
  void generate_service_async_interface(t_service* tservice);
  void generate_service_helpers   (t_service* tservice);
  void generate_service_client    (t_service* tservice);
  void generate_service_async_client(t_service* tservice);
  void generate_service_server    (t_service* tservice);
  void generate_process_function  (t_service* tservice, t_function* tfunction);

  void generate_java_union(t_struct* tstruct);
  void generate_union_constructor(ofstream& out, t_struct* tstruct);
  void generate_union_getters_and_setters(ofstream& out, t_struct* tstruct);
  void generate_union_abstract_methods(ofstream& out, t_struct* tstruct);
  void generate_check_type(ofstream& out, t_struct* tstruct);
  void generate_read_value(ofstream& out, t_struct* tstruct);
  void generate_write_value(ofstream& out, t_struct* tstruct);
  void generate_get_field_desc(ofstream& out, t_struct* tstruct);
  void generate_get_struct_desc(ofstream& out, t_struct* tstruct);
  void generate_get_field_name(ofstream& out, t_struct* tstruct);

  void generate_union_comparisons(ofstream& out, t_struct* tstruct);
  void generate_union_hashcode(ofstream& out, t_struct* tstruct);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field        (std::ofstream& out,
                                          t_field*    tfield,
                                          std::string prefix="");

  void generate_deserialize_struct       (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_deserialize_container    (std::ofstream& out,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_deserialize_set_element  (std::ofstream& out,
                                          t_set*      tset,
                                          std::string prefix="");

  void generate_deserialize_map_element  (std::ofstream& out,
                                          t_map*      tmap,
                                          std::string prefix="");

  void generate_deserialize_list_element (std::ofstream& out,
                                          t_list*     tlist,
                                          std::string prefix="");

  void generate_serialize_field          (std::ofstream& out,
                                          t_field*    tfield,
                                          std::string prefix="");

  void generate_serialize_struct         (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_serialize_container      (std::ofstream& out,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_serialize_map_element    (std::ofstream& out,
                                          t_map*      tmap,
                                          std::string iter,
                                          std::string map);

  void generate_serialize_set_element    (std::ofstream& out,
                                          t_set*      tmap,
                                          std::string iter);

  void generate_serialize_list_element   (std::ofstream& out,
                                          t_list*     tlist,
                                          std::string iter);

  void generate_java_doc                 (std::ofstream& out,
                                          t_field*    field);

  void generate_java_doc                 (std::ofstream& out,
                                          t_doc*      tdoc);

  void generate_java_doc                 (std::ofstream& out,
                                          t_function* tdoc);

  void generate_java_docstring_comment   (std::ofstream &out,
                                          string contents);

  void generate_deep_copy_container(std::ofstream& out, std::string source_name_p1, std::string source_name_p2, std::string result_name, t_type* type);
  void generate_deep_copy_non_container(std::ofstream& out, std::string source_name, std::string dest_name, t_type* type);

  bool is_comparable(t_struct* tstruct);
  bool is_comparable(t_type* type);

  bool has_bit_vector(t_struct* tstruct);

  /**
   * Helper rendering functions
   */

  std::string java_package();
  std::string java_type_imports();
  std::string java_thrift_imports();
  std::string type_name(t_type* ttype, bool in_container=false, bool in_init=false, bool skip_generic=false);
  std::string base_type_name(t_base_type* tbase, bool in_container=false);
  std::string declare_field(t_field* tfield, bool init=false);
  std::string function_signature(t_function* tfunction, std::string prefix="");
  std::string function_signature_async(t_function* tfunction, std::string result_handler_symbol, bool use_base_method = false, std::string prefix="");
  std::string argument_list(t_struct* tstruct, bool include_types = true);
  std::string async_function_call_arglist(t_function* tfunc, std::string result_handler_symbol, bool use_base_method = true, bool include_types = true);
  std::string async_argument_list(t_function* tfunct, t_struct* tstruct, std::string result_handler_symbol, bool include_types=false);
  std::string type_to_enum(t_type* ttype);
  std::string get_enum_class_name(t_type* type);
  void generate_struct_desc(ofstream& out, t_struct* tstruct);
  void generate_field_descs(ofstream& out, t_struct* tstruct);
  void generate_field_name_constants(ofstream& out, t_struct* tstruct);

  bool type_can_be_null(t_type* ttype) {
    ttype = get_true_type(ttype);

    return
      ttype->is_container() ||
      ttype->is_struct() ||
      ttype->is_xception() ||
      ttype->is_string();
  }

  std::string constant_name(std::string name);

 private:

  /**
   * File streams
   */

  std::string package_name_;
  std::ofstream f_service_;
  std::string package_dir_;

  bool bean_style_;
  bool nocamel_style_;
  bool gen_hash_code_;

};


/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_java_generator::init_generator() {
  // Make output directory
  MKDIR(get_out_dir().c_str());
  package_name_ = program_->get_namespace("java");

  string dir = package_name_;
  string subdir = get_out_dir();
  string::size_type loc;
  while ((loc = dir.find(".")) != string::npos) {
    subdir = subdir + "/" + dir.substr(0, loc);
    MKDIR(subdir.c_str());
    dir = dir.substr(loc+1);
  }
  if (dir.size() > 0) {
    subdir = subdir + "/" + dir;
    MKDIR(subdir.c_str());
  }

  package_dir_ = subdir;
}

/**
 * Packages the generated file
 *
 * @return String of the package, i.e. "package com.facebook.thriftdemo;"
 */
string t_java_generator::java_package() {
  if (!package_name_.empty()) {
    return string("package ") + package_name_ + ";\n\n";
  }
  return "";
}

/**
 * Prints standard java imports
 *
 * @return List of imports for Java types that are used in here
 */
string t_java_generator::java_type_imports() {
  string hash_builder;
  if (gen_hash_code_) {
    hash_builder = "import org.apache.commons.lang.builder.HashCodeBuilder;\n";
  }

  return
    string() +
    hash_builder +
    "import java.util.List;\n" +
    "import java.util.ArrayList;\n" +
    "import java.util.Map;\n" +
    "import java.util.HashMap;\n" +
    "import java.util.Set;\n" +
    "import java.util.HashSet;\n" +
    "import java.util.Collections;\n" +
    "import java.util.BitSet;\n" +
    "import java.util.Arrays;\n" +
    "import org.slf4j.Logger;\n" +
    "import org.slf4j.LoggerFactory;\n\n";
}

/**
 * Prints standard java imports
 *
 * @return List of imports necessary for thrift
 */
string t_java_generator::java_thrift_imports() {
  return
    string() +
    "import com.facebook.thrift.*;\n" +
    "import com.facebook.thrift.async.*;\n" +
    "import com.facebook.thrift.meta_data.*;\n" +
    "import com.facebook.thrift.server.*;\n" +
    "import com.facebook.thrift.transport.*;\n" +
    "import com.facebook.thrift.protocol.*;\n\n";
}

/**
 * Nothing in Java
 */
void t_java_generator::close_generator() {}

/**
 * Generates a typedef. This is not done in Java, since it does
 * not support arbitrary name replacements, and it'd be a wacky waste
 * of overhead to make wrapper classes.
 *
 * @param ttypedef The type definition
 */
void t_java_generator::generate_typedef(t_typedef* ttypedef) {}

/**
 * Enums are a class with a set of static constants.
 *
 * @param tenum The enumeration
 */
void t_java_generator::generate_enum(t_enum* tenum) {
  // Make output file
  string f_enum_name = package_dir_+"/"+(tenum->get_name())+".java";
  ofstream f_enum;
  f_enum.open(f_enum_name.c_str());
  record_genfile(f_enum_name);

  // Comment and package it
  f_enum <<
    autogen_comment() <<
    java_package() << endl;

  // Add java imports
  f_enum << string() +
    "import java.util.Set;\n" +
    "import java.util.HashSet;\n" +
    "import java.util.Collections;\n" +
    "import com.facebook.thrift.IntRangeSet;\n" +
    "import java.util.Map;\n" +
    "import java.util.HashMap;\n" << endl;

  f_enum <<
    "public class " << tenum->get_name() << " ";
  scope_up(f_enum);

  vector<t_enum_value*> constants = tenum->get_constants();
  vector<t_enum_value*>::iterator c_iter;
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int32_t value = (*c_iter)->get_value();

    generate_java_doc(f_enum, *c_iter);
    indent(f_enum) <<
      "public static final int " << (*c_iter)->get_name() <<
      " = " << value << ";" << endl;
  }

  // Create a static Set with all valid values for this enum
  f_enum << endl;
  indent(f_enum) << "public static final IntRangeSet VALID_VALUES = new IntRangeSet(";
  indent_up();
  bool first = true;
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    // populate set
    f_enum << (first ? "" : ", ") << endl;
    first = false;
    indent(f_enum) << (*c_iter)->get_name();
  }
  f_enum << " );" << endl << endl;
  indent_down();

  indent(f_enum) << "public static final Map<Integer, String> VALUES_TO_NAMES = new HashMap<Integer, String>() {{" << endl;

  indent_up();
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    indent(f_enum) << "put(" << (*c_iter)->get_name() << ", \"" << (*c_iter)->get_name() <<"\");" << endl;
  }
  indent_down();


  indent(f_enum) << "}};" << endl;

  scope_down(f_enum);

  f_enum.close();
}

/**
 * Generates a class that holds all the constants.
 */
void t_java_generator::generate_consts(std::vector<t_const*> consts) {
  if (consts.empty()) {
    return;
  }

  string f_consts_name = package_dir_+"/Constants.java";
  ofstream f_consts;
  f_consts.open(f_consts_name.c_str());
  record_genfile(f_consts_name);

  // Print header
  f_consts <<
    autogen_comment() <<
    java_package() <<
    java_type_imports();

  f_consts <<
    "public class Constants {" << endl <<
    endl;
  indent_up();
  vector<t_const*>::iterator c_iter;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    print_const_value(f_consts,
                      (*c_iter)->get_name(),
                      (*c_iter)->get_type(),
                      (*c_iter)->get_value(),
                      false);
  }
  indent_down();
  indent(f_consts) <<
    "}" << endl;
  f_consts.close();
}


/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
void t_java_generator::print_const_value(std::ofstream& out, string name, t_type* type, t_const_value* value, bool in_static, bool defval) {
  type = get_true_type(type);

  indent(out);
  if (!defval) {
    out <<
      (in_static ? "" : "public static final ") <<
      type_name(type) << " ";
  }
  if (type->is_base_type()) {
    string v2 = render_const_value(out, name, type, value);
    out << name << " = " << v2 << ";" << endl << endl;
  } else if (type->is_enum()) {
    out << name << " = " << value->get_integer() << ";" << endl << endl;
  } else if (type->is_struct() || type->is_xception()) {
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    out << name << " = new " << type_name(type, false, true) << "();" << endl;
    if (!in_static) {
      indent(out) << "static {" << endl;
      indent_up();
    }
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      t_type* field_type = NULL;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == NULL) {
        throw "type error: " + type->get_name() + " has no field " + v_iter->first->get_string();
      }
      string val = render_const_value(out, name, field_type, v_iter->second);
      indent(out) << name << ".";
      std::string cap_name = get_cap_name(v_iter->first->get_string());
      out << "set" << cap_name << "(" << val << ");" << endl;
    }
    if (!in_static) {
      indent_down();
      indent(out) << "}" << endl;
    }
    out << endl;
  } else if (type->is_map()) {
    out << name << " = new " << type_name(type, false, true) << "();" << endl;
    if (!in_static) {
      indent(out) << "static {" << endl;
      indent_up();
    }
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string key = render_const_value(out, name, ktype, v_iter->first);
      string val = render_const_value(out, name, vtype, v_iter->second);
      indent(out) << name << ".put(" << key << ", " << val << ");" << endl;
    }
    if (!in_static) {
      indent_down();
      indent(out) << "}" << endl;
    }
    out << endl;
  } else if (type->is_list() || type->is_set()) {
    out << name << " = new " << type_name(type, false, true) << "();" << endl;
    if (!in_static) {
      indent(out) << "static {" << endl;
      indent_up();
    }
    t_type* etype;
    if (type->is_list()) {
      etype = ((t_list*)type)->get_elem_type();
    } else {
      etype = ((t_set*)type)->get_elem_type();
    }
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string val = render_const_value(out, name, etype, *v_iter);
      indent(out) << name << ".add(" << val << ");" << endl;
    }
    if (!in_static) {
      indent_down();
      indent(out) << "}" << endl;
    }
    out << endl;
  } else {
    throw "compiler error: no const of type " + type->get_name();
  }
}

string t_java_generator::render_const_value(ofstream& out, string name, t_type* type, t_const_value* value) {
  type = get_true_type(type);
  std::ostringstream render;

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      render << "\"" + value->get_string() + "\"";
      break;
    case t_base_type::TYPE_BOOL:
      render << ((value->get_integer() > 0) ? "true" : "false");
      break;
    case t_base_type::TYPE_BYTE:
      render << "(byte)" << value->get_integer();
      break;
    case t_base_type::TYPE_I16:
      render << "(short)" << value->get_integer();
      break;
    case t_base_type::TYPE_I32:
      render << value->get_integer();
      break;
    case t_base_type::TYPE_I64:
      render << value->get_integer() << "L";
      break;
    case t_base_type::TYPE_DOUBLE:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        render << "(double)" << value->get_integer();
      } else {
        render << value->get_double();
      }
      break;
    case t_base_type::TYPE_FLOAT:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        render << "(float)" << value->get_integer();
      } else {
        render << "(float)" << value->get_double();
      }
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    render << value->get_integer();
  } else {
    string t = tmp("tmp");
    print_const_value(out, t, type, value, true);
    render << t;
  }

  return render.str();
}

/**
 * Generates a struct definition for a thrift data type. This will be a TBase
 * implementor.
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_struct(t_struct* tstruct) {
  if (tstruct->is_union()) {
    generate_java_union(tstruct);
  } else {
    generate_java_struct(tstruct, false);
  }
}

/**
 * Exceptions are structs, but they inherit from Exception
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_xception(t_struct* txception) {
  generate_java_struct(txception, true);
}


/**
 * Java struct definition.
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_struct(t_struct* tstruct,
                                            bool is_exception) {
  // Make output file
  string f_struct_name = package_dir_+"/"+(tstruct->get_name())+".java";
  ofstream f_struct;
  f_struct.open(f_struct_name.c_str());
  record_genfile(f_struct_name);

  f_struct <<
    autogen_comment() <<
    java_package() <<
    java_type_imports() <<
    java_thrift_imports();

  generate_java_struct_definition(f_struct,
                                  tstruct,
                                  is_exception);
  f_struct.close();
}

/**
 * Java union definition.
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_union(t_struct* tstruct) {
  // Make output file
  string f_struct_name = package_dir_+"/"+(tstruct->get_name())+".java";
  ofstream f_struct;
  f_struct.open(f_struct_name.c_str());
  record_genfile(f_struct_name);

  f_struct <<
    autogen_comment() <<
    java_package() <<
    java_type_imports() <<
    java_thrift_imports();

  generate_java_doc(f_struct, tstruct);

  bool is_final = (tstruct->annotations_.find("final") != tstruct->annotations_.end());

  indent(f_struct) <<
    "public " << (is_final ? "final " : "") << "class " << tstruct->get_name()
    << " extends TUnion ";

  if (is_comparable(tstruct)) {
    f_struct << "implements Comparable<" << type_name(tstruct) << "> ";
  }

  scope_up(f_struct);

  indent(f_struct) << "public static boolean DEFAULT_PRETTY_PRINT = true;" << endl;

  generate_struct_desc(f_struct, tstruct);
  generate_field_descs(f_struct, tstruct);

  f_struct << endl;

  generate_field_name_constants(f_struct, tstruct);

  f_struct << endl;

  generate_java_meta_data_map(f_struct, tstruct);

  generate_union_constructor(f_struct, tstruct);

  f_struct << endl;

  generate_union_abstract_methods(f_struct, tstruct);

  f_struct << endl;

  generate_union_getters_and_setters(f_struct, tstruct);

  f_struct << endl;

  generate_union_comparisons(f_struct, tstruct);

  f_struct << endl;

  generate_union_hashcode(f_struct, tstruct);

  f_struct << endl;

  generate_java_struct_tostring(f_struct, tstruct);

  f_struct << endl;

  scope_down(f_struct);

  f_struct.close();
}

void t_java_generator::generate_union_constructor(ofstream& out, t_struct* tstruct) {
  indent(out) << "public " << type_name(tstruct) << "() {" << endl;
  indent(out) << "  super();" << endl;
  indent(out) << "}" << endl << endl;

  indent(out) << "public " << type_name(tstruct) << "(int setField, Object value) {" << endl;
  indent(out) << "  super(setField, value);" << endl;
  indent(out) << "}" << endl << endl;

  indent(out) << "public " << type_name(tstruct) << "(" << type_name(tstruct) << " other) {" << endl;
  indent(out) << "  super(other);" << endl;
  indent(out) << "}" << endl;

  indent(out) << "public " << tstruct->get_name() << " deepCopy() {" << endl;
  indent(out) << "  return new " << tstruct->get_name() << "(this);" << endl;
  indent(out) << "}" << endl << endl;

  // generate "constructors" for each field
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out) << "public static " << type_name(tstruct) << " " << (*m_iter)->get_name() << "(" << type_name((*m_iter)->get_type()) << " value) {" << endl;
    indent(out) << "  " << type_name(tstruct) << " x = new " << type_name(tstruct) << "();" << endl;
    indent(out) << "  x.set" << get_cap_name((*m_iter)->get_name()) << "(value);" << endl;
    indent(out) << "  return x;" << endl;
    indent(out) << "}" << endl << endl;
  }
}

void t_java_generator::generate_union_getters_and_setters(ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  bool first = true;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (first) {
      first = false;
    } else {
      out << endl;
    }

    t_field* field = (*m_iter);

    generate_java_doc(out, field);
    indent(out) << "public " << type_name(field->get_type())
                << " " << get_simple_getter_name(field) << "() {" << endl;
    indent(out) << "  if (" << generate_setfield_check(field) << ") {" << endl;
    indent(out) << "    return (" << type_name(field->get_type(), true) << ")getFieldValue();" << endl;
    indent(out) << "  } else {" << endl;
    indent(out) << "    throw new RuntimeException(\"Cannot get field '" << field->get_name()
      << "' because union is currently set to \" + getFieldDesc(getSetField()).name);" << endl;
    indent(out) << "  }" << endl;
    indent(out) << "}" << endl;

    out << endl;

    generate_java_doc(out, field);
    indent(out) << "public void set" << get_cap_name(field->get_name()) << "(" << type_name(field->get_type()) << " value) {" << endl;
    if (type_can_be_null(field->get_type())) {
      indent(out) << "  if (value == null) throw new NullPointerException();" << endl;
    }
    indent(out) << "  setField_ = " << upcase_string(field->get_name()) << ";" << endl;
    indent(out) << "  value_ = value;" << endl;
    indent(out) << "}" << endl;
  }
}

void t_java_generator::generate_union_abstract_methods(ofstream& out, t_struct* tstruct) {
  generate_check_type(out, tstruct);
  out << endl;
  generate_read_value(out, tstruct);
  out << endl;
  generate_write_value(out, tstruct);
  out << endl;
  generate_get_field_desc(out, tstruct);
  out << endl;
  generate_get_struct_desc(out, tstruct);
}

void t_java_generator::generate_check_type(ofstream& out, t_struct* tstruct) {
  indent(out) << "@Override" << endl;
  indent(out) << "protected void checkType(short setField, Object value) throws ClassCastException {" << endl;
  indent_up();

  indent(out) << "switch (setField) {" << endl;
  indent_up();

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_field* field = (*m_iter);

    indent(out) << "case " << upcase_string(field->get_name()) << ":" << endl;
    indent(out) << "  if (value instanceof " << type_name(field->get_type(), true, false, true) << ") {" << endl;
    indent(out) << "    break;" << endl;
    indent(out) << "  }" << endl;
    indent(out) << "  throw new ClassCastException(\"Was expecting value of type "
      << type_name(field->get_type(), true, false) << " for field '" << field->get_name()
      << "', but got \" + value.getClass().getSimpleName());" << endl;
    // do the real check here
  }

  indent(out) << "default:" << endl;
  indent(out) << "  throw new IllegalArgumentException(\"Unknown field id \" + setField);" << endl;

  indent_down();
  indent(out) << "}" << endl;

  indent_down();
  indent(out) << "}" << endl;
}

void t_java_generator::generate_read_value(ofstream& out, t_struct* tstruct) {
  indent(out) << "@Override" << endl;
  indent(out) << "protected Object readValue(TProtocol iprot, TField field) throws TException {" << endl;

  indent_up();

  indent(out) << "switch (field.id) {" << endl;
  indent_up();

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_field* field = (*m_iter);

    indent(out) << "case " << upcase_string(field->get_name()) << ":" << endl;
    indent_up();
    indent(out) << "if (field.type == " << constant_name(field->get_name()) << "_FIELD_DESC.type) {" << endl;
    indent_up();
    indent(out) << type_name(field->get_type(), true, false) << " " << field->get_name() << ";" << endl;
    generate_deserialize_field(out, field, "");
    indent(out) << "return " << field->get_name() << ";" << endl;
    indent_down();
    indent(out) << "} else {" << endl;
    indent(out) << "  TProtocolUtil.skip(iprot, field.type);" << endl;
    indent(out) << "  return null;" << endl;
    indent(out) << "}" << endl;
    indent_down();
  }

  indent(out) << "default:" << endl;
  indent(out) << "  TProtocolUtil.skip(iprot, field.type);" << endl;
  indent(out) << "  return null;" << endl;

  indent_down();
  indent(out) << "}" << endl;

  indent_down();
  indent(out) << "}" << endl;
}

void t_java_generator::generate_write_value(ofstream& out, t_struct* tstruct) {
  indent(out) << "@Override" << endl;
  indent(out) << "protected void writeValue(TProtocol oprot, short setField, Object value) throws TException {" << endl;

  indent_up();

  indent(out) << "switch (setField) {" << endl;
  indent_up();

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_field* field = (*m_iter);

    indent(out) << "case " << upcase_string(field->get_name()) << ":" << endl;
    indent_up();
    indent(out) << type_name(field->get_type(), true, false) << " " << field->get_name()
      << " = (" <<  type_name(field->get_type(), true, false) << ")getFieldValue();" << endl;
    generate_serialize_field(out, field, "");
    indent(out) << "return;" << endl;
    indent_down();
  }

  indent(out) << "default:" << endl;
  indent(out) << "  throw new IllegalStateException(\"Cannot write union with unknown field \" + setField);" << endl;

  indent_down();
  indent(out) << "}" << endl;

  indent_down();



  indent(out) << "}" << endl;
}

void t_java_generator::generate_get_field_desc(ofstream& out, t_struct* tstruct) {
  indent(out) << "@Override" << endl;
  indent(out) << "protected TField getFieldDesc(int setField) {" << endl;
  indent_up();

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  indent(out) << "switch (setField) {" << endl;
  indent_up();

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_field* field = (*m_iter);
    indent(out) << "case " << upcase_string(field->get_name()) << ":" << endl;
    indent(out) << "  return " << constant_name(field->get_name()) << "_FIELD_DESC;" << endl;
  }

  indent(out) << "default:" << endl;
  indent(out) << "  throw new IllegalArgumentException(\"Unknown field id \" + setField);" << endl;

  indent_down();
  indent(out) << "}" << endl;

  indent_down();
  indent(out) << "}" << endl;
}

void t_java_generator::generate_get_struct_desc(ofstream& out, t_struct* tstruct) {
  indent(out) << "@Override" << endl;
  indent(out) << "protected TStruct getStructDesc() {" << endl;
  indent(out) << "  return STRUCT_DESC;" << endl;
  indent(out) << "}" << endl;
}

void t_java_generator::generate_union_comparisons(ofstream& out, t_struct* tstruct) {
  // equality
  indent(out) << "public boolean equals(Object other) {" << endl;
  indent(out) << "  if (other instanceof " << tstruct->get_name() << ") {" << endl;
  indent(out) << "    return equals((" << tstruct->get_name() << ")other);" << endl;
  indent(out) << "  } else {" << endl;
  indent(out) << "    return false;" << endl;
  indent(out) << "  }" << endl;
  indent(out) << "}" << endl;

  out << endl;

  indent(out) << "public boolean equals(" << tstruct->get_name() << " other) {" << endl;
  indent(out) << "  return getSetField() == other.getSetField() && ((value_ instanceof byte[]) ? " << endl;
  indent(out) << "    Arrays.equals((byte[])getFieldValue(), (byte[])other.getFieldValue()) : getFieldValue().equals(other.getFieldValue()));" << endl;
  indent(out) << "}" << endl;
  out << endl;

  if (is_comparable(tstruct)) {
    indent(out) << "@Override" << endl;
    indent(out) << "public int compareTo(" << type_name(tstruct) << " other) {" << endl;
    indent(out) << "  int lastComparison = TBaseHelper.compareTo(getSetField(), other.getSetField());" << endl;
    indent(out) << "  if (lastComparison != 0) {" << endl;
    indent(out) << "    return TBaseHelper.compareTo((Comparable)getFieldValue(), (Comparable)other.getFieldValue());" << endl;
    indent(out) << "  }" << endl;
    indent(out) << "  return lastComparison;" << endl;
    indent(out) << "}" << endl;
    out << endl;
  }
}

void t_java_generator::generate_union_hashcode(ofstream& out, t_struct* tstruct) {
  if (gen_hash_code_) {
    indent(out) << "@Override" << endl;
    indent(out) << "public int hashCode() {" << endl;
    indent(out) << "  return new HashCodeBuilder().append(getSetField()).append(getFieldValue()).toHashCode();" << endl;
    indent(out) << "}";
  } else {
    indent(out) << "/**" << endl;
    indent(out) << " * If you'd like this to perform more respectably, use the hashcode generator option." << endl;
    indent(out) << " */" << endl;
    indent(out) << "@Override" << endl;
    indent(out) << "public int hashCode() {" << endl;
    indent(out) << "  return 0;" << endl;
    indent(out) << "}" << endl;
  }
}

/**
 *
 */
void t_java_generator::generate_java_constructor(ofstream &out,
                                                 t_struct* tstruct,
                                                 const vector<t_field*>& fields) {
  vector<t_field*>::const_iterator m_iter;
  indent(out) <<
    "public " << tstruct->get_name() << "(" << endl;
  indent_up();
  for (m_iter = fields.begin(); m_iter != fields.end(); ) {
    indent(out) << type_name((*m_iter)->get_type()) << " " <<
      (*m_iter)->get_name();
    ++m_iter;
    if (m_iter != fields.end()) {
      out << "," << endl;
    }
  }
  out << ")" << endl;
  indent_down();
  indent(out) << "{" << endl;
  indent_up();
  indent(out) << "this();" << endl;
  for (m_iter = fields.begin(); m_iter != fields.end(); ++m_iter) {
    indent(out) << "this." << (*m_iter)->get_name() << " = " <<
      (*m_iter)->get_name() << ";" << endl;
    generate_isset_set(out, (*m_iter));
  }
  indent_down();
  indent(out) << "}" << endl << endl;
}

/**
 * Java struct definition. This has various parameters, as it could be
 * generated standalone or inside another class as a helper. If it
 * is a helper than it is a static class.
 *
 * @param tstruct      The struct definition
 * @param is_exception Is this an exception?
 * @param in_class     If inside a class, needs to be static class
 * @param is_result    If this is a result it needs a different writer
 */
void t_java_generator::generate_java_struct_definition(ofstream &out,
                                                       t_struct* tstruct,
                                                       bool is_exception,
                                                       bool in_class,
                                                       bool is_result) {
  generate_java_doc(out, tstruct);

  bool is_final = (tstruct->annotations_.find("final") != tstruct->annotations_.end());

  indent(out) <<
    "public " << (is_final ? "final " : "") <<
     (in_class ? "static " : "") << "class " << tstruct->get_name() << " ";

  if (is_exception) {
    out << "extends Exception ";
  }
  out << "implements TBase, java.io.Serializable, Cloneable";

  if (is_comparable(tstruct)) {
    out << ", Comparable<" << type_name(tstruct) << ">";
  }

  out << " ";

  scope_up(out);

  generate_struct_desc(out, tstruct);

  // Members are public for -java, private for -javabean
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  generate_field_descs(out, tstruct);

  out << endl;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (bean_style_) {
      indent(out) << "private ";
    } else {
      generate_java_doc(out, *m_iter);
      indent(out) << "public ";
    }
    out << declare_field(*m_iter, false) << endl;

  }
  generate_field_name_constants(out, tstruct);

  indent(out) << "public static boolean DEFAULT_PRETTY_PRINT = true;" << endl;

  // isset data
  if (members.size() > 0) {
    out << endl;

    indent(out) << "// isset id assignments" << endl;

    int i = 0;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if (!type_can_be_null((*m_iter)->get_type())) {
        indent(out) << "private static final int " << isset_field_id(*m_iter)
          << " = " << i << ";" <<  endl;
        i++;
      }
    }

    if (i > 0) {
      indent(out) << "private BitSet __isset_bit_vector = new BitSet(" << i << ");" << endl;
    }

    out << endl;
  }

  generate_java_meta_data_map(out, tstruct);


  // Static initializer to populate global class to struct metadata map
  indent(out) << "static {" << endl;
  indent_up();
  indent(out) << "FieldMetaData.addStructMetaDataMap(" << type_name(tstruct) << ".class, metaDataMap);" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;

  vector<t_field*> required_members;
  vector<t_field*> non_optional_members;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if ((*m_iter)->get_req() == t_field::T_REQUIRED) {
      required_members.push_back(*m_iter);
    }
    if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
      non_optional_members.push_back(*m_iter);
    }
  }

  // Default constructor
  indent(out) <<
    "public " << tstruct->get_name() << "() {" << endl;
  indent_up();
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_type* t = get_true_type((*m_iter)->get_type());
    if ((*m_iter)->get_value() != NULL) {
      print_const_value(out, "this." + (*m_iter)->get_name(), t, (*m_iter)->get_value(), true, true);
    }
  }
  indent_down();
  indent(out) << "}" << endl << endl;

  if (!required_members.empty()) {
    // Constructor for all required fields
    generate_java_constructor(out, tstruct, required_members);
  }

  if (non_optional_members.size() > required_members.size()) {
    // Constructor for all non-optional fields
    generate_java_constructor(out, tstruct, non_optional_members);
  }

  if (members.size() > non_optional_members.size()) {
    // Constructor for all fields
    generate_java_constructor(out, tstruct, members);
  }

  // copy constructor
  indent(out) << "/**" << endl;
  indent(out) << " * Performs a deep copy on <i>other</i>." << endl;
  indent(out) << " */" << endl;
  indent(out) << "public " << tstruct->get_name() << "(" << tstruct->get_name() << " other) {" << endl;
  indent_up();

  if (has_bit_vector(tstruct)) {
    indent(out) << "__isset_bit_vector.clear();" << endl;
    indent(out) << "__isset_bit_vector.or(other.__isset_bit_vector);" << endl;
  }

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_field* field = (*m_iter);
    std::string field_name = field->get_name();
    t_type* type = field->get_type();
    bool can_be_null = type_can_be_null(type);

    if (can_be_null) {
      indent(out) << "if (other." << generate_isset_check(field) << ") {" << endl;
      indent_up();
    }

    if (type->is_container()) {
      generate_deep_copy_container(out, "other", field_name, "__this__" + field_name, type);
      indent(out) << "this." << field_name << " = __this__" << field_name << ";" << endl;
    } else {
      indent(out) << "this." << field_name << " = ";
      generate_deep_copy_non_container(out, "other." + field_name, field_name, type);
      out << ";" << endl;
    }

    if (can_be_null) {
      indent_down();
      indent(out) << "}" << endl;
    }
  }

  indent_down();
  indent(out) << "}" << endl << endl;

  // clone method, so that you can deep copy an object when you don't know its class.
  indent(out) << "public " << tstruct->get_name() << " deepCopy() {" << endl;
  indent(out) << "  return new " << tstruct->get_name() << "(this);" << endl;
  indent(out) << "}" << endl << endl;

  indent(out) << "@Deprecated" << endl;
  indent(out) << "public " << tstruct->get_name() << " clone() {" << endl;
  indent(out) << "  return new " << tstruct->get_name() << "(this);" << endl;
  indent(out) << "}" << endl << endl;

  generate_java_bean_boilerplate(out, tstruct);
  generate_generic_field_getters_setters(out, tstruct);
  generate_generic_isset_method(out, tstruct);

  generate_java_struct_equality(out, tstruct);
  if (is_comparable(tstruct)) {
    generate_java_struct_compare_to(out, tstruct);
  }

  generate_java_struct_reader(out, tstruct);
  if (is_result) {
    generate_java_struct_result_writer(out, tstruct);
  } else {
    generate_java_struct_writer(out, tstruct);
  }
  generate_java_struct_tostring(out, tstruct);
  generate_java_validator(out, tstruct);
  scope_down(out);
  out << endl;
}

/**
 * Generates equals methods and a hashCode method for a structure.
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_struct_equality(ofstream& out,
                                                     t_struct* tstruct) {
  out << indent() << "@Override" << endl <<
    indent() << "public boolean equals(Object that) {" << endl;
  indent_up();
  out <<
    indent() << "if (that == null)" << endl <<
    indent() << "  return false;" << endl <<
    indent() << "if (that instanceof " << tstruct->get_name() << ")" << endl <<
    indent() << "  return this.equals((" << tstruct->get_name() << ")that);" << endl <<
    indent() << "return false;" << endl;
  scope_down(out);
  out << endl;

  out <<
    indent() << "public boolean equals(" << tstruct->get_name() << " that) {" << endl;
  indent_up();
  out <<
    indent() << "if (that == null)" << endl <<
    indent() << "  return false;" << endl;

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    out << endl;

    t_type* t = get_true_type((*m_iter)->get_type());
    // Most existing Thrift code does not use isset or optional/required,
    // so we treat "default" fields as required.
    bool is_optional = (*m_iter)->get_req() == t_field::T_OPTIONAL;
    bool can_be_null = type_can_be_null(t);
    string name = (*m_iter)->get_name();

    string this_present = "true";
    string that_present = "true";
    string unequal;

    if (is_optional || can_be_null) {
      this_present += " && this." + generate_isset_check(*m_iter);
      that_present += " && that." + generate_isset_check(*m_iter);
    }

    out <<
      indent() << "boolean this_present_" << name << " = "
               << this_present << ";" << endl <<
      indent() << "boolean that_present_" << name << " = "
               << that_present << ";" << endl <<
      indent() << "if (" << "this_present_" << name
               << " || that_present_" << name << ") {" << endl;
    indent_up();
    out <<
      indent() << "if (!(" << "this_present_" << name
               << " && that_present_" << name << "))" << endl <<
      indent() << "  return false;" << endl;

    if (t->is_base_type() && ((t_base_type*)t)->is_binary()) {
      unequal = "!java.util.Arrays.equals(this." + name + ", that." + name + ")";
    } else if (can_be_null) {
      unequal = "!this." + name + ".equals(that." + name + ")";
    } else {
      unequal = "this." + name + " != that." + name;
    }

    out <<
      indent() << "if (" << unequal << ")" << endl <<
      indent() << "  return false;" << endl;

    scope_down(out);
  }
  out << endl;
  indent(out) << "return true;" << endl;
  scope_down(out);
  out << endl;

  out << indent() << "@Override" << endl <<
    indent() << "public int hashCode() {" << endl;
  indent_up();
  if (gen_hash_code_) {
    indent(out) << "HashCodeBuilder builder = new HashCodeBuilder();" << endl;

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      out << endl;

      t_type* t = get_true_type((*m_iter)->get_type());
      bool is_optional = (*m_iter)->get_req() == t_field::T_OPTIONAL;
      bool can_be_null = type_can_be_null(t);
      string name = (*m_iter)->get_name();

      string present = "true";

      if (is_optional || can_be_null) {
        present += " && (" + generate_isset_check(*m_iter) + ")";
      }

      out <<
        indent() << "boolean present_" << name << " = "
                 << present << ";" << endl <<
        indent() << "builder.append(present_" << name << ");" << endl <<
        indent() << "if (present_" << name << ")" << endl <<
        indent() << "  builder.append(" << name << ");" << endl;
    }

    out << endl;
    indent(out) << "return builder.toHashCode();" << endl;
  } else {
    indent(out) << "return 0;" << endl;
  }
  indent_down();
  indent(out) << "}" << endl << endl;
}

void t_java_generator::generate_java_struct_compare_to(ofstream& out, t_struct* tstruct) {
  indent(out) << "public int compareTo(" << type_name(tstruct) << " other) {" << endl;
  indent_up();

  indent(out) << "if (!getClass().equals(other.getClass())) {" << endl;
  indent(out) << "  return getClass().getName().compareTo(other.getClass().getName());" << endl;
  indent(out) << "}" << endl;
  out << endl;

  indent(out) << "int lastComparison = 0;" << endl;
  indent(out) << type_name(tstruct) << " typedOther = (" << type_name(tstruct) << ")other;" << endl;
  out << endl;

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    t_field* field = *m_iter;
    indent(out) << "lastComparison = Boolean.valueOf(" << generate_isset_check(field) << ").compareTo(" << generate_isset_check(field) << ");" << endl;
    indent(out) << "if (lastComparison != 0) {" << endl;
    indent(out) << "  return lastComparison;" << endl;
    indent(out) << "}" << endl;

    indent(out) << "lastComparison = TBaseHelper.compareTo(" << field->get_name() << ", typedOther." << field->get_name() << ");" << endl;
    indent(out) << "if (lastComparison != 0) {" << endl;
    indent(out) << "  return lastComparison;" << endl;
    indent(out) << "}" << endl;
  }

  indent(out) << "return 0;" << endl;

  indent_down();
  indent(out) << "}" << endl << endl;
}

/**
 * Generates a function to read all the fields of the struct.
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_struct_reader(ofstream& out,
                                                   t_struct* tstruct) {
  out <<
    indent() << "public void read(TProtocol iprot) throws TException {" << endl;
  indent_up();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  // Declare stack tmp variables and read struct header
  out <<
    indent() << "TField field;" << endl <<
    indent() << "iprot.readStructBegin(metaDataMap);" << endl;

  // Loop over reading in fields
  indent(out) <<
    "while (true)" << endl;
    scope_up(out);

    // Read beginning field marker
    indent(out) <<
      "field = iprot.readFieldBegin();" << endl;

    // Check for field STOP marker and break
    indent(out) <<
      "if (field.type == TType.STOP) { " << endl;
    indent_up();
    indent(out) <<
      "break;" << endl;
    indent_down();
    indent(out) <<
      "}" << endl;

    // Switch statement on the field we are reading
    indent(out) <<
      "switch (field.id)" << endl;

      scope_up(out);

      // Generate deserialization code for known cases
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        indent(out) <<
          "case " << upcase_string((*f_iter)->get_name()) << ":" << endl;
        indent_up();
        indent(out) <<
          "if (field.type == " << type_to_enum((*f_iter)->get_type()) << ") {" << endl;
        indent_up();

        generate_deserialize_field(out, *f_iter, "this.");
        generate_isset_set(out, *f_iter);
        indent_down();
        out <<
          indent() << "} else { " << endl <<
          indent() << "  TProtocolUtil.skip(iprot, field.type);" << endl <<
          indent() << "}" << endl <<
          indent() << "break;" << endl;
        indent_down();
      }

      // In the default case we skip the field
      out <<
        indent() << "default:" << endl <<
        indent() << "  TProtocolUtil.skip(iprot, field.type);" << endl <<
        indent() << "  break;" << endl;

      scope_down(out);

    // Read field end marker
    indent(out) <<
      "iprot.readFieldEnd();" << endl;

    scope_down(out);

    out <<
      indent() << "iprot.readStructEnd();" << endl << endl;

    // in non-beans style, check for required fields of primitive type
    // (which can be checked here but not in the general validate method)
    if (!bean_style_){
      out << endl << indent() << "// check for required fields of primitive type, which can't be checked in the validate method" << endl;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_req() == t_field::T_REQUIRED && !type_can_be_null((*f_iter)->get_type())) {
          out <<
            indent() << "if (!" << generate_isset_check(*f_iter) << ") {" << endl <<
            indent() << "  throw new TProtocolException(\"Required field '" << (*f_iter)->get_name() << "' was not found in serialized data! Struct: \" + toString());" << endl <<
            indent() << "}" << endl;
        }
      }
    }

    // performs various checks (e.g. check that all required fields are set)
    indent(out) << "validate();" << endl;

  indent_down();
  out <<
    indent() << "}" << endl <<
    endl;
}

// generates java method to perform various checks
// (e.g. check that all required fields are set)
void t_java_generator::generate_java_validator(ofstream& out,
                                                   t_struct* tstruct){
  indent(out) << "public void validate() throws TException {" << endl;
  indent_up();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  out << indent() << "// check for required fields" << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED) {
      if (bean_style_) {
        out <<
          indent() << "if (!" << generate_isset_check(*f_iter) << ") {" <<
            endl <<
          indent() << "  throw new TProtocolException(" <<
            "TProtocolException.MISSING_REQUIRED_FIELD, " <<
            "\"Required field '" << (*f_iter)->get_name() <<
            "' is unset! Struct:\" + toString());" << endl <<
          indent() << "}" << endl << endl;
      } else{
        if (type_can_be_null((*f_iter)->get_type())) {
          indent(out) << "if (" << (*f_iter)->get_name() << " == null) {" <<
            endl;
          indent(out) << "  throw new TProtocolException(" <<
            "TProtocolException.MISSING_REQUIRED_FIELD, " <<
            "\"Required field '" << (*f_iter)->get_name() <<
            "' was not present! Struct: \" + toString());" << endl;
          indent(out) << "}" << endl;
        } else {
          indent(out) << "// alas, we cannot check '" << (*f_iter)->get_name()
            << "' because it's a primitive and you chose the non-beans "
            << "generator." << endl;
        }
      }
    }
  }

  // check that fields of type enum have valid values
  out << indent() << "// check that fields of type enum have valid values" << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field* field = (*f_iter);
    t_type* type = field->get_type();
    // if field is an enum, check that its value is valid
    if (type->is_enum()){
      indent(out) << "if (" << generate_isset_check(field) << " && !" << get_enum_class_name(type) << ".VALID_VALUES.contains(" << field->get_name() << ")){" << endl;
      indent_up();
      indent(out) << "throw new TProtocolException(\"The field '" << field->get_name() << "' has been assigned the invalid value \" + " << field->get_name() << ");" << endl;
      indent_down();
      indent(out) << "}" << endl;
    }
  }

  indent_down();
  indent(out) << "}" << endl << endl;
}

/**
 * Generates a function to write all the fields of the struct
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_struct_writer(ofstream& out,
                                                   t_struct* tstruct) {
  out <<
    indent() << "public void write(TProtocol oprot) throws TException {" << endl;
  indent_up();

  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  // performs various checks (e.g. check that all required fields are set)
  indent(out) << "validate();" << endl << endl;

  indent(out) << "oprot.writeStructBegin(STRUCT_DESC);" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    bool null_allowed = type_can_be_null((*f_iter)->get_type());
    if (null_allowed) {
      out <<
        indent() << "if (this." << (*f_iter)->get_name() << " != null) {" << endl;
      indent_up();
    }
    bool optional = (*f_iter)->get_req() == t_field::T_OPTIONAL;
    if (optional) {
      indent(out) << "if (" << generate_isset_check((*f_iter)) << ") {" << endl;
      indent_up();
    }

    indent(out) << "oprot.writeFieldBegin(" << constant_name((*f_iter)->get_name()) << "_FIELD_DESC);" << endl;

    // Write field contents
    generate_serialize_field(out, *f_iter, "this.");

    // Write field closer
    indent(out) <<
      "oprot.writeFieldEnd();" << endl;

    if (optional) {
      indent_down();
      indent(out) << "}" << endl;
    }
    if (null_allowed) {
      indent_down();
      indent(out) << "}" << endl;
    }
  }
  // Write the struct map
  out <<
    indent() << "oprot.writeFieldStop();" << endl <<
    indent() << "oprot.writeStructEnd();" << endl;

  indent_down();
  out <<
    indent() << "}" << endl <<
    endl;
}

/**
 * Generates a function to write all the fields of the struct,
 * which is a function result. These fields are only written
 * if they are set in the Isset array, and only one of them
 * can be set at a time.
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_struct_result_writer(ofstream& out,
                                                          t_struct* tstruct) {
  out <<
    indent() << "public void write(TProtocol oprot) throws TException {" << endl;
  indent_up();

  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  indent(out) << "oprot.writeStructBegin(STRUCT_DESC);" << endl;

  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
      out <<
        endl <<
        indent() << "if ";
    } else {
      out << " else if ";
    }

    out << "(this." << generate_isset_check(*f_iter) << ") {" << endl;

    indent_up();

    indent(out) << "oprot.writeFieldBegin(" << constant_name((*f_iter)->get_name()) << "_FIELD_DESC);" << endl;

    // Write field contents
    generate_serialize_field(out, *f_iter, "this.");

    // Write field closer
    indent(out) <<
      "oprot.writeFieldEnd();" << endl;

    indent_down();
    indent(out) << "}";
  }
  // Write the struct map
  out <<
    endl <<
    indent() << "oprot.writeFieldStop();" << endl <<
    indent() << "oprot.writeStructEnd();" << endl;

  indent_down();
  out <<
    indent() << "}" << endl <<
    endl;
}

void t_java_generator::generate_reflection_getters(ostringstream& out, t_type* type, string field_name, string cap_name) {
  indent(out) << "case " << upcase_string(field_name) << ":" << endl;
  indent_up();

  if (type->is_base_type() && !type->is_string()) {
    t_base_type* base_type = (t_base_type*)type;

    indent(out) << "return new " << type_name(type, true, false) << "(" << (base_type->is_bool() ? "is" : "get") << cap_name << "());" << endl << endl;
  } else {
    indent(out) << "return get" << cap_name << "();" << endl << endl;
  }

  indent_down();
}

void t_java_generator::generate_reflection_setters(ostringstream& out, t_type* type, string field_name, string cap_name) {
  indent(out) << "case " << upcase_string(field_name) << ":" << endl;
  indent_up();
  indent(out) << "if (value == null) {" << endl;
  indent(out) << "  unset" << get_cap_name(field_name) << "();" << endl;
  indent(out) << "} else {" << endl;
  indent(out) << "  set" << cap_name << "((" << type_name(type, true, false) << ")value);" << endl;
  indent(out) << "}" << endl;
  indent(out) << "break;" << endl << endl;

  indent_down();
}

void t_java_generator::generate_generic_field_getters_setters(std::ofstream& out, t_struct* tstruct) {

  std::ostringstream getter_stream;
  std::ostringstream setter_stream;

  // build up the bodies of both the getter and setter at once
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field* field = *f_iter;
    t_type* type = get_true_type(field->get_type());
    std::string field_name = field->get_name();
    std::string cap_name = get_cap_name(field_name);

    indent_up();
    generate_reflection_setters(setter_stream, type, field_name, cap_name);
    generate_reflection_getters(getter_stream, type, field_name, cap_name);
    indent_down();
  }


  // create the setter
  // TODO: fix this code
  indent(out) << "@SuppressWarnings(\"unchecked\")" << endl;
  indent(out) << "public void setFieldValue(int fieldID, Object value) {" << endl;
  indent_up();

  indent(out) << "switch (fieldID) {" << endl;

  out << setter_stream.str();

  indent(out) << "default:" << endl;
  indent(out) << "  throw new IllegalArgumentException(\"Field \" + fieldID + \" doesn't exist!\");" << endl;

  indent(out) << "}" << endl;

  indent_down();
  indent(out) << "}" << endl << endl;

  // create the getter
  indent(out) << "public Object getFieldValue(int fieldID) {" << endl;
  indent_up();

  indent(out) << "switch (fieldID) {" << endl;

  out << getter_stream.str();

  indent(out) << "default:" << endl;
  indent(out) << "  throw new IllegalArgumentException(\"Field \" + fieldID + \" doesn't exist!\");" << endl;

  indent(out) << "}" << endl;

  indent_down();

  indent(out) << "}" << endl << endl;
}

// Creates a generic isSet method that takes the field number as argument
void t_java_generator::generate_generic_isset_method(std::ofstream& out, t_struct* tstruct){
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  // create the isSet method
  indent(out) << "// Returns true if field corresponding to fieldID is set (has been assigned a value) and false otherwise" << endl;
  indent(out) << "public boolean isSet(int fieldID) {" << endl;
  indent_up();
  indent(out) << "switch (fieldID) {" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field* field = *f_iter;
    indent(out) << "case " << upcase_string(field->get_name()) << ":" << endl;
    indent_up();
    indent(out) << "return " << generate_isset_check(field) << ";" << endl;
    indent_down();
  }

  indent(out) << "default:" << endl;
  indent(out) << "  throw new IllegalArgumentException(\"Field \" + fieldID + \" doesn't exist!\");" << endl;

  indent(out) << "}" << endl;

  indent_down();
  indent(out) << "}" << endl << endl;
}

std::string t_java_generator::get_simple_getter_name(t_field* field) {
    std::string field_name = field->get_name();
    std::string cap_name = get_cap_name(field_name);
    t_type* type = get_true_type(field->get_type());

    if (type->is_base_type() &&
        ((t_base_type*)type)->get_base() == t_base_type::TYPE_BOOL) {
      return " is" + cap_name;
    } else {
      return " get" + cap_name;
    }
}

/**
 * Generates a set of Java Bean boilerplate functions (setters, getters, etc.)
 * for the given struct.
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_bean_boilerplate(ofstream& out,
                                                      t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field* field = *f_iter;
    t_type* type = get_true_type(field->get_type());
    std::string field_name = field->get_name();
    std::string cap_name = get_cap_name(field_name);

    // Simple getter
    std::string getter_name = get_simple_getter_name(field);
    generate_java_doc(out, field);
    indent(out) << "public " << type_name(type)
                << " " << getter_name << "() {" << endl;
    indent_up();
    indent(out) << "return this." << field_name << ";" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;

    // Simple setter
    generate_java_doc(out, field);
    indent(out) << "public " << type_name(tstruct) << " set" << cap_name << "(" << type_name(type) <<
      " " << field_name << ") {" << endl;
    indent_up();
    indent(out) << "this." << field_name << " = " << field_name << ";" <<
      endl;
    generate_isset_set(out, field);
    indent(out) << "return this;" << endl;

    indent_down();
    indent(out) << "}" << endl << endl;

    // Unsetter
    indent(out) << "public void unset" << cap_name << "() {" << endl;
    indent_up();
    if (type_can_be_null(type)) {
      indent(out) << "this." << field_name << " = null;" << endl;
    } else {
      indent(out) << "__isset_bit_vector.clear(" << isset_field_id(field) << ");" << endl;
    }
    indent_down();
    indent(out) << "}" << endl << endl;

    // isSet method
    indent(out) << "// Returns true if field " << field_name << " is set (has been assigned a value) and false otherwise" << endl;
    indent(out) << "public boolean is" << get_cap_name("set") << cap_name << "() {" << endl;
    indent_up();
    if (type_can_be_null(type)) {
      indent(out) << "return this." << field_name << " != null;" << endl;
    } else {
      indent(out) << "return __isset_bit_vector.get(" << isset_field_id(field) << ");" << endl;
    }
    indent_down();
    indent(out) << "}" << endl << endl;

    indent(out) << "public void set" << cap_name << get_cap_name("isSet") << "(boolean value) {" << endl;
    indent_up();
    if (type_can_be_null(type)) {
      indent(out) << "if (!value) {" << endl;
      indent(out) << "  this." << field_name << " = null;" << endl;
      indent(out) << "}" << endl;
    } else {
      indent(out) << "__isset_bit_vector.set(" << isset_field_id(field) << ", value);" << endl;
    }
    indent_down();
    indent(out) << "}" << endl << endl;
  }
}


void t_java_generator::generate_default_toString(ofstream& out,
    t_struct* tstruct) {
  out << indent() << "@Override" << endl <<
    indent() << "public String toString() {" << endl;
  indent_up();
  out << indent() << "return toString(DEFAULT_PRETTY_PRINT);" << endl;
  indent_down();
  out << indent() <<"}" << endl << endl;
}

void t_java_generator::generate_toString_prettyprint(ofstream& out) {
  out << indent() << "@Override" << endl <<
    indent() << "public String toString(boolean prettyPrint) {" << endl;
  indent_up();
  out << indent() << "return toString(1, prettyPrint);" << endl;
  indent_down();
  out << indent() << "}" << endl << endl;
}

/**
 * Generates a toString() method for the given struct
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_struct_tostring(ofstream& out,
                                                     t_struct* tstruct) {

  generate_default_toString(out, tstruct);
  generate_toString_prettyprint(out);
  out << indent() << "@Override" << endl;
  out << indent() << "public String toString(int indent, boolean prettyPrint) {"
    << endl;
  indent_up();
  out << indent()
    << "String indentStr = prettyPrint ? TBaseHelper.getIndentedString(indent) "
    << ": \"\";"
    << endl;
  out << indent()
    << "String newLine = prettyPrint ? \"\\n\" : \"\";" << endl
    << "String space = prettyPrint ? \" \" : \"\";"
    << endl;
  out <<
    indent() << "StringBuilder sb = new StringBuilder(\""
    << tstruct->get_name() << "\");" << endl;
  out << indent() << "sb.append(space);" << endl;
  out << indent() << "sb.append(\"(\");" << endl;
  out << indent() << "sb.append(newLine);" << endl;
  out << indent() << "boolean first = true;" << endl << endl;

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (tstruct->is_union()) {
      indent(out) << "// Only print this field if it is the set field" << endl;
      indent(out) << "if (" << generate_setfield_check(*f_iter) << ")" << endl;
      scope_up(out);
    }

    bool could_be_unset = (*f_iter)->get_req() == t_field::T_OPTIONAL;
    if (could_be_unset) {
      indent(out) << "if (" << generate_isset_check(*f_iter) << ")" << endl;
      scope_up(out);
    }

    t_field* field = (*f_iter);

    if (!first) {
      indent(out) << "if (!first) sb.append(\",\" + newLine);" << endl;
    }
    indent(out) << "sb.append(indentStr);" << endl;
    indent(out) << "sb.append(\"" << (*f_iter)->get_name() << "\");" << endl;
    indent(out) << "sb.append(space);" << endl;
    indent(out) << "sb.append(\":\").append(space);" << endl;
    bool can_be_null = type_can_be_null(field->get_type());
    std::string field_getter = "this." + get_simple_getter_name(*f_iter) + "()";
    if (can_be_null) {
      indent(out) << "if (" << field_getter << " == null) {" << endl;
      indent(out) << "  sb.append(\"null\");" << endl;
      indent(out) << "} else {" << endl;
      indent_up();
    }

    if (field->get_type()->is_base_type() && ((t_base_type*)(field->get_type()))->is_binary()) {
      indent(out) << "  int __" << field->get_name() << "_size = Math.min(" << field_getter << ".length, 128);" << endl;
      indent(out) << "  for (int i = 0; i < __" << field->get_name() << "_size; i++) {" << endl;
      indent(out) << "    if (i != 0) sb.append(\" \");" << endl;
      indent(out) << "    sb.append(Integer.toHexString(" << field_getter << "[i]).length() > 1 ? Integer.toHexString(" << field_getter << "[i]).substring(Integer.toHexString(" << field_getter << "[i]).length() - 2).toUpperCase() : \"0\" + Integer.toHexString(" << field_getter << "[i]).toUpperCase());" <<endl;
      indent(out) << "  }" << endl;
      indent(out) << "  if (" << field_getter << ".length > 128) sb.append(\" ...\");" << endl;
    } else if(field->get_type()->is_enum()) {
      indent(out) << "String " << field->get_name() << "_name = " << get_enum_class_name(field->get_type()) << ".VALUES_TO_NAMES.get(this." << (*f_iter)->get_name() << ");"<< endl;
      indent(out) << "if (" << field->get_name() << "_name != null) {" << endl;
      indent(out) << "  sb.append(" << field->get_name() << "_name);" << endl;
      indent(out) << "  sb.append(\" (\");" << endl;
      indent(out) << "}" << endl;
      indent(out) << "sb.append(" << field_getter << ");" << endl;
      indent(out) << "if (" << field->get_name() << "_name != null) {" << endl;
      indent(out) << "  sb.append(\")\");" << endl;
      indent(out) << "}" << endl;
    } else {
      indent(out) << "sb.append(TBaseHelper.toString(" << field_getter
        << ", indent + 1, prettyPrint));" << endl;
    }

    if (can_be_null) {
      indent_down();
      indent(out) << "}" << endl;
    }
    indent(out) << "first = false;" << endl;

    if (could_be_unset) {
      scope_down(out);
    }

    if (tstruct->is_union()) {
      scope_down(out);
    }

    first = false;
  }
  out << indent() << "sb.append(newLine + TBaseHelper.reduceIndent(indentStr));"
    << endl;
  out <<
    indent() << "sb.append(\")\");" << endl <<
    indent() << "return sb.toString();" << endl;

  indent_down();
  indent(out) << "}" << endl <<
    endl;
}

/**
 * Generates a static map with meta data to store information such as fieldID
 * to fieldName mapping
 *
 * @param tstruct The struct definition
 */
void t_java_generator::generate_java_meta_data_map(ofstream& out,
                                                   t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  // Static Map with fieldID -> FieldMetaData mappings
  indent(out) <<
      "public static final Map<Integer, FieldMetaData> metaDataMap;" << endl;

  // Populate map
  indent(out) << "static {" << endl;
  indent_up();
  indent(out) << "Map<Integer, FieldMetaData> tmpMetaDataMap = "
      "new HashMap<Integer, FieldMetaData>();" << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    t_field* field = *f_iter;
    std::string field_name = field->get_name();
    indent(out) << "tmpMetaDataMap.put(" << upcase_string(field_name) <<
        ", new FieldMetaData(\"" << field_name << "\", ";

    // Set field requirement type (required, optional, etc.)
    if (field->get_req() == t_field::T_REQUIRED) {
      out << "TFieldRequirementType.REQUIRED, ";
    } else if (field->get_req() == t_field::T_OPTIONAL) {
      out << "TFieldRequirementType.OPTIONAL, ";
    } else {
      out << "TFieldRequirementType.DEFAULT, ";
    }

    // Create value meta data
    generate_field_value_meta_data(out, field->get_type());
    out  << "));" << endl;
  }
  indent(out) <<
      "metaDataMap = Collections.unmodifiableMap(tmpMetaDataMap);" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;
}

/**
 * Returns a string with the java representation of the given thrift type
 * (e.g. for the type struct it returns "TType.STRUCT")
 */
std::string t_java_generator::get_java_type_string(t_type* type) {
  if (type->is_list()){
    return "TType.LIST";
  } else if (type->is_map()) {
    return "TType.MAP";
  } else if (type->is_set()) {
    return "TType.SET";
  } else if (type->is_struct() || type->is_xception()) {
    return "TType.STRUCT";
  } else if (type->is_enum()) {
    return "TType.I32";
  } else if (type->is_typedef()) {
    return get_java_type_string(((t_typedef*)type)->get_type());
  } else if (type->is_base_type()) {
    switch (((t_base_type*)type)->get_base()) {
      case t_base_type::TYPE_VOID   : return      "TType.VOID"; break;
      case t_base_type::TYPE_STRING : return    "TType.STRING"; break;
      case t_base_type::TYPE_BOOL   : return      "TType.BOOL"; break;
      case t_base_type::TYPE_BYTE   : return      "TType.BYTE"; break;
      case t_base_type::TYPE_I16    : return       "TType.I16"; break;
      case t_base_type::TYPE_I32    : return       "TType.I32"; break;
      case t_base_type::TYPE_I64    : return       "TType.I64"; break;
      case t_base_type::TYPE_DOUBLE : return    "TType.DOUBLE"; break;
      case t_base_type::TYPE_FLOAT  : return     "TType.FLOAT"; break;
      default : throw std::runtime_error("Unknown thrift type \"" + type->get_name() + "\" passed to t_java_generator::get_java_type_string!"); break; // This should never happen!
    }
  } else {
    throw std::runtime_error("Unknown thrift type \"" + type->get_name() + "\" passed to t_java_generator::get_java_type_string!"); // This should never happen!
  }
}

void t_java_generator::generate_field_value_meta_data(std::ofstream& out, t_type* type){
  out << endl;
  indent_up();
  indent_up();
  if (type->is_struct()){
    indent(out) << "new StructMetaData(TType.STRUCT, " << type_name(type) << ".class";
  } else if (type->is_container()){
    if (type->is_list()){
      indent(out) << "new ListMetaData(TType.LIST, ";
      t_type* elem_type = ((t_list*)type)->get_elem_type();
      generate_field_value_meta_data(out, elem_type);
    } else if (type->is_set()){
      indent(out) << "new SetMetaData(TType.SET, ";
      t_type* elem_type = ((t_list*)type)->get_elem_type();
      generate_field_value_meta_data(out, elem_type);
    } else{ // map
      indent(out) << "new MapMetaData(TType.MAP, ";
      t_type* key_type = ((t_map*)type)->get_key_type();
      t_type* val_type = ((t_map*)type)->get_val_type();
      generate_field_value_meta_data(out, key_type);
      out << ", ";
      generate_field_value_meta_data(out, val_type);
    }
  } else {
    indent(out) << "new FieldValueMetaData(" << get_java_type_string(type);
  }
  out << ")";
  indent_down();
  indent_down();
}


/**
 * Generates a thrift service. In C++, this comprises an entirely separate
 * header and source file. The header file defines the methods and includes
 * the data types defined in the main header file, and the implementation
 * file contains implementations of the basic printer and default interfaces.
 *
 * @param tservice The service definition
 */
void t_java_generator::generate_service(t_service* tservice) {
  // Make output file
  string f_service_name = package_dir_+"/"+service_name_+".java";
  f_service_.open(f_service_name.c_str());
  record_genfile(f_service_name);

  f_service_ <<
    autogen_comment() <<
    java_package() <<
    java_type_imports() <<
    java_thrift_imports();

  f_service_ <<
    "public class " << service_name_ << " {" << endl <<
    endl;
  indent_up();

  // Generate the three main parts of the service
  generate_service_interface(tservice);
  generate_service_async_interface(tservice);
  generate_service_client(tservice);
  generate_service_async_client(tservice);
  generate_service_server(tservice);
  generate_service_helpers(tservice);

  indent_down();
  f_service_ <<
    "}" << endl;
  f_service_.close();
}

/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 */
void t_java_generator::generate_service_interface(t_service* tservice) {
  string extends = "";
  string extends_iface = "";
  if (tservice->get_extends() != NULL) {
    extends = type_name(tservice->get_extends());
    extends_iface = " extends " + extends + ".Iface";
  }

  generate_java_doc(f_service_, tservice);
  f_service_ << indent() << "public interface Iface" << extends_iface <<
    " {" << endl << endl;
  indent_up();
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    generate_java_doc(f_service_, *f_iter);
    indent(f_service_) << "public " << function_signature(*f_iter) << ";" << endl << endl;
  }
  indent_down();
  f_service_ << indent() << "}" << endl << endl;
}

void t_java_generator::generate_service_async_interface(t_service* tservice) {
  string extends = "";
  string extends_iface = "";
  if (tservice->get_extends() != NULL) {
    extends = type_name(tservice->get_extends());
    extends_iface = " extends " + extends + ".AsyncIface";
  }

  f_service_ << indent() << "public interface AsyncIface" << extends_iface << " {" << endl << endl;
  indent_up();
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(f_service_) << "public " << function_signature_async(*f_iter, "resultHandler", true) << " throws TException;" << endl << endl;
  }
  indent_down();
  f_service_ << indent() << "}" << endl << endl;
}

/**
 * Generates structs for all the service args and return types
 *
 * @param tservice The service
 */
void t_java_generator::generate_service_helpers(t_service* tservice) {
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* ts = (*f_iter)->get_arglist();
    generate_java_struct_definition(f_service_, ts, false, true);
    generate_function_helpers(*f_iter);
  }
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_java_generator::generate_service_client(t_service* tservice) {
  string extends = "";
  string extends_client = "";
  if (tservice->get_extends() != NULL) {
    extends = type_name(tservice->get_extends());
    extends_client = " extends " + extends + ".Client";
  } else {
    // Client base class
    extends_client = " extends EventHandlerBase";
  }

  indent(f_service_) <<
    "public static class Client" << extends_client << " implements Iface {" << endl;
  indent_up();

  indent(f_service_) <<
    "public Client(TProtocol prot)" << endl;
  scope_up(f_service_);
  indent(f_service_) <<
    "this(prot, prot);" << endl;
  scope_down(f_service_);
  f_service_ << endl;

  indent(f_service_) <<
    "public Client(TProtocol iprot, TProtocol oprot)" << endl;
  scope_up(f_service_);
  if (extends.empty()) {
    f_service_ <<
      indent() << "iprot_ = iprot;" << endl <<
      indent() << "oprot_ = oprot;" << endl;
  } else {
    f_service_ <<
      indent() << "super(iprot, oprot);" << endl;
  }
  scope_down(f_service_);
  f_service_ << endl;

  if (extends.empty()) {
    f_service_ <<
      indent() << "protected TProtocol iprot_;"  << endl <<
      indent() << "protected TProtocol oprot_;"  << endl <<
      endl <<
      indent() << "protected int seqid_;" << endl <<
      endl;

    indent(f_service_) <<
      "public TProtocol getInputProtocol()" << endl;
    scope_up(f_service_);
    indent(f_service_) <<
      "return this.iprot_;" << endl;
    scope_down(f_service_);
    f_service_ << endl;

    indent(f_service_) <<
      "public TProtocol getOutputProtocol()" << endl;
    scope_up(f_service_);
    indent(f_service_) <<
      "return this.oprot_;" << endl;
    scope_down(f_service_);
    f_service_ << endl;

  }

  // Generate client method implementations
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    string funname = (*f_iter)->get_name();
    string service_func_name = "\"" + tservice->get_name() + "." + (*f_iter)->get_name() + "\"";
    // Open function
    indent(f_service_) <<
      "public " << function_signature(*f_iter) << endl;
    scope_up(f_service_);

    indent(f_service_) << "ContextStack ctx"
                << " = getContextStack("
                << service_func_name << ", null);" << endl
                << "this.setContextStack(ctx);" << endl
                << indent() << "send_" << funname << "(";

    // Get the struct of function call params
    t_struct* arg_struct = (*f_iter)->get_arglist();

    // Declare the function arguments
    const vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator fld_iter;
    bool first = true;
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      if (first) {
       first = false;
      } else {
        f_service_ << ", ";
      }
      f_service_ << (*fld_iter)->get_name();
    }
    f_service_ << ");" << endl;

    if (!(*f_iter)->is_oneway()) {
      f_service_ << indent();
      if (!(*f_iter)->get_returntype()->is_void()) {
        f_service_ << "return ";
      }
      f_service_ <<
        "recv_" << funname << "();" << endl;
    }

    scope_down(f_service_);
    f_service_ << endl;

    t_function send_function(g_type_void,
                             string("send_") + (*f_iter)->get_name(),
                             (*f_iter)->get_arglist());

    string argsname = (*f_iter)->get_name() + "_args";

    // Open function
    indent(f_service_) <<
      "public " << function_signature(&send_function) << endl;
    scope_up(f_service_);

    // Serialize the request
    indent(f_service_) << "ContextStack ctx = "
      << "this.getContextStack();" << endl;
    indent(f_service_) << "super.preWrite(ctx, " << service_func_name
       << ", null);" << endl;

    f_service_ <<
      indent() << "oprot_.writeMessageBegin(new TMessage(\"" << funname << "\", TMessageType.CALL, seqid_));" << endl <<
      indent() << argsname << " args = new " << argsname << "();" << endl;

    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      f_service_ <<
        indent() << "args." << (*fld_iter)->get_name() << " = " << (*fld_iter)->get_name() << ";" << endl;
    }

    string bytes = tmp("_bytes");

    f_service_ <<
      indent() << "args.write(oprot_);" << endl <<
      indent() << "oprot_.writeMessageEnd();" << endl <<
      indent() << "oprot_.getTransport().flush();" << endl <<
      indent() << "super.postWrite(ctx, " << service_func_name
               << ", args);" << endl <<
      indent() << "return;" << endl;

    scope_down(f_service_);
    f_service_ << endl;

    // Generate recv function only if not a oneway function
    if (!(*f_iter)->is_oneway()) {
      string resultname = (*f_iter)->get_name() + "_result";

      t_struct noargs(program_);
      t_function recv_function((*f_iter)->get_returntype(),
                               string("recv_") + (*f_iter)->get_name(),
                               &noargs,
                               (*f_iter)->get_xceptions());
      // Open the recv function
      indent(f_service_) <<
        "public " << function_signature(&recv_function) << endl;
      scope_up(f_service_);


      indent(f_service_) << "ContextStack ctx = "
         << "super.getContextStack();" << endl; // newversion
      indent(f_service_) << "long bytes;" << endl;

      indent(f_service_) << "TMessageType mtype;" << endl <<
          indent() << "super.preRead(ctx, " << service_func_name
           << ");" << endl;

      // TODO(mcslee): Message validation here, was the seqid etc ok?

      f_service_ <<
        indent() << "TMessage msg = iprot_.readMessageBegin();" << endl <<
        indent() << "if (msg.type == TMessageType.EXCEPTION) {" << endl <<
        indent() << "  TApplicationException x = TApplicationException.read(iprot_);" << endl <<
        indent() << "  iprot_.readMessageEnd();" << endl <<
        indent() << "  throw x;" << endl <<
        indent() << "}" << endl <<
        indent() << resultname << " result = new " << resultname << "();" << endl <<
        indent() << "result.read(iprot_);" << endl <<
        indent() << "iprot_.readMessageEnd();" << endl <<
        indent() << "super.postRead(ctx, " << service_func_name
                 << ", result);" << endl << endl;


      // Careful, only return _result if not a void function
      if (!(*f_iter)->get_returntype()->is_void()) {
        f_service_ <<
          indent() << "if (result." << generate_isset_check("success") << ") {" << endl <<
          indent() << "  return result.success;" << endl <<
          indent() << "}" << endl;
      }

      t_struct* xs = (*f_iter)->get_xceptions();
      const std::vector<t_field*>& xceptions = xs->get_members();
      vector<t_field*>::const_iterator x_iter;
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        f_service_ <<
          indent() << "if (result." << (*x_iter)->get_name() << " != null) {" << endl <<
          indent() << "  throw result." << (*x_iter)->get_name() << ";" << endl <<
          indent() << "}" << endl;
      }

      // If you get here it's an exception, unless a void function
      if ((*f_iter)->get_returntype()->is_void()) {
        indent(f_service_) <<
          "return;" << endl;
      } else {
        f_service_ <<
          indent() << "throw new TApplicationException(TApplicationException.MISSING_RESULT, \"" << (*f_iter)->get_name() << " failed: unknown result\");" << endl;
      }

      // Close function
      scope_down(f_service_);
      f_service_ << endl;
    }
  }

  indent_down();
  indent(f_service_) <<
    "}" << endl;
}

void t_java_generator::generate_service_async_client(t_service* tservice) {
  string extends = "TAsyncClient";
  string extends_client = "";
  if (tservice->get_extends() != NULL) {
    extends = type_name(tservice->get_extends()) + ".AsyncClient";
  }

  indent(f_service_) <<
    "public static class AsyncClient extends " << extends << " implements AsyncIface {" << endl;
  indent_up();

  // Factory method
  indent(f_service_) << "public static class Factory implements TAsyncClientFactory<AsyncClient> {" << endl;
  indent(f_service_) << "  private TAsyncClientManager clientManager;" << endl;
  indent(f_service_) << "  private TProtocolFactory protocolFactory;" << endl;
  indent(f_service_) << "  public Factory(TAsyncClientManager clientManager, TProtocolFactory protocolFactory) {" << endl;
  indent(f_service_) << "    this.clientManager = clientManager;" << endl;
  indent(f_service_) << "    this.protocolFactory = protocolFactory;" << endl;
  indent(f_service_) << "  }" << endl;
  indent(f_service_) << "  public AsyncClient getAsyncClient(TNonblockingTransport transport) {" << endl;
  indent(f_service_) << "    return new AsyncClient(protocolFactory, clientManager, transport);" << endl;
  indent(f_service_) << "  }" << endl;
  indent(f_service_) << "}" << endl << endl;

  indent(f_service_) << "public AsyncClient(TProtocolFactory protocolFactory, TAsyncClientManager clientManager, TNonblockingTransport transport) {" << endl;
  indent(f_service_) << "  super(protocolFactory, clientManager, transport);" << endl;
  indent(f_service_) << "}" << endl << endl;

  // Generate client method implementations
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    string funname = (*f_iter)->get_name();
    t_type* ret_type = (*f_iter)->get_returntype();
    t_struct* arg_struct = (*f_iter)->get_arglist();
    string funclassname = funname + "_call";
    const vector<t_field*>& fields = arg_struct->get_members();
    const std::vector<t_field*>& xceptions = (*f_iter)->get_xceptions()->get_members();
    vector<t_field*>::const_iterator fld_iter;
    string args_name = (*f_iter)->get_name() + "_args";
    string result_name = (*f_iter)->get_name() + "_result";
    string result_handler_symbol;
    string client_sybmol = tmp("client");
    string protocol_factory_symbol = tmp("protocolFactory");
    string transport_symbol = tmp("transport");

    // Main method body
    result_handler_symbol = tmp("resultHandler");
    f_service_ <<
      indent() << "public " << function_signature_async(*f_iter, result_handler_symbol, false) << " throws TException {" << endl <<
      indent() << "  checkReady();" << endl <<
      indent() << "  " << funclassname << " method_call = new " << funclassname << "("
               << async_argument_list(*f_iter, arg_struct, result_handler_symbol) << ", this, ___protocolFactory, ___transport);" << endl <<
      indent() << "  this.___currentMethod = method_call;" << endl <<
      indent() << "  ___manager.call(method_call);" << endl <<
      indent() << "}" << endl << endl;

    // TAsyncMethod object for this function call
    indent(f_service_) << "public static class " + funclassname + " extends TAsyncMethodCall {" << endl;
    indent_up();

    // Member variables
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      indent(f_service_) << "private " + type_name((*fld_iter)->get_type()) + " " + (*fld_iter)->get_name() + ";" << endl;
    }

    // NOTE since we use a new Client instance to deserialize, let's keep seqid to 0 for now
    // indent(f_service_) << "private int seqid;" << endl << endl;

    // Constructor
    result_handler_symbol = tmp("resultHandler");
    indent(f_service_) << "public " + funclassname + "(" + async_argument_list(*f_iter, arg_struct, result_handler_symbol, true)
                       << ", TAsyncClient " << client_sybmol
                       << ", TProtocolFactory " << protocol_factory_symbol
                       << ", TNonblockingTransport " << transport_symbol
                       << ") throws TException {" << endl;
    indent(f_service_) << "  super("
                       << client_sybmol << ", "
                       << protocol_factory_symbol << ", "
                       << transport_symbol << ", "
                       << result_handler_symbol << ", " << ((*f_iter)->is_oneway() ? "true" : "false") << ");" << endl;

    // Assign member variables
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      indent(f_service_) << "  this." + (*fld_iter)->get_name() + " = " + (*fld_iter)->get_name() + ";" << endl;
    }

    indent(f_service_) << "}" << endl << endl;

    indent(f_service_) << "public void write_args(TProtocol prot) throws TException {" << endl;
    indent_up();

    // Serialize request
    // NOTE we are leaving seqid as 0, for now (see above)
    string pservice_func_name = "\"" + tservice->get_name() + "." + funname + "\"";
    f_service_ <<
      indent() << "prot.writeMessageBegin(new TMessage(\"" << funname << "\", TMessageType.CALL, 0));" << endl <<
      indent() << args_name << " args = new " << args_name << "();" << endl;

    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      f_service_ << indent() << "args.set" << get_cap_name((*fld_iter)->get_name()) << "(" << (*fld_iter)->get_name() << ");" << endl;
    }

    f_service_ <<
      indent() << "args.write(prot);" << endl <<
      indent() << "prot.writeMessageEnd();" << endl;

    indent_down();
    indent(f_service_) << "}" << endl << endl;

    // Return method
    indent(f_service_) << "public " + type_name(ret_type) + " getResult() throws ";
    vector<t_field*>::const_iterator x_iter;
    for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
      f_service_ << type_name((*x_iter)->get_type(), false, false) + ", ";
    }
    f_service_ << "TException {" << endl;

    indent_up();
    f_service_ <<
      indent() << "if (getState() != State.RESPONSE_READ) {" << endl <<
      indent() << "  throw new IllegalStateException(\"Method call not finished!\");" << endl <<
      indent() << "}" << endl <<
      indent() << "TMemoryInputTransport memoryTransport = new TMemoryInputTransport(getFrameBuffer().array());" << endl <<
      indent() << "TProtocol prot = super.client.getProtocolFactory().getProtocol(memoryTransport);" << endl;
    if (!(*f_iter)->is_oneway()) {
      indent(f_service_);
      if (!ret_type->is_void()) {
        f_service_ << "return ";
      }
      f_service_ << "(new Client(prot)).recv_" + funname + "();" << endl;
    }

    // Close function
    indent_down();
    indent(f_service_) << "}" << endl;

    // Close class
    indent_down();
    indent(f_service_) << "}" << endl << endl;
  }

  // Close AsyncClient
  scope_down(f_service_);
  f_service_ << endl;
}

/**
 * Generates a service server definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_java_generator::generate_service_server(t_service* tservice) {
  // Generate the dispatch methods
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  // Extends stuff
  string extends = "";
  string extends_processor = "";
  if (tservice->get_extends() != NULL) {
    extends = type_name(tservice->get_extends());
    extends_processor = " extends " + extends + ".Processor";
  }

  // Generate the header portion
  indent(f_service_) <<
    "public static class Processor" << extends_processor << " implements TProcessor {" << endl;
  indent_up();

  indent(f_service_) << "private static final Logger LOGGER = LoggerFactory.getLogger(Processor.class.getName());" << endl;

  indent(f_service_) <<
    "public Processor(Iface iface)" << endl;
  scope_up(f_service_);
  if (!extends.empty()) {
    f_service_ <<
      indent() << "super(iface);" << endl;
  }
  f_service_ <<
    indent() << "iface_ = iface;" << endl;

  if (extends.empty()) {
    f_service_ <<
      indent() << "event_handler_ = new TProcessorEventHandler(); // Empty handler" << endl;
  }

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_service_ <<
      indent() << "processMap_.put(\"" << (*f_iter)->get_name() << "\", new " << (*f_iter)->get_name() << "());" << endl;
  }

  scope_down(f_service_);
  f_service_ << endl;

  if (extends.empty()) {
    f_service_ <<
      indent() << "protected static interface ProcessFunction {" << endl <<
      indent() << "  public void process(int seqid, TProtocol iprot, TProtocol oprot, TConnectionContext server_ctx) throws TException;" << endl <<
      indent() << "}" << endl <<
      endl;

    f_service_ <<
      indent() <<
      "public void setEventHandler(TProcessorEventHandler handler) {" << endl <<
      indent() << "  this.event_handler_ = handler;" << endl <<
      indent() << "}" << endl <<
      endl;
  }

  f_service_ <<
    indent() << "private Iface iface_;" << endl;

  if (extends.empty()) {
    f_service_ <<
      indent() << "protected TProcessorEventHandler event_handler_;" << endl;
  }

  if (extends.empty()) {
    f_service_ <<
      indent() << "protected final HashMap<String,ProcessFunction> processMap_ = new HashMap<String,ProcessFunction>();" << endl;
  }

  f_service_ << endl;

  // Generate the server implementation
  indent(f_service_) <<
    "public boolean process(TProtocol iprot, TProtocol oprot, TConnectionContext server_ctx) throws TException" << endl;
  scope_up(f_service_);

  f_service_ <<
    indent() << "TMessage msg = iprot.readMessageBegin();" << endl;

  // TODO(mcslee): validate message, was the seqid etc. legit?

  f_service_ <<
    indent() << "ProcessFunction fn = processMap_.get(msg.name);" << endl <<
    indent() << "if (fn == null) {" << endl <<
    indent() << "  TProtocolUtil.skip(iprot, TType.STRUCT);" << endl <<
    indent() << "  iprot.readMessageEnd();" << endl <<
    indent() << "  TApplicationException x = new TApplicationException(TApplicationException.UNKNOWN_METHOD, \"Invalid method name: '\"+msg.name+\"'\");" << endl <<
    indent() << "  oprot.writeMessageBegin(new TMessage(msg.name, TMessageType.EXCEPTION, msg.seqid));" << endl <<
    indent() << "  x.write(oprot);" << endl <<
    indent() << "  oprot.writeMessageEnd();" << endl <<
    indent() << "  oprot.getTransport().flush();" << endl <<
    indent() << "  return true;" << endl <<
    indent() << "}" << endl <<
    indent() << "fn.process(msg.seqid, iprot, oprot, server_ctx);" << endl;

  f_service_ <<
    indent() << "return true;" << endl;

  scope_down(f_service_);
  f_service_ << endl;

  // Generate the process subfunctions
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    generate_process_function(tservice, *f_iter);
  }

  indent_down();
  indent(f_service_) <<
    "}" << endl <<
    endl;
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_java_generator::generate_function_helpers(t_function* tfunction) {
  if (tfunction->is_oneway()) {
    return;
  }

  t_struct result(program_, tfunction->get_name() + "_result");
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

  generate_java_struct_definition(f_service_, &result, false, true, true);
}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_java_generator::generate_process_function(t_service* tservice,
                                                 t_function* tfunction) {
  // Open class
  indent(f_service_) <<
    "private class " << tfunction->get_name() << " implements ProcessFunction {" << endl;
  indent_up();

  // Open function
  indent(f_service_) <<
    "public void process(int seqid, TProtocol iprot, TProtocol oprot, TConnectionContext server_ctx) throws TException" << endl;
  scope_up(f_service_);

  string argsname = tfunction->get_name() + "_args";
  string resultname = tfunction->get_name() + "_result";
  string pservice_func_name = "\"" + tservice->get_name() + "." + tfunction->get_name() + "\"";
  f_service_ <<
    indent() << "Object handler_ctx = event_handler_.getContext("
             << pservice_func_name << ", server_ctx);" << endl <<
    indent() << argsname << " args = new " << argsname << "();" << endl <<
    indent() << "event_handler_.preRead(handler_ctx, "
             << pservice_func_name << ");" << endl <<
    indent() << "args.read(iprot);" << endl <<
    indent() << "iprot.readMessageEnd();" << endl <<
    indent() << "event_handler_.postRead(handler_ctx, "
             << pservice_func_name << ", args);" << endl;

  t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;

  // Declare result for non oneway function
  if (!tfunction->is_oneway()) {
    f_service_ <<
      indent() << resultname << " result = new " << resultname << "();" << endl;
  }

  // Try block for a function with exceptions
  if (xceptions.size() > 0) {
    f_service_ <<
      indent() << "try {" << endl;
    indent_up();
  }

  // Generate the function call
  t_struct* arg_struct = tfunction->get_arglist();
  const std::vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator f_iter;

  f_service_ << indent();
  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
    f_service_ << "result.success = ";
  }
  f_service_ <<
    "iface_." << tfunction->get_name() << "(";
  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      f_service_ << ", ";
    }
    f_service_ << "args." << (*f_iter)->get_name();
  }
  f_service_ << ");" << endl;

  // Set isset on success field
  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void() && !type_can_be_null(tfunction->get_returntype())) {
    f_service_ <<
      indent() << "result.set" << get_cap_name("success") << get_cap_name("isSet") << "(true);" << endl;
  }

  if (!tfunction->is_oneway() && xceptions.size() > 0) {
    indent_down();
    f_service_ << indent() << "}";
    for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
      f_service_ << " catch (" << type_name((*x_iter)->get_type(), false, false) << " " << (*x_iter)->get_name() << ") {" << endl;
      if (!tfunction->is_oneway()) {
        indent_up();
        f_service_ <<
          indent() << "result." << (*x_iter)->get_name() << " = " << (*x_iter)->get_name() << ";" << endl;
        indent_down();
        f_service_ << indent() << "}";
      } else {
        f_service_ << "}";
      }
    }
    f_service_ << " catch (Throwable th) {" << endl;
    indent_up();
    string pservice_func_name = "\"" + tservice->get_name() + "." + tfunction->get_name() + "\"";
    string pservice_func_name_error = tservice->get_name() + "." + tfunction->get_name();
    f_service_ <<
      indent() << "LOGGER.error(\"Internal error processing " << pservice_func_name_error << "\", th);" << endl <<
      indent() << "event_handler_.handlerError(handler_ctx, \""
               << pservice_func_name_error << "\", th);" << endl <<
      indent() << "TApplicationException x = new TApplicationException(TApplicationException.INTERNAL_ERROR, \"Internal error processing " << pservice_func_name_error << "\");" << endl <<
      indent() << "event_handler_.preWrite(handler_ctx, \""
               << pservice_func_name_error << "\", null);" << endl <<
      indent() << "oprot.writeMessageBegin(new TMessage(\"" << pservice_func_name_error << "\", TMessageType.EXCEPTION, seqid));" << endl <<
      indent() << "x.write(oprot);" << endl <<
      indent() << "oprot.writeMessageEnd();" << endl <<
      indent() << "oprot.getTransport().flush();" << endl <<
      indent() << "event_handler_.postWrite(handler_ctx, "
               << pservice_func_name << ", null);" << endl <<
      indent() << "return;" << endl;
    indent_down();
    f_service_ << indent() << "}" << endl;
  }

  // Shortcut out here for oneway functions
  if (tfunction->is_oneway()) {
    f_service_ <<
      indent() << "return;" << endl;
    scope_down(f_service_);

    // Close class
    indent_down();
    f_service_ <<
      indent() << "}" << endl <<
      endl;
    return;
  }

  //string pservice_func_name = "\"" + tservice->get_name() + "." + tfunction->get_name() + "\"";
  f_service_ <<
    indent() << "event_handler_.preWrite(handler_ctx, "
             << pservice_func_name << ", result);" << endl <<
    indent() << "oprot.writeMessageBegin(new TMessage(\"" << tfunction->get_name() << "\", TMessageType.REPLY, seqid));" << endl <<
    indent() << "result.write(oprot);" << endl <<
    indent() << "oprot.writeMessageEnd();" << endl <<
    indent() << "oprot.getTransport().flush();" << endl <<
    indent() << "event_handler_.postWrite(handler_ctx, "
             << pservice_func_name << ", result);" << endl;

  // Close function
  scope_down(f_service_);
  f_service_ << endl;

  // Close class
  indent_down();
  f_service_ <<
    indent() << "}" << endl <<
    endl;
}

/**
 * Deserializes a field of any type.
 *
 * @param tfield The field
 * @param prefix The variable name or container for this field
 */
void t_java_generator::generate_deserialize_field(ofstream& out,
                                                  t_field* tfield,
                                                  string prefix) {
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
          out << "readBinary();";
        } else {
          out << "readString();";
        }
        break;
      case t_base_type::TYPE_BOOL:
        out << "readBool();";
        break;
      case t_base_type::TYPE_BYTE:
        out << "readByte();";
        break;
      case t_base_type::TYPE_I16:
        out << "readI16();";
        break;
      case t_base_type::TYPE_I32:
        out << "readI32();";
        break;
      case t_base_type::TYPE_I64:
        out << "readI64();";
        break;
      case t_base_type::TYPE_DOUBLE:
        out << "readDouble();";
        break;
      case t_base_type::TYPE_FLOAT:
        out << "readFloat();";
        break;
      default:
        throw "compiler error: no Java name for base type " + t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      out << "readI32();";
    }
    out <<
      endl;
  } else {
    printf("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
           tfield->get_name().c_str(), type_name(type).c_str());
  }
}

/**
 * Generates an unserializer for a struct, invokes read()
 */
void t_java_generator::generate_deserialize_struct(ofstream& out,
                                                   t_struct* tstruct,
                                                   string prefix) {
  out <<
    indent() << prefix << " = new " << type_name(tstruct) << "();" << endl <<
    indent() << prefix << ".read(iprot);" << endl;
}

/**
 * Deserializes a container by reading its size and then iterating
 */
void t_java_generator::generate_deserialize_container(ofstream& out,
                                                      t_type* ttype,
                                                      string prefix) {
  scope_up(out);

  string obj;

  if (ttype->is_map()) {
    obj = tmp("_map");
  } else if (ttype->is_set()) {
    obj = tmp("_set");
  } else if (ttype->is_list()) {
    obj = tmp("_list");
  }

  // Declare variables, read header
  if (ttype->is_map()) {
    indent(out) << "TMap " << obj << " = iprot.readMapBegin();" << endl;
  } else if (ttype->is_set()) {
    indent(out) << "TSet " << obj << " = iprot.readSetBegin();" << endl;
  } else if (ttype->is_list()) {
    indent(out) << "TList " << obj << " = iprot.readListBegin();" << endl;
  }

  // Protocol may have explicit non-negative size for a collection,
  // or specify -1 for size, and expect user to peek into collection
  // for one element at a time.

  indent(out)
    << prefix << " = new " << type_name(ttype, false, true)
    // size the collection correctly,
    // use initial capacity of 0 if there is no explicit size
    << "(Math.max(0, "
    << (ttype->is_list() ? "" : "2*" )
    << obj << ".size"
    << "));" << endl;

  // For loop iterates over elements
  // Use explicit size, if provided, or peek for one element at a time if not
  string i = tmp("_i");
  indent(out)
    << "for (int " << i << " = 0; " << endl;

  indent(out)
    << "     (" << obj << ".size < 0) ? ";

  if (ttype->is_map()) {
    out << "iprot.peekMap()";
  } else if (ttype->is_set()) {
    out << "iprot.peekSet()";
  } else if (ttype->is_list()) {
    out << "iprot.peekList()";
  }

  out << " : (" << i << " < " << obj << ".size" << "); " << endl;
  indent(out) << "     ++" << i << ")" << endl;

    scope_up(out);

    if (ttype->is_map()) {
      generate_deserialize_map_element(out, (t_map*)ttype, prefix);
    } else if (ttype->is_set()) {
      generate_deserialize_set_element(out, (t_set*)ttype, prefix);
    } else if (ttype->is_list()) {
      generate_deserialize_list_element(out, (t_list*)ttype, prefix);
    }

    scope_down(out);

  // Read container end
  if (ttype->is_map()) {
    indent(out) << "iprot.readMapEnd();" << endl;
  } else if (ttype->is_set()) {
    indent(out) << "iprot.readSetEnd();" << endl;
  } else if (ttype->is_list()) {
    indent(out) << "iprot.readListEnd();" << endl;
  }

  scope_down(out);
}


/**
 * Generates code to deserialize a map
 */
void t_java_generator::generate_deserialize_map_element(ofstream& out,
                                                        t_map* tmap,
                                                        string prefix) {
  string key = tmp("_key");
  string val = tmp("_val");
  t_field fkey(tmap->get_key_type(), key);
  t_field fval(tmap->get_val_type(), val);

  indent(out) <<
    declare_field(&fkey) << endl;
  indent(out) <<
    declare_field(&fval) << endl;

  generate_deserialize_field(out, &fkey);
  generate_deserialize_field(out, &fval);

  indent(out) <<
    prefix << ".put(" << key << ", " << val << ");" << endl;
}

/**
 * Deserializes a set element
 */
void t_java_generator::generate_deserialize_set_element(ofstream& out,
                                                        t_set* tset,
                                                        string prefix) {
  string elem = tmp("_elem");
  t_field felem(tset->get_elem_type(), elem);

  indent(out) <<
    declare_field(&felem) << endl;

  generate_deserialize_field(out, &felem);

  indent(out) <<
    prefix << ".add(" << elem << ");" << endl;
}

/**
 * Deserializes a list element
 */
void t_java_generator::generate_deserialize_list_element(ofstream& out,
                                                         t_list* tlist,
                                                         string prefix) {
  string elem = tmp("_elem");
  t_field felem(tlist->get_elem_type(), elem);

  indent(out) <<
    declare_field(&felem) << endl;

  generate_deserialize_field(out, &felem);

  indent(out) <<
    prefix << ".add(" << elem << ");" << endl;
}


/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_java_generator::generate_serialize_field(ofstream& out,
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
                              prefix + tfield->get_name());
  } else if (type->is_container()) {
    generate_serialize_container(out,
                                 type,
                                 prefix + tfield->get_name());
  } else if (type->is_base_type() || type->is_enum()) {

    string name = prefix + tfield->get_name();
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
          out << "writeBinary(" << name << ");";
        } else {
          out << "writeString(" << name << ");";
        }
        break;
      case t_base_type::TYPE_BOOL:
        out << "writeBool(" << name << ");";
        break;
      case t_base_type::TYPE_BYTE:
        out << "writeByte(" << name << ");";
        break;
      case t_base_type::TYPE_I16:
        out << "writeI16(" << name << ");";
        break;
      case t_base_type::TYPE_I32:
        out << "writeI32(" << name << ");";
        break;
      case t_base_type::TYPE_I64:
        out << "writeI64(" << name << ");";
        break;
      case t_base_type::TYPE_DOUBLE:
        out << "writeDouble(" << name << ");";
        break;
      case t_base_type::TYPE_FLOAT:
        out << "writeFloat(" << name << ");";
        break;
      default:
        throw "compiler error: no Java name for base type " + t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      out << "writeI32(" << name << ");";
    }
    out << endl;
  } else {
    printf("DO NOT KNOW HOW TO SERIALIZE FIELD '%s%s' TYPE '%s'\n",
           prefix.c_str(),
           tfield->get_name().c_str(),
           type_name(type).c_str());
  }
}

/**
 * Serializes all the members of a struct.
 *
 * @param tstruct The struct to serialize
 * @param prefix  String prefix to attach to all fields
 */
void t_java_generator::generate_serialize_struct(ofstream& out,
                                                 t_struct* tstruct,
                                                 string prefix) {
  out <<
    indent() << prefix << ".write(oprot);" << endl;
}

/**
 * Serializes a container by writing its size then the elements.
 *
 * @param ttype  The type of container
 * @param prefix String prefix for fields
 */
void t_java_generator::generate_serialize_container(ofstream& out,
                                                    t_type* ttype,
                                                    string prefix) {
  scope_up(out);

  if (ttype->is_map()) {
    indent(out) <<
      "oprot.writeMapBegin(new TMap(" <<
      type_to_enum(((t_map*)ttype)->get_key_type()) << ", " <<
      type_to_enum(((t_map*)ttype)->get_val_type()) << ", " <<
      prefix << ".size()));" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "oprot.writeSetBegin(new TSet(" <<
      type_to_enum(((t_set*)ttype)->get_elem_type()) << ", " <<
      prefix << ".size()));" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "oprot.writeListBegin(new TList(" <<
      type_to_enum(((t_list*)ttype)->get_elem_type()) << ", " <<
      prefix << ".size()));" << endl;
  }

  string iter = tmp("_iter");
  if (ttype->is_map()) {
    indent(out) <<
      "for (Map.Entry<" <<
      type_name(((t_map*)ttype)->get_key_type(), true, false) << ", " <<
      type_name(((t_map*)ttype)->get_val_type(), true, false) << "> " << iter <<
      " : " <<
      prefix << ".entrySet())";
  } else if (ttype->is_set()) {
    indent(out) <<
      "for (" <<
      type_name(((t_set*)ttype)->get_elem_type()) << " " << iter <<
      " : " <<
      prefix << ")";
  } else if (ttype->is_list()) {
    indent(out) <<
      "for (" <<
      type_name(((t_list*)ttype)->get_elem_type()) << " " << iter <<
      " : " <<
      prefix << ")";
  }

    scope_up(out);

    if (ttype->is_map()) {
      generate_serialize_map_element(out, (t_map*)ttype, iter, prefix);
    } else if (ttype->is_set()) {
      generate_serialize_set_element(out, (t_set*)ttype, iter);
    } else if (ttype->is_list()) {
      generate_serialize_list_element(out, (t_list*)ttype, iter);
    }

    scope_down(out);

  if (ttype->is_map()) {
    indent(out) <<
      "oprot.writeMapEnd();" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "oprot.writeSetEnd();" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "oprot.writeListEnd();" << endl;
  }

  scope_down(out);
}

/**
 * Serializes the members of a map.
 */
void t_java_generator::generate_serialize_map_element(ofstream& out,
                                                      t_map* tmap,
                                                      string iter,
                                                      string map) {
  t_field kfield(tmap->get_key_type(), iter + ".getKey()");
  generate_serialize_field(out, &kfield, "");
  t_field vfield(tmap->get_val_type(), iter + ".getValue()");
  generate_serialize_field(out, &vfield, "");
}

/**
 * Serializes the members of a set.
 */
void t_java_generator::generate_serialize_set_element(ofstream& out,
                                                      t_set* tset,
                                                      string iter) {
  t_field efield(tset->get_elem_type(), iter);
  generate_serialize_field(out, &efield, "");
}

/**
 * Serializes the members of a list.
 */
void t_java_generator::generate_serialize_list_element(ofstream& out,
                                                       t_list* tlist,
                                                       string iter) {
  t_field efield(tlist->get_elem_type(), iter);
  generate_serialize_field(out, &efield, "");
}

/**
 * Returns a Java type name
 *
 * @param ttype The type
 * @param container Is the type going inside a container?
 * @return Java type name, i.e. HashMap<Key,Value>
 */
string t_java_generator::type_name(t_type* ttype, bool in_container, bool in_init, bool skip_generic) {
  // In Java typedefs are just resolved to their real type
  ttype = get_true_type(ttype);
  string prefix;

  if (ttype->is_base_type()) {
    return base_type_name((t_base_type*)ttype, in_container);
  } else if (ttype->is_enum()) {
    return (in_container ? "Integer" : "int");
  } else if (ttype->is_map()) {
    t_map* tmap = (t_map*) ttype;
    if (in_init) {
      prefix = "HashMap";
    } else {
      prefix = "Map";
    }
    return prefix + (skip_generic ? "" : "<" +
      type_name(tmap->get_key_type(), true) + "," +
      type_name(tmap->get_val_type(), true) + ">");
  } else if (ttype->is_set()) {
    t_set* tset = (t_set*) ttype;
    if (in_init) {
      prefix = "HashSet";
    } else {
      prefix = "Set";
    }
    return prefix + (skip_generic ? "" : "<" + type_name(tset->get_elem_type(), true) + ">");
  } else if (ttype->is_list()) {
    t_list* tlist = (t_list*) ttype;
    if (in_init) {
      prefix = "ArrayList";
    } else {
      prefix = "List";
    }
    return prefix + (skip_generic ? "" : "<" + type_name(tlist->get_elem_type(), true) + ">");
  }

  // Check for namespacing
  t_program* program = ttype->get_program();
  if (program != NULL && program != program_) {
    string package = program->get_namespace("java");
    if (!package.empty()) {
      return package + "." + ttype->get_name();
    }
  }

  return ttype->get_name();
}

/**
 * Returns the C++ type that corresponds to the thrift type.
 *
 * @param tbase The base type
 * @param container Is it going in a Java container?
 */
string t_java_generator::base_type_name(t_base_type* type,
                                        bool in_container) {
  t_base_type::t_base tbase = type->get_base();

  switch (tbase) {
  case t_base_type::TYPE_VOID:
    return "void";
  case t_base_type::TYPE_STRING:
    if (type->is_binary()) {
      return "byte[]";
    } else {
      return "String";
    }
  case t_base_type::TYPE_BOOL:
    return (in_container ? "Boolean" : "boolean");
  case t_base_type::TYPE_BYTE:
    return (in_container ? "Byte" : "byte");
  case t_base_type::TYPE_I16:
    return (in_container ? "Short" : "short");
  case t_base_type::TYPE_I32:
    return (in_container ? "Integer" : "int");
  case t_base_type::TYPE_I64:
    return (in_container ? "Long" : "long");
  case t_base_type::TYPE_DOUBLE:
    return (in_container ? "Double" : "double");
  case t_base_type::TYPE_FLOAT:
    return (in_container ? "Float" : "float");
  default:
    throw "compiler error: no C++ name for base type " + t_base_type::t_base_name(tbase);
  }
}

/**
 * Declares a field, which may include initialization as necessary.
 *
 * @param ttype The type
 */
string t_java_generator::declare_field(t_field* tfield, bool init) {
  // TODO(mcslee): do we ever need to initialize the field?
  string result = type_name(tfield->get_type()) + " " + tfield->get_name();
  if (init) {
    t_type* ttype = get_true_type(tfield->get_type());
    if (ttype->is_base_type() && tfield->get_value() != NULL) {
      ofstream dummy;
      result += " = " + render_const_value(dummy, tfield->get_name(), ttype, tfield->get_value());
    } else if (ttype->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)ttype)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw "NO T_VOID CONSTRUCT";
      case t_base_type::TYPE_STRING:
        result += " = null";
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
        result += " = (double)0";
        break;
      case t_base_type::TYPE_FLOAT:
        result += " = (float)0";
        break;
    }

    } else if (ttype->is_enum()) {
      result += " = 0";
    } else if (ttype->is_container()) {
      result += " = new " + type_name(ttype, false, true) + "()";
    } else {
      result += " = new " + type_name(ttype, false, true) + "()";;
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
string t_java_generator::function_signature(t_function* tfunction,
                                            string prefix) {
  t_type* ttype = tfunction->get_returntype();
  std::string result =
    type_name(ttype) + " " + prefix + tfunction->get_name() + "(" + argument_list(tfunction->get_arglist()) + ") throws ";
  t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;
  for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
    result += type_name((*x_iter)->get_type(), false, false) + ", ";
  }
  result += "TException";
  return result;
}

/**
  * Renders a function signature of the form 'void name(args, resultHandler)'
  *
  * @params tfunction Function definition
  * @return String of rendered function definition
  */
string t_java_generator::function_signature_async(t_function* tfunction, string result_handler_symbol, bool use_base_method, string prefix) {
  std::string arglist = async_function_call_arglist(tfunction, result_handler_symbol, use_base_method, true);
  std::string result = prefix + "void " + tfunction->get_name() + "(" + arglist + ")";
  return result;
}

string t_java_generator::async_function_call_arglist(t_function* tfunc, string result_handler_symbol, bool use_base_method, bool include_types) {
  std::string arglist = "";
  if (tfunc->get_arglist()->get_members().size() > 0) {
    arglist = argument_list(tfunc->get_arglist(), include_types) + ", ";
  }

  if (include_types) {
    arglist += "AsyncMethodCallback ";
  }
  arglist += result_handler_symbol;

  return arglist;
}

/**
 * Renders a comma separated field list, with type names
 */
string t_java_generator::argument_list(t_struct* tstruct, bool include_types) {
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
    if (include_types) {
      result += type_name((*f_iter)->get_type()) + " ";
    }
    result += (*f_iter)->get_name();
  }
  return result;
}

string t_java_generator::async_argument_list(t_function* tfunct, t_struct* tstruct, string result_handler_symbol, bool include_types) {
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
    if (include_types) {
      result += type_name((*f_iter)->get_type()) + " ";
    }
    result += (*f_iter)->get_name();
  }
  if (!first) {
    result += ", ";
  }
  if (include_types) {
    result += "AsyncMethodCallback ";
  }
  result += result_handler_symbol;
  return result;
}

/**
 * Converts the parse type to a C++ enum string for the given type.
 */
string t_java_generator::type_to_enum(t_type* type) {
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

/**
 * Applies the correct style to a string based on the value of nocamel_style_
 */
std::string t_java_generator::get_cap_name(std::string name){
  if (nocamel_style_) {
    return "_" + name;
  } else {
    name[0] = toupper(name[0]);
    return name;
  }
}

string t_java_generator::constant_name(string name) {
  string constant_name;

  bool is_first = true;
  bool was_previous_char_upper = false;
  for (string::iterator iter = name.begin(); iter != name.end(); ++iter) {
    string::value_type character = (*iter);

    bool is_upper = isupper(character);

    if (is_upper && !is_first && !was_previous_char_upper) {
      constant_name += '_';
    }
    constant_name += toupper(character);

    is_first = false;
    was_previous_char_upper = is_upper;
  }

  return constant_name;
}

void t_java_generator::generate_java_docstring_comment(ofstream &out, string contents) {
  generate_docstring_comment(out,
    "/**\n",
    " * ", contents,
    " */\n");
}

void t_java_generator::generate_java_doc(ofstream &out,
                                         t_field* field) {
  if (field->get_type()->is_enum()) {
    string combined_message = field->get_doc() + "\n@see " + get_enum_class_name(field->get_type());
    generate_java_docstring_comment(out, combined_message);
  } else {
    generate_java_doc(out, (t_doc*)field);
  }
}

/**
 * Emits a JavaDoc comment if the provided object has a doc in Thrift
 */
void t_java_generator::generate_java_doc(ofstream &out,
                                         t_doc* tdoc) {
  if (tdoc->has_doc()) {
    generate_java_docstring_comment(out, tdoc->get_doc());
  }
}

/**
 * Emits a JavaDoc comment if the provided function object has a doc in Thrift
 */
void t_java_generator::generate_java_doc(ofstream &out,
                                         t_function* tfunction) {
  if (tfunction->has_doc()) {
    stringstream ss;
    ss << tfunction->get_doc();
    const vector<t_field*>& fields = tfunction->get_arglist()->get_members();
    vector<t_field*>::const_iterator p_iter;
    for (p_iter = fields.begin(); p_iter != fields.end(); ++p_iter) {
      t_field* p = *p_iter;
      ss << "\n@param " << p->get_name();
      if (p->has_doc()) {
        ss << " " << p->get_doc();
      }
    }
    generate_docstring_comment(out,
      "/**\n",
      " * ", ss.str(),
      " */\n");
  }
}

void t_java_generator::generate_deep_copy_container(ofstream &out, std::string source_name_p1, std::string source_name_p2,
                                                    std::string result_name, t_type* type) {

  t_container* container = (t_container*)type;
  std::string source_name;
  if (source_name_p2 == "")
      source_name = source_name_p1;
  else
      source_name = source_name_p1 + "." + source_name_p2;

  indent(out) << type_name(type, true, false) << " " << result_name << " = new " << type_name(container, false, true) << "();" << endl;

  std::string iterator_element_name = source_name_p1 + "_element";
  std::string result_element_name = result_name + "_copy";

  if(container->is_map()) {
    t_type* key_type = ((t_map*)container)->get_key_type();
    t_type* val_type = ((t_map*)container)->get_val_type();

    indent(out) <<
      "for (Map.Entry<" << type_name(key_type, true, false) << ", " << type_name(val_type, true, false) << "> " << iterator_element_name << " : " << source_name << ".entrySet()) {" << endl;
    indent_up();

    out << endl;

    indent(out) << type_name(key_type, true, false) << " " << iterator_element_name << "_key = " << iterator_element_name << ".getKey();" << endl;
    indent(out) << type_name(val_type, true, false) << " " << iterator_element_name << "_value = " << iterator_element_name << ".getValue();" << endl;

    out << endl;

    if (key_type->is_container()) {
      generate_deep_copy_container(out, iterator_element_name + "_key", "", result_element_name + "_key", key_type);
    } else {
      indent(out) << type_name(key_type, true, false) << " " << result_element_name << "_key = ";
      generate_deep_copy_non_container(out, iterator_element_name + "_key", result_element_name + "_key", key_type);
      out << ";" << endl;
    }

    out << endl;

    if (val_type->is_container()) {
      generate_deep_copy_container(out, iterator_element_name + "_value", "", result_element_name + "_value", val_type);
    } else {
      indent(out) << type_name(val_type, true, false) << " " << result_element_name << "_value = ";
      generate_deep_copy_non_container(out, iterator_element_name + "_value", result_element_name + "_value", val_type);
      out << ";" << endl;
    }

    out << endl;

    indent(out) << result_name << ".put(" << result_element_name << "_key, " << result_element_name << "_value);" << endl;

    indent_down();
    indent(out) << "}" << endl;

  } else {
    t_type* elem_type;

    if (container->is_set()) {
      elem_type = ((t_set*)container)->get_elem_type();
    } else {
      elem_type = ((t_list*)container)->get_elem_type();
    }

    indent(out)
      << "for (" << type_name(elem_type, true, false) << " " << iterator_element_name << " : " << source_name << ") {" << endl;

    indent_up();

    if (elem_type->is_container()) {
      // recursive deep copy
      generate_deep_copy_container(out, iterator_element_name, "", result_element_name, elem_type);
      indent(out) << result_name << ".add(" << result_element_name << ");" << endl;
    } else {
      // iterative copy
      if(((t_base_type*)elem_type)->is_binary()){
        indent(out) << "byte[] temp_binary_element = ";
        generate_deep_copy_non_container(out, iterator_element_name, "temp_binary_element", elem_type);
        out << ";" << endl;
        indent(out) << result_name << ".add(temp_binary_element);" << endl;
      }
      else{
        indent(out) << result_name << ".add(";
        generate_deep_copy_non_container(out, iterator_element_name, result_name, elem_type);
        out << ");" << endl;
      }
    }

    indent_down();

    indent(out) << "}" << endl;

  }
}

void t_java_generator::generate_deep_copy_non_container(ofstream& out, std::string source_name, std::string dest_name, t_type* type) {
  if (type->is_base_type() || type->is_enum() || type->is_typedef()) {
    // binary fields need to be copied with System.arraycopy
    if (((t_base_type*)type)->is_binary()){
      out << "new byte[" << source_name << ".length];" << endl;
      indent(out) << "System.arraycopy(" << source_name << ", 0, " << dest_name << ", 0, " << source_name << ".length)";
    }
    // everything else can be copied directly
    else
      out << source_name;
  } else {
    out << "new " << type_name(type, true, true) << "(" << source_name << ")";
  }
}

std::string t_java_generator::isset_field_id(t_field* field) {
  return "__" + upcase_string(field->get_name() + "_isset_id");
}

std::string t_java_generator::generate_isset_check(t_field* field) {
  return generate_isset_check(field->get_name());
}

std::string t_java_generator::generate_isset_check(std::string field_name) {
  return "is" + get_cap_name("set") + get_cap_name(field_name) + "()";
}

std::string t_java_generator::generate_setfield_check(t_field* field) {
  return generate_setfield_check(field->get_name());
}

std::string t_java_generator::generate_setfield_check(std::string field_name) {
  return "getSetField() == " + upcase_string(field_name);
}

void t_java_generator::generate_isset_set(ofstream& out, t_field* field) {
  if (!type_can_be_null(field->get_type())) {
    indent(out) << "set" << get_cap_name(field->get_name()) << get_cap_name("isSet") << "(true);" << endl;
  }
}

std::string t_java_generator::get_enum_class_name(t_type* type) {
  string package = "";
  t_program* program = type->get_program();
  if (program != NULL && program != program_) {
    package = program->get_namespace("java") + ".";
  }
  return package + type->get_name();
}

void t_java_generator::generate_struct_desc(ofstream& out, t_struct* tstruct) {
  indent(out) <<
    "private static final TStruct STRUCT_DESC = new TStruct(\"" << tstruct->get_name() << "\");" << endl;
}

void t_java_generator::generate_field_descs(ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out) <<
      "private static final TField " << constant_name((*m_iter)->get_name()) <<
      "_FIELD_DESC = new TField(\"" << (*m_iter)->get_name() << "\", " <<
      type_to_enum((*m_iter)->get_type()) << ", " <<
      "(short)" << (*m_iter)->get_key() << ");" << endl;
  }
}

void t_java_generator::generate_field_name_constants(ofstream& out, t_struct* tstruct) {
  // Members are public for -java, private for -javabean
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out) << "public static final int " << upcase_string((*m_iter)->get_name()) << " = " << (*m_iter)->get_key() << ";" << endl;
  }
}

bool t_java_generator::is_comparable(t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (!is_comparable((*m_iter)->get_type())) {
      return false;
    }
  }
  return true;
}

bool t_java_generator::is_comparable(t_type* type) {
  return false;
  if (type->is_container()) {
    if (type->is_list()) {
      return is_comparable(((t_list*)type)->get_elem_type());
    } else {
      return false;
    }
  } else if (type->is_struct()) {
    return is_comparable((t_struct*)type);
  } else {
    return true;
  }
}

bool t_java_generator::has_bit_vector(t_struct* tstruct) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (!type_can_be_null((*m_iter)->get_type())) {
      return true;
    }
  }
  return false;
}

THRIFT_REGISTER_GENERATOR(java, "Java",
"    beans:           Generate bean-style output files.\n"
"    nocamel:         Do not use CamelCase field accessors with beans.\n"
"    hashcode:        Generate quality hashCode methods.\n"
);
