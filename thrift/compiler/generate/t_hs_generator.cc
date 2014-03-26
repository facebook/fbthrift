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

#include "t_oop_generator.h"

// platform.h
#define MKDIR(x) mkdir(x, S_IRWXU | S_IRWXG | S_IRWXO)

using std::map;
using std::ofstream;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::vector;

static const string nl = "\n";  // avoid ostream << std::endl flushes

/**
 * Haskell code generator.
 *
 */
class t_hs_generator : public t_oop_generator {
 public:
  t_hs_generator(t_program* program,
                 const map<string, string>& parsed_options,
                 const string& option_string)
    : t_oop_generator(program)
  {
    (void) parsed_options;
    (void) option_string;
    out_dir_base_ = "gen-hs";
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

  string render_const_value(t_type* type, t_const_value* value);

  /**
   * Struct generation code
   */

  void generate_hs_struct            (t_struct* tstruct,
                                      bool is_exception);

  void generate_hs_struct_definition (ofstream &out,
                                      t_struct* tstruct,
                                      bool is_xception = false,
                                      bool helper = false);

  void generate_hs_struct_reader     (ofstream& out,
                                      t_struct* tstruct);

  void generate_hs_struct_writer     (ofstream& out,
                                      t_struct* tstruct);

  void generate_hs_struct_arbitrary  (ofstream& out,
                                      t_struct* tstruct);

  void generate_hs_function_helpers  (t_function* tfunction);

  /**
   * Service-level generation functions
   */

  void generate_service_helpers   (t_service* tservice);
  void generate_service_interface (t_service* tservice);
  void generate_service_client    (t_service* tservice);
  void generate_service_server    (t_service* tservice);
  void generate_service_fuzzer    (t_service* tservice);
  void generate_process_function  (t_service* tservice,
                                   t_function* tfunction);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field        (ofstream &out,
                                          t_field* tfield,
                                          string prefix);

  void generate_deserialize_struct       (ofstream &out,
                                          t_struct* tstruct);

  void generate_deserialize_container    (ofstream &out,
                                          t_type* ttype);

  void generate_deserialize_set_element  (ofstream &out,
                                          t_set* tset);


  void generate_deserialize_list_element (ofstream &out,
                                          t_list* tlist,
                                          string prefix = "");

  void generate_deserialize_type          (ofstream &out,
                                           t_type* type);

  void generate_serialize_field          (ofstream &out,
                                          t_field* tfield,
                                          string name = "");

  void generate_serialize_struct         (ofstream &out,
                                          t_struct* tstruct,
                                          string prefix = "");

  void generate_serialize_container      (ofstream &out,
                                          t_type* ttype,
                                          string prefix = "");

  void generate_serialize_map_element    (ofstream &out,
                                          t_map* tmap,
                                          string kiter,
                                          string viter);

  void generate_serialize_set_element    (ofstream &out,
                                          t_set* tmap,
                                          string iter);

  void generate_serialize_list_element   (ofstream &out,
                                          t_list* tlist,
                                          string iter);

  /**
   * Helper rendering functions
   */

  string hs_autogen_comment();
  string hs_language_pragma();
  string hs_imports();

  string type_name(t_type* ttype,
                   string function_prefix = "");

  string function_type(t_function* tfunc,
                       bool options = false,
                       bool io = false,
                       bool method = false);

  string type_to_enum(t_type* ttype);

  string render_hs_type(t_type* type,
                        bool needs_parens);

  string render_hs_type_for_function_name(t_type *type);

 private:

  ofstream f_types_;
  ofstream f_consts_;
  ofstream f_service_;
  ofstream f_iface_;
  ofstream f_client_;
  ofstream f_service_fuzzer_;
};

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_hs_generator::init_generator() {
  // Make output directory
  MKDIR(get_out_dir().c_str());

  // Make output file
  string pname = capitalize(program_name_);
  string f_types_name = get_out_dir() + pname + "_Types.hs";
  f_types_.open(f_types_name.c_str());
  record_genfile(f_types_name);

  string f_consts_name = get_out_dir() + pname + "_Consts.hs";
  f_consts_.open(f_consts_name.c_str());
  record_genfile(f_consts_name);

  // Print header
  f_types_ << hs_language_pragma() << nl;
  f_types_ << hs_autogen_comment() << nl;
  f_types_ << "module " << pname << "_Types where" << nl;
  f_types_ << hs_imports() << nl;

  f_consts_ << hs_language_pragma() << nl;
  f_consts_ << hs_autogen_comment() << nl;
  f_consts_ << "module " << pname << "_Consts where" << nl;
  f_consts_ << hs_imports() << nl;
  f_consts_ << "import " << pname << "_Types" << nl;
}

string t_hs_generator::hs_language_pragma() {
  return string("{-# LANGUAGE DeriveDataTypeable #-}\n"
                "{-# LANGUAGE OverloadedStrings #-}\n"
                "{-# OPTIONS_GHC -fno-warn-missing-fields #-}\n"
                "{-# OPTIONS_GHC -fno-warn-missing-signatures #-}\n"
                "{-# OPTIONS_GHC -fno-warn-name-shadowing #-}\n"
                "{-# OPTIONS_GHC -fno-warn-unused-imports #-}\n"
                "{-# OPTIONS_GHC -fno-warn-unused-matches #-}\n");
}

/**
 * Autogen'd comment
 */
string t_hs_generator::hs_autogen_comment() {
  return string("-----------------------------------------------------------------\n") +
                "-- Autogenerated by Thrift\n" +
                "--                                                             --\n" +
                "-- DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING\n" +
                "--  @""generated\n" +
                "-----------------------------------------------------------------\n";
}

/**
 * Prints standard thrift imports
 */
string t_hs_generator::hs_imports() {
  const vector<t_program*>& includes = program_->get_includes();
  string result = string(
      "import Prelude ( Bool(..), Enum, Float, Double, String, Maybe(..),\n"
      "                 Eq, Show, Ord,\n"
      "                 return, length, IO, fromIntegral, fromEnum, toEnum,\n"
      "                 enumFromTo, Bounded, minBound, maxBound,\n"
      "                 (.), (&&), (||), (==), (++), ($), (-))\n"
      "\n"
      "import Control.Exception\n"
      "import Control.Monad ( liftM, ap )\n"
      "import Data.ByteString.Lazy\n"
      "import Data.Hashable\n"
      "import Data.Int\n"
      "import Data.Text.Lazy ( Text )\n"
      "import qualified Data.Text.Lazy as TL\n"
      "import Data.Typeable ( Typeable )\n"
      "import qualified Data.HashMap.Strict as Map\n"
      "import qualified Data.HashSet as Set\n"
      "import qualified Data.Vector as Vector\n"
      "import Test.QuickCheck.Arbitrary ( Arbitrary(..) )\n"
      "import Test.QuickCheck ( elements )\n"
      "\n"
      "import Thrift\n"
      "import Thrift.Types ()\n"
      "import Thrift.Arbitraries\n"
      "\n");

  for (size_t i = 0; i < includes.size(); ++i)
    result += "import qualified " + capitalize(includes[i]->get_name()) + "_Types\n";

  if (includes.size() > 0)
    result += "\n";

  return result;
}

/**
 * Closes the type files
 */
void t_hs_generator::close_generator() {
  // Close types file
  f_types_.close();
  f_consts_.close();
}

/**
 * Generates a typedef. Ez.
 *
 * @param ttypedef The type definition
 */
void t_hs_generator::generate_typedef(t_typedef* ttypedef) {
  string tname = capitalize(ttypedef->get_symbolic());
  string tdef = render_hs_type(ttypedef->get_type(), false);
  indent(f_types_) << "type " << tname << " = " << tdef << nl;
  f_types_ << nl;
}

/**
 * Generates code for an enumerated type.
 * the values.
 *
 * @param tenum The enumeration
 */
void t_hs_generator::generate_enum(t_enum* tenum) {
  indent(f_types_) << "data " << capitalize(tenum->get_name()) << " = ";
  indent_up();
  vector<t_enum_value*> constants = tenum->get_constants();
  vector<t_enum_value*>::iterator c_iter;

  bool first = true;
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    string name = capitalize((*c_iter)->get_name());
    f_types_ << (first ? "" : "|");
    f_types_ << name;
    first = false;
  }
  indent(f_types_) << "deriving (Show,Eq, Typeable, Ord, Bounded)" << nl;
  indent_down();

  string ename = capitalize(tenum->get_name());

  indent(f_types_) << "instance Enum " << ename << " where" << nl;
  indent_up();
  indent(f_types_) << "fromEnum t = case t of" << nl;
  indent_up();
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int value = (*c_iter)->get_value();
    string name = capitalize((*c_iter)->get_name());
    indent(f_types_) << name << " -> " << value << nl;
  }
  indent_down();
  indent(f_types_) << "toEnum t = case t of" << nl;
  indent_up();
  for(c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    int value = (*c_iter)->get_value();
    string name = capitalize((*c_iter)->get_name());
    indent(f_types_) << value << " -> " << name << nl;
  }
  indent(f_types_) << "_ -> throw ThriftException" << nl;
  indent_down();
  indent_down();

  indent(f_types_) << "instance Hashable " << ename << " where" << nl;
  indent_up();
  indent(f_types_) << "hashWithSalt salt = hashWithSalt salt . fromEnum" << nl;
  indent_down();

  indent(f_types_) << "instance Arbitrary " << ename << " where" << nl;
  indent_up();
  indent(f_types_) << "arbitrary = elements (enumFromTo minBound maxBound)" << nl;
  indent_down();
}

/**
 * Generate a constant value
 */
void t_hs_generator::generate_const(t_const* tconst) {
  t_type* type = tconst->get_type();
  string name = decapitalize(tconst->get_name());

  t_const_value* value = tconst->get_value();

  indent(f_consts_) << name << " :: " << render_hs_type(type, false) << nl;
  indent(f_consts_) << name << " = " << render_const_value(type, value) << nl;
  f_consts_ << nl;
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
string t_hs_generator::render_const_value(t_type* type, t_const_value* value) {
  type = get_true_type(type);
  ostringstream out;

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {

    case t_base_type::TYPE_STRING:
      out << '"' << get_escaped_string(value) << '"';
      break;

    case t_base_type::TYPE_BOOL:
      out << (value->get_integer() > 0 ? "True" : "False");
      break;

    case t_base_type::TYPE_BYTE:
      out << "(" << value->get_integer() << " :: Int8)";
      break;

    case t_base_type::TYPE_I16:
      out << "(" << value->get_integer() << " :: Int16)";
      break;

    case t_base_type::TYPE_I32:
      out << "(" << value->get_integer() << " :: Int32)";
      break;

    case t_base_type::TYPE_I64:
      out << "(" << value->get_integer() << " :: Int64)";
      break;

    case t_base_type::TYPE_FLOAT:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        out << "(" << value->get_integer() << " :: Float)";
      } else {
        out << "(" << value->get_double() << " :: Float)";
      }
      break;

    case t_base_type::TYPE_DOUBLE:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        out << "(" << value->get_integer() << " :: Double)";
      } else {
        out << "(" << value->get_double() << " :: Double)";
      }
      break;

    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }

  } else if (type->is_enum()) {
    t_enum* tenum = (t_enum*)type;
    vector<t_enum_value*> constants = tenum->get_constants();
    vector<t_enum_value*>::iterator c_iter;
    for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
      int val = (*c_iter)->get_value();
      if (val == value->get_integer()) {
        indent(out) << capitalize((*c_iter)->get_name());
        break;
      }
    }

  } else if (type->is_struct() || type->is_xception()) {
    string cname = type_name(type);
    indent(out) << cname << "{";

    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;

    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;

    bool first = true;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      t_type* field_type = nullptr;

      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter)
        if ((*f_iter)->get_name() == v_iter->first->get_string())
          field_type = (*f_iter)->get_type();

      if (field_type == nullptr)
        throw "type error: " + type->get_name() + " has no field " + v_iter->first->get_string();

      string fname = v_iter->first->get_string();
      string const_value = render_const_value(field_type, v_iter->second);

      out << (first ? "" : ",");
      out << "f_" << cname << "_" << fname << " = Just (" << const_value << ")";
      first = false;
    }

    indent(out) << "}";

  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();

    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;

    out << "(Map.fromList [";

    bool first = true;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string key = render_const_value(ktype, v_iter->first);
      string val = render_const_value(vtype, v_iter->second);
      out << (first ? "" : ",");
      out << "(" << key << "," << val << ")";
      first = false;
    }
    out << "])";

  } else if (type->is_list() || type->is_set()) {
    t_type* etype = type->is_list()
        ? ((t_list*) type)->get_elem_type()
        : ((t_set*) type)->get_elem_type();

    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;

    if (type->is_set())
      out << "(Set.fromList [";
    else
      out << "(Vector.fromList ";

    bool first = true;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      out << (first ? "" : ",");
      out << render_const_value(etype, *v_iter);
      first = false;
    }

    out << "])";

  } else {
    throw "CANNOT GENERATE CONSTANT FOR TYPE: " + type->get_name();
  }

  return out.str();
}

/**
 * Generates a "struct"
 */
void t_hs_generator::generate_struct(t_struct* tstruct) {
  generate_hs_struct(tstruct, false);
}

/**
 * Generates a struct definition for a thrift exception. Basically the same
 * as a struct, but also has an exception declaration.
 *
 * @param txception The struct definition
 */
void t_hs_generator::generate_xception(t_struct* txception) {
  generate_hs_struct(txception, true);
}

/**
 * Generates a Haskell struct
 */
void t_hs_generator::generate_hs_struct(t_struct* tstruct,
                                        bool is_exception) {
  generate_hs_struct_definition(f_types_,tstruct, is_exception,false);
}

/**
 * Generates a struct definition for a thrift data type.
 *
 * @param tstruct The struct definition
 */
void t_hs_generator::generate_hs_struct_definition(ofstream& out,
                                                   t_struct* tstruct,
                                                   bool is_exception,
                                                   bool helper) {
  (void) helper;
  string tname = type_name(tstruct);
  string name = tstruct->get_name();

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  indent(out) << "data " << tname << " = " << tname;
  if (members.size() > 0) {
    out << "{";

    bool first = true;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      string mname = (*m_iter)->get_name();
      out << (first ? "" : ",");
      out << "f_" << tname << "_" << mname << " :: Maybe " << render_hs_type((*m_iter)->get_type(), true);
      first = false;
    }
    out << "}";
  }

  out << " deriving (Show,Eq,Typeable)" << nl;

  if (is_exception)
    out << "instance Exception " << tname << nl;

  indent(out) << "instance Hashable " << tname << " where" << nl;
  indent_up();
  indent(out) << "hashWithSalt salt record = salt";
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    string mname = (*m_iter)->get_name();
    indent(out) << " `hashWithSalt` " << "f_" << tname << "_" << mname << " record";
  }
  indent(out) << nl;
  indent_down();

  generate_hs_struct_arbitrary(out, tstruct);
  generate_hs_struct_writer(out, tstruct);
  generate_hs_struct_reader(out, tstruct);
}

void t_hs_generator::generate_hs_struct_arbitrary(ofstream& out, t_struct* tstruct) {
  string tname = type_name(tstruct);
  string name = tstruct->get_name();
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  indent(out) << "instance Arbitrary "<<tname<<" where "<<nl;
  indent_up();
  if (members.size() > 0) {
    indent(out) << "arbitrary = (liftM "<<tname<<") ";
    indent_up(); indent_up(); indent_up(); indent_up();
    bool first=true;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if(first) {
        first=false;
      }
      else {
        indent(out) << "`ap`";
      }
      out << "((liftM Just) arbitrary)" << nl;
    }
    indent_down(); indent_down(); indent_down(); indent_down();
  } else { /* 0 == members.size() */
     indent(out) << "arbitrary = elements [" <<tname<< "]" << nl;
  }
  indent_down();
}

/**
 * Generates the read method for a struct
 */
void t_hs_generator::generate_hs_struct_reader(ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  string sname = type_name(tstruct);
  string str = tmp("_str");
  string t = tmp("_t");
  string id = tmp("_id");

  indent(out) << "read_" << sname << "_fields iprot record = do" << nl;
  indent_up();

  // Read beginning field marker
  indent(out) << "(_," << t << "," << id << ") <- readFieldBegin iprot" << nl;

  // Check for field STOP marker and break
  indent(out) << "if " << t << " == T_STOP then return record else" << nl;

  indent_up();
  indent(out) << "case " << id << " of " << nl;
  indent_up();

  // Generate deserialization code for known cases
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    int32_t key = (*f_iter)->get_key();
    string etype = type_to_enum((*f_iter)->get_type());
    indent(out) << key << " -> " << "if " << t << " == " << etype << " then do" << nl;

    indent_up();
    indent(out) << "s <- ";
    generate_deserialize_field(out, *f_iter,str);
    out << nl;

    string fname = decapitalize((*f_iter)->get_name());
    indent(out) << "read_" << sname << "_fields iprot record{f_" << sname << "_" << fname << "=Just s}" << nl;

    indent(out) << "else do" << nl;

    indent_up();
    indent(out) << "skip iprot " << t << nl;

    indent(out) << "read_" << sname << "_fields iprot record" << nl;

    indent_down();
    indent_down();
  }

  // In the default case we skip the field
  indent(out) << "_ -> do" << nl;
  indent_up();
  indent(out) << "skip iprot " << t << nl;
  indent(out) << "readFieldEnd iprot" << nl;
  indent(out) << "read_" << sname << "_fields iprot record" << nl;
  indent_down();
  indent_down();
  indent_down();
  indent_down();

  // read
  indent(out) << "read_" << sname << " iprot = do" << nl;
  indent_up();
  indent(out) << "_ <- readStructBegin iprot" << nl;
  indent(out) << "record <- read_" << sname << "_fields iprot (" << sname << "{";

  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    out << (first ? "" : ",");
    out << "f_" << sname << "_" << decapitalize((*f_iter)->get_name()) << "=Nothing";
    first = false;
  }

  out << "})" << nl;
  indent(out) << "readStructEnd iprot" << nl;
  indent(out) << "return record" << nl;
  indent_down();
}

void t_hs_generator::generate_hs_struct_writer(ofstream& out,
                                               t_struct* tstruct) {
  string name = type_name(tstruct);
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;
  string str = tmp("_str");
  string f = tmp("_f");

  indent(out) << "write_" << name << " oprot record = do" << nl;
  indent_up();
  indent(out) << "writeStructBegin oprot \"" << name << "\"" << nl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Write field header
    string mname = (*f_iter)->get_name();
    indent(out) << "case f_" << name << "_" << mname << " record of {Nothing -> return (); Just _v -> do" << nl;

    indent_up();
    indent(out) << "writeFieldBegin oprot (\"" << (*f_iter)->get_name() << "\","
                << type_to_enum((*f_iter)->get_type()) << ","
                << (*f_iter)->get_key() << ")" << nl;

    // Write field contents
    indent(out);
    generate_serialize_field(out, *f_iter, "_v");
    out << nl;

    // Write field closer
    indent(out) << "writeFieldEnd oprot}" << nl;
    indent_down();
  }

  // Write the struct map
  indent(out) << "writeFieldStop oprot" << nl;
  indent(out) << "writeStructEnd oprot" << nl;

  indent_down();
}

/**
 * Generates a thrift service.
 *
 * @param tservice The service definition
 */
void t_hs_generator::generate_service(t_service* tservice) {
  string f_service_name = get_out_dir() + capitalize(service_name_) + ".hs";
  f_service_.open(f_service_name.c_str());
  record_genfile(f_service_name);

  f_service_ << hs_language_pragma() << nl;
  f_service_ << hs_autogen_comment() << nl;
  f_service_ << "module " << capitalize(service_name_) << " where" << nl;
  f_service_ << hs_imports() << nl;

  if (tservice->get_extends()) {
    f_service_ << "import qualified " << capitalize(tservice->get_extends()->get_name()) << nl;
  }

  f_service_ << "import " << capitalize(program_name_) << "_Types" << nl;
  f_service_ << "import qualified " << capitalize(service_name_) << "_Iface as Iface" << nl;

  // Generate the three main parts of the service
  generate_service_helpers(tservice);
  generate_service_interface(tservice);
  generate_service_client(tservice);
  generate_service_server(tservice);
  generate_service_fuzzer(tservice);

  // Close service file
  f_service_.close();
}

/**
 * Generates helper functions for a service.
 *
 * @param tservice The service to generate a header definition for
 */
void t_hs_generator::generate_service_helpers(t_service* tservice) {
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  indent(f_service_) << "-- HELPER FUNCTIONS AND STRUCTURES --" << nl;
  indent(f_service_) << nl;

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* ts = (*f_iter)->get_arglist();
    generate_hs_struct_definition(f_service_,ts, false);
    generate_hs_function_helpers(*f_iter);
  }
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_hs_generator::generate_hs_function_helpers(t_function* tfunction) {
  t_struct result(program_, decapitalize(tfunction->get_name()) + "_result");
  t_field success(tfunction->get_returntype(), "success", 0);

  if (!tfunction->get_returntype()->is_void())
    result.append(&success);

  t_struct* xs = tfunction->get_xceptions();
  const vector<t_field*>& fields = xs->get_members();

  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter)
    result.append(*f_iter);

  generate_hs_struct_definition(f_service_,&result, false);
}

/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 */
void t_hs_generator::generate_service_interface(t_service* tservice) {
  string f_iface_name = get_out_dir() + capitalize(service_name_) + "_Iface.hs";
  f_iface_.open(f_iface_name.c_str());
  record_genfile(f_iface_name);

  f_iface_ << hs_language_pragma() << nl;
  f_iface_ << hs_autogen_comment() << nl;

  f_iface_ << "module " << capitalize(service_name_) << "_Iface where" << nl;

  f_iface_ << hs_imports() << nl;
  f_iface_ << "import " << capitalize(program_name_) << "_Types" << nl;
  f_iface_ << nl;

  string sname = capitalize(service_name_);
  if (tservice->get_extends() != nullptr) {
    string extends = type_name(tservice->get_extends());

    indent(f_iface_) << "import " << extends << "_Iface" << nl;
    indent(f_iface_) << "class " << extends << "_Iface a => " << sname << "_Iface a where" << nl;

  } else {
    indent(f_iface_) << "class " << sname << "_Iface a where" << nl;
  }

  indent_up();

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    string ft = function_type(*f_iter, true, true, true);
    indent(f_iface_) << decapitalize((*f_iter)->get_name()) << " :: a -> " << ft  << nl;
  }

  indent_down();
  f_iface_.close();
}

/**
 * Generates a service client definition. Note that in Haskell, the client doesn't implement iface. This is because
 * The client does not (and should not have to) deal with arguments being Nothing.
 *
 * @param tservice The service to generate a server for.
 */
void t_hs_generator::generate_service_client(t_service* tservice) {
  string f_client_name = get_out_dir() + capitalize(service_name_) + "_Client.hs";
  f_client_.open(f_client_name.c_str());
  record_genfile(f_client_name);
  f_client_ << hs_language_pragma() << nl;
  f_client_ << hs_autogen_comment() << nl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;

  string extends = "";
  string exports = "";

  bool first = true;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    exports += (first ? "" : ",");
    string funname = (*f_iter)->get_name();
    exports += decapitalize(funname);
    first = false;
  }

  string sname = capitalize(service_name_);
  indent(f_client_) << "module " << sname << "_Client(" << exports << ") where" << nl;

  if (tservice->get_extends() != nullptr) {
    extends = type_name(tservice->get_extends());
    indent(f_client_) << "import " << extends << "_Client" << nl;
  }

  indent(f_client_) << "import Data.IORef" << nl;
  indent(f_client_) << hs_imports() << nl;
  indent(f_client_) << "import " << capitalize(program_name_) << "_Types" << nl;
  indent(f_client_) << "import " << capitalize(service_name_) << nl;

  // DATS RITE A GLOBAL VAR
  indent(f_client_) << "seqid = newIORef 0" << nl;

  // Generate client method implementations
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arg_struct = (*f_iter)->get_arglist();
    const vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator fld_iter;
    string funname = (*f_iter)->get_name();

    string fargs = "";
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter)
      fargs += " arg_" + decapitalize((*fld_iter)->get_name());

    // Open function
    indent(f_client_) << decapitalize(funname) << " (ip,op)" <<  fargs << " = do" << nl;
    indent_up();
    indent(f_client_) <<  "send_" << funname << " op" << fargs;

    f_client_ << nl;

    if (!(*f_iter)->is_oneway())
      indent(f_client_) << "recv_" << funname << " ip" << nl;

    indent_down();

    indent(f_client_) << "send_" << funname << " op" << fargs << " = do" << nl;
    indent_up();

    indent(f_client_) << "seq <- seqid" << nl;
    indent(f_client_) << "seqn <- readIORef seq" << nl;
    string argsname = capitalize((*f_iter)->get_name() + "_args");

    // Serialize the request header
    string fname = (*f_iter)->get_name();
    indent(f_client_) << "writeMessageBegin op (\"" << fname << "\", M_CALL, seqn)" << nl;
    indent(f_client_) << "write_" << argsname << " op (" << argsname << "{";

    bool first = true;
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      string fieldname = (*fld_iter)->get_name();
      f_client_ << (first ? "" : ",");
      f_client_ << "f_" << argsname << "_" << fieldname << "=Just arg_" << fieldname;
      first = false;
    }
    f_client_ << "})" << nl;

    // Write to the stream
    indent(f_client_) << "writeMessageEnd op" << nl;
    indent(f_client_) << "tFlush (getTransport op)" << nl;
    indent_down();

    if (!(*f_iter)->is_oneway()) {
      string resultname = capitalize((*f_iter)->get_name() + "_result");
      t_struct noargs(program_);

      string funname = string("recv_") + (*f_iter)->get_name();
      t_function recv_function((*f_iter)->get_returntype(), funname, &noargs);

      // Open function
      indent(f_client_) << funname << " ip = do" << nl;
      indent_up();

      // TODO(mcslee): Validate message reply here, seq ids etc.
      indent(f_client_) << "(fname, mtype, rseqid) <- readMessageBegin ip" << nl;
      indent(f_client_) << "if mtype == M_EXCEPTION then do" << nl;
      indent(f_client_) << "  x <- readAppExn ip" << nl;
      indent(f_client_) << "  readMessageEnd ip" << nl;
      indent(f_client_) << "  throw x" << nl;
      indent(f_client_) << "  else return ()" << nl;

      t_struct* xs = (*f_iter)->get_xceptions();
      const vector<t_field*>& xceptions = xs->get_members();

      indent(f_client_) << "res <- read_" << resultname << " ip" << nl;
      indent(f_client_) << "readMessageEnd ip" << nl;

      // Careful, only return _result if not a void function
      if (!(*f_iter)->get_returntype()->is_void()) {
        indent(f_client_) << "case f_" << resultname << "_success res of" << nl;
        indent_up();

        indent(f_client_) << "Just v -> return v" << nl;
        indent(f_client_) << "Nothing -> do" << nl;
        indent_up();
      }

      vector<t_field*>::const_iterator x_iter;
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        string xname = (*x_iter)->get_name();
        indent(f_client_) << "case f_" << resultname << "_" << xname << " res of" << nl;
        indent_up();

        indent(f_client_) << "Nothing -> return ()" << nl;
        indent(f_client_) << "Just _v -> throw _v" << nl;
        indent_down();
      }

      // Careful, only return _result if not a void function
      if ((*f_iter)->get_returntype()->is_void()) {
        indent(f_client_) << "return ()" << nl;

      } else {
        string tname = (*f_iter)->get_name();
        indent(f_client_) << "throw (AppExn AE_MISSING_RESULT \"" << tname << " failed: unknown result\")" << nl;
        indent_down();
        indent_down();
      }

      // Close function
      indent_down();
    }
  }

  f_client_.close();
}

/**
 * Generates a service server definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_hs_generator::generate_service_server(t_service* tservice) {
  // Generate the dispatch methods
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  // Generate the process subfunctions
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter)
    generate_process_function(tservice, *f_iter);

  indent(f_service_) << "proc_ handler (iprot,oprot) (name,typ,seqid) = case name of" << nl;
  indent_up();

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    string fname = (*f_iter)->get_name();
    indent(f_service_) << "\"" << fname << "\" -> process_" << decapitalize(fname) << " (seqid,iprot,oprot,handler)" << nl;
  }

  indent(f_service_) << "_ -> ";
  if (tservice->get_extends() != nullptr) {
    f_service_ << type_name(tservice->get_extends()) << ".proc_ handler (iprot,oprot) (name,typ,seqid)" << nl;

  } else {
    f_service_ << "do" << nl;
    indent_up();
    indent(f_service_) << "skip iprot T_STRUCT" << nl;
    indent(f_service_) << "readMessageEnd iprot" << nl;
    indent(f_service_) << "writeMessageBegin oprot (name,M_EXCEPTION,seqid)" << nl;
    indent(f_service_) << "writeAppExn oprot (AppExn AE_UNKNOWN_METHOD (\"Unknown function \" ++ TL.unpack name))" << nl;
    indent(f_service_) << "writeMessageEnd oprot" << nl;
    indent(f_service_) << "tFlush (getTransport oprot)" << nl;
    indent_down();
  }

  indent_down();

  // Generate the server implementation
  indent(f_service_) << "process handler (iprot, oprot) = do" << nl;
  indent_up();

  indent(f_service_) << "(name, typ, seqid) <- readMessageBegin iprot" << nl;
  indent(f_service_) << "proc_ handler (iprot,oprot) (name,typ,seqid)" << nl;
  indent(f_service_) << "return True" << nl;
  indent_down();
}

bool hasNoArguments(t_function* func) {
    return (func->get_arglist()->get_members().empty());
}

string t_hs_generator::render_hs_type_for_function_name(t_type* type) {
    string type_str = render_hs_type(type, false);
    int found = -1;

    while (true) {
        found = type_str.find_first_of("[]. ", found + 1);
        if (string::npos == size_t(found)) {
            break;
        }

        if (type_str[found] == '.')
            type_str[found] = '_';
        else
            type_str[found] = 'Z';
    }
    return type_str;
}

/**
 * Generates a fuzzer for a service.
 *
 * @param tservice The service to generate a fuzzer for.
 */
void t_hs_generator::generate_service_fuzzer(t_service *tservice) {
    string f_service_name = get_out_dir()+capitalize(service_name_)+"_Fuzzer.hs";
    f_service_fuzzer_.open(f_service_name.c_str());
    record_genfile(f_service_name);

    // Generate module declaration
    f_service_fuzzer_ <<
        //"{-# LANGUAGE ScopedTypeVariables, DeriveDataTypeable #-}" << nl <<
        hs_language_pragma() << nl <<
        hs_autogen_comment() << nl <<
        "module " << capitalize(service_name_) << "_Fuzzer (main) where" << nl;

    // Generate imports specific to the .thrift file.
    f_service_fuzzer_ <<
        "import " << capitalize(program_name_) << "_Types" << nl <<
        "import qualified " << capitalize(service_name_) << "_Client as Client" << nl;

    const vector<t_program*>& includes = program_->get_includes();
    for (size_t i = 0; i < includes.size(); ++i) {
        f_service_fuzzer_ << "import " + capitalize(includes[i]->get_name())
                          << "_Types\n";
    }

    // Generate non-specific body code
    f_service_fuzzer_
        << nl << hs_imports()
              << "import Prelude ((>>), print)"
        << nl << "import qualified Prelude as P"
        << nl << "import Control.Monad (forM)"
        << nl << "import qualified Data.List as L"
        << nl << "import Data.Maybe (fromJust)"
        << nl << "import qualified Data.Map as Map"
        << nl << "import GHC.Int (Int64, Int32)"
        << nl << "import Data.ByteString.Lazy (ByteString)"
        << nl << "import System.Environment (getArgs)"
        << nl << "import Test.QuickCheck (arbitrary)"
        << nl << "import Test.QuickCheck.Gen (Gen(..))"
        << nl << "import Thrift.FuzzerSupport"
        << nl << ""
        << nl << ""
        << nl << "handleOptions :: ([Options -> Options], [String], [String]) -> Options"
        << nl << "handleOptions (transformers, (serviceName:[]), []) | serviceName `P.elem` serviceNames"
        << nl << "    = (P.foldl (P.flip ($)) defaultOptions transformers) { opt_service = serviceName } "
        << nl << "handleOptions (_, (serviceName:[]), []) | P.otherwise"
        << nl << "    = P.error $ usage ++ \"\\nUnknown serviceName \" ++ serviceName ++ \", should be one of \" ++ (P.show serviceNames)"
        << nl << "handleOptions (_, [], _) = P.error $ usage ++ \"\\nMissing mandatory serviceName to fuzz.\""
        << nl << "handleOptions (_, _a, []) = P.error $ usage ++ \"\\nToo many serviceNames, pick one.\""
        << nl << "handleOptions (_, _, e) = P.error $ usage ++ (P.show e)"
        << nl << ""
        << nl << "main :: IO ()"
        << nl << "main = do"
        << nl << "    args <- getArgs"
        << nl << "    let config = handleOptions (getOptions args)"
        << nl << "    fuzz config"
        << nl << ""
        << nl << "selectFuzzer :: Options -> (Options -> IO ())"
        << nl << "selectFuzzer (Options _host _port service _timeout _framed _verbose) "
        << nl << "    = fromJust $ P.lookup service fuzzerFunctions"
        << nl << ""
        << nl << "fuzz :: Options -> IO ()"
        << nl << "fuzz config = (selectFuzzer config) config"
        << nl << ""
        << nl << "-- Dynamic content"
        << nl << ""
        << nl << "-- Configuration via command-line parsing";

    // Generate service methods list and method->fuzzer mappings

    // We'll only generate fuzzers for functions that take arguments.
    vector<t_function*> functions = tservice->get_functions();
    vector<t_function*>::const_iterator functions_end;
    functions_end = remove_if(functions.begin(), functions.end(), hasNoArguments);
    vector<t_function*>::const_iterator f_iter;

    // service methods list
    f_service_fuzzer_ << nl
        << nl << "serviceNames :: [String]"
        << nl << "serviceNames = [";

    bool first = true;
    for (f_iter = functions.begin(); f_iter != functions_end; ++f_iter) {
        auto funname = decapitalize((*f_iter)->get_name());
        if (first) {
            first = false;
        } else {
            f_service_fuzzer_ << ", ";
        }
        f_service_fuzzer_ << "\"" << funname << "\"";
    }
    f_service_fuzzer_ << "]" << nl;

    // map from method names to fuzzer functions
    f_service_fuzzer_
        << nl << "fuzzerFunctions :: [(String, (Options -> IO ()))]"
        << nl << "fuzzerFunctions = [";
    first = true;
    for (f_iter = functions.begin(); f_iter != functions_end; ++f_iter) {
        auto funname = decapitalize((*f_iter)->get_name());
        if (first) {
            first = false;
        } else {
            f_service_fuzzer_ << ", ";
        }
        f_service_fuzzer_
            << "(" << "\"" << funname << "\""
            << ", " << funname << "_fuzzer" << ")";
    }

    f_service_fuzzer_ << "]" << nl;

    // Generate data generators for each data type used in any service method
    f_service_fuzzer_ << nl << "-- Random data generation" << nl;

    // Generate the set of parameter types used in any function
    // Use the t_type_fingerprint_less to to compare pointers to
    // t_type objects using their fingerprints.
    set<t_type*,t_type_fingerprint_less> used_types;
    for (f_iter = functions.begin(); f_iter != functions_end; ++f_iter) {
        const vector<t_field*>& fields = (*f_iter)->get_arglist()->get_members();
        vector<t_field*>::const_iterator fld_iter;
        for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
            used_types.insert((*fld_iter)->get_type());
        }
    }

    // all the data generators we need
    set<t_type*>::const_iterator type_iter;
    for (type_iter = used_types.begin(); type_iter != used_types.end(); ++type_iter) {
        const string& inf_type = "inf_" + render_hs_type_for_function_name(*type_iter);
        const string& hs_type = render_hs_type(*type_iter, true);

        f_service_fuzzer_
            << inf_type
            << " :: IO [" << hs_type << "]" << nl
            << inf_type
            << " = infexamples (arbitrary :: Gen " << hs_type << ")"
            << nl << nl;
    }

    // For each service method that has arguments, generate the
    //   exception handler and fuzzer functions

    f_service_fuzzer_ << "-- Fuzzers and exception handlers" << nl;

    for (f_iter = functions.begin(); f_iter != functions_end; ++f_iter) {
       auto funname = decapitalize((*f_iter)->get_name());
        // fuzzer signature
       f_service_fuzzer_ << funname << "_fuzzer :: Options -> IO ()" << nl;
       // function
       f_service_fuzzer_ << funname << "_fuzzer opts = do" << nl;
       indent_up();
       const vector<t_field*>& fields = (*f_iter)->get_arglist()->get_members();
       vector<t_field*>::const_iterator fld_iter;
       char var = 'a';
       // random data sources
       for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
           indent(f_service_fuzzer_)
               << var
               << " <- "
               << "inf_"
               << render_hs_type_for_function_name((*fld_iter)->get_type())
               << nl;
           var++;
       }
       // fuzzer invocation
       indent(f_service_fuzzer_) << "_ <- forM ";
       int argCount = fields.size();
//       assert (1 <= argCount);
       if (argCount == 1) {
           f_service_fuzzer_ << "a ";
       } else {
           f_service_fuzzer_ << "(L.zip";
           if(argCount != 2) {
               f_service_fuzzer_ << char('0' + argCount);
           }
           for (var = 'a'; var < 'a' + argCount; var++) {
               f_service_fuzzer_ << " " << var;
           }
           f_service_fuzzer_ << ") ";
       }
       f_service_fuzzer_
           << "$ \\param -> if opt_framed opts" << nl << indent() << indent()
           << "then withThriftDo opts (withFramedTransport opts) ("
             << funname << "_fuzzOnce param) ("
             << funname << "_exceptionHandler param)" << nl << indent() << indent()
           << "else withThriftDo opts (withHandle opts) ("
             << funname << "_fuzzOnce param) ("
             << funname << "_exceptionHandler param)" << nl << indent()
           << "return ()" << nl;
       indent_down();

       // exception handler
       f_service_fuzzer_ << nl
           << funname << "_exceptionHandler :: Show a => a -> IO ()" << nl
           << funname << "_exceptionHandler a = do" << nl;
       indent_up();
       indent(f_service_fuzzer_)
           << "P.putStrLn $ \"Got exception on data:\"" << nl << indent()
           << "print a" << nl;
       indent_down();

       // Thrift invoker
       f_service_fuzzer_
           << nl
           << funname << "_fuzzOnce (";

       first = true;
       for (var = 'a'; var < 'a' + argCount; var++) {
           if (first) {
               first = false;
           } else {
               f_service_fuzzer_ << ", ";
           }
           f_service_fuzzer_ << var;
       }
       f_service_fuzzer_ << ") client = Client." << funname << " client ";
       for (var = 'a'; var < 'a' + argCount; var++) {
           f_service_fuzzer_ << var << " ";
       }
       f_service_fuzzer_ << " >> return ()";

       f_service_fuzzer_ << nl << nl;

    }

    f_service_fuzzer_.close();

}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_hs_generator::generate_process_function(t_service* tservice,
                                               t_function* tfunction) {
  (void) tservice;
  // Open function
  string funname = decapitalize(tfunction->get_name());
  indent(f_service_) << "process_" << funname << " (seqid, iprot, oprot, handler) = do" << nl;
  indent_up();

  string argsname = capitalize(tfunction->get_name()) + "_args";
  string resultname = capitalize(tfunction->get_name()) + "_result";

  // Generate the function call
  t_struct* arg_struct = tfunction->get_arglist();
  const vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator f_iter;

  indent(f_service_) << "args <- read_" << argsname << " iprot" << nl;
  indent(f_service_) << "readMessageEnd iprot" << nl;

  t_struct* xs = tfunction->get_xceptions();
  const vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;

  size_t n = xceptions.size();
  if (!tfunction->is_oneway()) {
    if (!tfunction->get_returntype()->is_void())
      n++;

    indent(f_service_) << "rs <- return (" << resultname;

    for(size_t i = 0; i < n; i++)
      f_service_ << " Nothing";

    f_service_ << ")" << nl;
  }

  indent(f_service_) << "res <- ";
  // Try block for a function with exceptions
  if (xceptions.size() > 0) {
    for(size_t i = 0; i < xceptions.size(); i++) {
      f_service_ << "(Control.Exception.catch" << nl;
      indent_up();
      indent(f_service_);
    }
  }

  f_service_ << "(do" << nl;
  indent_up();
  indent(f_service_);

  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void())
    f_service_ << "res <- ";

  f_service_ << "Iface." << decapitalize(tfunction->get_name()) << " handler";
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter)
    f_service_ <<  " (f_" << argsname <<  "_" << (*f_iter)->get_name() << " args)";

  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
    f_service_ << nl;
    indent(f_service_) << "return rs{f_" << resultname << "_success= Just res}";

  } else if (!tfunction->is_oneway()) {
    f_service_ << nl;
    indent(f_service_) << "return rs";
  }

  f_service_ << ")" << nl;
  indent_down();

  if (xceptions.size() > 0 && !tfunction->is_oneway()) {
    for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
      indent(f_service_) << "(\\e  -> " << nl;
      indent_up();

      if (!tfunction->is_oneway()) {
        indent(f_service_) << "return rs{f_" << resultname << "_" << (*x_iter)->get_name() << " =Just e}";

      } else {
        indent(f_service_) << "return ()";
      }

      f_service_ << "))" << nl;
      indent_down();
      indent_down();
    }
  }

  // Shortcut out here for oneway functions
  if (tfunction->is_oneway()) {
    indent(f_service_) << "return ()" << nl;
    indent_down();
    return;
  }

  indent(f_service_ ) << "writeMessageBegin oprot (\"" << tfunction->get_name() << "\", M_REPLY, seqid);" << nl;
  indent(f_service_ ) << "write_" << resultname << " oprot res" << nl;
  indent(f_service_ ) << "writeMessageEnd oprot" << nl;
  indent(f_service_ ) << "tFlush (getTransport oprot)" << nl;

  // Close function
  indent_down();
}

/**
 * Deserializes a field of any type.
 */
void t_hs_generator::generate_deserialize_field(ofstream &out,
                                                t_field* tfield,
                                                string prefix) {
  (void) prefix;
  t_type* type = tfield->get_type();
  generate_deserialize_type(out,type);
}

/**
 * Deserializes a field of any type.
 */
void t_hs_generator::generate_deserialize_type(ofstream &out,
                                               t_type* type) {
  type = get_true_type(type);

  if (type->is_void())
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE";

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out, (t_struct*)type);

  } else if (type->is_container()) {
    generate_deserialize_container(out, type);

  } else if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();

    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "compiler error: cannot serialize void field in a struct";
      break;
    case t_base_type::TYPE_STRING:
      out << (((t_base_type*)type)->is_binary() ? "readBinary" : "readString");
      break;
    case t_base_type::TYPE_BOOL:
      out << "readBool";
      break;
    case t_base_type::TYPE_BYTE:
      out << "readByte";
      break;
    case t_base_type::TYPE_I16:
      out << "readI16";
      break;
    case t_base_type::TYPE_I32:
      out << "readI32";
      break;
    case t_base_type::TYPE_I64:
      out << "readI64";
      break;
    case t_base_type::TYPE_FLOAT:
      out << "readFloat";
      break;
    case t_base_type::TYPE_DOUBLE:
      out << "readDouble";
      break;
    default:
      throw "compiler error: no PHP name for base type " + t_base_type::t_base_name(tbase);
    }
    out << " iprot";

  } else if (type->is_enum()) {
    string ename = capitalize(type->get_name());
    out << "(do {i <- readI32 iprot; return $ toEnum $ fromIntegral i})";

  } else {
    printf("DO NOT KNOW HOW TO DESERIALIZE TYPE '%s'\n",
           type->get_name().c_str());
  }
}


/**
 * Generates an unserializer for a struct, calling read()
 */
void t_hs_generator::generate_deserialize_struct(ofstream &out,
                                                 t_struct* tstruct) {
  out << "(" << type_name(tstruct, "read_") << " iprot)";
}

/**
 * Serialize a container by writing out the header followed by
 * data and then a footer.
 */
void t_hs_generator::generate_deserialize_container(ofstream &out,
                                                    t_type* ttype) {
  string size = tmp("_size");
  string ktype = tmp("_ktype");
  string vtype = tmp("_vtype");
  string etype = tmp("_etype");
  string con = tmp("_con");

  t_field fsize(g_type_i32, size);
  t_field fktype(g_type_byte, ktype);
  t_field fvtype(g_type_byte, vtype);
  t_field fetype(g_type_byte, etype);

  // Declare variables, read header
  if (ttype->is_map()) {
    out << "(let {f 0 = return []; f n = do {k <- ";
    generate_deserialize_type(out,((t_map*)ttype)->get_key_type());

    out << "; v <- ";
    generate_deserialize_type(out,((t_map*)ttype)->get_val_type());

    out << ";r <- f (n-1); return $ (k,v):r}} in do {(" << ktype << "," << vtype << "," << size << ") <- readMapBegin iprot; l <- f " << size << "; return $ Map.fromList l})";

  } else if (ttype->is_set()) {
    out << "(let {f 0 = return []; f n = do {v <- ";
    generate_deserialize_type(out,((t_map*)ttype)->get_key_type());
    out << ";r <- f (n-1); return $ v:r}} in do {(" << etype << "," << size << ") <- readSetBegin iprot; l <- f " << size << "; return $ Set.fromList l})";

  } else if (ttype->is_list()) {
    out << "(let f n = Vector.replicateM (fromIntegral n) (";
    generate_deserialize_type(out,((t_map*)ttype)->get_key_type());
    out << ") in do {(" << etype << "," << size << ") <- readListBegin iprot; f " << size << "})";
  }
}

/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_hs_generator::generate_serialize_field(ofstream &out,
                                              t_field* tfield,
                                              string name) {
  t_type* type = get_true_type(tfield->get_type());

  // Do nothing for void types
  if (type->is_void())
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " + tfield->get_name();

  if (name.length() == 0)
    name = decapitalize(tfield->get_name());

  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct(out, (t_struct*)type, name);

  } else if (type->is_container()) {
    generate_serialize_container(out, type, name);

  } else if (type->is_base_type() || type->is_enum()) {
    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw
          "compiler error: cannot serialize void field in a struct: " + name;
        break;

      case t_base_type::TYPE_STRING:
        out << (((t_base_type*)type)->is_binary() ? "writeBinary" : "writeString") << " oprot " << name;
        break;

      case t_base_type::TYPE_BOOL:
        out << "writeBool oprot " << name;
       break;

      case t_base_type::TYPE_BYTE:
        out << "writeByte oprot " << name;
        break;

      case t_base_type::TYPE_I16:
        out << "writeI16 oprot " << name;
        break;

      case t_base_type::TYPE_I32:
        out << "writeI32 oprot " << name;
        break;

      case t_base_type::TYPE_I64:
        out << "writeI64 oprot " << name;
        break;

      case t_base_type::TYPE_FLOAT:
        out << "writeFloat oprot " << name;
        break;

      case t_base_type::TYPE_DOUBLE:
        out << "writeDouble oprot " << name;
        break;

      default:
        throw "compiler error: no hs name for base type " + t_base_type::t_base_name(tbase);
      }

    } else if (type->is_enum()) {
      string ename = capitalize(type->get_name());
      out << "writeI32 oprot (fromIntegral $ fromEnum " << name << ")";
    }

  } else {
    printf("DO NOT KNOW HOW TO SERIALIZE FIELD '%s' TYPE '%s'\n",
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
void t_hs_generator::generate_serialize_struct(ofstream &out,
                                               t_struct* tstruct,
                                               string prefix) {
  out << type_name(tstruct, "write_") << " oprot " << prefix;
}

void t_hs_generator::generate_serialize_container(ofstream &out,
                                                  t_type* ttype,
                                                  string prefix) {
  if (ttype->is_map()) {
    string k = tmp("_kiter");
    string v = tmp("_viter");
    out << "(let {f [] = return (); f ((" << k << "," << v << "):t) = do {";
    generate_serialize_map_element(out, (t_map*)ttype, k, v);
    out << ";f t}} in do {writeMapBegin oprot (" << type_to_enum(((t_map*)ttype)->get_key_type()) << "," << type_to_enum(((t_map*)ttype)->get_val_type()) << ",fromIntegral $ Map.size " << prefix << "); f (Map.toList " << prefix << ");writeMapEnd oprot})";

  } else if (ttype->is_set()) {
    string v = tmp("_viter");
    out << "(let {f [] = return (); f (" << v << ":t) = do {";
    generate_serialize_set_element(out, (t_set*)ttype, v);
    out << ";f t}} in do {writeSetBegin oprot (" << type_to_enum(((t_set*)ttype)->get_elem_type()) << ",fromIntegral $ Set.size " << prefix << "); f (Set.toList " << prefix << ");writeSetEnd oprot})";

  } else if (ttype->is_list()) {
    string v = tmp("_viter");
    out << "(let f = Vector.mapM_ (\\" << v << " -> ";
    generate_serialize_list_element(out, (t_list*)ttype, v);
    out << ") in do {writeListBegin oprot (" << type_to_enum(((t_list*)ttype)->get_elem_type()) << ",fromIntegral $ Vector.length " << prefix << "); f " << prefix << ";writeListEnd oprot})";
  }

}

/**
 * Serializes the members of a map.
 *
 */
void t_hs_generator::generate_serialize_map_element(ofstream &out,
                                                    t_map* tmap,
                                                    string kiter,
                                                    string viter) {
  t_field kfield(tmap->get_key_type(), kiter);
  out << "do {";
  generate_serialize_field(out, &kfield);
  out << ";";

  t_field vfield(tmap->get_val_type(), viter);
  generate_serialize_field(out, &vfield);
  out << "}";
}

/**
 * Serializes the members of a set.
 */
void t_hs_generator::generate_serialize_set_element(ofstream &out,
                                                    t_set* tset,
                                                    string iter) {
  t_field efield(tset->get_elem_type(), iter);
  generate_serialize_field(out, &efield);
}

/**
 * Serializes the members of a list.
 */
void t_hs_generator::generate_serialize_list_element(ofstream &out,
                                                     t_list* tlist,
                                                     string iter) {
  t_field efield(tlist->get_elem_type(), iter);
  generate_serialize_field(out, &efield);
}

string t_hs_generator::function_type(t_function* tfunc, bool options, bool io, bool method) {
  string result = "";

  const vector<t_field*>& fields = tfunc->get_arglist()->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (options) result += "Maybe ";
    result += render_hs_type((*f_iter)->get_type(), options);
    result += " -> ";
  }

  if (fields.empty() && !method)
    result += "() -> ";

  if (io)
    result += "IO ";

  result += render_hs_type(tfunc->get_returntype(), io);
  return result;
}


string t_hs_generator::type_name(t_type* ttype, string function_prefix) {
  string prefix = "";
  t_program* program = ttype->get_program();

  if (program != nullptr && program != program_)
    if (!ttype->is_service())
      prefix = capitalize(program->get_name()) + "_Types.";

  return prefix + function_prefix + capitalize(ttype->get_name());
}

/**
 * Converts the parse type to a Protocol.t_type enum
 */
string t_hs_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:   return "T_VOID";
    case t_base_type::TYPE_STRING: return "T_STRING";
    case t_base_type::TYPE_BOOL:   return "T_BOOL";
    case t_base_type::TYPE_BYTE:   return "T_BYTE";
    case t_base_type::TYPE_I16:    return "T_I16";
    case t_base_type::TYPE_I32:    return "T_I32";
    case t_base_type::TYPE_I64:    return "T_I64";
    case t_base_type::TYPE_FLOAT:  return "T_FLOAT";
    case t_base_type::TYPE_DOUBLE: return "T_DOUBLE";
    }

  } else if (type->is_enum()) {
    return "T_I32";

  } else if (type->is_struct() || type->is_xception()) {
    return "T_STRUCT";

  } else if (type->is_map()) {
    return "T_MAP";

  } else if (type->is_set()) {
    return "T_SET";

  } else if (type->is_list()) {
    return "T_LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

/**
 * Converts the parse type to an haskell type
 */
string t_hs_generator::render_hs_type(t_type* type, bool needs_parens) {
  type = get_true_type(type);
  string type_repr;

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:   return "()";
    case t_base_type::TYPE_STRING: return (((t_base_type*)type)->is_binary() ? "ByteString" : "Text");
    case t_base_type::TYPE_BOOL:   return "Bool";
    case t_base_type::TYPE_BYTE:   return "Int8";
    case t_base_type::TYPE_I16:    return "Int16";
    case t_base_type::TYPE_I32:    return "Int32";
    case t_base_type::TYPE_I64:    return "Int64";
    case t_base_type::TYPE_FLOAT:  return "Float";
    case t_base_type::TYPE_DOUBLE: return "Double";
    }

  } else if (type->is_enum()) {
    return capitalize(((t_enum*)type)->get_name());

  } else if (type->is_struct() || type->is_xception()) {
    return type_name((t_struct*)type);

  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    type_repr = "Map.HashMap " + render_hs_type(ktype, true) + " " + render_hs_type(vtype, true);

  } else if (type->is_set()) {
    t_type* etype = ((t_set*)type)->get_elem_type();
    type_repr = "Set.HashSet " + render_hs_type(etype, true) ;

  } else if (type->is_list()) {
    t_type* etype = ((t_list*)type)->get_elem_type();
    type_repr = "Vector.Vector " + render_hs_type(etype, true);

  } else {
    throw "INVALID TYPE IN type_to_enum: " + type->get_name();
  }

  return needs_parens ? "(" + type_repr + ")" : type_repr;
}

THRIFT_REGISTER_GENERATOR(hs, "Haskell", "")
