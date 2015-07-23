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

#ifndef T_JAVA_GENERATOR_H
#define T_JAVA_GENERATOR_H

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

  void init_generator() override;
  void close_generator() override;

  void generate_consts(std::vector<t_const*> consts) override;

  /**
   * Program-level generation functions
   */

  void generate_typedef(t_typedef* ttypedef) override;
  void generate_enum(t_enum* tenum) override;
  void generate_struct(t_struct* tstruct) override;
  void generate_union   (t_struct*   tunion);
  void generate_xception(t_struct* txception) override;
  void generate_service(t_service* tservice) override;
  void generate_default_toString(ofstream&, t_struct*);
  void generate_toString_prettyprint(std::ofstream&);

  virtual void print_const_value(std::ostream& out,
                                 std::string name,
                                 t_type* type,
                                 t_const_value* value,
                                 bool in_static,
                                 bool defval=false);
  virtual std::string render_const_value(std::ostream& out,
                                        std::string name,
                                        t_type* type,
                                        t_const_value* value);

  /**
   * Service-level generation functions
   */

  void generate_java_struct(t_struct* tstruct, bool is_exception);

  void generate_java_constructor(ofstream &out,
                                 t_struct* tstruct,
                                 const vector<t_field*>& fields);
  void generate_java_struct_definition(std::ofstream& out,
                                       t_struct* tstruct,
                                       bool is_xception=false,
                                       bool in_class=false,
                                       bool is_result=false);
  void generate_java_struct_equality(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_compare_to(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_reader(std::ofstream& out, t_struct* tstruct);
  void generate_java_validator(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_result_writer(std::ofstream& out,
                                          t_struct* tstruct);
  void generate_java_struct_writer(std::ofstream& out, t_struct* tstruct);
  void generate_java_struct_tostring(std::ofstream& out, t_struct* tstruct);
  void generate_java_meta_data_map(std::ofstream& out, t_struct* tstruct);
  void generate_field_value_meta_data(std::ofstream& out, t_type* type);
  std::string get_java_type_string(t_type* type);
  void generate_reflection_setters(std::ostringstream& out,
                                   t_type* type,
                                   std::string field_name,
                                   std::string cap_name);
  void generate_reflection_getters(std::ostringstream& out,
                                   t_type* type,
                                   std::string field_name,
                                   std::string cap_name);
  void generate_generic_field_getters_setters(std::ofstream& out,
                                              t_struct* tstruct);
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

  bool is_comparable(t_type* type, std::vector<t_type*>* enclosing=nullptr);
  bool struct_has_all_comparable_fields(t_struct* tstruct,
                                        std::vector<t_type*>* enclosing);

  bool type_has_naked_binary(t_type* type);
  bool struct_has_naked_binary_fields(t_struct* tstruct);

  bool has_bit_vector(t_struct* tstruct);


  /**
   * Helper rendering functions
   */

  std::string java_package();
  virtual std::string java_type_imports();
  std::string java_thrift_imports();
  std::string java_suppress_warnings_enum();
  std::string java_suppress_warnings_consts();
  std::string java_suppress_warnings_union();
  std::string java_suppress_warnings_struct();
  std::string java_suppress_warnings_service();
  virtual std::string type_name(t_type* ttype,
                                bool in_container=false,
                                bool in_init=false,
                                bool skip_generic=false);
  std::string base_type_name(t_base_type* tbase, bool in_container=false);
  std::string declare_field(t_field* tfield, bool init=false);
  std::string function_signature(t_function* tfunction, std::string prefix="");
  std::string function_signature_async(t_function* tfunction,
                                       std::string result_handler_symbol,
                                       bool use_base_method=false,
                                       std::string prefix="");
  std::string argument_list(t_struct* tstruct,
                            bool include_types=true);
  std::string async_function_call_arglist(t_function* tfunc,
                                          std::string result_handler_symbol,
                                          bool use_base_method=true,
                                          bool include_types=true);
  std::string async_argument_list(t_function* tfunct,
                                  t_struct* tstruct,
                                  std::string result_handler_symbol,
                                  bool include_types=false);
  std::string type_to_enum(t_type* ttype);
  std::string get_enum_class_name(t_type* type);
  void generate_struct_desc(ofstream& out, t_struct* tstruct);
  void generate_field_descs(ofstream& out, t_struct* tstruct);
  void generate_field_name_constants(ofstream& out, t_struct* tstruct);

  virtual const std::string& get_package_dir() {
    return package_dir_;
  }

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

#endif
