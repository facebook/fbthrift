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

#include <cctype>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>

#include <folly/Format.h>
#include <folly/gen/Base.h>
#include <folly/gen/String.h>

#include <thrift/compiler/generate/t_oop_generator.h>
#include <thrift/compiler/platform.h>

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

    // Set option flags based on parsed options
    gen_haddock_ = parsed_options.find("enable_haddock") !=
      parsed_options.end();
    use_list_ = parsed_options.find("use_list") != parsed_options.end();
    use_string_ = parsed_options.find("use_string") != parsed_options.end();
    use_strict_text_ =
      parsed_options.find("use_strict_text") != parsed_options.end();
    use_int_ = parsed_options.find("use_int") != parsed_options.end();

  }

  /**
   * Option Flags
   */

  bool gen_haddock_;
  bool use_list_;
  bool use_string_;
  bool use_strict_text_;
  bool use_int_;

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

  string render_const_value(t_type* type, const t_const_value* value);

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

  void generate_hs_typemap           (ofstream& out,
                                      t_struct* tstruct);

  void generate_hs_default           (ofstream& out,
                                      t_struct* tstruct);


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
                                          t_struct* tstruct,
                                          string name = "");

  void generate_deserialize_container    (ofstream &out,
                                          t_type* ttype,
                                          string arg = "");

  void generate_deserialize_set_element  (ofstream &out,
                                          t_set* tset);


  void generate_deserialize_list_element (ofstream &out,
                                          t_list* tlist,
                                          string prefix = "");

  void generate_deserialize_type          (ofstream &out,
                                           t_type* type,
                                           string arg = "");

  void generate_serialize_type           (ofstream &out,
                                          t_type* type,
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

  string qualified_type_name(t_type* ttype,
                   string function_prefix = "");

  string unqualified_type_name(t_type* ttype,
                               string function_prefix = "");

  string type_name_qualifier(t_type* ttype);

  string field_name(string tname, string fname);

  string function_type(t_function* tfunc,
                       bool options = false,
                       bool io = false,
                       bool method = false);

  string type_to_enum(t_type* ttype);

  string type_to_default(t_type* ttype);

  string render_hs_type(t_type* type,
                        bool needs_parens);

  string type_to_constructor(t_type* ttype);

  string render_hs_type_for_function_name(t_type *type);

  string module_part(const string& qualified_name);
  string name_part(const string& qualified_name);

  const string& get_package_dir() const;
  string get_module_prefix(const t_program* program) const;

 private:

  ofstream f_types_;
  ofstream f_consts_;
  ofstream f_service_;
  ofstream f_iface_;
  ofstream f_client_;
  ofstream f_service_fuzzer_;

  string package_dir_;
};

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_hs_generator::init_generator() {
  // Make output directory
  package_dir_ = get_out_dir();
  MKDIR(package_dir_.c_str());

  string hs_namespace = program_->get_namespace("hs");
  if (!hs_namespace.empty()) {
    vector<string> components;
    folly::split(".", hs_namespace, components, false /* ignoreEmpty */);
    for (const auto& component : components) {
      if (component.empty() || !std::isupper(component[0])) {
        throw "compiler error: Invalid Haskell Module " + hs_namespace;
      }
      package_dir_ += component;
      package_dir_.push_back('/');
      MKDIR(package_dir_.c_str());
    }
  }

  // Make output file
  string pname = capitalize(program_name_);
  string f_types_name = get_package_dir() + pname + "_Types.hs";
  f_types_.open(f_types_name.c_str());
  record_genfile(f_types_name);

  string f_consts_name = get_package_dir() + pname + "_Consts.hs";
  f_consts_.open(f_consts_name.c_str());
  record_genfile(f_consts_name);

  // Print header
  string module_prefix = get_module_prefix(program_);
  f_types_ << hs_language_pragma() << nl;
  f_types_ << hs_autogen_comment() << nl;
  f_types_ << "module " << module_prefix << pname << "_Types where" << nl;
  f_types_ << hs_imports() << nl;

  f_consts_ << hs_language_pragma() << nl;
  f_consts_ << hs_autogen_comment() << nl;
  f_consts_ << "module " << module_prefix << pname << "_Consts where" << nl;
  f_consts_ << hs_imports() << nl;
  f_consts_ << "import qualified " << module_prefix << pname << "_Types" << nl;
}

string t_hs_generator::hs_language_pragma() {

  return string("{-# LANGUAGE DeriveDataTypeable #-}\n"
                "{-# LANGUAGE OverloadedStrings #-}\n"
                "{-# OPTIONS_GHC -fno-warn-missing-fields #-}\n"
                "{-# OPTIONS_GHC -fno-warn-missing-signatures #-}\n"
                "{-# OPTIONS_GHC -fno-warn-name-shadowing #-}\n"
                "{-# OPTIONS_GHC -fno-warn-unused-imports #-}\n"
                "{-# OPTIONS_GHC -fno-warn-unused-matches #-}\n")
    + (use_int_ ? string(
        "{-# LANGUAGE CPP #-}\n"
        "#include \"MachDeps.h\"\n"
        "#if SIZEOF_HSINT < 8\n"
        "#error Can't use the 'use_int' flag on <64-bit architectures\n"
        "#endif\n")
        : "");
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
      "import Prelude ( Bool(..), Enum, Float, IO, Double, String, Maybe(..),\n"
      "                 Eq, Show, Ord,\n"
      "                 concat, error, fromIntegral, fromEnum, length, map,\n"
      "                 maybe, not, null, otherwise, return, show, toEnum,\n"
      "                 enumFromTo, Bounded, minBound, maxBound, seq, succ,\n"
      "                 pred, enumFrom, enumFromThen, enumFromThenTo,\n"
      "                 (.), (&&), (||), (==), (++), ($), (-), (>>=), (>>))\n"
      "\n"
      "import qualified Control.Applicative as Applicative (ZipList(..))\n"
      "import Control.Applicative ( (<*>) )\n"
      "import qualified Control.DeepSeq as DeepSeq\n"
      "import qualified Control.Exception as Exception\n"
      "import qualified Control.Monad as Monad ( liftM, ap, when )\n"
      "import qualified Data.ByteString.Lazy as BS\n"
      "import Data.Functor ( (<$>) )\n"
      "import qualified Data.Hashable as Hashable\n"
      "import qualified Data.Int as Int\n"
      "import Data.List\n"
      "import qualified Data.Maybe as Maybe (catMaybes)\n"
      "import qualified Data.Text.Lazy.Encoding as "
        "Encoding ( decodeUtf8, encodeUtf8 )\n"
      "import qualified Data.Text.Lazy as LT\n"
      "import qualified Data.Typeable as Typeable ( Typeable )\n"
      "import qualified Data.HashMap.Strict as Map\n"
      "import qualified Data.HashSet as Set\n"
      "import qualified Data.Vector as Vector\n"
      "import qualified Test.QuickCheck.Arbitrary as "
        "Arbitrary ( Arbitrary(..) )\n"
      "import qualified Test.QuickCheck as QuickCheck ( elements )\n"
      "\n"
      "import qualified Thrift\n"
      "import qualified Thrift.Types as Types\n"
      "import qualified Thrift.Serializable as Serializable\n"
      "import qualified Thrift.Arbitraries as Arbitraries\n"
      "\n");

  if (use_strict_text_)
    result += "import qualified Data.Text as T\n\n";

  for (const auto& program_include : includes) {
    auto module_prefix = get_module_prefix(program_include);
    auto base_name = capitalize(program_include->get_name()) + "_Types";
    result += folly::to<string>(
      "import qualified ", module_prefix, base_name, " as ", base_name, "\n");
  }

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
  indent(f_types_)
    << "deriving (Show,Eq, Typeable.Typeable, Ord, Bounded)" << nl;
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
  indent(f_types_)
    << "_ -> Exception.throw Thrift.ThriftException" << nl;
  indent_down();
  indent(f_types_) << "succ t = case t of" << nl;
  indent_up();
  for(c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    string name = capitalize((*c_iter)->get_name());
    auto succ = c_iter + 1;
    string succ_value = succ == constants.end()
      ? string("Exception.throw Thrift.ThriftException")
      : capitalize((*succ)->get_name());
    indent(f_types_) << name << " -> " << succ_value << nl;
  }
  indent_down();
  indent(f_types_) << "pred t = case t of" << nl;
  indent_up();
  for(c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    string name = capitalize((*c_iter)->get_name());
    string succ_value = c_iter == constants.begin()
      ? string("Exception.throw Thrift.ThriftException")
      : capitalize((*(c_iter - 1))->get_name());
    indent(f_types_) << name << " -> " << succ_value << nl;
  }
  indent_down();
  indent(f_types_) << "enumFrom x = enumFromTo x maxBound" << nl;
  indent(f_types_) << "enumFromTo x y = takeUpToInc y $ iterate succ x" << nl;
  indent_up();
  indent(f_types_) << "where" << nl;
  indent(f_types_) << "takeUpToInc _ [] = []" << nl;
  indent(f_types_) << "takeUpToInc m (x:_) | m == x = [x]" << nl;
  indent(f_types_) << "takeUpToInc m (x:xs) | otherwise = "
                   << "x : takeUpToInc m xs" << nl;
  indent_down();
  indent(f_types_) << "enumFromThen _ _ = "
                   << "Exception.throw Thrift.ThriftException" << nl;
  indent(f_types_) << "enumFromThenTo _ _ _ = "
                   << "Exception.throw Thrift.ThriftException" << nl;
  indent_down();

  indent(f_types_) << "instance Hashable.Hashable " << ename
                   << " where" << nl;
  indent_up();
  indent(f_types_) << "hashWithSalt salt = Hashable.hashWithSalt salt . "
                   << "fromEnum" << nl;
  indent_down();

  indent(f_types_) << "instance DeepSeq.NFData " << ename
                   << " where" << nl;
  indent_up();
  indent(f_types_) << "rnf x = x `seq` ()" << nl;
  indent_down();

  indent(f_types_)
    << "instance Arbitrary.Arbitrary " << ename
    << " where" << nl;
  indent_up();
  indent(f_types_)
    << "arbitrary = QuickCheck.elements (enumFromTo minBound maxBound)"
    << nl;
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
string t_hs_generator::render_const_value(
    t_type* type,
    const t_const_value* value) {
  if (value == nullptr)
    return type_to_default(type);

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
    case t_base_type::TYPE_I16:
    case t_base_type::TYPE_I32:
    case t_base_type::TYPE_I64:
      out << value->get_integer();
      break;

    case t_base_type::TYPE_FLOAT:
    case t_base_type::TYPE_DOUBLE:
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
    t_enum* tenum = (t_enum*)type;
    vector<t_enum_value*> constants = tenum->get_constants();
    vector<t_enum_value*>::iterator c_iter;
    for (auto& c_iter : constants) {
      int val = c_iter->get_value();
      if (val == value->get_integer()) {
        const t_program* prog = type->get_program();
        if (prog != nullptr)
          out << capitalize(prog->get_name()) << "_Types.";
        out << capitalize(c_iter->get_name());
        break;
      }
    }

  } else if (type->is_struct() || type->is_xception()) {
    string cname = qualified_type_name(type);
    out << module_part(cname) << "default_" << name_part(cname) << "{";

    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;

    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;

    bool first = true;
    for (auto& v_iter : val) {
      t_field* field = nullptr;

      for (auto& f_iter : fields)
        if (f_iter->get_name() == v_iter.first->get_string())
          field = f_iter;

      if (field == nullptr)
        throw "type error: " + cname + " has no field " + v_iter.first->get_string();

      string fname = v_iter.first->get_string();
      string const_value = render_const_value(field->get_type(), v_iter.second);

      out << (first ? "" : ", ");
      out << field_name(cname, fname) << " = ";
      if (field->get_req() == t_field::T_OPTIONAL ||
          ((t_type*)field->get_type())->is_xception()) {
        out << "Just ";
      }
      out << const_value;
      first = false;
    }

    out << "}";

  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();

    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;

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
      out << "(" << (use_list_ ? "" : "Vector.fromList [");

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
  string tname = unqualified_type_name(tstruct);
  string name = tstruct->get_name();

  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  if (gen_haddock_)
    indent(out) << "-- | Definition of the " << tname << " struct" << nl;
  indent(out) << "data " << tname << " = " << tname << nl;

  if (members.size() > 0) {
    indent_up();
    bool first = true;
    for (auto* m_iter : members) {
      if (first) {
        indent(out) << "{ ";
        first = false;
      } else {
        indent(out) << ", ";
      }
      string mname = m_iter->get_name();
      out << field_name(tname, mname) << " :: ";
      if (m_iter->get_req() == t_field::T_OPTIONAL ||
          ((t_type*)m_iter->get_type())->is_xception()) {
        out << "Maybe ";
      }
      out << render_hs_type(m_iter->get_type(), true) << nl;
      if (gen_haddock_)
        indent(out) << "  -- ^ " << mname
                    << " field of the " << tname << " struct" << nl;
    }
    indent(out) << "}";
    indent_down();
  }

  out << " deriving (Show,Eq,Typeable.Typeable)" << nl;

  if (is_exception)
    out << "instance Exception.Exception " << tname << nl;

  indent(out) << "instance Serializable.ThriftSerializable "
              << tname << " where" << nl;
  indent_up();
  indent(out) << "encode = encode_" << tname << nl;
  indent(out) << "decode = decode_" << tname << nl;
  indent_down();

  indent(out) << "instance Hashable.Hashable " << tname << " where" << nl;
  indent_up();
  indent(out) << "hashWithSalt salt record = salt";
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    string mname = (*m_iter)->get_name();
    indent(out) << " `Hashable.hashWithSalt` " << field_name(tname, mname)
                << " record";
  }
  indent(out) << nl;
  indent_down();

  indent(out) << "instance DeepSeq.NFData " << tname << " where" << nl;
  indent_up();
  string record = tmp("_record");
  indent(out) << "rnf " << record << " =" << nl;
  indent_up();
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    string mname = (*m_iter)->get_name();
    indent(out) << "DeepSeq.rnf (" << field_name(tname, mname) << " "
                << record << ") `seq`" << nl;
  }
  indent(out) << "()" << nl;
  indent_down();
  indent_down();

  generate_hs_struct_arbitrary(out, tstruct);
  generate_hs_struct_writer(out, tstruct);
  generate_hs_struct_reader(out, tstruct);
  generate_hs_typemap(out, tstruct);
  generate_hs_default(out, tstruct);
}

void t_hs_generator::generate_hs_struct_arbitrary(ofstream& out, t_struct* tstruct) {
  string tname = unqualified_type_name(tstruct);
  string name = tstruct->get_name();
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;

  indent(out) << "instance Arbitrary.Arbitrary "
              <<tname<<" where "<<nl;
  indent_up();
  if (members.size() > 0) {
    indent(out) << "arbitrary = Monad.liftM " << tname;
    indent_up(); indent_up(); indent_up(); indent_up();
    bool first=true;
    for (auto* m_iter : members) {
      if(first) {
        first=false;
        out << " ";
      }
      else {
        indent(out) << "`Monad.ap`";
      }
      out << "(";
      if (m_iter->get_req() == t_field::T_OPTIONAL ||
          ((t_type*)m_iter->get_type())->is_xception()) {
        out << "Monad.liftM Just ";
      }
      out << "Arbitrary.arbitrary)" << nl;
    }
    indent_down(); indent_down(); indent_down(); indent_down();

    // Shrink
    indent(out) << "shrink obj | obj == default_" << tname << " = []" << nl;
    indent(out) << "           | otherwise = Maybe.catMaybes" << nl;
    indent_up();
    first = true;
    for (auto& m_iter : members) {
      if (first) {
        first = false;
        indent(out) << "[ ";
      } else {
        indent(out) << ", ";
      }
      string fname = field_name(tname, m_iter->get_name());
      out << "if obj == default_" << tname;
      out << "{" << fname << " = " << fname << " obj} ";
      out << "then Nothing ";
      out << "else Just $ default_" << tname;
      out << "{" << fname << " = " << fname << " obj}" << nl;
    }
    indent(out) << "]" << nl;
    indent_down();
  } else { /* 0 == members.size() */
     indent(out) << "arbitrary = QuickCheck.elements ["
                 << tname << "]" << nl;
  }
  indent_down();
}

/**
 * Generates the read method for a struct
 */
void t_hs_generator::generate_hs_struct_reader(ofstream& out, t_struct* tstruct) {
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  string sname = unqualified_type_name(tstruct);
  string id = tmp("_id");
  string val = tmp("_val");

  if (gen_haddock_) {
    indent(out) << "-- | Translate a 'Types.ThriftVal' to a '"
                << sname << "'" << nl;
  }
  indent(out) << "to_" << sname << " :: Types.ThriftVal -> "
              << sname << nl;
  indent(out) << "to_" << sname << " (Types.TStruct fields) = "
              << sname << "{" << nl;
  indent_up();

  bool first = true;

  // Generate deserialization code for known cases
  for (auto* f_iter : fields) {
    int32_t key = f_iter->get_key();
    string etype = type_to_enum(f_iter->get_type());
    string fname = f_iter->get_name();

    if (first) {
      first = false;
    } else {
      out << "," << nl;
    }

    // Fill in Field
    indent(out) << field_name(sname, fname) << " = ";

    out << "maybe (";
    if (f_iter->get_req() == t_field::T_REQUIRED) {
      out << "error \"Missing required field: " << fname << "\"";
    } else {
      if ((f_iter->get_req() == t_field::T_OPTIONAL ||
           ((t_type*)f_iter->get_type())->is_xception()) &&
          f_iter->get_value() == nullptr) {
        out << "Nothing";
      } else {
        out << field_name(sname, fname) << " default_" << sname;
      }
    }
    out << ") ";

    out << "(\\(_," << val << ") -> ";
    if (f_iter->get_req() == t_field::T_OPTIONAL ||
        ((t_type*)f_iter->get_type())->is_xception())
      out << "Just ";
    generate_deserialize_field(out, f_iter, val);
    out << ")";

    out << " (Map.lookup (" << key << ") fields)";
  }

  out << nl;
  indent(out) << "}" << nl;
  indent_down();

  // read
  string tmap = unqualified_type_name(tstruct, "typemap_");
  indent(out) << "to_" << sname << " _ = error \"not a struct\"" << nl;

  if (gen_haddock_)
    indent(out) << "-- | Read a '" << sname
                << "' struct with the given 'Thrift.Protocol'" << nl;
  indent(out) << "read_" << sname <<
    " :: (Thrift.Transport t, Thrift.Protocol p) => p t -> IO " << sname << nl;
  indent(out) << "read_" << sname << " iprot = to_" << sname;
  out << " <$> Thrift.readVal iprot (Types.T_STRUCT "
      << tmap << ")" << nl;

  // decode
  if (gen_haddock_)
    indent(out) << "-- | Deserialize a '" << sname << "' in pure code" << nl;
  indent(out) << "decode_" << sname
    << " :: (Thrift.Protocol p, Thrift.Transport t) => "
    << "p t -> BS.ByteString -> " << sname << nl;
  indent(out) << "decode_" << sname << " iprot bs = to_" << sname << " $ ";
  out << "Thrift.deserializeVal iprot (Types.T_STRUCT "
      << tmap << ") bs" << nl;
}

void t_hs_generator::generate_hs_struct_writer(ofstream& out,
                                               t_struct* tstruct) {
  string name = unqualified_type_name(tstruct);
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;
  string str = tmp("_str");
  string f = tmp("_f");
  string v = tmp("_v");

  if (gen_haddock_) {
    indent(out) << "-- | Translate a '" << name
                << "' to a 'Types.ThriftVal'" << nl;
  }
  indent(out) << "from_" << name << " :: "
              << name << " -> Types.ThriftVal" << nl;
  indent(out) << "from_" << name
              << " record = Types.TStruct $ Map.fromList ";
  indent_up();

  // Get Exceptions
  bool hasExn = false;
  for (auto* f_iter : fields) {
    if (((t_type*)f_iter->get_type())->is_xception()) {
      hasExn = true;
      break;
    }
  }

  bool isfirst = true;
  if (hasExn) {
    out << endl;
    indent(out) << "(let exns = Maybe.catMaybes ";
    indent_up();
    for (auto* f_iter : fields) {
      if (((t_type*)f_iter->get_type())->is_xception()) {
        if (isfirst) {
          out << "[ ";
          isfirst = false;
        } else {
          out << ", ";
        }
        string mname = f_iter->get_name();
        int32_t key = f_iter->get_key();
        out << "(\\" << v << " -> (" << key << ", (\"" << mname << "\",";
        generate_serialize_type(out, f_iter->get_type(), v);
        out << "))) <$> " << field_name(name, mname) << " record";
      }
    }
    if (!isfirst) {
      out << "]" << nl;
    }
    indent_down();
    indent(out) << "in if not (null exns) then exns else ";
    indent_up();
  } else {
    out << "$ ";
  }

  out << "Maybe.catMaybes" << nl;
  // Get the Rest
  isfirst = true;
  for (auto* f_iter : fields) {
    // Write field header
    if (isfirst) {
      indent(out) << "[ ";
      isfirst = false;
    } else {
      indent(out) << ", ";
    }
    string mname = f_iter->get_name();
    int32_t key = f_iter->get_key();
    out << "(\\";
    out << v << " -> ";
    if (f_iter->get_req() != t_field::T_OPTIONAL &&
        !((t_type*)f_iter->get_type())->is_xception()) {
      out << "Just ";
    }
    out << "(" << key << ", (\"" << mname << "\",";
    generate_serialize_type(out, f_iter->get_type(), v);
    out << "))) ";
    if (f_iter->get_req() != t_field::T_OPTIONAL &&
        !((t_type*)f_iter->get_type())->is_xception()) {
      out << "$";
    } else {
      out << "<$>";
    }
    out << " " << field_name(name, mname) << " record" << nl;
  }

  // Write the struct map
  if (isfirst) {
    indent(out) << "[]" << nl;
  } else {
    indent(out) << "]" << nl;
  }
  if (hasExn) {
    indent(out) << ")" << nl;
    indent_down();
  }

  indent_down();

  // write
  if (gen_haddock_)
    indent(out) << "-- | Write a '"
                << name << "' with the given 'Thrift.Protocol'" << nl;
  indent(out) << "write_" << name
              << " :: (Thrift.Protocol p, Thrift.Transport t) => p t -> "
              << name << " -> IO ()" << nl;
  indent(out) << "write_" << name
              << " oprot record = Thrift.writeVal oprot $ from_";
  out << name << " record" << nl;

  // encode
    if (gen_haddock_)
      indent(out) << "-- | Serialize a '" << name << "' in pure code" << nl;
  indent(out) << "encode_" << name
              << " :: (Thrift.Protocol p, Thrift.Transport t) => p t -> "
              << name << " -> BS.ByteString" << nl;
  indent(out) << "encode_" << name
              << " oprot record = Thrift.serializeVal oprot $ ";
  out << "from_" << name << " record" << nl;
}

/**
 * Generates a thrift service.
 *
 * @param tservice The service definition
 */
void t_hs_generator::generate_service(t_service* tservice) {
  string f_service_name = get_package_dir() + capitalize(service_name_) + ".hs";
  f_service_.open(f_service_name.c_str());
  record_genfile(f_service_name);

  f_service_ << hs_language_pragma() << nl;
  f_service_ << hs_autogen_comment() << nl;

  string module_prefix = get_module_prefix(program_);
  f_service_ << "module "
             << module_prefix << capitalize(service_name_)
             << " where" << nl;
  f_service_ << hs_imports() << nl;

  if (tservice->get_extends()) {
    auto extends_service = tservice->get_extends();
    auto extends_program = extends_service->get_program();
    f_service_ << "import qualified "
               << get_module_prefix(extends_program)
               << capitalize(extends_service->get_name()) << nl;
  }

  f_service_ << "import qualified "
             << module_prefix << capitalize(program_name_) << "_Types" << nl;
  f_service_ << "import qualified "
             << module_prefix << capitalize(service_name_)
             << "_Iface as Iface" << nl;

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
  t_struct result(program_, field_name(tfunction->get_name(), "result"));
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
 * Generate the map from field names to (type, id)
 * @param tstruct the Struct
 */
void t_hs_generator::generate_hs_typemap(ofstream& out,
                                         t_struct* tstruct) {
  string name = unqualified_type_name(tstruct);
  const auto& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  if (gen_haddock_)
    indent(out) << "-- | 'TypeMap' for the '" << name << "' struct" << nl;
  indent(out) << "typemap_" << name << " :: Types.TypeMap" << nl;
  indent(out) << "typemap_" << name << " = Map.fromList [";
  bool first = true;
  for (const auto& f_iter : fields) {
    string mname = f_iter->get_name();
    if (!first) {
      out << ",";
    }

    t_type* type = get_true_type(f_iter->get_type());
    int32_t key = f_iter->get_key();
    out << "(\"" << mname << "\",(" << key << "," << type_to_enum(type) << "))";
    first = false;
  }
  out << "]" << nl;
}

/**
 * generate the struct with default values filled in
 * @param tstruct the Struct
 */
void t_hs_generator::generate_hs_default(ofstream& out,
                                         t_struct* tstruct) {
  string name = unqualified_type_name(tstruct);
  string fname = unqualified_type_name(tstruct, "default_");
  const auto& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  if (gen_haddock_)
    indent(out) << "-- | Default values for the '" << name << "' struct" << nl;
  indent(out) << fname << " :: " << name << nl;
  indent(out) << fname << " = " << name << "{" << nl;
  indent_up();
  bool first = true;
  for (const auto& f_iter : fields) {
    string mname = f_iter->get_name();
    if (first) {
      first = false;
    } else {
      out << "," << nl;
    }

    t_type* type = get_true_type(f_iter->get_type());
    const t_const_value* value = f_iter->get_value();
    indent(out) << field_name(name, mname) << " = ";
    if (f_iter->get_req() == t_field::T_OPTIONAL ||
        ((t_type*)f_iter->get_type())->is_xception()) {
      if (value == nullptr) {
        out << "Nothing";
      } else {
        out << "Just " << render_const_value(type, value);
      }
    } else {
      out << render_const_value(type, value);
    }
  }
  out << "}" << nl;
  indent_down();
}


/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 */
void t_hs_generator::generate_service_interface(t_service* tservice) {
  string f_iface_name =
    get_package_dir() + capitalize(service_name_) + "_Iface.hs";
  f_iface_.open(f_iface_name.c_str());
  record_genfile(f_iface_name);

  f_iface_ << hs_language_pragma() << nl;
  f_iface_ << hs_autogen_comment() << nl;

  string module_prefix = get_module_prefix(program_);
  f_iface_ << "module " << module_prefix
           << capitalize(service_name_) << "_Iface where" << nl;

  f_iface_ << hs_imports() << nl;
  f_iface_ << "import qualified "
           << module_prefix << capitalize(program_name_) << "_Types" << nl;
  f_iface_ << nl;

  string sname = capitalize(service_name_);
  if (tservice->get_extends() != nullptr) {
    auto extends_program = tservice->get_extends()->get_program();
    string extends = unqualified_type_name(tservice->get_extends());
    indent(f_iface_) << "import qualified "
                     << get_module_prefix(extends_program)
                     << extends << "_Iface" << nl;
    indent(f_iface_) << "class " << extends << "_Iface."
                                 << extends << "_Iface a => "
                                 << sname << "_Iface a where" << nl;

  } else {
    string qualifier = capitalize(program_name_) + "_Iface.";
    indent(f_iface_) << "class "  << sname << "_Iface a where" << nl;
  }

  indent_up();

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    string ft = function_type(*f_iter, true, true, true);
    // corner case to avoid "quaified name in binding position"
    // Haskell compiler case
    string qualifier_Iface = program_name_ + "_Iface.";
    if (module_part(ft) == qualifier_Iface)  {
      ft = name_part(ft);
    }
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
  string f_client_name =
    get_package_dir() + capitalize(service_name_) + "_Client.hs";
  f_client_.open(f_client_name.c_str());
  record_genfile(f_client_name);
  f_client_ << hs_language_pragma() << nl;
  f_client_ << hs_autogen_comment() << nl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;

  string extends = "";
  string exports = "";

  bool first_fn = true;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    exports += (first_fn ? "" : ",");
    string fn_name = (*f_iter)->get_name();
    exports += decapitalize(fn_name);
    first_fn = false;
  }

  string sname = capitalize(service_name_);
  string module_prefix = get_module_prefix(program_);
  indent(f_client_) << "module "
                    << module_prefix << sname << "_Client(" << exports
                    << ") where" << nl;

  if (tservice->get_extends() != nullptr) {
    auto extends_program = tservice->get_extends()->get_program();
    extends = unqualified_type_name(tservice->get_extends());
    indent(f_client_) << "import qualified "
                      << get_module_prefix(extends_program)
                      << extends << "_Client" << nl;
  }

  indent(f_client_) << "import Data.IORef" << nl;
  indent(f_client_) << hs_imports() << nl;
  indent(f_client_) << "import qualified "
                    << module_prefix << capitalize(program_name_) << "_Types"
                    << nl;
  indent(f_client_) << "import qualified "
                    << module_prefix << capitalize(service_name_) << nl;

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
    string qualifier = capitalize(service_name_) + ".";
    string argsname = capitalize((*f_iter)->get_name() + "_args");

    // Serialize the request header
    string fname = (*f_iter)->get_name();
    indent(f_client_) << "Thrift.writeMessage op (\"" << fname
                      << "\", Types.M_CALL, seqn) $" << nl;
    indent_up();
    indent(f_client_) << qualifier << "write_" << argsname
                      << " op (" << qualifier <<  argsname << "{";

    bool first = true;
    for (auto& fld_iter : fields) {
      string fieldname = fld_iter->get_name();
      f_client_ << (first ? "" : ",");
      f_client_ << qualifier << field_name(argsname, fieldname) << "=";
      if (fld_iter->get_req() == t_field::T_OPTIONAL ||
          ((t_type*)fld_iter->get_type())->is_xception())
        f_client_ << "Just ";
      f_client_ << "arg_" << fieldname;
      first = false;
    }
    f_client_ << "})" << nl;
    indent_down();

    // Write to the stream
    indent(f_client_) << "Thrift.tFlush (Thrift.getTransport op)" << nl;
    indent_down();

    if (!(*f_iter)->is_oneway()) {
      string resultname = capitalize((*f_iter)->get_name() + "_result");
      t_struct noargs(program_);

      string recv_fn_name = string("recv_") + (*f_iter)->get_name();
      t_function recv_function((*f_iter)->get_returntype(), recv_fn_name,
                               &noargs);

      // Open function
      indent(f_client_) << recv_fn_name << " ip =" << nl;
      indent_up();

      indent(f_client_) << "Thrift.readMessage ip "
                        << "$ \\(fname,mtype,rseqid) -> do" <<nl;
      indent_up();
      indent(f_client_)
        << "Monad.when (mtype == Types.M_EXCEPTION) "
        << "$ Thrift.readAppExn ip >>= Exception.throw" << nl;

      indent(f_client_) << "res <- " << qualifier << "read_"
                        << resultname << " ip" << nl;

      t_struct* xs = (*f_iter)->get_xceptions();
      const vector<t_field*>& xceptions = xs->get_members();

      // Check all the exceptions
      for (auto x_iter : xceptions) {
        indent(f_client_) << "maybe (return ()) Exception.throw ("
                          << qualifier
                          << field_name(resultname, x_iter->get_name())
                          << " res)" << nl;
      }

      if (!(*f_iter)->get_returntype()->is_void())
        indent(f_client_) << "return $ " << qualifier
                          << field_name(resultname, "success") << " res" << nl;
      else
        indent(f_client_) << "return ()" << endl;

      // Close function
      indent_down();
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
    indent(f_service_) << "\"" << fname
                       << "\" -> process_" << decapitalize(fname)
                       << " (seqid,iprot,oprot,handler)" << nl;
  }

  indent(f_service_) << "_ -> ";
  if (tservice->get_extends() != nullptr) {
    f_service_ << get_module_prefix(tservice->get_extends()->get_program())
               << unqualified_type_name(tservice->get_extends())
               << ".proc_ handler (iprot,oprot) (name,typ,seqid)" << nl;
  } else {
    f_service_ << "do" << nl;
    indent_up();
    indent(f_service_)
      << "_ <- Thrift.readVal iprot (Types.T_STRUCT Map.empty)" << nl;
    indent(f_service_)
      << "Thrift.writeMessage oprot (name,Types.M_EXCEPTION,seqid) $"
      << nl;
    indent_up();
    indent(f_service_)
      << "Thrift.writeAppExn oprot (Thrift.AppExn Thrift.AE_UNKNOWN_METHOD"
      << " (\"Unknown function \" ++ LT.unpack name))" << nl;
    indent_down();
    indent(f_service_) << "Thrift.tFlush (Thrift.getTransport oprot)" << nl;
    indent_down();
  }

  indent_down();

  // Generate the server implementation
  indent(f_service_) << "process handler (iprot, oprot) =" << nl;
  indent_up();

  indent(f_service_) << "Thrift.readMessage iprot ";
  f_service_ << "(proc_ handler (iprot,oprot)) >> return True" << nl;
  indent_down();
}

static bool hasNoArguments(t_function* func) {
    return (func->get_arglist()->get_members().empty());
}

static bool isIntegralType(t_base_type::t_base tbase) {
    switch (tbase) {
    case t_base_type::TYPE_BYTE:
    case t_base_type::TYPE_I16:
    case t_base_type::TYPE_I32:
    case t_base_type::TYPE_I64:
      return true;
    default: return false;
    }
}

string t_hs_generator::render_hs_type_for_function_name(t_type* type) {
    string type_str = render_hs_type(type, false);
    int found = -1;

    while (true) {
        found = type_str.find_first_of("[](). ", found + 1);
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
    using namespace folly;

    string f_service_name =
      get_package_dir() + capitalize(service_name_) + "_Fuzzer.hs";
    f_service_fuzzer_.open(f_service_name.c_str());
    record_genfile(f_service_name);

    string module_prefix = get_module_prefix(program_);
    // Generate module declaration
    f_service_fuzzer_ <<
        //"{-# LANGUAGE ScopedTypeVariables, DeriveDataTypeable #-}" << nl <<
        hs_language_pragma() << nl <<
        hs_autogen_comment() << nl <<
        "module " << module_prefix << capitalize(service_name_)
                  << "_Fuzzer (main) where" << nl;

    // Generate imports specific to the .thrift file.
    f_service_fuzzer_
        << "import qualified "
        << module_prefix << capitalize(program_name_) << "_Types" << nl
        << "import qualified "
        << module_prefix << capitalize(service_name_) << "_Client as Client"
        << nl;

    const vector<t_program*>& includes = program_->get_includes();
    for (const auto& program_include : includes) {
      f_service_fuzzer_ << "import qualified "
                        << get_module_prefix(program_include)
                        << capitalize(program_include->get_name())
                        << "_Types" << nl;
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

    map<string, t_type*> used_types;
    for (f_iter = functions.begin(); f_iter != functions_end; ++f_iter) {
        const vector<t_field*>& fields = (*f_iter)->get_arglist()->get_members();
        vector<t_field*>::const_iterator fld_iter;
        for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
            auto type = (*fld_iter)->get_type();
            used_types.emplace(render_hs_type(type, false), type);
        }
    }

    // all the data generators we need
    for (const auto& used_type : used_types) {
        auto type_iter = used_type.second;
        const string& inf_type = "inf_" + render_hs_type_for_function_name(type_iter);
        const string& hs_type = render_hs_type(type_iter, true);

        f_service_fuzzer_
            << inf_type
            << " :: IO [" << hs_type << "]" << nl
            << inf_type
            << " = infexamples (Arbitrary.arbitrary :: Gen "
            << hs_type << ")"
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
       int varNum = 1;
       // random data sources
       for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
           indent(f_service_fuzzer_)
               << "a" << varNum
               << " <- "
               << "Applicative.ZipList <$> "
               << "inf_"
               << render_hs_type_for_function_name((*fld_iter)->get_type())
               << nl;
           varNum++;
       }
       // fuzzer invocation
       indent(f_service_fuzzer_) << "_ <- ";
       int argCount = fields.size();
       auto argList =
           gen::seq(1, argCount)
           | gen::map([](int n) { return sformat("a{}", n); });

       auto spaceSeparatedArgList = argList | gen::unsplit<string>(" ");
       auto showArgList =
           argList
           | gen::map([](const string& s) { return sformat("Show {}", s); })
           | gen::unsplit<string>(", ");
       auto showElemList =
           argList
           | gen::map([](const string& s) { return sformat("show {}", s); })
           | gen::unsplit<string>(" ++ ");
       auto paramString =
           sformat("({})", argList | gen::unsplit<string>(", "));

//       assert (1 <= argCount);
       if (argCount == 1) {
           f_service_fuzzer_
               << "forM (Applicative.getZipList a1) "
               << funname << "_fuzzFunc";
       } else {
           f_service_fuzzer_
               << "P.sequence . Applicative.getZipList $ "
               << funname << "_fuzzFunc <$> "
               << (argList | gen::unsplit<string>(" <*> "));
       }
       f_service_fuzzer_
           << nl << indent()
           << "return ()" << nl << indent();

       f_service_fuzzer_
           << "where" << nl << indent()
           << funname << "_fuzzFunc ";
       f_service_fuzzer_ << spaceSeparatedArgList;
       f_service_fuzzer_ << sformat(" = let param = {} in", paramString);
       f_service_fuzzer_ << nl << indent() << indent()
           << "if opt_framed opts"
           << nl << indent() << indent()
           << "then withThriftDo opts (withFramedTransport opts) ("
             << funname << "_fuzzOnce param) ("
             << funname << "_exceptionHandler param)"
           << nl << indent() << indent()
           << "else withThriftDo opts (withHandle opts) ("
             << funname << "_fuzzOnce param) ("
             << funname << "_exceptionHandler param)" << nl;

       indent_down();

       // exception handler
       f_service_fuzzer_ << nl << funname
           << "_exceptionHandler :: (" << showArgList << ") => "
           << paramString << " -> IO ()";
       f_service_fuzzer_ << nl << funname << "_exceptionHandler "
           << paramString << " = do" << nl;
       indent_up();
       indent(f_service_fuzzer_)
           << "P.putStrLn $ \"Got exception on data:\"" << nl << indent()
           << "P.putStrLn $ \"(\" ++ " << showElemList << " ++ \")\"";
       indent_down();

       // Thrift invoker
       f_service_fuzzer_ << nl << funname
           << "_fuzzOnce " << paramString << " client = Client.";
       f_service_fuzzer_ << funname
           << " client " << spaceSeparatedArgList << " >> return ()";
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

  string qualifier = capitalize(service_name_) + ".";
  string argsname = capitalize(tfunction->get_name()) + "_args";
  string resultname = capitalize(tfunction->get_name()) + "_result";

  // Generate the function call
  t_struct* arg_struct = tfunction->get_arglist();
  const vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator f_iter;

  indent(f_service_) << "args <- " << qualifier << "read_"
                     << argsname << " iprot" << nl;

  t_struct* xs = tfunction->get_xceptions();
  const vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;

  size_t n = xceptions.size() + 1;
  // Try block for a function with exceptions
  if (n > 0) {
    for(size_t i = 0; i < n; i++) {
      indent(f_service_) << "(Exception.catch" << nl;
      indent_up();
    }
  }

  if (n > 0) {
    indent(f_service_) << "(do" << nl;
    indent_up();
  }
  indent(f_service_);

  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void())
    f_service_ << "val <- ";

  f_service_ << "Iface." << decapitalize(tfunction->get_name()) << " handler";
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter)
    f_service_ << " (" <<
      field_name(argsname, (*f_iter)->get_name()) << " args)";

  if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
    f_service_ << nl;
    indent(f_service_) << "let res = default_" << resultname << "{" <<
      field_name(resultname, "success") << " = val}";

  } else if (!tfunction->is_oneway()) {
    f_service_ << nl;
    indent(f_service_) << "let res = default_" << resultname;
  }
  f_service_ << nl;

  // Shortcut out here for oneway functions
  if (tfunction->is_oneway()) {
    indent(f_service_) << "return ()";
  } else {
    indent(f_service_ ) << "Thrift.writeMessage oprot (\""
                        << tfunction->get_name()
                        << "\", Types.M_REPLY, seqid) $" << nl;
    indent(f_service_ ) << "  write_" << resultname << " oprot res" << nl;
    indent(f_service_ ) << "Thrift.tFlush (Thrift.getTransport oprot)";
  }
  if (n > 0) {
    f_service_ << ")";
    indent_down();
  }
  f_service_ << nl;

  if (n > 0) {
    for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
      indent(f_service_) << "(\\e  -> do" << nl;
      indent_up();

      if (!tfunction->is_oneway()) {
        indent(f_service_) << "let res = default_" << resultname << "{"
                           << field_name(resultname, (*x_iter)->get_name())
                           << " = Just e}" << nl;
        indent(f_service_) << "Thrift.writeMessage oprot (\""
                           << tfunction->get_name()
                           << "\", Types.M_REPLY, seqid) $" << nl;
        indent(f_service_) << "  write_" << resultname << " oprot res" << nl;
        indent(f_service_) << "Thrift.tFlush (Thrift.getTransport oprot)";
      } else {
        indent(f_service_) << "return ()";
      }

      f_service_ << "))" << nl;
      indent_down();
      indent_down();
    }
    indent(f_service_) << "((\\_ -> do" << nl;
    indent_up();

    if (!tfunction->is_oneway()) {
      indent(f_service_)
        << "Thrift.writeMessage oprot (\"" << tfunction->get_name()
        << "\", Types.M_EXCEPTION, seqid) $" << nl;
      indent_up();
      indent(f_service_)
        << "Thrift.writeAppExn oprot (Thrift.AppExn Thrift.AE_UNKNOWN \"\")"
        << nl;
      indent_down();
      indent(f_service_) << "Thrift.tFlush (Thrift.getTransport oprot)";
    } else {
      indent(f_service_) << "return ()";
    }

    f_service_ << ") :: Exception.SomeException -> IO ()))" << endl;

    indent_down();
    indent_down();
  }
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
  generate_deserialize_type(out,type, prefix);
}

/**
 * Deserializes a field of any type.
 */
void t_hs_generator::generate_deserialize_type(ofstream &out,
                                               t_type* type,
                                               string arg) {
  type = get_true_type(type);
  string val = tmp("_val");
  out << "(case " << arg << " of {" << type_to_constructor(type) << " " << val << " -> ";

  if (type->is_void())
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE";

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out, (t_struct*)type, val);

  } else if (type->is_container()) {
    generate_deserialize_container(out, type, val);

  } else if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    if (tbase == t_base_type::TYPE_STRING && !((t_base_type*)type)->is_binary()) {
      if (use_string_)
        out << "LT.unpack $ ";
      else if (use_strict_text_)
        out << "LT.toStrict $ ";
      out << "Encoding.decodeUtf8 ";
    } else if (use_int_ && isIntegralType(tbase)) {
      out << "fromIntegral $ ";
    }
    out << val;
  } else if (type->is_enum()) {
    out << "toEnum $ fromIntegral " << val;

  } else {
    throw "DO NOT KNOW HOW TO DESERIALIZE TYPE " + type->get_name();
  }
  out << "; _ -> error \"wrong type\"})";
}


/**
 * Generates an unserializer for a struct, calling read()
 */
void t_hs_generator::generate_deserialize_struct(ofstream &out,
                                                 t_struct* tstruct,
                                                 string name) {
  out << "(" << qualified_type_name(tstruct, "to_")
      << " (Types.TStruct " << name << "))";
}

/**
 * Serialize a container by writing out the header followed by
 * data and then a footer.
 */
void t_hs_generator::generate_deserialize_container(ofstream &out,
                                                    t_type* ttype,
                                                    string arg) {

  string val = tmp("_v");
  // Declare variables, read header
  if (ttype->is_map()) {
    string key = tmp("_k");
    out << "(Map.fromList $ map (\\(" << key << "," << val << ") -> (";
    generate_deserialize_type(out,((t_map*)ttype)->get_key_type(),key);

    out << ",";
    generate_deserialize_type(out,((t_map*)ttype)->get_val_type(),val);

    out << ")) " << arg << ")";

  } else if (ttype->is_set()) {
    out << "(Set.fromList $ map (\\" << val << " -> ";
    generate_deserialize_type(out,((t_map*)ttype)->get_key_type(),val);
    out << ") " << arg << ")";

  } else if (ttype->is_list()) {
    out << "(";
    if (!use_list_)
      out << "Vector.fromList $ ";
    out << "map (\\" << val << " -> ";
    generate_deserialize_type(out,((t_map*)ttype)->get_key_type(),val);
    out << ") " << arg << ")";
  }
}

/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_hs_generator::generate_serialize_type(ofstream &out,
                                              t_type* type,
                                              string name) {

  type = get_true_type(type);
  // Do nothing for void types
  if (type->is_void())
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE";

  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct(out, (t_struct*)type, name);

  } else if (type->is_container()) {
    generate_serialize_container(out, type, name);

  } else if (type->is_base_type() || type->is_enum()) {
    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      out << type_to_constructor(type) << " ";
      if (tbase == t_base_type::TYPE_STRING && !((t_base_type*)type)->is_binary()) {
        out << "$ Encoding.encodeUtf8 ";
        if (use_string_)
          out << "$ LT.pack ";
        else if (use_strict_text_)
          out << "$ LT.fromStrict ";
      } else if (use_int_ && isIntegralType(tbase)) {
        out << "$ fromIntegral ";
      }
      out << name;
    } else if (type->is_enum()) {
      string ename = capitalize(type->get_name());
      out << "Types.TI32 $ fromIntegral $ fromEnum " << name;
    }

  } else {
    throw "DO NOT KNOW HOW TO SERIALIZE FIELD OF TYPE " + type->get_name();
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
  out << qualified_type_name(tstruct, "from_") << " " << prefix;
}

void t_hs_generator::generate_serialize_container(ofstream &out,
                                                  t_type* ttype,
                                                  string prefix) {
  string k = tmp("_k");
  string v = tmp("_v");

  if (ttype->is_map()) {
    t_type* ktype = ((t_map*)ttype)->get_key_type();
    t_type* vtype = ((t_map*)ttype)->get_val_type();
    out << "Types.TMap "
        << type_to_enum(ktype) << " " << type_to_enum(vtype);
    out << " $ map (\\(" << k << "," << v << ") -> (";
    generate_serialize_type(out, ktype, k);
    out << ", ";
    generate_serialize_type(out, vtype, v);
    out << ")) $ Map.toList " << prefix;

  } else if (ttype->is_set()) {
    out << "Types.TSet "
        << type_to_enum(((t_list*)ttype)->get_elem_type());
    out <<" $ map (\\" << v << " -> ";
    generate_serialize_type(out, ((t_list*)ttype)->get_elem_type(), v);
    out << ") $ Set.toList " << prefix;

  } else if (ttype->is_list()) {
    out << "Types.TList "
        << type_to_enum(((t_list*)ttype)->get_elem_type());
    out <<" $ map (\\" << v << " -> ";
    generate_serialize_type(out, ((t_list*)ttype)->get_elem_type(), v);
    out << ") $ " << (use_list_ ? "" : "Vector.toList ") << prefix;
  }

}

string t_hs_generator::function_type(t_function* tfunc, bool options, bool io, bool method) {
  string result = "";

  const vector<t_field*>& fields = tfunc->get_arglist()->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (auto& f_iter : fields) {
    if (f_iter->get_req() == t_field::T_OPTIONAL ||
        ((t_type*)f_iter->get_type())->is_xception())
      result += "Maybe ";
    result += render_hs_type(f_iter->get_type(), options);
    result += " -> ";
  }

  if (fields.empty() && !method)
    result += "() -> ";

  if (io)
    result += "IO ";

  result += render_hs_type(tfunc->get_returntype(), io);
  return result;
}

string t_hs_generator::qualified_type_name(t_type* ttype,
                                           string function_prefix) {

  return type_name_qualifier(ttype)  +
         unqualified_type_name(ttype, function_prefix);

}

string t_hs_generator::unqualified_type_name(t_type* ttype,
                                            string function_prefix) {
  return function_prefix + capitalize(ttype->get_name());
}

string t_hs_generator::type_name_qualifier(t_type* ttype) {
  const t_program* program = ttype->get_program();

  if (ttype->is_service()) {
    return capitalize(program->get_name()) + "_Iface.";
  } else {
    return capitalize(program->get_name()) + "_Types.";
  }
}

string t_hs_generator::field_name(string tname, string fname) {
  return module_part(tname) + decapitalize(name_part(tname)) + "_" + fname;
}

/**
 * Takes a name, if it is qualified, such as `X.f` or `X.Constructor` it
 * returns the module part including the trailing period, such as `X.`.  If the
 * name is unqualified it returns an empty string.
 */
string t_hs_generator::module_part(const string& qualified_name) {
  string::size_type pos = qualified_name.rfind('.');
  if (pos == string::npos) {
    return string();
  } else {
    return qualified_name.substr(0, pos+1);
  }
}

/**
 * Takes a name, if it is qualified, such as `X.f` or `X.Constructor`, it
 * returns the name part, such as `f` or `Constructor`.  If the name is
 * unqualified it is the identity funciton.
 */
string t_hs_generator::name_part(const string& qualified_name) {
  string::size_type pos = qualified_name.rfind('.');
  if (pos == string::npos) {
    return qualified_name;
  } else {
    return qualified_name.substr(pos+1, string::npos);
  }
}

const string& t_hs_generator::get_package_dir() const {
  return package_dir_;
}

/**
 * Takes a Thrift program file. It returns the Haskell module prefix, which
 * is the Haskell namespace with trailing period. If Haskell namespace is not
 * specified, it returns an empty string
 */
string t_hs_generator::get_module_prefix(const t_program* program) const {
  string hs_namespace = program->get_namespace("hs");
  if (!hs_namespace.empty()) {
    hs_namespace += '.';
  }

  return hs_namespace;
}

/**
 * Converts the parse type to a Protocol.t_type enum
 */
string t_hs_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_VOID:   return "Types.T_VOID";
    case t_base_type::TYPE_STRING: return "Types.T_STRING";
    case t_base_type::TYPE_BOOL:   return "Types.T_BOOL";
    case t_base_type::TYPE_BYTE:   return "Types.T_BYTE";
    case t_base_type::TYPE_I16:    return "Types.T_I16";
    case t_base_type::TYPE_I32:    return "Types.T_I32";
    case t_base_type::TYPE_I64:    return "Types.T_I64";
    case t_base_type::TYPE_FLOAT:  return "Types.T_FLOAT";
    case t_base_type::TYPE_DOUBLE: return "Types.T_DOUBLE";
    }

  } else if (type->is_enum()) {
    return "Types.T_I32";

  } else if (type->is_struct() || type->is_xception()) {
    return "(Types.T_STRUCT "
           + qualified_type_name((t_struct*)type, "typemap_") + ")";

  } else if (type->is_map()) {
    string ktype = type_to_enum(((t_map*)type)->get_key_type());
    string vtype = type_to_enum(((t_map*)type)->get_val_type());
    return "(Types.T_MAP " + ktype + " " + vtype + ")";

  } else if (type->is_set()) {
    return "(Types.T_SET "
           + type_to_enum(((t_list*)type)->get_elem_type()) + ")";

  } else if (type->is_list()) {
    return "(Types.T_LIST "
           + type_to_enum(((t_list*)type)->get_elem_type()) + ")";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

/**
 * Converts the parse type to a default value
 */
string t_hs_generator::type_to_default(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:   return "error \"No default value for type T_VOID\"";
    case t_base_type::TYPE_STRING: return "\"\"";
    case t_base_type::TYPE_BOOL:   return "False";
    case t_base_type::TYPE_BYTE:   return "0";
    case t_base_type::TYPE_I16:    return "0";
    case t_base_type::TYPE_I32:    return "0";
    case t_base_type::TYPE_I64:    return "0";
    case t_base_type::TYPE_FLOAT:  return "0";
    case t_base_type::TYPE_DOUBLE: return "0";
    }

  } else if (type->is_enum()) {
    return "(toEnum 0)";

  } else if (type->is_struct() || type->is_xception()) {
    return qualified_type_name((t_struct*)type, "default_");

  } else if (type->is_map()) {
    return "Map.empty";

  } else if (type->is_set()) {
    return "Set.empty";

  } else if (type->is_list() && use_list_) {
    return "[]";
  } else if (type->is_list()) {
    return "Vector.empty";
  }

  throw "INVALID TYPE IN type_to_default: " + type->get_name();
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
    case t_base_type::TYPE_STRING:
      if (((t_base_type*)type)->is_binary())
        return "BS.ByteString";
      else if (use_string_)
        return "String";
      else if (use_strict_text_)
        return "T.Text";
      else
        return "LT.Text";
    case t_base_type::TYPE_BOOL:   return "Bool";
    case t_base_type::TYPE_BYTE:   return use_int_ ? "Int.Int" : "Int.Int8";
    case t_base_type::TYPE_I16:    return use_int_ ? "Int.Int" : "Int.Int16";
    case t_base_type::TYPE_I32:    return use_int_ ? "Int.Int" : "Int.Int32";
    case t_base_type::TYPE_I64:    return use_int_ ? "Int.Int" : "Int.Int64";
    case t_base_type::TYPE_FLOAT:  return "Float";
    case t_base_type::TYPE_DOUBLE: return "Double";
    }

  } else if (type->is_enum()) {
    return qualified_type_name((t_enum*)type);

  } else if (type->is_struct() || type->is_xception()) {
    return qualified_type_name((t_struct*)type);

  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    type_repr = "Map.HashMap " + render_hs_type(ktype, true) + " " + render_hs_type(vtype, true);

  } else if (type->is_set()) {
    t_type* etype = ((t_set*)type)->get_elem_type();
    type_repr = "Set.HashSet " + render_hs_type(etype, true) ;

  } else if (type->is_list()) {
    t_type* etype = ((t_list*)type)->get_elem_type();
    string inner_type = render_hs_type(etype, true);
    if (use_list_)
      type_repr = "[" + inner_type + "]";
    else
      type_repr = "Vector.Vector " + inner_type;

  } else {
    throw "INVALID TYPE IN type_to_enum: " + type->get_name();
  }

  return needs_parens ? "(" + type_repr + ")" : type_repr;
}

/**
 * Converts the parse type to a haskell constructor
 */
string t_hs_generator::type_to_constructor(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:   throw "invalid type: T_VOID";
    case t_base_type::TYPE_STRING: return "Types.TString";
    case t_base_type::TYPE_BOOL:   return "Types.TBool";
    case t_base_type::TYPE_BYTE:   return "Types.TByte";
    case t_base_type::TYPE_I16:    return "Types.TI16";
    case t_base_type::TYPE_I32:    return "Types.TI32";
    case t_base_type::TYPE_I64:    return "Types.TI64";
    case t_base_type::TYPE_FLOAT:  return "Types.TFloat";
    case t_base_type::TYPE_DOUBLE: return "Types.TDouble";
    }

  } else if (type->is_enum()) {
    return "Types.TI32";

  } else if (type->is_struct() || type->is_xception()) {
    return "Types.TStruct";

  } else if (type->is_map()) {
    return "Types.TMap _ _";

  } else if (type->is_set()) {
    return "Types.TSet _";

  } else if (type->is_list()) {
    return "Types.TList _";
  }
  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}


THRIFT_REGISTER_GENERATOR(hs, "Haskell", "")
