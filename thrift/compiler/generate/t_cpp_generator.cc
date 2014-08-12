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

#include <cinttypes>
#include <cassert>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "thrift/compiler/platform.h"
#include "thrift/compiler/generate/t_oop_generator.h"

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/Hash.h>
#include <folly/String.h>

using namespace std;

namespace {

// shortcut
std::string escape(folly::StringPiece sp) {
  return folly::cEscape<std::string>(sp);
}

enum TerseWrites { TW_DISABLED = 0, TW_SAFE = 1, TW_ALL = 2 };

template <class Options>
TerseWrites parseTerseWrites(const Options& options) {
  auto iter = options.find("terse_writes");
  return (iter == options.end()) ? TW_DISABLED :
         (iter->second == "safe") ? TW_SAFE : TW_ALL;
}

}  // namespace

/**
 * C++ code generator. This is legitimacy incarnate.
 *
 */
class t_cpp_generator : public t_oop_generator {
 public:
  t_cpp_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
    : t_oop_generator(program)
  {
    std::map<std::string, std::string>::const_iterator iter;

    iter = parsed_options.find("json");
    gen_json_ = (iter != parsed_options.end());

    iter = parsed_options.find("include_prefix");
    use_include_prefix_ = (iter != parsed_options.end());
    if (iter != parsed_options.end() && !iter->second.empty()) {
      program->set_include_prefix(iter->second);
    }

    iter = parsed_options.find("cob_style");
    gen_cob_style_ = (iter != parsed_options.end());

    iter = parsed_options.find("no_client_completion");
    gen_no_client_completion_ = (iter != parsed_options.end());

    iter = parsed_options.find("templates");
    gen_templates_ = (iter != parsed_options.end());

    gen_templates_only_ =
      (iter != parsed_options.end() && iter->second == "only");

    iter = parsed_options.find("perfhash");
    gen_perfhash_ = (iter != parsed_options.end());

    iter = parsed_options.find("enum_strict");
    gen_enum_strict_ = (iter != parsed_options.end());

    iter = parsed_options.find("bootstrap");
    bootstrap_ = (iter != parsed_options.end());

    terse_writes_ = parseTerseWrites(parsed_options);

    iter = parsed_options.find("frozen");
    frozen_ = (iter != parsed_options.end());
    frozen_packed_ = (iter != parsed_options.end() && iter->second == "packed");

    iter = parsed_options.find("frozen2");
    frozen2_ = (iter != parsed_options.end());

    // Reminder: Add documentation for new options at the end of this file!

    out_dir_base_ = "gen-cpp";
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

  void generate_typedef(t_typedef* ttypedef);
  void generate_enum(t_enum* tenum);
  void generate_cpp_union(t_struct* tstruct);
  void generate_union_json_reader(std::ofstream& out, t_struct* tstruct);
  void generate_union_reader(std::ofstream& out, t_struct* tstruct);
  void generate_union_writer(std::ofstream& out, t_struct* tstruct);
  void generate_forward_declaration(t_struct* tstruct);
  void generate_struct(t_struct* tstruct) {
    if (tstruct->is_union()) {
      generate_cpp_union(tstruct);
    } else {
      generate_cpp_struct(tstruct, false);
    }
  }
  void generate_xception(t_struct* txception) {
    generate_cpp_struct(txception, true);
  }
  void generate_cpp_struct(t_struct* tstruct, bool is_exception);

  void generate_service(t_service* tservice);

  void print_const_value(std::ofstream& out,
                         std::string name,
                         t_type* type,
                         t_const_value* value);
  std::string render_const_value     (std::ofstream& out,
                                      t_type*        type,
                                      t_const_value* value,
                                      bool           allow_null_val = false);
  string get_type_access_suffix(t_type* type);

  void generate_struct_isset         (std::ofstream& out,
                                      const std::vector<t_field*>& members,
                                      bool bitfields = false);

  void generate_struct_definition    (std::ofstream& out, t_struct* tstruct,
                                      bool is_exception=false,
                                      bool pointers=false,
                                      bool read=true,
                                      bool write=true,
                                      bool swap=false,
                                      bool needs_copy_constructor=false);
  void generate_copy_constructor    (std::ofstream& out, t_struct* tstruct);
  void generate_equal_operator      (std::ofstream& out, t_struct* tstruct);
  void generate_frozen_struct_definition(t_struct* tstruct);
  void generate_frozen2_struct_definition(t_struct* tstruct);

  void generate_json_reader          (std::ofstream& out, t_struct* tstruct);
  void generate_json_struct          (std::ofstream& out, t_struct* tstruct,
                                      const string& prefix_thrift,
                                      const string& prefix_json,
                                      bool dereference = false);
  void generate_json_enum            (std::ofstream& out, t_enum* tstruct,
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
                                      const string& prefix_thrift = "",
                                      const string& prefix_json = "");
  void generate_json_list_element    (std::ofstream& out, t_list* tlist,
                                      bool use_push,
                                      const string& i,
                                      const string& prefix_thrift = "",
                                      const string& prefix_json = "");
  void generate_json_map_element     (std::ofstream& out, t_map* tmap,
                                      const string& key,
                                      const string& value,
                                      const string& prefix_thrift = "");
  void generate_struct_reader        (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_clear         (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_writer        (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_result_writer (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_swap          (std::ofstream& out, t_struct* tstruct);
  void generate_struct_merge         (std::ofstream& out, t_struct* tstruct);

  /**
   * Service-level generation functions
   */

  void generate_service_interface (t_service* tservice, string style);
  void generate_service_interface_factory (t_service* tservice, string style);
  void generate_service_null      (t_service* tservice, string style);
  void generate_service_multiface (t_service* tservice);
  void generate_service_helpers   (t_service* tservice);
  void generate_service_client    (t_service* tservice, string style);
  void generate_service_processor (t_service* tservice, string style);
  void generate_service_skeleton  (t_service* tservice);
  void generate_process_function  (t_service* tservice, t_function* tfunction,
                                   string style, bool specialized=false);
  void generate_function_helpers  (t_service* tservice, t_function* tfunction);
  void generate_service_async_skeleton (t_service* tservice);
  void generate_service_perfhash_keywords (t_service* tservice);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field        (std::ofstream& out,
                                          t_field*    tfield,
                                          std::string prefix="",
                                          std::string suffix="");

  void generate_deserialize_struct       (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string prefix="",
                                          bool pointer = false);

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
                                          std::string prefix,
                                          bool push_back,
                                          std::string index);

  void generate_serialize_field          (std::ofstream& out,
                                          t_field*    tfield,
                                          std::string prefix="",
                                          std::string suffix="");

  void generate_serialize_struct         (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string prefix="",
                                          bool pointer=false);

  void generate_serialize_container      (std::ofstream& out,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_serialize_map_element    (std::ofstream& out,
                                          t_map*      tmap,
                                          std::string iter);

  void generate_serialize_set_element    (std::ofstream& out,
                                          t_set*      tmap,
                                          std::string iter);

  void generate_serialize_list_element   (std::ofstream& out,
                                          t_list*     tlist,
                                          std::string iter);

  void generate_function_call            (ostream& out,
                                          t_function* tfunction,
                                          string target,
                                          string iface,
                                          string arg_prefix);

  void generate_hash_and_equal_to(ofstream& out, t_type* ttype, const std::string& type_name);

  /*
   * Helper rendering functions
   */

  std::string namespace_prefix(std::string ns, std::string delimiter="::")
    const;
  std::string namespace_open(std::string ns);
  std::string namespace_close(std::string ns);
  enum {
    IN_TYPEDEF = 1 << 0,
    IN_ARG = 1 << 1,
    CHASE_TYPEDEFS = 1 << 2,
    ALWAYS_NAMESPACE = 1 << 3,
  };
  std::string type_name(t_type* ttype, int flags=0);
  std::string base_type_name(t_base_type::t_base tbase);

  std::string generate_reflection_initializer_name(t_type* ttype);
  std::string generate_reflection_datatype(t_type* ttype);
  void generate_struct_reflection(ostream& out, t_struct* tstruct);
  std::string declare_field(t_field* tfield, bool init=false, bool pointer=false, bool constant=false, bool reference=false, bool unique=false);
  std::string declare_frozen_field(const t_field* tfield);
  std::string function_signature(t_function* tfunction, std::string style, std::string prefix="", bool name_params=true);
  std::string cob_function_signature(t_function* tfunction, std::string prefix="", bool name_params=true);
  std::string argument_list(t_struct* tstruct, bool name_params=true, bool start_comma=false);
  std::string type_to_enum(t_type* ttype);
  bool try_terse_write_predicate(ofstream& out, t_field* t, bool pointers, TerseWrites terse_writes,
                                 string& predicate);

  void generate_enum_constant_list(std::ofstream& f,
                                   const std::vector<t_enum_value*>& constants,
                                   bool quote_names,
                                   bool include_values,
                                   const char* typed_name = nullptr);

  bool is_reference(const t_field* f) const {
    return f->annotations_.count("cpp.ref") != 0;
  }

  bool is_required(const t_field* f) const {
    return f->get_req() == t_field::T_REQUIRED;
  }

  bool is_optional(const t_field* f) const {
    return f->get_req() == t_field::T_OPTIONAL;
  }

  bool has_isset(const t_field* f) const {
    return !is_required(f) && !is_reference(f);
  }


  bool is_complex_type(t_type* ttype) {
    ttype = get_true_type(ttype);

    return
      ttype->is_container() ||
      ttype->is_struct() ||
      ttype->is_xception() ||
      (ttype->is_base_type() && (((t_base_type*)ttype)->get_base() == t_base_type::TYPE_STRING));
  }

  bool type_can_throw(const t_type* type);
 private:
  static const std::string reflection_ns_prefix_;

  bool type_can_throw(const t_type* type, std::set<const t_type*>& visited);

  /**
   * Returns the include prefix to use for a file generated by program, or the
   * empty string if no include prefix should be used.
   */
  std::string get_include_prefix(const t_program& program) const;

  /**
   * Returns an include guard using namespace, program name and suffix
   */
  string get_include_guard(const char* suffix) const;

  /**
   * True iff we should generate a function parse json to thrift object.
   */
  bool gen_json_;

  /**
   * True iff we should generate templatized reader/writer methods.
   */
  bool gen_templates_;

  /**
   * True iff we should generate process function pointers for only templatized
   * reader/writer methods.
   */
  bool gen_templates_only_;

  /**
   * True iff we should use a path prefix in our #include statements for other
   * thrift-generated header files.
   */
  bool use_include_prefix_;

  /**
   * True iff we should generate "Continuation OBject"-style classes as well.
   */
  bool gen_cob_style_;

  /**
   * True if we should omit calls to completion__() in CobClient class.
   */
  bool gen_no_client_completion_;

  /**
   * True if we should generate a perfect hash instead of processMap_
   * when dispatching server commands.
   *
   * Requires gperf in the path.
   */
  bool gen_perfhash_;

  /**
   * True iff we should use new, C++ 11 style class enums.
   */
  bool gen_enum_strict_;

  /*
   * True if we're bootstrapping; avoid reflection, etc.
   */
  bool bootstrap_;

  /*
   * Should write function avoid emitting values that are unchanged
   * from default, if not explicitly optional/required?
   * Caveats, when whitelisted:
   *  - don't change default values when turned on.
   *  - read() should be done into fresh or __clear()ed objects.
   */
  TerseWrites terse_writes_;

  /*
   * Should write template specializations for frozen structs, suitable for
   * mmaping from disk
   */
  bool frozen_;
  bool frozen2_;

  /*
   * Add #pragma pack for frozen structs
  */
  bool frozen_packed_;

  /**
   * Strings for namespace, computed once up front then used directly
   */

  std::string ns_prefix_;
  std::string ns_open_;
  std::string ns_close_;

  std::string ns_reflection_prefix_;
  std::string ns_reflection_open_;
  std::string ns_reflection_close_;

  /**
   * File streams, stored here to avoid passing them as parameters to every
   * function.
   */

  std::ofstream f_types_;
  std::ofstream f_types_impl_;
  std::ofstream f_types_tcc_;
  std::ofstream f_types_layouts_;
  std::ofstream f_types_layouts_impl_;
  std::ofstream f_header_;
  std::ofstream f_service_;
  std::ofstream f_service_tcc_;
  std::ofstream f_service_gperf_;
  std::ofstream f_reflection_;
  std::ofstream f_reflection_impl_;

  std::set<uint64_t> reflection_hashes_;

  // The ProcessorGenerator is used to generate parts of the code,
  // so it needs access to many of our protected members and methods.
  //
  // TODO: The code really should be cleaned up so that helper methods for
  // writing to the output files are separate from the generator classes
  // themselves.
  friend class ProcessorGenerator;
};

const std::string t_cpp_generator::reflection_ns_prefix_ =
  "::apache::thrift::reflection::";

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_cpp_generator::init_generator() {
  // Make output directory
  MKDIR(get_out_dir().c_str());

  // Make output file
  string f_types_name = get_out_dir() + program_name_ + "_types.h";
  f_types_.open(f_types_name.c_str());
  record_genfile(f_types_name);

  string f_types_impl_name = get_out_dir() + program_name_ + "_types.cpp";
  f_types_impl_.open(f_types_impl_name.c_str());
  record_genfile(f_types_impl_name);

  if (frozen2_) {
    string f_types_layouts_name = get_out_dir() + program_name_ + "_layouts.h";
    f_types_layouts_.open(f_types_layouts_name.c_str());
    record_genfile(f_types_layouts_name);
    string f_types_layouts_impl_name =
        get_out_dir() + program_name_ + "_layouts.cpp";
    f_types_layouts_impl_.open(f_types_layouts_impl_name.c_str());
    record_genfile(f_types_layouts_impl_name);
    f_types_layouts_ <<
      autogen_comment();
    f_types_layouts_impl_ <<
      autogen_comment();
  }

  if (gen_templates_) {
    // If we don't open the stream, it appears to just discard data,
    // which is fine.
    string f_types_tcc_name = get_out_dir()+program_name_+"_types.tcc";
    f_types_tcc_.open(f_types_tcc_name.c_str());
    record_genfile(f_types_tcc_name);
  }

  if (!bootstrap_) {
    string f_reflection_name = get_out_dir()+program_name_+"_reflection.h";
    f_reflection_.open(f_reflection_name.c_str());
    record_genfile(f_reflection_name);

    string f_reflection_impl_name=get_out_dir()+program_name_+"_reflection.cpp";
    f_reflection_impl_.open(f_reflection_impl_name.c_str());
    record_genfile(f_reflection_impl_name);
  }

  // Print header
  f_types_ <<
    autogen_comment();
  f_types_impl_ <<
    autogen_comment();
  f_types_tcc_ <<
    autogen_comment();

  f_reflection_ <<
    autogen_comment();

  f_reflection_impl_ <<
    autogen_comment();

  // Start ifndef
  f_types_ << get_include_guard("_TYPES_H");
  f_types_tcc_ << get_include_guard("_TYPES_TCC");

  f_reflection_ << get_include_guard("_REFLECTION_H");

  // Include base types
  f_types_ <<
    "#include <thrift/lib/cpp/Thrift.h>" << endl <<
    "#include <thrift/lib/cpp/TApplicationException.h>" << endl <<
    "#include <thrift/lib/cpp/protocol/TProtocol.h>" << endl <<
    "#include <thrift/lib/cpp/transport/TTransport.h>" << endl <<
    endl;

  if (frozen_) {
    f_types_ <<
      "#include <thrift/lib/cpp/Frozen.h>" << endl <<
      endl;
  }

  if (frozen2_) {
    f_types_layouts_ << get_include_guard("_LAYOUTS_H") << endl
                     << "#include <thrift/lib/cpp2/frozen/Frozen.h>" << endl
                     << "#include \"" << get_include_prefix(*get_program())
                     << program_name_ << "_types.h\"" << endl;
    for (auto* include : program_->get_includes()) {
      f_types_layouts_ << "#include \"" << get_include_prefix(*include)
                       << include->get_name() << "_layouts.h\"" << endl;
    }
    f_types_layouts_
        << endl << "namespace apache { namespace thrift { namespace frozen {"
        << endl;

    f_types_layouts_impl_
        << "#include \"" << get_include_prefix(*get_program()) << program_name_
        << "_layouts.h\"" << endl << endl
        << "namespace apache { namespace thrift { namespace frozen {" << endl;
  }

  if (gen_json_) {
    f_types_ << "#include <folly/json.h>" << endl <<
      endl << "#include <folly/Range.h>" << endl <<
      endl << "#include <folly/Conv.h>" << endl <<
      endl << "#include <math.h>" << endl <<
      endl << "#include <thrift/lib/cpp/Thrift.h>" << endl <<
      endl << "using namespace folly::json;" << endl;
  }

  if (!bootstrap_) {
    // Forward declare Schema, but don't include the reflection header
    // as we could end up with conflicting definitions.
    f_types_ <<
      "namespace apache { namespace thrift { namespace reflection {" << endl <<
      "class Schema;" << endl <<
      "}}}" << endl;

    f_reflection_ <<
      "namespace apache { namespace thrift { namespace reflection {" << endl <<
      "class Schema;" << endl <<
      "}}}" << endl
      << endl;
  }

  // Include other Thrift includes
  for (auto* include : program_->get_includes()) {
    f_types_ << "#include \"" << get_include_prefix(*include)
             << include->get_name() << "_types.h\"" << endl;

    // XXX(simpkins): If gen_templates_ is enabled, we currently assume all
    // included files were also generated with templates enabled.
    f_types_tcc_ << "#include \"" << get_include_prefix(*include)
                 << include->get_name() << "_types.tcc\"" << endl;
  }
  f_types_ << endl;

  // Include custom headers
  const vector<string>& cpp_includes = program_->get_cpp_includes();
  for (size_t i = 0; i < cpp_includes.size(); ++i) {
    if (cpp_includes[i][0] == '<') {
      f_types_ <<
        "#include " << cpp_includes[i] << endl;
    } else {
      f_types_ <<
        "#include \"" << cpp_includes[i] << "\"" << endl;
    }
  }
  f_types_ <<
    endl;

  // Include the types file
  f_types_impl_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl <<
    endl;
  f_types_tcc_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl <<
    endl;

  std::ofstream& out = (gen_templates_ ? f_types_tcc_ : f_types_impl_);
  if (!bootstrap_) {
    out <<
      "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
      "_reflection.h\"" << endl <<
      endl;
  }

  // The swap() code needs <algorithm> for std::swap()
  // ltstr needs string.h for strcmp
  f_types_impl_ << "#include <algorithm>" << endl
                << "#include <string.h>" << endl << endl;

  f_reflection_impl_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_reflection.h\"" << endl <<
    "#include <thrift/lib/cpp/Reflection.h>" << endl <<
    endl;

  // Open namespace
  ns_open_ = namespace_open(program_->get_namespace("cpp"));
  ns_close_ = namespace_close(program_->get_namespace("cpp"));
  ns_prefix_ = namespace_prefix(program_->get_namespace("cpp"));

  ns_reflection_open_ = ns_open_+" namespace "+program_name_+"_reflection_ {";
  ns_reflection_close_ = "}" + ns_close_;
  ns_reflection_prefix_ = ns_prefix_ + program_name_ + "_reflection_::";


  f_types_ <<
    ns_open_ << endl <<
    endl;

  f_types_impl_ <<
    ns_open_ << endl <<
    endl;

  f_types_tcc_ <<
    ns_open_ << endl <<
    endl;

  f_reflection_ <<
    ns_reflection_open_ << endl <<
    endl;

  f_reflection_impl_ <<
    ns_reflection_open_ << endl <<
    endl;
}

/**
 * Closes the output files.
 */
void t_cpp_generator::close_generator() {
  // Close namespace
  f_types_ <<
    ns_close_ << endl <<
    endl;
  f_types_impl_ <<
    ns_close_ << endl;
  f_types_tcc_ <<
    ns_close_ << endl <<
    endl;

  f_reflection_ <<
    ns_reflection_close_ << endl <<
    endl;

  f_reflection_impl_ <<
    ns_reflection_close_ << endl <<
    endl;

  // Include the types.tcc file from the types header file,
  // so clients don't have to explicitly include the tcc file.
  // TODO(simpkins): Make this a separate option.
  if (gen_templates_) {
    f_types_ <<
      "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
      "_types.tcc\"" << endl <<
      endl;
  }

  // Close ifndef
  f_types_ <<
    "#endif" << endl;
  f_types_tcc_ <<
    "#endif" << endl;

  f_reflection_ <<
    "#endif" << endl;

  // Close output file
  if (frozen2_) {
    f_types_layouts_
      << endl
      << "}}} // apache::thrift::frozen " << endl
      << "#endif" << endl;
    f_types_layouts_impl_
      << endl
      << "}}} // apache::thrift::frozen " << endl;
    f_types_layouts_.close();
    f_types_layouts_impl_.close();
  }
  f_types_.close();
  f_types_impl_.close();
  f_types_tcc_.close();

  f_reflection_.close();
  f_reflection_impl_.close();
}

/**
 * Generates a typedef. This is just a simple 1-liner in C++
 *
 * @param ttypedef The type definition
 */
void t_cpp_generator::generate_typedef(t_typedef* ttypedef) {
  f_types_ <<
    indent() << "typedef " << type_name(ttypedef->get_type(), IN_TYPEDEF) <<
    " " << ttypedef->get_symbolic() << ";" << endl << endl;


  t_type* ttype = get_true_type(ttypedef);
  generate_hash_and_equal_to(f_types_, ttype, ttypedef->get_symbolic());
}

/*
 * Should generate template specialization for std::hash and std::equal_to
 * which can be used by unordered container.
 */
void t_cpp_generator::generate_hash_and_equal_to(ofstream& out,
                                                 t_type* ttype,
                                                 const std::string& type_name) {
  const bool genHash = ttype->annotations_.count("cpp.declare_hash") > 0;
  const bool genEqualTo = ttype->annotations_.count("cpp.declare_equal_to") > 0;
  if (genHash || genEqualTo) {
    out << ns_close_ << endl << endl
      << indent() << namespace_open("std") << endl;

    const std::string fullName = ns_prefix_ + type_name;
    if (genHash) {
      out << indent() << "template<> struct hash<typename "
        << fullName << "> {" << endl
        << indent() << "size_t operator()(const " << fullName << "&) const;"
        << endl << indent() << "};" << endl;
    }
    if (genEqualTo) {
      out << indent() << "template<> struct equal_to<typename "
        << fullName << "> {" << endl
        << indent() << "bool operator()(const " << fullName << "&, " << endl
        << indent() << "const " << fullName << "&) const;"
        << endl << indent() << "};" << endl;
    }

    out << indent() << namespace_close("std") << endl << endl
      << indent() << ns_open_ << endl << endl;
  }
}

void t_cpp_generator::generate_enum_constant_list(std::ofstream& f,
                                                  const vector<t_enum_value*>& constants,
                                                  bool quote_names,
                                                  bool include_values,
                                                  const char* typed_name) {
  f << " {" << endl;
  indent_up();

  vector<t_enum_value*>::const_iterator c_iter;
  bool first = true;
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    if (first) {
      first = false;
    } else {
      f << "," << endl;
    }
    f << indent();
    if (quote_names) {
      f << "\"";
    }
    if (typed_name) {
      f << typed_name << "::";
    }
    f << (*c_iter)->get_name();
    if (quote_names) {
      f << "\"";
    }
    if (include_values) {
      f << " = " << (*c_iter)->get_value();
    }
  }

  indent_down();
  f << endl << "};" << endl << endl;
}

/**
 * Generates code for an enumerated type. In C++, this is essentially the same
 * as the thrift definition itself, using the enum keyword in C++.
 *
 * @param tenum The enumeration
 */
void t_cpp_generator::generate_enum(t_enum* tenum) {
  vector<t_enum_value*> constants = tenum->get_constants();
  auto value_type   = gen_enum_strict_ ? tenum->get_name().c_str() : "int";
  auto typed_prefix = gen_enum_strict_ ? tenum->get_name().c_str() : nullptr;
  auto enum_suffix  = gen_enum_strict_ ? " class "                 : " ";
  auto name = tenum->get_name();
  auto fullname = folly::to<string>(ns_prefix_, name);

  f_types_ << indent() << "enum" << enum_suffix << name;
  generate_enum_constant_list(f_types_, constants, false, true);

  std::string minName;
  std::string maxName;
  int32_t minValue = std::numeric_limits<int32_t>::max();
  int32_t maxValue = std::numeric_limits<int32_t>::min();

  for (auto& ev : constants) {
    const std::string& name = ev->get_name();
    int32_t value = ev->get_value();
    // <=, >= so we capture the names in case we actually use
    // numeric_limits<int32_t>::max / min as values
    if (value <= minValue) {
      minName = name;
      minValue = value;
    }
    if (value >= maxValue) {
      maxName = name;
      maxValue = value;
    }
  }

  /**
     Generate a character array of enum names for debugging purposes.
  */
  f_types_impl_ <<
    indent() << value_type << " _k" << name << "Values[] =";
  generate_enum_constant_list(
      f_types_impl_, constants, false, false, typed_prefix);

  f_types_impl_ <<
    indent() << "const char* _k" << name << "Names[] =";
  generate_enum_constant_list(
      f_types_impl_, constants, true, false, typed_prefix);

  auto valuesToNames = folly::to<string>("_", name, "_VALUES_TO_NAMES");
  auto namesToValues = folly::to<string>("_", name, "_NAMES_TO_VALUES");
  f_types_ <<
    indent() << "extern const std::map<" << value_type <<", const char*> " <<
    valuesToNames << ";" << endl << endl;

  f_types_ <<
    indent() << "extern const std::map<const char*, " << value_type <<
    ", apache::thrift::ltstr> " << namesToValues << ";" << endl << endl;

  if (!minName.empty()) {
    f_types_ <<
      ns_close_ << endl <<
      "namespace apache { namespace thrift { " << endl <<
      "template<>" << endl <<
      "struct TEnumTraits<" << fullname <<
      "> : public TEnumTraitsBase<" << fullname << ">" << endl <<
      "{" << endl <<
      "inline static constexpr " << fullname << " min() {" << endl <<
      indent() << "return " << fullname << "::" << minName << ";" << endl <<
      "}" << endl <<
      "inline static constexpr " << fullname << " max() {" << endl <<
      indent() << "return " << fullname << "::" << maxName << ";" << endl <<
      "}" << endl <<
      "};" << endl <<
      "}} // apache:thrift" << endl << endl <<
      ns_open_ << endl;
  }

  f_types_impl_ <<
    indent() << "const std::map<" << value_type << ", const char*> " <<
    valuesToNames << "(apache::thrift::TEnumIterator<" <<
    value_type << ">(" << constants.size() <<
    ", _k" << name << "Values" <<
    ", _k" << name << "Names), " <<
    "apache::thrift::TEnumIterator<" << value_type <<
    ">(-1, NULL, NULL));" << endl << endl;

  f_types_impl_ <<
    indent() << "const std::map<const char*, " << value_type <<
    ", apache::thrift::ltstr> " << namesToValues <<
    "(apache::thrift::TEnumInverseIterator<" << value_type <<
    ">(" << constants.size() << ", _k" << name << "Values" <<
    ", _k" << name << "Names), " <<
    "apache::thrift::TEnumInverseIterator<" << value_type <<
    ">(-1, NULL, NULL));" << endl << endl;

  f_types_impl_ <<
    ns_close_ << endl <<
    "namespace apache { namespace thrift {" << endl <<
    "template<>" << endl <<
    "const char* TEnumTraitsBase<" << fullname <<
      ">::findName(" << fullname << " value) {" << endl <<
    indent() << "return findName(" << ns_prefix_ <<
      valuesToNames << ", value);" << endl <<
    "} " << endl << endl <<
    "template<>" << endl <<
    "bool TEnumTraitsBase<" << fullname <<
      ">::findValue(const char* name, " << fullname << "* out) {" << endl <<
    indent() << "return findValue(" <<
      ns_prefix_ << namesToValues << ", name, out);" << endl <<
    "} " << endl <<
    "}} // apache::thrift " << endl << endl <<
    ns_open_ << endl;
}

/**
 * Generates a class that holds all the constants.
 */
void t_cpp_generator::generate_consts(std::vector<t_const*> consts) {
  string f_consts_name = get_out_dir()+program_name_+"_constants.h";
  ofstream f_consts;
  f_consts.open(f_consts_name.c_str());
  record_genfile(f_consts_name);

  string f_consts_impl_name = get_out_dir()+program_name_+"_constants.cpp";
  ofstream f_consts_impl;
  f_consts_impl.open(f_consts_impl_name.c_str());
  record_genfile(f_consts_impl_name);

  // Print header
  f_consts <<
    autogen_comment();
  f_consts_impl <<
    autogen_comment();

  // Start ifndef
  f_consts << get_include_guard("_CONSTANTS_H") <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl <<
    endl <<
    ns_open_ << endl <<
    endl;

  f_consts_impl <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_constants.h\"" << endl <<
    endl <<
    ns_open_ << endl <<
    endl;

  f_consts <<
    "class " << program_name_ << "Constants {" << endl <<
    " public:" << endl <<
    "  " << program_name_ << "Constants();" << endl <<
    endl;
  indent_up();
  vector<t_const*>::iterator c_iter;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    string name = (*c_iter)->get_name();
    t_type* type = (*c_iter)->get_type();
    f_consts <<
      indent() << type_name(type) << " " << name << ";" << endl;
  }
  indent_down();
  f_consts <<
    "};" << endl;

  f_consts_impl <<
    "const " << program_name_ << "Constants g_" << program_name_ << "_constants;" << endl <<
    endl <<
    program_name_ << "Constants::" << program_name_ << "Constants() {" << endl;
  indent_up();
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    print_const_value(f_consts_impl,
                      (*c_iter)->get_name(),
                      (*c_iter)->get_type(),
                      (*c_iter)->get_value());
  }
  indent_down();
  indent(f_consts_impl) <<
    "}" << endl;

  f_consts <<
    endl <<
    "extern const " << program_name_ << "Constants g_" << program_name_ << "_constants;" << endl <<
    endl <<
    ns_close_ << endl <<
    endl <<
    "#endif" << endl;
  f_consts.close();

  f_consts_impl <<
    endl <<
    ns_close_ << endl <<
    endl;
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
void t_cpp_generator::print_const_value(ofstream& out, string name, t_type* type, t_const_value* value) {
  type = get_true_type(type);
  if (type->is_base_type() || type->is_enum()) {
    string v2 = render_const_value(out, type, value);
    indent(out) << name << " = " << v2 << ";" << endl;
  } else if (type->is_struct() || type->is_xception()) {
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
      string val = render_const_value(out, field_type, v_iter->second);
      indent(out) << name << "." << v_iter->first->get_string() << " = " << val << ";" << endl;
      indent(out) << name << ".__isset." << v_iter->first->get_string() << " = true;" << endl;
    }
    out << endl;
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string key = render_const_value(out, ktype, v_iter->first);
      string val = render_const_value(out, vtype, v_iter->second);
      indent(out) << name << ".insert(std::make_pair(" << key << ", " << val << "));" << endl;
    }
    out << endl;
  } else if (type->is_list()) {
    t_type* etype = ((t_list*)type)->get_elem_type();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string val = render_const_value(out, etype, *v_iter);
      indent(out) << name << ".push_back(" << val << ");" << endl;
    }
    out << endl;
  } else if (type->is_set()) {
    t_type* etype = ((t_set*)type)->get_elem_type();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string val = render_const_value(out, etype, *v_iter);
      indent(out) << name << ".insert(" << val << ");" << endl;
    }
    out << endl;
  } else {
    throw "INVALID TYPE IN print_const_value: " + type->get_name();
  }
}

string t_cpp_generator::get_type_access_suffix(t_type* type) {
  if (type->is_typedef()) {
    t_type* base_type = static_cast<t_typedef*>(type)->get_type();
    const auto& indir = base_type->annotations_.find("cpp.indirection");
    if (indir != base_type->annotations_.end()) {
      return indir->second;
    }
  }
  return "";
}

/**
 *
 */
string t_cpp_generator::render_const_value(
                          ofstream&      out,
                          t_type*        type,
                          t_const_value* value,
                          bool           allow_null_val) {
  std::ostringstream render;
  if (value == nullptr) {
    if (allow_null_val) {
      if (type->is_enum()) {
        render << "static_cast<" << type_name(type) << ">(0)";
      } else if (type->is_string()) {
        render << "\"\"";
      } else {
        render << "0";
      }
    } else {
      throw "render_const_value called with value == NULL";
    }
  } else if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      render << "\"" + value->get_string() + "\"";
      break;
    case t_base_type::TYPE_BOOL:
      render << ((value->get_integer() > 0) ? "true" : "false");
      break;
    case t_base_type::TYPE_BYTE:
    case t_base_type::TYPE_I16:
    case t_base_type::TYPE_I32:
      render << value->get_integer();
      break;
    case t_base_type::TYPE_I64:
      render << value->get_integer() << "LL";
      break;
    case t_base_type::TYPE_DOUBLE:
    case t_base_type::TYPE_FLOAT:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        render << value->get_integer();
      } else {
        render << value->get_double();
      }
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    if (gen_enum_strict_) {
      // Search the enum definitions for the label with the given value
      const t_enum_value* val =
        static_cast<t_enum*>(type)->find_value(value->get_integer());
      if (val == nullptr) {
        std::ostringstream except;
        except << "Unrecognized value " << value->get_integer()  <<
                  " for enum \"" << type_name(type) << "\"";
        throw except.str();
      }
      render << type_name(type) << "::" << val->get_name();
    } else {
      render << "(" << type_name(type) << ")" << value->get_integer();
    }
  } else {
    string t = tmp("tmp");
    indent(out) << type_name(type) << " " << t << ";" << endl;
    print_const_value(out, t, type, value);
    render << t;
  }

  return render.str();
}

void t_cpp_generator::generate_forward_declaration(t_struct* tstruct) {
  // Forward declare struct def
  f_types_ <<
    indent() << "class " << tstruct->get_name() << ";" << endl <<
    endl;
  struct TypeInfo {
    uint64_t id;
    std::string name;
  };
}

/**
 * Generates a struct definition for a thrift data type. This is a class
 * with data members and a read/write() function, plus a mirroring isset
 * inner class.
 *
 * @param tstruct The struct definition
 */
void t_cpp_generator::generate_cpp_struct(t_struct* tstruct, bool is_exception) {
  bool needs_copy_constructor = false;
  for (auto const& member : tstruct->get_members()) {
    if (is_reference(member)) {
      needs_copy_constructor = true;
    }
  }

  generate_struct_definition(f_types_, tstruct, is_exception,
                             false, true, true, true, needs_copy_constructor);

  if ((frozen_ && !is_exception) ||
      tstruct->annotations_.count("frozen") != 0) {
    generate_frozen_struct_definition(tstruct);
  }

  if ((frozen2_ && !is_exception) ||
      tstruct->annotations_.count("frozen2") != 0) {
    generate_frozen2_struct_definition(tstruct);
  }

  std::ofstream& out = (gen_templates_ ? f_types_tcc_ : f_types_impl_);

  if (!bootstrap_) {
    generate_struct_reflection(f_types_impl_, tstruct);
  }
  if (needs_copy_constructor) {
    generate_copy_constructor(f_types_impl_, tstruct);
  }
  generate_equal_operator(f_types_impl_, tstruct);

  generate_json_reader(f_types_impl_, tstruct);
  generate_struct_reader(out, tstruct);
  generate_struct_clear(f_types_impl_, tstruct);
  generate_struct_writer(out, tstruct);
  generate_struct_swap(f_types_impl_, tstruct);
  generate_struct_merge(f_types_impl_, tstruct);
}

/**
 * Given a Union definition like
 *
 * union Message {
 *   1: AStruct a_struct,
 *   2: i32 first_int,
 *   3: string str,
 *   4: i32 another_int,
 * }
 *
 * The generated class looks like
 *
 * class Message {
 *  public:
 *   enum class Type {
 *     __EMPTY__,
 *     a_struct,
 *     first_int,
 *     str,
 *     another_int,
 *   };
 *
 *   union storage_type {
 *     AStruct a_struct;
 *     int first_int;
 *     string str;
 *     int another_int;
 *
 *     storage_type() {}
 *     ~storage_type() {}
 *   };
 *
 *   Message(): type_(__EMPTY__) {
 *   }
 *
 *   void __clear() {
 *     if (type_ == __EMPTY__) { return; }
 *
 *     switch (type_) {
 *       case a_struct:
 *         value_.a_struct.~AStruct();
 *         return;
 *
 *       ..... and so on ....
 *     }
 *
 *     type_ = Type::__EMPTY__;
 *   }
 *
 *   // To set a value from constructor parameters
 *   void set_a_struct(Ts&&...) {
 *     __clear();
 *     type_ = a_struct;
 *     new (&value_.a_struct) (Ts...);
 *   }
 *
 *   // To get value const reference. We do not give non-const reference
 *   // because user can delete it without updating type_, breaking the
 *   // invariant.
 *   const a_struct& get_a_struct() const {
 *     assert(type_ == a_struct);
 *     return value_.a_struct;
 *   }
 *
 *   // This is to efficiently move value out of union
 *   a_struct move_a_struct() const {
 *     assert(type_ == a_struct);
 *     return std::move(value_.a_struct);
 *   }
 *
 *   Type getType() const {
 *     return type_;
 *   }
 *
 *   // Standard thrift read/write methods
 *
 *  protected:
 *   Type type_;
 *   storage_type value_;
 * };
 */
void t_cpp_generator::generate_cpp_union(t_struct* tstruct) {
  auto& out = f_types_;
  const vector<t_field*>& members = tstruct->get_members();

  string extends = " : public apache::thrift::TStructType<"
                   + tstruct->get_name() + ">";

  // Open struct def
  indent(out) << "class " << tstruct->get_name()
              << " : public apache::thrift::TStructType<" << tstruct->get_name()
              << "> {" << endl;
  indent(out) << " public:" << endl;
  indent_up();

  // Generate type enum
  indent(out) << "enum class Type {" << endl;
  indent_up();
    indent(out) << "__EMPTY__ = 0," << endl;
    for (auto& member: members) {
      auto name = member->get_name();
      auto key = member->get_key();
      indent(out) << name << " = " << key << "," << endl;
    }

  indent_down();
  indent(out) << "};" << endl << endl;

  // Default constructor, type_ == __EMPTY__ defines uninitialized UNION
  indent(out) << tstruct->get_name() << "() : type_(Type::__EMPTY__) {}"
              << endl;

  // Helper method to write switch cases
  auto writeSwitchCase = [&] (
      const string& switchOn,
      std::function<string(t_field*)> f,
      const string& defaultCase = "assert(false);") {
    indent(out) << "switch (" << switchOn << ") {" << endl;
    for (auto& member: members) {
      indent(out) << "  case Type::" << member->get_name() << ": {" << endl;
      indent(out) << "    " << f(member) << endl;
      indent(out) << "    break;" << endl;
      indent(out) << "  }" << endl;
    }
    indent(out) << "  default: " << defaultCase << endl;
    indent(out) << "}" << endl;
  };

  indent(out) << tstruct->get_name() << "(const " << tstruct->get_name()
              << "& rhs) : type_(Type::__EMPTY__) {" << endl;
  indent_up();
    indent(out) << "if (rhs.type_ == Type::__EMPTY__) { return; }" << endl;
    writeSwitchCase("rhs.type_", [] (t_field* f) {
      auto name = f->get_name();
      return "set_" + name + "(rhs.value_." + name + ");";
    });
  indent_down();
  indent(out) << "}" << endl << endl;

  indent(out) << tstruct->get_name() << "& operator=(const "
              << tstruct->get_name() << "& rhs) {" << endl;
  indent_up();
    indent(out) << "__clear();" << endl;
    indent(out) << "if (rhs.type_ == Type::__EMPTY__) { return *this; }"
                << endl;

    writeSwitchCase("rhs.type_", [] (t_field* f) {
      auto name = f->get_name();
      return "set_" + name + "(rhs.value_." + name + ");";
    });
    indent(out) << "return *this;" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;

  indent(out) << tstruct->get_name() << "(" << tstruct->get_name()
              << "&& rhs) : type_(Type::__EMPTY__) {" << endl;
  indent_up();
    indent(out) << "__clear();" << endl;
    indent(out) << "if (rhs.type_ == Type::__EMPTY__) { return; }" << endl;
    writeSwitchCase("rhs.type_", [] (t_field* f) {
      auto name = f->get_name();
      return "set_" + name + "(std::move(rhs.value_." + name + "));";
    });
    indent(out) << "rhs.__clear();" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;

  indent(out) << tstruct->get_name() << "& operator=("
              << tstruct->get_name() << "&& rhs) {" << endl;
  indent_up();
    indent(out) << "__clear();" << endl;
    indent(out) << "if (rhs.type_ == Type::__EMPTY__) { return *this; }"
                << endl;
    writeSwitchCase("rhs.type_", [] (t_field* f) {
      auto name = f->get_name();
      return "set_" + name + "(std::move(rhs.value_." + name + "));";
    });
    indent(out) << "rhs.__clear();" << endl;
    indent(out) << "return *this;" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;

  out << endl << indent() << "void __clear() {" << endl;
  indent_up();
    indent(out) << "if (type_ == Type::__EMPTY__) { return; }" << endl;
    writeSwitchCase("type_", [this] (t_field* f) {
      // One can not write
      //
      // value_.string_field.~std::string();
      //
      // as the compiler complains about '::' (I think the parsing breaks).
      // Therefore we add a 'using namespace' directive and then use
      //
      // value_.string_field.~string();

      string ret;
      auto type = f->get_type();
      if (type->is_base_type() && !type->is_string()) {
        return ret;
      }

      string nspace;
      auto tname = type_name(f->get_type());

      auto ltpos = tname.find_first_of('<');
      auto colonPos = tname.find_last_of(':', ltpos);
      if (colonPos != string::npos) {
        nspace = tname.substr(0, colonPos - 1);
        tname = tname.substr(colonPos + 1);
      }

      if (!nspace.empty()) {
        ret = "using namespace " + nspace + ";\n";
      }

      ret += "value_." + f->get_name() + ".~" + tname + "();\n";
      return ret;
    });
    indent(out) << "type_ = Type::__EMPTY__;" << endl;
  indent_down();
  indent(out) << "}" << endl;

  if (tstruct->annotations_.find("final") == tstruct->annotations_.end()) {
    indent(out) << "virtual ~" << tstruct->get_name() << "() throw() {" << endl;
    indent_up();
    indent(out) << "__clear();" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;
  }

  // Declare the storage type
  indent(out) << "union storage_type {" << endl;
  indent_up();
  for (auto& member: members) {
    indent(out) << type_name(member->get_type()) << " "
                << member->get_name() << ";" << endl;
  }
  indent(out) << endl;

  indent(out) << "storage_type() {}" << endl;
  indent(out) << "~storage_type() {}" << endl;
  indent_down();
  indent(out) << "};" << endl << endl;


  // Generate an equality testing operator.  Make it inline since the compiler
  // will do a better job than we would when deciding whether to inline it.
  indent(out) << "bool operator==(const " << tstruct->get_name() << "& rhs) "
                 "const {" << endl;
  indent_up();
    indent(out) << "if (type_ != rhs.type_) { return false; }" << endl;
    writeSwitchCase("type_", [] (t_field* f) {
      auto name = "value_." + f->get_name();
      return "return " + name + " == rhs." + name + ";\n";
    }, "return true;");
  indent_down();
  indent(out) << "}" << endl << endl;

  indent(out) << "bool operator!=(const " << tstruct->get_name() << "& rhs) "
              << "const {" << endl;
  indent_up();
    indent(out) << "return !(*this == rhs);" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;

  indent(out) << "bool operator<(const " << tstruct->get_name() << "& rhs) "
              << "const {" << endl;
  indent_up();
    indent(out) << "if (type_ != rhs.type_) return type_ < rhs.type_;" << endl;
    writeSwitchCase("type_", [] (t_field* f) {
      auto name = "value_." + f->get_name();
      return "return " + name + " < rhs." + name + ";\n";
    }, "return false;");
    indent(out) << "return false;" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;

  // Generate `set` methods
  for (auto& member: members) {
    indent(out) << "template<typename... T>" << endl;
    indent(out) << "void set_" << member->get_name() << "(T&&... t) {" << endl;
    indent(out) << "  __clear();" << endl;
    indent(out) << "  type_ = Type::" << member->get_name() << ";" << endl;
    indent(out) << "  new (&value_." << member->get_name() << ")"
                << " " << type_name(member->get_type())
                << "(std::forward<T>(t)...);" << endl;
    indent(out) << "}" << endl << endl;
  }

  // Generate `get` methods
  for (auto& member: members) {
    auto type = type_name(member->get_type());
    auto name = member->get_name();
    indent(out) << "const " << type << "& get_" << name << "() const {" << endl;
    indent(out) << "  assert(type_ == Type::" << member->get_name() << ");"
                << endl;
    indent(out) << "  return value_." << name << ";" << endl;
    indent(out) << "}" << endl << endl;
  }

  // Generate `mutable` methods
  for (auto& member: members) {
    auto type = type_name(member->get_type());
    auto name = member->get_name();
    indent(out) << type << "& mutable_" << name << "() {" << endl;
    indent(out) << "  assert(type_ == Type::" << member->get_name() << ");"
                << endl;
    indent(out) << "  return value_." << name << ";" << endl;
    indent(out) << "}" << endl << endl;
  }

  // Generate `move` methods
  for (auto& member: members) {
    auto type = type_name(member->get_type());
    auto name = member->get_name();
    indent(out) << type << " move_" << name << "() {" << endl;
    indent(out) << "  assert(type_ == Type::" << member->get_name() << ");"
                << endl;
    indent(out) << "  return std::move(value_." << name << ");" << endl;
    indent(out) << "}" << endl << endl;
  }

  // Method to know type
  indent(out) << "Type getType() const { return type_; }" << endl << endl;

  if (gen_json_) {
    out << indent() << "void readFromJson(const char* jsonText, size_t len);"
      << endl;
    out << indent() << "void readFromJson(const char* jsonText);" << endl;

    generate_union_json_reader(f_types_impl_, tstruct);
  }


  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t read(Protocol_* iprot);" << endl;

    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t write(Protocol_* oprot) const;" << endl;
  } else {
    out <<
      indent() << "uint32_t read(" <<
      "apache::thrift::protocol::TProtocol* iprot);" << endl;

    out <<
      indent() << "uint32_t write(" <<
      "apache::thrift::protocol::TProtocol* oprot) const;" << endl;
  }

  indent_down();

  // Declare all fields
  indent(out) << " private:" << endl;
  indent(out) << "  Type type_;" << endl;
  indent(out) << "  storage_type value_;" << endl << endl;
  indent(out) << "};" << endl << endl;

  generate_union_reader(gen_templates_ ? f_types_tcc_ : f_types_impl_, tstruct);
  generate_union_writer(gen_templates_ ? f_types_tcc_ : f_types_impl_, tstruct);

}

void t_cpp_generator::generate_union_json_reader(ofstream& out,
                                                 t_struct* tstruct) {
  string name = tstruct->get_name();
  const vector<t_field*>& members = tstruct->get_members();

  indent(out) << "void " << name
    << "::readFromJson(const char* jsonText, size_t len)" << endl;
  scope_up(out);
  indent(out) << "__clear();" << endl;
  indent(out) << "folly::dynamic parsed = "
    << "folly::parseJson(folly::StringPiece(jsonText, len));" << endl;

  indent(out) << "if (!parsed.isObject() || parsed.size() != 1) {" << endl;
  indent(out) << "  throw apache::thrift::TLibraryException("
              <<        "\"Can't parse " << name << "\");" << endl;
  indent(out) << "}" << endl << endl;

  for (auto& member: members) {
    auto elem = "parsed[\"" + member->get_name() + "\"]";
    indent(out) << "if (" << elem << " != nullptr) {" << endl;
    indent_up();
    indent(out) << "set_" << member->get_name() << "();" << endl;
    generate_json_field(out, member, "this->value_.", "", elem);
    indent_down();
    indent(out) << "}" << endl;
  }

  indent_down();
  indent(out) << "}" << endl;

  indent(out) << "void " << name << "::readFromJson(const char* jsonText)"
    << endl;
  scope_up(out);
  indent(out) << "readFromJson(jsonText, strlen(jsonText));" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;
}

void t_cpp_generator::generate_union_reader(ofstream& out,
                                            t_struct* tstruct) {

  const vector<t_field*>& members = tstruct->get_members();

  if (gen_templates_) {
    indent(out) << "template <class Protocol_>" << endl;
    indent(out) << "uint32_t " << tstruct->get_name()
                << "::read(Protocol_* iprot) {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::read(apache::thrift::protocol::TProtocol* iprot) {" << endl;
  }

  indent_up();

  bool can_throw = false;
  for (const auto& mem : members) {
    if (type_can_throw(mem->get_type())) {
      can_throw = true;
      break;
    }
  }

  if (can_throw) {
    indent(out) << "using apache::thrift::protocol::TProtocolException;"
                << endl;
    indent(out) << "std::exception_ptr exception;" << endl << endl;
  }

  indent(out) << "uint32_t xfer = 0;" << endl;
  indent(out) << "std::string fname;" << endl;
  indent(out) << "apache::thrift::protocol::TType ftype;" << endl;
  indent(out) << "int16_t fid;" << endl;

  indent(out) << "xfer += iprot->readStructBegin(fname);" << endl;
  indent(out) << "xfer += iprot->readFieldBegin(fname, ftype, fid);" << endl;

  indent(out) << "if (ftype == apache::thrift::protocol::T_STOP) {" << endl;
  indent(out) << "  __clear();" << endl;
  indent(out) << "} else {" << endl;
  indent_up();

  indent(out) << "switch (fid) {" << endl;
  indent_up();

  for (auto& member: members) {
    indent(out) << "case " << member->get_key() << ": {" << endl;
    indent_up();

    auto e = type_to_enum(member->get_type());
    indent(out) << "if (ftype == " << e << ") {" << endl;
    indent(out) << "  set_" << member->get_name() << "();" << endl;
    indent_up();
    generate_deserialize_field(out, member, "this->value_.");
    indent_down();
    indent(out) << "} else {" << endl;
    indent(out) << "xfer += iprot->skip(ftype);" << endl;
    indent(out) << "}" << endl;

    indent(out) << "break;" << endl;
    indent_down();
    indent(out) << "}" << endl;
  }

  indent(out) << "default: xfer += iprot->skip(ftype); break;" << endl;
  indent_down();
  indent(out) << "}" << endl;

  indent(out) << "xfer += iprot->readFieldEnd();" << endl;
  indent(out) << "xfer += iprot->readFieldBegin(fname, ftype, fid);" << endl;
  indent(out) << "xfer += iprot->readFieldEnd();" << endl;
  indent_down();
  indent(out) << "}" << endl;

  indent(out) << "xfer += iprot->readStructEnd();" << endl;

  // Throw if any required fields (in contained structs) are missing.
  // We do this after reading the struct end so that
  // there might possibly be a chance of continuing.
  out << endl;
  if (can_throw) {
    out << indent() << "if (exception != std::exception_ptr()) {" << endl;
    indent_up();
    out << indent() << "std::rethrow_exception(exception);" << endl;
    indent_down();
    out << indent() << "}" << endl;
  }
  for (const auto& mem : members) {
    if (mem->get_req() == t_field::T_REQUIRED ||
        mem->get_req() == t_field::T_OPTIONAL) {
      throw "compiler error: Union field " + tstruct->get_name() + "." +
            mem->get_name() + " cannot be required or optional";
    }
  }

  indent(out) << "return xfer;" << endl;

  indent_down();
  indent(out) << "}" << endl << endl;
}

void t_cpp_generator::generate_union_writer(ofstream& out,
                                            t_struct* tstruct) {

  string name = tstruct->get_name();
  const vector<t_field*>& members = tstruct->get_members();

  if (gen_templates_) {
    indent(out) << "template <class Protocol_>" << endl;
    indent(out) << "uint32_t " << tstruct->get_name()
                << "::write(Protocol_* oprot) const {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::write(apache::thrift::protocol::TProtocol* oprot) const {" << endl;
  }
  indent_up();

  indent(out) << "uint32_t xfer = 0;" << endl;

  indent(out) << "xfer += oprot->writeStructBegin(\"" << name << "\");" << endl;
  indent(out) << "switch (type_) {" << endl;
  indent_up();

  for (auto& member: members) {
    indent(out) << "case " << "Type::" << member->get_name() << ": {" << endl;
    indent_up();

    auto e = type_to_enum(member->get_type());
    indent(out) << "xfer += oprot->writeFieldBegin("
                << "\"" << member->get_name() << "\", " << e << ", "
                << member->get_key() << ");" << endl;

    generate_serialize_field(out, member, "this->value_.");

    indent(out) << "xfer += oprot->writeFieldEnd();" << endl;
    indent(out) << "break;" << endl;
    indent_down();
    indent(out) << "}" << endl;
  }

  // Add a case for __EMPTY__
  indent(out) << "case Type::__EMPTY__:;" << endl;

  indent_down();
  indent(out) << "}" << endl;

  indent(out) << "xfer += oprot->writeFieldStop();" << endl;
  indent(out) << "xfer += oprot->writeStructEnd();" << endl;
  indent(out) << "return xfer;" << endl;

  indent_down();
  indent(out) << "}" << endl << endl;
}

void t_cpp_generator::generate_struct_isset(ofstream& out,
                                            const vector<t_field*>& members,
                                            bool bitfields) {
  out <<
    endl <<
    indent() << "struct __isset {" << endl;
  indent_up();

    indent(out) <<
      "__isset() { __clear(); } " << endl;
    indent(out) << "void __clear() {" << endl;
    indent_up();
    for (t_field* field : members) {
      if (has_isset(field)) {
        indent(out) << field->get_name() << " = false;" << endl;
      }
    }
    indent_down();
    indent(out) << "}" << endl;

    for (t_field* field : members) {
      if (has_isset(field)) {
        indent(out) <<
          "bool " << field->get_name();
        if (bitfields) {
          out << " : 1";
        }
        out << ";" << endl;
      }
    }

  indent_down();
  indent(out) <<
    "} __isset;" << endl;
}

void t_cpp_generator::generate_copy_constructor(ofstream& out,
                                                t_struct* tstruct) {
  auto src = tmp("src");

  indent(out) << tstruct->get_name() << "::" << tstruct->get_name() << "(const "
              << tstruct->get_name() << "& " << src << ") {" << endl;
  indent_up();

  for (auto const& member : tstruct->get_members()) {
    auto name = member->get_name();
    if (is_reference(member)) {
      auto type = type_name(member->get_type());
      indent(out) << "if (" << src << "." << name << ")" << endl;
      indent_up();
      indent(out) << name << ".reset(new " << type << "(*" << src << "." << name
                  << "));" << endl;
      indent_down();
    } else {
      indent(out) << name << " = " << src << "." << name << ";" << endl;
    }
    if (has_isset(member)) {
      indent(out) << "__isset." << name << " = " << src << ".__isset." << name
                  << ";" << endl;
    }
  }

  indent_down();
  indent(out) << "}" << endl;
}

void t_cpp_generator::generate_equal_operator(ofstream& out,
                                              t_struct* tstruct) {
  auto src_name = tmp("rhs");
  indent(out) << "bool " << tstruct->get_name() << "::"
              << "operator == (const " << tstruct->get_name()
              << " & rhs) const {" << endl;
  indent_up();
  for (auto* member : tstruct->get_members()) {
    // Most existing Thrift code does not use isset or optional/required,
    // so we treat "default" fields as required.
    auto ref = is_reference(member);
    auto name = member->get_name();
    if (ref) {
      out << indent() << "if (bool(" << name << ") != bool(rhs." << name << "))"
          << endl;
      out << indent() << "  return false;" << endl;
      out << indent() << "else if (bool(" << name << ") && !(*" << name
          << " == *rhs." << name << "))" << endl;
      out << indent() << "  return false;" << endl;
    } else if (is_optional(member)) {
      out << indent() << "if (__isset." << name << " != rhs.__isset." << name
          << ")" << endl;
      out << indent() << "  return false;" << endl;
      out << indent() << "else if (__isset." << name << " && !(" << name
          << " == rhs." << name << "))" << endl;
      out << indent() << "  return false;" << endl;
    } else {
      out << indent() << "if (!(this->" << name << " == rhs." << name << "))"
          << endl;
      out << indent() << "  return false;" << endl;
    }
  }
  indent(out) << "return true;" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;
}

/**
 * Writes the struct definition into the header file
 *
 * @param out Output stream
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_definition(ofstream& out,
                                                 t_struct* tstruct,
                                                 bool is_exception,
                                                 bool pointers,
                                                 bool read,
                                                 bool write,
                                                 bool swap,
                                                 bool needs_copy_constructor) {
  string extends = "";
  if (is_exception) {
    extends = " : public apache::thrift::TExceptionType<"
                + tstruct->get_name() + ">";
  } else {
    extends = " : public apache::thrift::TStructType<"
                + tstruct->get_name() + ">";
  }

  if (swap) {
    // Generate a namespace-scope swap() function
    out <<
      indent() << "void swap(" << tstruct->get_name() << " &a, " <<
      tstruct->get_name() << " &b);" << endl <<
      endl;
  }
  // Open struct def
  out <<
    indent() << "class " << tstruct->get_name() << extends << " {" << endl <<
    indent() << " public:" << endl <<
    endl;
  indent_up();

  if (!bootstrap_) {
    out <<
      indent() << "static const uint64_t _reflection_id = " <<
      tstruct->get_type_id() << "U;" << endl <<
      indent() << "static void _reflection_register(" <<
      reflection_ns_prefix_ << "Schema&);" << endl;
  }

  // Get members
  vector<t_field*>::const_iterator m_iter;
  const vector<t_field*>& members = tstruct->get_members();

  // Isset struct has boolean fields, but only for non-required fields.
  bool has_nonrequired_fields = false;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if ((*m_iter)->get_req() != t_field::T_REQUIRED) {
      has_nonrequired_fields = true;
      break;
    }
  }
  const bool should_generate_isset =
    has_nonrequired_fields && (!pointers || read);

  if (!pointers) {
    // Default constructor
    indent(out) <<
      tstruct->get_name() << "()";

    bool init_ctor = false;

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      t_type* t = get_true_type((*m_iter)->get_type());
      if (t->is_base_type() || t->is_enum()) {
        t_const_value* cv = (*m_iter)->get_value();
        string dval = render_const_value(out, t, cv, true);
        if (!init_ctor) {
          init_ctor = true;
          out << " : ";
          out << (*m_iter)->get_name() << "(" << dval << ")";
        } else {
          out << ", " << (*m_iter)->get_name() << "(" << dval << ")";
        }
      }
    }
    out << " {" << endl;
    indent_up();
    // TODO(dreiss): When everything else in Thrift is perfect,
    // do more of these in the initializer list.
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      t_type* t = get_true_type((*m_iter)->get_type());

      if (!t->is_base_type()) {
        t_const_value* cv = (*m_iter)->get_value();
        if (cv != nullptr) {
          print_const_value(out, (*m_iter)->get_name(), t, cv);
        }
      }
    }
    scope_down(out);
  }

  if (!pointers &&
      is_exception &&
      tstruct->annotations_.find("message") != tstruct->annotations_.end() &&
      members.size() == 1) {
    // Constructor with message as parameter
    const string message = tstruct->annotations_.find("message")->second;

    indent(out) <<
      tstruct->get_name() << "(const std::string& msg) : " <<
        message << "(msg) {}";
  }

  if (!pointers &&
      tstruct->annotations_.find("cpp.nomove") ==
        tstruct->annotations_.end()) {
    // [CPP 11] Exlicitly specify defaulted move constructor / assignment
    // operator: according to the standard (12.8 [class.copy] / 9, 20)
    // virtual destructor that we may generate below prevents struct from
    // having implicitly declared defaulted move constructor / assignment
    // operator.
    out << endl;

    if (needs_copy_constructor) {
      indent(out) <<
        tstruct->get_name() << "(const " << tstruct->get_name() <<
        "&);" << endl;
    } else {
      indent(out) <<
        tstruct->get_name() << "(const " << tstruct->get_name() <<
        "&) = default;" << endl;
    }

    indent(out) << tstruct->get_name() << "& operator=(const "
                << tstruct->get_name() << "& src)";
    if (needs_copy_constructor) {
      out << " {" << endl;
      indent_up();
      indent(out) << tstruct->get_name() << " tmp(src);" << endl;
      indent(out) << "swap(*this, tmp);" << endl;
      indent(out) << "return *this;" << endl;
      indent_down();
      indent(out) << "}" << endl;
    } else {
      out << "= default;" << endl;
    }

    if (tstruct->annotations_.find("cpp.noexcept_move_ctor") ==
          tstruct->annotations_.end()) {
      indent(out) << tstruct->get_name() << "(" << tstruct->get_name() <<
        "&&) = default;" << endl;
    } else {
      // The default move ctor will have 'noexcept' annotation if all members
      // have move ctor with that annot. But it will not have this annotation
      // if any of its data members doesn't. Sometimes, user code always want
      // this annotation(for example in order to put into folly::MPMCQueue<>).
      // "cpp.noexcept_move_ctor" will tell the compiler to force gen customized
      // move ctor with 'noexcept' annotation.
      indent(out) << tstruct->get_name() << "(" << tstruct->get_name() <<
        "&& other) noexcept";
      std::string prefix = "  : ";
      for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        out << endl;
        indent(out) << prefix << (*m_iter)->get_name()
                    << "{std::move(other." << (*m_iter)->get_name() << ")}";
        prefix[2] = ',';
      }
      if (should_generate_isset) {
        out << endl;
        indent(out) << prefix << "__isset(other.__isset)";
      }
      out << " {" << endl;
      indent(out) << "}" << endl;
    }

    indent(out) << tstruct->get_name() << "& operator=(" <<
      tstruct->get_name() <<
      "&&) = default;" << endl;
  }

  if (!pointers) {
    out << endl << indent() << "void __clear();" << endl;
  }

  if (tstruct->annotations_.find("final") == tstruct->annotations_.end()) {
    out << endl << indent() << "virtual ~" << tstruct->get_name()
        << "() throw() {}" << endl << endl;
  }

  // Declare all fields
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    auto ref = is_reference(*m_iter);

    indent(out) <<
      declare_field(*m_iter, false, pointers &&
                    !get_true_type((*m_iter)->get_type())->is_xception(),
                    !read, false, ref) << endl;
  }

  if (should_generate_isset) {
    generate_struct_isset(out, members);
  }

  out << endl;

  if (!pointers) {
    indent(out) << "bool operator == (const " << tstruct->get_name()
                << " &) const;" << endl;
    indent(out) << "bool operator != (const " << tstruct->get_name()
                << "& rhs) const {" << endl;
    indent(out) << "  return !(*this == rhs);" << endl;
    indent(out) << "}" << endl << endl;

    // Generate the declaration of a less-than operator.  This must be
    // implemented by the application developer if they wish to use it.  (They
    // will get a link error if they try to use it without an implementation.)
    out <<
      indent() << "bool operator < (const "
               << tstruct->get_name() << " & ) const;" << endl << endl;
  }

  if (gen_json_) {
    out << indent() << "void readFromJson(const char* jsonText, size_t len);"
      << endl;
    out << indent() << "void readFromJson(const char* jsonText);" << endl;
  }
  if (read) {
    if (gen_templates_) {
      out <<
        indent() << "template <class Protocol_>" << endl <<
        indent() << "uint32_t read(Protocol_* iprot);" << endl;
    } else {
      out <<
        indent() << "uint32_t read(" <<
        "apache::thrift::protocol::TProtocol* iprot);" << endl;
    }
  }
  if (write) {
    if (gen_templates_) {
      out <<
        indent() << "template <class Protocol_>" << endl <<
        indent() << "uint32_t write(Protocol_* oprot) const;" << endl;
    } else {
      out <<
        indent() << "uint32_t write(" <<
        "apache::thrift::protocol::TProtocol* oprot) const;" << endl;
    }
  }
  out << endl;

  if (is_exception) {
    string what = "\"" + type_name(tstruct) + "\"";

    std::map<string, string>::iterator msg_it = tstruct->annotations_.find("message");
    if (msg_it != tstruct->annotations_.end()) {
      what = msg_it->second + ".c_str()";
    }

    out <<
      indent() << "virtual const char* what() const throw() {" << endl <<
      indent() << "  return " << what << ";" << endl <<
      indent() << "}" << endl << endl;
  }

  auto methods = tstruct->annotations_.find("cpp.methods");
  if (methods != tstruct->annotations_.end()) {
    out <<
      indent() << "// user defined code (cpp.methods = ...)" << endl <<
      methods->second << endl;
  }

  indent_down();
  indent(out) <<
    "};" << endl <<
    endl;

  out << indent() << "class " << tstruct->get_name() << ";" << endl;


  // Generate a namespace-scope merge() function
  out << indent() << "void merge("
      << "const " << tstruct->get_name() << "& from, "
      << tstruct->get_name() << "& to);" << endl;
  out << indent() << "void merge("
      << tstruct->get_name() << "&& from, "
      << tstruct->get_name() << "& to);" << endl;

  generate_hash_and_equal_to(out, tstruct, tstruct->get_name());
}

bool is_boolean_type(t_type* ttype) {
  if (ttype->is_base_type()) {
    t_base_type* btype = static_cast<t_base_type*>(ttype);
    if (t_base_type::TYPE_BOOL == btype->get_base()) {
      return true;
    }
  }
  return false;
}

/**
 * Frozen struct definition is simply a template specialization of the
 * Frozen<T> type:
 *
 * template<>
 * struct Frozen<NormalType> {...};
 *
 * Field types are defined strictly as:
 *   typename Freezer<FieldType>::FrozenType fieldName;
 * with the exception of booleans, which are defined as 1-bit fields, like:
 *   bool boolFieldName : 1;
 *
 * __isset is defined as usual, though also uses bitfields to minimize storage.
 *
 * Freezer struct definition is a specialization of the Freezer<T> helper class
 * (See Frozen.h)
 */
void t_cpp_generator::generate_frozen_struct_definition(t_struct* tstruct) {
  string fullName = ns_prefix_ + tstruct->get_name();
  string freezerName = "Freezer<" + fullName + ", void>";
  string frozenName = "Frozen<" + fullName + ", void>";
  // Switch to apache::thrift ns
  f_types_ <<
    ns_close_ << endl <<
    "namespace apache { namespace thrift {" <<
    endl;
  // Frozen Type
  f_types_ << endl;
  if (frozen_packed_) {
    f_types_ << "#pragma pack(push, 1)" << endl;
  }
  f_types_ <<
    "template<>" << endl <<
    "struct " << frozenName << " {" <<
    endl;
  indent_up();
  // Members
  const vector<t_field*>& members = tstruct->get_members();

  // Isset struct has boolean fields, but only for non-required fields.
  bool has_nonrequired_fields = false;
  for (t_field* field : members) {
    indent(f_types_) <<
      declare_frozen_field(field) <<
      endl;
    if (field->get_req() != t_field::T_REQUIRED) {
      has_nonrequired_fields = true;
    }
  }

  if (has_nonrequired_fields) {
    generate_struct_isset(f_types_, members, true);
  }

  indent_down();
  f_types_ <<
    "};" << // struct frozenName
    endl;
  if (frozen_packed_) {
    f_types_ << "#pragma pack(pop)" << endl;
  }
  f_types_ <<
    "}} // apache::thrift " << endl << endl <<
    ns_open_ << endl;

  // .cpp
  // extraSizeImpl
  f_types_impl_ <<
    ns_close_ << endl <<
    "namespace apache { namespace thrift {" <<
    endl;
  f_types_impl_ <<
    "template<>" << endl <<
    "size_t " << freezerName << "::extraSizeImpl(" <<
    endl;
  indent_up();
  indent_up();
  indent(f_types_impl_) << "const " << freezerName << "::ThawedType& src) {" <<
    endl;
  indent_down();
  f_types_impl_ << indent() << "return 0";
  indent_up();
  for (t_field* field : members) {
    if (!is_boolean_type(field->get_type())) {
      f_types_impl_ << endl <<
        indent() << "+ extraSize(src." << field->get_name() << ")";
    }
  }
  f_types_impl_ << ";" << endl;
  indent_down();
  indent_down();
  f_types_impl_ << indent() << "}" << endl;

  // freezeImpl
  f_types_impl_ << endl <<
    "template<>" << endl <<
    "void " << freezerName << "::freezeImpl(" <<
    endl;
  indent_up();
  indent_up();
  f_types_impl_ <<
    indent() << "const " << freezerName << "::ThawedType& src," << endl <<
    indent() << freezerName << "::FrozenType& dst," << endl <<
    indent() << "byte*& buffer) {" << endl;
  indent_down();
  for (t_field* field : members) {
    const string& fname = field->get_name();

    if (is_boolean_type(field->get_type())) {
      indent(f_types_impl_) << "dst." << fname << " = src." << fname << ";" <<
        endl;
    } else {
      indent(f_types_impl_) <<
        "freeze(" << "src." << fname << ", " <<
                     "dst." << fname << ", " <<
                     "buffer);" << endl;
    }
    if (has_isset(field)) {
      indent(f_types_impl_) <<
        "dst.__isset." << fname << " = src.__isset." << fname << ";" <<
        endl;
    }
  }
  indent_down();
  f_types_impl_ << indent() << "}" << endl;

  // thawImpl
  f_types_impl_ << endl <<
    "template<>" << endl <<
    "void " << freezerName << "::thawImpl(" <<
    endl;
  indent_up();
  indent_up();
  f_types_impl_ <<
    indent() << "const " << freezerName << "::FrozenType& src," << endl <<
    indent() << freezerName << "::ThawedType& dst) {" << endl;
  indent_down();
  for (const t_field* field : members) {
    const string& fname = field->get_name();
    if (is_boolean_type(field->get_type())) {
      indent(f_types_impl_) << "dst." << fname << " = src." << fname << ";" <<
        endl;
    } else {
      indent(f_types_impl_) <<
        "thaw(" << "src." << fname << ", " << "dst." << fname << ");" << endl;
    }
    if (has_isset(field)) {
      indent(f_types_impl_) <<
        "dst.__isset." << fname << " = " << "src.__isset." << fname << ";" <<
        endl;
    }
  }
  indent_down();
  f_types_impl_ << indent() << "}" << endl;

  f_types_impl_ <<
    "}} // apache::thrift " << endl << endl <<
    ns_open_ << endl;
}

/**
 * Generate Frozen2 Layout specializations (see cpp2/frozen/Frozen.h).
 */
void t_cpp_generator::generate_frozen2_struct_definition(t_struct* tstruct) {
  string structName = type_name(tstruct,  ALWAYS_NAMESPACE);
  auto members = tstruct->get_members();
  auto optSuffix = [](t_field::e_req req)->const char * {
    switch (req) {
    case t_field::T_OPTIONAL:
      return "_OPT";
    case t_field::T_REQUIRED:
      return "_REQ";
    default:
      return "";
    }
  };

  std::sort(members.begin(),
            members.end(),
            [](const t_field* a,
               const t_field* b) {
              return a->get_key() < b->get_key();
            });

  auto emitFieldFormat = [&](
      ostream& os, const string& format, const t_field* f) {
    os << folly::vformat(
              format,
              map<string, string>{
                  {"name", f->get_name()},
                  {"type", type_name(f->get_type(), ALWAYS_NAMESPACE)},
                  {"id", folly::to<string>(f->get_key())},
                  {"_opt", optSuffix(f->get_req())}});
  };

  // Header
  f_types_layouts_ << endl;
  f_types_layouts_ << folly::format("FROZEN_TYPE({},", structName);
  for (const t_field* field : members) {
    emitFieldFormat(f_types_layouts_,
                    "\n  FROZEN_FIELD{_opt}({name}, {id}, {type})",
                    field);
  }
  f_types_layouts_ << "\n  FROZEN_VIEW(";
  for (const t_field* field : members) {
    emitFieldFormat(f_types_layouts_,
                    "\n    FROZEN_VIEW_FIELD{_opt}({name}, {type})",
                    field);
  }
  f_types_layouts_ << "));" << endl;

  // Implementation
  f_types_layouts_impl_ << endl;
  for (auto step : {make_pair("CTOR", "CTOR_FIELD{_opt}({name}, {id})"),
                    make_pair("LAYOUT", "LAYOUT_FIELD{_opt}({name})"),
                    make_pair("FREEZE", "FREEZE_FIELD{_opt}({name})"),
                    make_pair("THAW", "THAW_FIELD{_opt}({name})"),
                    make_pair("DEBUG", "DEBUG_FIELD({name})"),
                    make_pair("CLEAR", "CLEAR_FIELD({name})"),
                    make_pair("SAVE", "SAVE_FIELD({name})"),
                    make_pair("LOAD", "LOAD_FIELD({name}, {id})")}) {
    f_types_layouts_impl_
        << folly::format("FROZEN_{}({},", step.first, structName);
    for (const t_field* field : members) {
      emitFieldFormat(
          f_types_layouts_impl_, string("\n  FROZEN_") + step.second, field);
    }
    f_types_layouts_impl_ << ");\n";
  }
}

void t_cpp_generator::generate_json_field(ofstream& out,
                                          t_field* tfield,
                                          const string& prefix_thrift,
                                          const string& suffix_thrift,
                                          const string& prefix_json) {
  t_type* f_type = tfield->get_type();
  t_type* type = get_true_type(f_type);

  if (type->is_void()) {
    throw "CANNOT READ JSON FIELD WITH void TYPE: " +
      prefix_thrift + tfield->get_name();
  }

  string name = prefix_thrift + tfield->get_name() +
      get_type_access_suffix(f_type) + suffix_thrift;

  if (type->is_struct() || type->is_xception()) {
    generate_json_struct(out,
        (t_struct*)type,
        name,
        prefix_json,
        is_reference(tfield));
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
    string asTypeString = "";
    string typeConversionString = "";
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    string number_limit = "";
    switch (tbase) {
      case t_base_type::TYPE_VOID:
        break;
      case t_base_type::TYPE_STRING:
        asTypeString = "String().toStdString()";
        break;
      case t_base_type::TYPE_BOOL:
        asTypeString = "Bool()";
        typeConversionString = "";
        break;
      case t_base_type::TYPE_BYTE:
        number_limit = "0x7f";
        asTypeString = "Int()";
        typeConversionString = "(int8_t)";
        break;
      case t_base_type::TYPE_I16:
        number_limit = "0x7fff";
        asTypeString = "Int()";
        typeConversionString = "(int16_t)";
        break;
      case t_base_type::TYPE_I32:
        number_limit = "0x7fffffffL";
        asTypeString = "Int()";
        typeConversionString = "(int32_t)";
        break;
      case t_base_type::TYPE_I64:
        asTypeString = "Int()";
        typeConversionString = "(int64_t)";
        break;
      case t_base_type::TYPE_DOUBLE:
        asTypeString = "Double()";
        break;
      case t_base_type::TYPE_FLOAT:
        asTypeString = "Double()";
        typeConversionString = "(float)";
        break;
      default:
        throw "compiler error: no C++ reader for base type "
          + t_base_type::t_base_name(tbase) + name;
    }

    if (number_limit.empty()) {
      indent(out) <<  name << " = " << typeConversionString <<
        prefix_json << ".as" << asTypeString << ";" <<
        endl;
    } else {
      string temp = tmp("_tmp");
      indent(out) <<  "int64_t " << temp << " = (int64_t)"
        << prefix_json << ".as" << asTypeString << ";" <<
        endl;
      indent(out) << "if (imaxabs(" << temp << ") > " << number_limit <<
        ") {" <<endl;
      indent_up();
      indent(out) << "throw apache::thrift::TLibraryException"
        << "(\"number exceeds limit in field\");"
        <<endl;
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

void t_cpp_generator::generate_json_enum(ofstream& out,
     t_enum* tenum,
     const string& prefix_thrift,
     const string& prefix_json) {

      string temp = tmp("_tmp");
      indent(out) <<  prefix_thrift << "=" <<  "(" + type_name(tenum) + ")" <<
      "(int32_t)" << prefix_json << ".asInt()"  << ";" << endl;
}

void t_cpp_generator::generate_json_struct(ofstream& out,
    t_struct* tstruct,
    const string& prefix_thrift,
    const string& prefix_json,
    bool dereference) {

  auto ref = ".";
  if (dereference) {
    ref = "->";
  }

  indent(out) << prefix_thrift << ref << "readFromJson(folly::toJson("
    << prefix_json << ").toStdString().c_str());" << endl;
}

void t_cpp_generator::generate_json_container(ofstream& out,
    t_type* ttype,
    const string& prefix_thrift,
    const string& prefix_json) {

  string size = tmp("_size");
  string i = tmp("_i");
  string json = tmp("_json");

  t_container* tcontainer = (t_container*)ttype;
  // One of them at least is != annotations_.end()
  bool use_push = tcontainer->annotations_.find("cpp.type")
    != tcontainer->annotations_.find("cpp.template");

  if (ttype->is_list()) {

    indent(out) << "folly::dynamic " << json << " = " << prefix_json <<
      ";" << endl;
    indent(out) <<
      prefix_thrift << ".clear();" << endl <<
      indent() << "uint32_t " << size << " = " << json << ".size();" << endl;

    if (ttype->is_list() && !use_push) {
      indent(out) << prefix_thrift << ".resize(" << size << ");" << endl;
    } else if (ttype->is_map() && ((t_map*)ttype)->is_unordered()) {
      indent(out) << prefix_thrift << ".reserve(" << size << ");" << endl;
    }

    out << indent() << "for (uint32_t " << i << " = 0; " << i << " < " << size
      << "; ++" << i << ")" << endl;
    scope_up(out);
    generate_json_list_element(out, (t_list*)ttype, use_push, i,
                               prefix_thrift, json + "[" + i + "]");
    scope_down(out);

  } else if (ttype->is_set()) {

    indent(out) << "folly::dynamic " << json << " = " << prefix_json << ";"
      << endl;
    indent(out) <<
      prefix_thrift << ".clear();" << endl;
    indent(out) << "uint32_t " << size << " = " << json << ".size();" << endl;

    out << indent() << "for (uint32_t " << i << " = 0; " << i << " < " << size
      << "; ++" << i << ")" << endl;
    scope_up(out);
    generate_json_set_element(out, (t_set*)ttype,
                              prefix_thrift, json + "[" + i + "]");
    scope_down(out);

  } else if (ttype->is_map()) {
    t_type* key_type = get_true_type(((t_map*)ttype)->get_key_type());
    if (!(key_type->is_base_type() || key_type->is_enum())) {
      return;
    }
    indent(out) << "folly::dynamic " << json << " = " << prefix_json  <<
      ";" << endl;
    indent(out) << prefix_thrift << ".clear();" << endl;
    string iter = tmp("_iter");
    indent(out) << "for (folly::dynamic::const_item_iterator " << iter << " = "
      << json << ".items().begin(); " << iter << " != "
      << json << ".items().end(); " << iter << "++)";
    scope_up(out);
    generate_json_map_element(out, (t_map*)ttype,
                              "(" + iter
                                + ")->first.asString().toStdString()",
                              iter + "->second",
                              prefix_thrift);
    scope_down(out);
  }
}

void t_cpp_generator::generate_json_set_element(ofstream& out,
                                                t_set* tset,
                                                const string& prefix_thrift,
                                                const string& prefix_json) {
  string elem = tmp("_elem");
  t_field felem(tset->get_elem_type(), elem);
  indent(out) << declare_field(&felem) << endl;
  generate_json_field(out, &felem, "", "", prefix_json);
  indent(out) << prefix_thrift << ".insert(" << elem << ");" << endl;
}

void t_cpp_generator::generate_json_list_element(ofstream& out,
                                                 t_list* tlist,
                                                 bool use_push,
                                                 const string& i,
                                                 const string& prefix_thrift,
                                                 const string& prefix_json) {
  if (use_push) {
    string elem = tmp("_elem");
    t_field felem(tlist->get_elem_type(), elem);
    indent(out) << declare_field(&felem) << endl;
    generate_json_field(out, &felem, "", "", prefix_json);
    indent(out) << prefix_thrift << ".push_back(" << elem << ");" << endl;
  } else {
    t_field felem(tlist->get_elem_type(), prefix_thrift + "[" + i + "]");
    generate_json_field(out, &felem, "", "", prefix_json);
  }
}

void t_cpp_generator::generate_json_map_element(ofstream& out,
                                                t_map* tmap,
                                                const string& key,
                                                const string& value,
                                                const string& prefix_thrift) {
  string _key = tmp("_key");
  string _val = tmp("_val");
  t_field kval(tmap->get_key_type(), _key);
  t_field fval(tmap->get_val_type(), _val);
  t_type* key_type = get_true_type(tmap->get_key_type());
  out << indent() << declare_field(&kval) << ";" << endl;
  out << indent() << declare_field(&fval) << ";" << endl;
  if (key_type->is_string()) {
    out <<
      indent() << _key << " = " << key << ";" << endl;
  } else if (key_type->is_bool()) {
    out <<
      indent() << "if (" << key << " == \"true\") {" << endl <<
      indent() << "  " << _key << " = true;" << endl <<
      indent() << "} else if (" << key << " == \"false\") {" << endl <<
      indent() << "  " << _key << " = false;" << endl <<
      indent() << "} else {" << endl <<
      indent() << "  throw apache::thrift::TLibraryException"
               << "(\"invalid boolean value\");" << endl <<
      indent() << "}" << endl;
  } else if (key_type->is_enum()) {
    out <<
      indent() << "try {" << endl <<
      indent() << "  " << _key << " = (" << type_name(key_type)
               << ")folly::to<int>(" << key << ");" << endl <<
      indent() << "} catch (std::range_error& ex) {" << endl <<
      indent() << "  throw apache::thrift::TLibraryException(ex.what());"
               << endl <<
      indent() << "}" << endl;
  } else if (key_type->is_base_type()) {
    out <<
      indent() << "try {" << endl <<
      indent() << "  " << _key << " = folly::to<" << type_name(key_type)
               << ">(" << key << ");" << endl <<
      indent() << "} catch (std::range_error& ex) {" << endl <<
      indent() << "  throw apache::thrift::TLibraryException(ex.what());"
               << endl <<
      indent() << "}" << endl;
  } else {
    throw string("Unexpected key type in generate_json_map_element");
  }
  generate_json_field(out, &fval, "", "", value);

  indent(out) << prefix_thrift << "[" << _key << "] = " << _val << ";" << endl;
}

void t_cpp_generator::generate_json_reader(ofstream& out,
    t_struct* tstruct) {
  if (!gen_json_) {
    return;
  }
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  string name = tstruct->get_name();

  indent(out) << "void " << name
    << "::readFromJson(const char* jsonText, size_t len)" << endl;
  scope_up(out);
  indent(out) << "folly::dynamic parsed = "
    << "folly::parseJson(folly::StringPiece(jsonText, len));" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    indent(out) << "if (parsed[\"" << (*f_iter)->get_name()
      << "\"] != nullptr) {" << endl;
    indent_up();
    generate_json_field(out, *f_iter, "this->", "",
                        "parsed[\"" + (*f_iter)->get_name() + "\"]");

    if (has_isset(*f_iter)) {
      indent(out) << "this->__isset." << (*f_iter)->get_name() << " = true;"
        << endl;
    }
    indent_down();
    indent(out) << "}";
    if ((*f_iter)->get_req() == t_field::T_REQUIRED) {
      out << " else {" << endl;
      indent_up();
      indent(out) << "throw apache::thrift::TLibraryException"
        << "(\"can't parse a required field!\");"
        << endl;
      indent_down();
      indent(out) << "}" << endl;
    } else if (has_isset(*f_iter)) {
      out << " else {" << endl;
      indent_up();
      indent(out) << "this->__isset." << (*f_iter)->get_name() << " = false;"
        << endl;
      indent_down();
      indent(out) << "}" <<endl;
    }
  }
  indent_down();
  indent(out) << "}" << endl;

  indent(out) << "void " << name << "::readFromJson(const char* jsonText)"
    << endl;
  scope_up(out);
  indent(out) << "readFromJson(jsonText, strlen(jsonText));" << endl;
  indent_down();
  indent(out) << "}" << endl << endl;
}

bool t_cpp_generator::type_can_throw(const t_type* type) {
  std::set<const t_type*> visited;
  return type_can_throw(type, visited);
}

bool t_cpp_generator::type_can_throw(const t_type* type,
                                     std::set<const t_type*>& visited) {
  if (visited.find(type) != visited.end()) {
    return false;
  }
  visited.insert(type);

  type = get_true_type(type);

  if (type->is_list()) {
    return type_can_throw(static_cast<const t_list*>(type)->get_elem_type(),
                          visited);
  } else if (type->is_set()) {
    return type_can_throw(static_cast<const t_set*>(type)->get_elem_type(),
                          visited);
  } else if (type->is_map()) {
    const t_map* map = static_cast<const t_map*>(type);
    return type_can_throw(map->get_key_type(), visited) ||
      type_can_throw(map->get_val_type(), visited);
  } else if (type->is_struct()) {
    const t_struct* tstruct = static_cast<const t_struct*>(type);
    const vector<t_field*>& fields = tstruct->get_members();
    for (auto f_iter : fields) {
      if (f_iter->get_req() == t_field::T_REQUIRED) {
        return true;
      } else if (type_can_throw(f_iter->get_type(), visited)) {
        return true;
      }
    }
    return false;
  }
  return false;
}

void t_cpp_generator::generate_struct_clear(ofstream& out,
    t_struct* tstruct,
    bool pointers) {
  if (!pointers) {
    indent(out) <<
      "void " << tstruct->get_name() <<
      "::__clear() {" << endl;

    indent_up();
    vector<t_field*>::const_iterator m_iter;
    const vector<t_field*>& members = tstruct->get_members();

    // Isset struct has boolean fields, but only for non-required fields.
    bool has_nonrequired_fields = false;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if ((*m_iter)->get_req() != t_field::T_REQUIRED) {
        has_nonrequired_fields = true;
        break;
      }
    }

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      t_type* f_type = (*m_iter)->get_type();
      t_type* t = get_true_type(f_type);
      string name = (*m_iter)->get_name() + get_type_access_suffix(f_type);
      if (t->is_base_type() || t->is_enum()) {
        t_const_value* cv = (*m_iter)->get_value();
        string dval = render_const_value(out, t, cv, true);
        indent(out) << name << " = " << dval << ";" << endl;
      } else if (t->is_struct() || t->is_xception()) {
        auto ref = is_reference(*m_iter);
        if (ref) {
          indent(out) << "if (" << name << ") " << name << "->__clear();" << endl;
        } else {
          indent(out) << name << ".__clear();" << endl;
        }
      } else if (t->is_container()) {
        indent(out) << name << ".clear();" << endl;
      } else {
        throw "UNKNOWN TYPE for member: " + name;
      }
    }
    if (has_nonrequired_fields && (!pointers)) {
      indent(out) << "__isset.__clear();" << endl;
    }
    indent_down();
    indent(out) << "}" << endl;
  }
}

/**
 * Makes a helper function to gen a struct reader.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_reader(ofstream& out,
    t_struct* tstruct,
    bool pointers) {
  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t " << tstruct->get_name() <<
      "::read(Protocol_* iprot) {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::read(apache::thrift::protocol::TProtocol* iprot) {" << endl;
  }
  indent_up();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  bool can_throw = false;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (type_can_throw((*f_iter)->get_type())) {
      can_throw = true;
      break;
    }
  }

  // Declare stack tmp variables
  out <<
    endl <<
    indent() << "uint32_t xfer = 0;" << endl <<
    indent() << "std::string fname;" << endl <<
    indent() << "apache::thrift::protocol::TType ftype;" << endl <<
    indent() << "int16_t fid;" << endl <<
    endl;

  if (!bootstrap_) {
    out <<
      indent() << reflection_ns_prefix_ <<
          "Schema * schema = iprot->getSchema();" << endl <<
      indent() << "if (schema != nullptr) {" << endl;

      indent_up();
      std::string initializer = generate_reflection_initializer_name(tstruct);
      out <<
        indent() << ns_reflection_prefix_ << initializer << "(*schema);" <<
          endl <<
        indent() << "iprot->setNextStructType(" <<
            tstruct->get_name() << "::_reflection_id);" << endl;
      indent_down();
      out <<
        indent() << "}" << endl;
  }

  out <<
    indent() << "xfer += iprot->readStructBegin(fname);" << endl <<
    endl <<
    indent() << "using apache::thrift::protocol::TProtocolException;" << endl <<
    endl;
  if (can_throw) {
    out << indent() << "std::exception_ptr exception;" << endl;
  }
  out << endl;

  // Required variables aren't in __isset, so we need tmp vars to check them.
  for (auto* field : fields) {
    if (is_required(field)) {
      indent(out) << "bool isset_" << field->get_name() << " = false;" << endl;
    }
  }
  out << endl;


  // Loop over reading in fields
  indent(out) <<
    "while (true)" << endl;
    scope_up(out);

    // Read beginning field marker
    indent(out) <<
      "xfer += iprot->readFieldBegin(fname, ftype, fid);" << endl;

    // Check for field STOP marker
    out <<
      indent() << "if (ftype == apache::thrift::protocol::T_STOP) {" << endl <<
      indent() << "  break;" << endl <<
      indent() << "}" << endl;

    // Switch statement on the field we are reading
    indent(out) <<
      "switch (fid)" << endl;

      scope_up(out);

      // Generate deserialization code for known cases
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        indent(out) <<
          "case " << (*f_iter)->get_key() << ":" << endl;
        indent_up();
        indent(out) <<
          "if (ftype == " << type_to_enum((*f_iter)->get_type()) << ") {" << endl;
        indent_up();

        const char* isset_prefix =
            has_isset(*f_iter) ? "this->__isset." : "isset_";

#if 0
        // This code throws an exception if the same field is encountered twice.
        // We've decided to leave it out for performance reasons.
        // TODO(dreiss): Generate this code and "if" it out to make it easier
        // for people recompiling thrift to include it.
        out <<
          indent() << "if (" << isset_prefix << (*f_iter)->get_name() << ")" << endl <<
          indent() << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl;
#endif

        if (pointers && !get_true_type((*f_iter)->get_type())->is_xception()) {
          generate_deserialize_field(out, *f_iter, "(*(this->", "))");
        } else {
          generate_deserialize_field(out, *f_iter, "this->");
        }
        if (has_isset(*f_iter) || is_required(*f_iter)) {
          out << indent() << isset_prefix << (*f_iter)->get_name() << " = true;"
              << endl;
        }
        indent_down();
        out <<
          indent() << "} else {" << endl <<
          indent() << "  xfer += iprot->skip(ftype);" << endl <<
          // TODO(dreiss): Make this an option when thrift structs
          // have a common base class.
          // indent() << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl <<
          indent() << "}" << endl <<
          indent() << "break;" << endl;
        indent_down();
      }

      // In the default case we skip the field
      out <<
        indent() << "default:" << endl <<
        indent() << "  xfer += iprot->skip(ftype);" << endl <<
        indent() << "  break;" << endl;

      scope_down(out);

    // Read field end marker
    indent(out) <<
      "xfer += iprot->readFieldEnd();" << endl;

    scope_down(out);

  out <<
    endl <<
    indent() << "xfer += iprot->readStructEnd();" << endl;

  // Throw if any required fields are missing.
  // We do this after reading the struct end so that
  // there might possibly be a chance of continuing.
  out << endl;
  if (can_throw) {
    out << indent() << "if (exception != std::exception_ptr()) {" << endl;
    indent_up();
    out << indent() << "std::rethrow_exception(exception);" << endl;
    indent_down();
    out << indent() << "}" << endl;
  }
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED)
      out <<
        indent() << "if (!isset_" << (*f_iter)->get_name() << ')' << endl <<
        indent() << "  throw TProtocolException(" <<
          "TProtocolException::MISSING_REQUIRED_FIELD, \"" <<
          "Required field '" << (*f_iter)->get_name() <<
          "' was not found in serialized data! Struct: " <<
          tstruct->get_name() << "\");" << endl;
  }

  indent(out) << "return xfer;" << endl;

  indent_down();
  indent(out) <<
    "}" << endl << endl;
}

/**
 * Generates a terse write predicate - checks if the value
 * has changed from its initial value.
 */
bool t_cpp_generator::try_terse_write_predicate(
    ofstream& out, t_field* tfield, bool pointers, TerseWrites terse_writes,
    string& predicate) {
  if (terse_writes == TW_DISABLED) {
    return false;
  }

  // Only do terse writes for fields where required/optional isn't specified.
  if (tfield->get_req() == t_field::T_REQUIRED ||
      tfield->get_req() == t_field::T_OPTIONAL) {
    return false;
  }
  t_type* type = get_true_type(tfield->get_type());
  t_const_value* tval = tfield->get_value();

  // Terse write is unsafe to use without explicitly setting default value,
  // as in PHP / Python that would change result of deserialization (comparing
  // with the case when terse_writes is not used): field set in C++ to default
  // value would be deserialized as null / None.
  if (terse_writes == TW_SAFE && tval == nullptr) {
    return false;
  }

  if (type->is_struct() || type->is_xception() ||
      // no support for void.
      (type->is_base_type() && ((t_base_type*)type)->is_void()) ||
      // only support string, if default empty.
      (type->is_base_type() && ((t_base_type*)type)->is_string() &&
       tval != nullptr && !tval->get_string().empty()) ||
      // only support container, if default empty.
      (type->is_container() && tval != nullptr &&
       ((tval->get_type() == t_const_value::CV_LIST &&
         !tval->get_list().empty()) ||
        (tval->get_type() == t_const_value::CV_MAP &&
         !tval->get_map().empty())))
      ) {
    return false;
  }

  // Containers -> "if (!x.empty())"
  if (type->is_container() ||
      (type->is_base_type() && ((t_base_type*)type)->is_string())) {
    predicate = "!this->" + tfield->get_name() +
      (pointers ? "->empty()" : ".empty()");
    return true;
  }
  // ints, enum -> "if (x != default value)
  if (type->is_base_type() || type->is_enum()) {
    predicate = (pointers ? "*(this->" : "this->") +
      tfield->get_name() + (pointers ? ") != " : " != ") +
      render_const_value(out, type, tval, true);
    return true;
  }
  return false;
}

/**
 * Generates the write function.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_writer(ofstream& out,
                                             t_struct* tstruct,
                                             bool pointers) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;
  string predicate;
  const TerseWrites terse_writes =
    std::max(terse_writes_, parseTerseWrites(tstruct->annotations_));

  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t " << tstruct->get_name() <<
      "::write(Protocol_* oprot) const {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::write(apache::thrift::protocol::TProtocol* oprot) const {" << endl;
  }
  indent_up();

  out <<
    indent() << "uint32_t xfer = 0;" << endl;

  indent(out) <<
    "xfer += oprot->writeStructBegin(\"" << name << "\");" << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    bool needs_closing_brace = false;
    if (is_optional(*f_iter) && has_isset(*f_iter)) {
      indent(out) << "if (this->__isset." << (*f_iter)->get_name() << ") {" << endl;
      indent_up();
      needs_closing_brace = true;
    } else if (try_terse_write_predicate(out, *f_iter, pointers, terse_writes,
                                         predicate)) {
      indent(out) << "if (" << predicate << ") {" << endl;
      indent_up();
      needs_closing_brace = true;
    }
    // Write field header
    out <<
      indent() << "xfer += oprot->writeFieldBegin(" <<
      "\"" << (*f_iter)->get_name() << "\", " <<
      type_to_enum((*f_iter)->get_type()) << ", " <<
      (*f_iter)->get_key() << ");" << endl;
    // Write field contents
    if (pointers && !get_true_type((*f_iter)->get_type())->is_xception()) {
      generate_serialize_field(out, *f_iter, "(*(this->", "))");
    } else {
      generate_serialize_field(out, *f_iter, "this->");
    }
    // Write field closer
    indent(out) <<
      "xfer += oprot->writeFieldEnd();" << endl;
    if (needs_closing_brace) {
      indent_down();
      indent(out) << '}' << endl;
    }
  }

  // Write the struct map
  out <<
    indent() << "xfer += oprot->writeFieldStop();" << endl <<
    indent() << "xfer += oprot->writeStructEnd();" << endl <<
    indent() << "return xfer;" << endl;

  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

/**
 * Struct writer for result of a function, which can have only one of its
 * fields set and does a conditional if else look up into the __isset field
 * of the struct.
 *
 * @param out Output stream
 * @param tstruct The result struct
 */
void t_cpp_generator::generate_struct_result_writer(ofstream& out,
                                                    t_struct* tstruct,
                                                    bool pointers) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t " << tstruct->get_name() <<
      "::write(Protocol_* oprot) const {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::write(apache::thrift::protocol::TProtocol* oprot) const {" << endl;
  }
  indent_up();

  out <<
    endl <<
    indent() << "uint32_t xfer = 0;" << endl <<
    endl;

  indent(out) <<
    "xfer += oprot->writeStructBegin(\"" << name << "\");" << endl;

  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
      out <<
        endl <<
        indent() << "if ";
    } else {
      out <<
        " else if ";
    }

    out << "(this->__isset." << (*f_iter)->get_name() << ") {" << endl;

    indent_up();

    // Write field header
    out <<
      indent() << "xfer += oprot->writeFieldBegin(" <<
      "\"" << (*f_iter)->get_name() << "\", " <<
      type_to_enum((*f_iter)->get_type()) << ", " <<
      (*f_iter)->get_key() << ");" << endl;
    // Write field contents
    if (pointers && !get_true_type((*f_iter)->get_type())->is_xception()) {
      generate_serialize_field(out, *f_iter, "(*(this->", "))");
    } else {
      generate_serialize_field(out, *f_iter, "this->");
    }
    // Write field closer
    indent(out) << "xfer += oprot->writeFieldEnd();" << endl;

    indent_down();
    indent(out) << "}";
  }

  // Write the struct map
  out <<
    endl <<
    indent() << "xfer += oprot->writeFieldStop();" << endl <<
    indent() << "xfer += oprot->writeStructEnd();" << endl <<
    indent() << "return xfer;" << endl;

  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

/**
 * Generates the swap function.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_swap(ofstream& out, t_struct* tstruct) {
  out <<
    indent() << "void swap(" << tstruct->get_name() << " &a, " <<
    tstruct->get_name() << " &b) {" << endl;
  indent_up();

  // Let argument-dependent name lookup find the correct swap() function to
  // use based on the argument types.  If none is found in the arguments'
  // namespaces, fall back to ::std::swap().
  out <<
    indent() << "using ::std::swap;" << endl;

  // Prevent unused variable warnings
  indent(out) << "(void)a;" << endl;
  indent(out) << "(void)b;" << endl;

  bool has_nonrequired_fields = false;
  const vector<t_field*>& fields = tstruct->get_members();
  for (vector<t_field*>::const_iterator f_iter = fields.begin();
       f_iter != fields.end();
       ++f_iter) {
    t_field *tfield = *f_iter;

    if (tfield->get_req() != t_field::T_REQUIRED) {
      has_nonrequired_fields = true;
    }

    out <<
      indent() << "swap(a." << tfield->get_name() <<
      ", b." << tfield->get_name() << ");" << endl;
  }

  if (has_nonrequired_fields) {
    out <<
      indent() << "swap(a.__isset, b.__isset);" << endl;
  }

  scope_down(out);
  out << endl;
}

/**
 * Generates the merge() function.
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_merge(ofstream& out, t_struct* tstruct) {
  using folly::vformat;
  auto struct_name = tstruct->get_name();

  // Generate two overloads of the function, the differences are abstracted into
  // the maps.
  vector<unordered_map<string, string>> code_maps = {
    {  // void merge(const Struct& from, Struct& to);
      {"from_arg", folly::format("const {}& from", struct_name).str()},
      {"to_arg", folly::format("{}& to", struct_name).str()},
      {"from_field_format", "from.{field_name}"},
      {"to_field_format", "to.{field_name}"},
    },
    {  // void merge(Struct&& from, Struct& to);
      {"from_arg", folly::format("{}&& from", struct_name).str()},
      {"to_arg", folly::format("{}& to", struct_name).str()},
      {"from_field_format", "std::move(from.{field_name})"},
      {"to_field_format", "to.{field_name}"},
    },
  };
  for (auto& code_map : code_maps) {
    indent(out) << vformat("void merge({from_arg}, {to_arg}) {{", code_map)
                << endl;
    indent_up();
    indent(out) << "using apache::thrift::merge;" << endl;
    for (auto field : tstruct->get_members()) {
      const bool isset = has_isset(field);
      code_map["field_name"] = field->get_name();

      if (isset) {
        indent(out) << vformat("if (from.__isset.{field_name}) {{", code_map)
                    << endl;
        indent_up();
      }
      indent(out)
        << folly::format("merge({}, {});",
                         vformat(code_map["from_field_format"], code_map),
                         vformat(code_map["to_field_format"], code_map))
        << endl;
      if (isset) {
        indent(out) << vformat("to.__isset.{field_name} = true;", code_map)
                    << endl;
        indent_down();
        indent(out) << "}" << endl;
      }
    }
    indent_down();
    indent(out) << "}" << endl;
    indent(out) << endl;
  }
}

/**
 * Generates a thrift service. In C++, this comprises an entirely separate
 * header and source file. The header file defines the methods and includes
 * the data types defined in the main header file, and the implementation
 * file contains implementations of the basic printer and default interfaces.
 *
 * @param tservice The service definition
 */
void t_cpp_generator::generate_service(t_service* tservice) {
  string svcname = tservice->get_name();
  string ns = namespace_prefix(tservice->get_program()->get_namespace("cpp"),
                               "_");

  // Make output files
  string f_header_name = get_out_dir()+svcname+".h";
  f_header_.open(f_header_name.c_str());
  record_genfile(f_header_name);

  // Print header file includes
  f_header_ <<
    autogen_comment();
  f_header_ <<
    "#ifndef " << ns << svcname << "_H" << endl <<
    "#define " << ns << svcname << "_H" << endl <<
    endl;
  if (gen_cob_style_) {
    f_header_ <<
      "#include <functional>" << endl <<
      // TODO(dreiss): Libify the base client so we don't have to include this.
      "#include <thrift/lib/cpp/transport/TTransportUtils.h>" << endl <<
      "namespace apache { namespace thrift { namespace async {" << endl <<
      "class TAsyncChannel;" << endl <<
      "}}}" << endl;
  }
  f_header_ <<
    "#include <thrift/lib/cpp/TDispatchProcessor.h>" << endl;
  if (gen_cob_style_) {
    f_header_ <<
      "#include <thrift/lib/cpp/async/TAsyncDispatchProcessor.h>" << endl;
  }
  f_header_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl;

  t_service* extends_service = tservice->get_extends();
  if (extends_service != nullptr) {
    f_header_ <<
      "#include \"" << get_include_prefix(*(extends_service->get_program())) <<
      extends_service->get_name() << ".h\"" << endl;
  }

  f_header_ <<
    endl <<
    ns_open_ << endl <<
    endl;

  // Service implementation file includes
  string f_service_name = get_out_dir()+svcname+".cpp";
  f_service_.open(f_service_name.c_str());
  record_genfile(f_service_name);
  f_service_ <<
    autogen_comment();

  f_service_ <<
    "#include \"" << get_include_prefix(*get_program()) << svcname << ".h\""
    << endl;

  if (gen_templates_) {
    f_service_ <<
      "#include \"" << get_include_prefix(*get_program()) << svcname <<
      ".tcc\"" << endl;

    string f_service_tcc_name = get_out_dir()+svcname+".tcc";
    f_service_tcc_.open(f_service_tcc_name.c_str());
    record_genfile(f_service_tcc_name);
    f_service_tcc_ <<
      autogen_comment();

    f_service_tcc_ <<
      "#ifndef " << ns << svcname << "_TCC" << endl <<
      "#define " << ns << svcname << "_TCC" << endl <<
      endl;
  }

  f_service_
    << "#include <folly/ScopeGuard.h>" << endl;
  if (gen_cob_style_) {
    f_service_
      << "#include <thrift/lib/cpp/async/TAsyncChannel.h>" << endl;
  }

  if (gen_templates_) {
    f_service_tcc_ <<
      "#include \"" << get_include_prefix(*get_program()) << svcname <<
      ".h\"" << endl;

    f_service_tcc_
      << "#include <folly/ScopeGuard.h>" << endl;
    if (gen_cob_style_) {
      f_service_tcc_
        << "#include <thrift/lib/cpp/async/TAsyncChannel.h>" << endl;
    }
  }

  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);
  if (!bootstrap_) {
    out << endl <<
      "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
      "_reflection.h\"" << endl;
  }

  f_service_ <<
    endl << ns_open_ << endl << endl;
  f_service_tcc_ <<
    endl << ns_open_ << endl << endl;

  if (gen_perfhash_) {
    f_service_ <<
      "#define THRIFT_INCLUDE_GPERF_OUTPUT" << endl <<
      "#include \"" << get_include_prefix(*get_program()) << svcname <<
      "_gperf.tcc\"" << endl <<
      "#undef THRIFT_INCLUDE_GPERF_OUTPUT" << endl;
    if (gen_templates_) {
      f_service_tcc_ <<
        "int " << svcname << "_method_lookup(const std::string&);" << endl;
    }
  }

  string f_service_gperf_name;
  string f_service_gperf_out_name;
  if (gen_perfhash_) {
    f_service_gperf_name = get_out_dir() + svcname + ".gperf";
    f_service_gperf_out_name = get_out_dir() + svcname + "_gperf.tcc";
    f_service_gperf_.open(f_service_gperf_name.c_str());
    record_genfile(f_service_gperf_name);

    // Raw include section
    f_service_gperf_ <<
      "%{" << endl <<
      autogen_comment() <<
      "#ifndef THRIFT_INCLUDE_GPERF_OUTPUT" << endl <<
      "#error This file may not be included directly." << endl <<
      "#endif" << endl <<
      "namespace {" << endl <<
      "%}" << endl;
    f_service_gperf_ <<
      "%language=C++" << endl <<
      "%compare-strncmp" << endl <<
      "%readonly-tables" << endl <<
      "%define hash-function-name thrift_method_hash" << endl <<
      "%define lookup-function-name thrift_method_lookup" << endl <<
      "%enum" << endl <<
      "%struct-type" << endl <<
      "struct ThriftMethodPerfHash { const char* name; int idx; }" <<
      endl <<
      "%%" << endl;
  }

  // Generate all the components
  generate_service_interface(tservice, "");
  generate_service_interface_factory(tservice, "");
  generate_service_null(tservice, "");
  generate_service_helpers(tservice);
  generate_service_client(tservice, "");
  generate_service_processor(tservice, "");
  generate_service_multiface(tservice);
  generate_service_skeleton(tservice);

  if (gen_perfhash_) {
    generate_service_perfhash_keywords(tservice);
  }

  // Generate all the cob components
  if (gen_cob_style_) {
    generate_service_interface(tservice, "CobCl");
    generate_service_interface(tservice, "CobSv");
    generate_service_interface_factory(tservice, "CobSv");
    generate_service_null(tservice, "CobSv");
    generate_service_client(tservice, "Cob");
    generate_service_processor(tservice, "Cob");
    generate_service_async_skeleton(tservice);
  }

  // Close the namespace
  f_service_ <<
    ns_close_ << endl <<
    endl;
  f_service_tcc_ <<
    ns_close_ << endl <<
    endl;
  f_header_ <<
    ns_close_ << endl <<
    endl;

  if (gen_perfhash_) {
    f_service_gperf_ <<
      "%%" << endl <<
      "}  // namespace" << endl <<
      "int " << svcname << "_method_lookup(const std::string& f) {" << endl <<
      "  const ThriftMethodPerfHash* h = Perfect_Hash::thrift_method_lookup(f.data(), "
      "f.size());" << endl <<
      "  return h ? h->idx : -1;" << endl <<
      "}" << endl;
    f_service_gperf_.close();

    string cmd =
      "gperf " + f_service_gperf_name + " --output-file=" +
      f_service_gperf_out_name;

    int result = system(cmd.c_str());
    if (result != 0) {
      int savedErrno = errno;
      string error = "Executing \"" + cmd + "\" failed: ";
      char buf[15];
      if (result == -1) {
        snprintf(buf, sizeof(buf), "%d", savedErrno);
        error += "errno=";
        error += buf;
      } else {
        snprintf(buf, sizeof(buf), "%d", result);
        error += "status=";
        error += buf;
      }
      throw error;
    }
  }

  // TODO(simpkins): Make this a separate option
  if (gen_templates_) {
    f_header_ <<
      "#include \"" << get_include_prefix(*get_program()) << svcname <<
      ".tcc\"" << endl <<
      "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
      "_types.tcc\"" << endl <<
      endl;
  }

  f_header_ <<
    "#endif" << endl;
  f_service_tcc_ <<
    "#endif" << endl;

  // Close the files
  f_service_tcc_.close();
  f_service_.close();
  f_header_.close();
}

/**
 * Generates helper functions for a service. Basically, this generates types
 * for all the arguments and results to functions.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_helpers(t_service* tservice) {
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* ts = (*f_iter)->get_arglist();
    string name_orig = ts->get_name();

    // TODO(dreiss): Why is this stuff not in generate_function_helpers?
    ts->set_name(tservice->get_name() + "_" + (*f_iter)->get_name() + "_args");
    generate_struct_definition(f_header_, ts, false);
    if (!bootstrap_) {
      generate_struct_reflection(f_service_, ts);
    }
    generate_struct_reader(out, ts);
    generate_struct_writer(out, ts);
    ts->set_name(tservice->get_name() + "_" + (*f_iter)->get_name() + "_pargs");
    generate_struct_definition(f_header_, ts, false, true, false, true);
    if (!bootstrap_) {
      generate_struct_reflection(f_service_, ts);
    }
    generate_struct_writer(out, ts, true);
    ts->set_name(name_orig);

    generate_function_helpers(tservice, *f_iter);
  }
}

/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_interface(t_service* tservice, string style) {

  string service_if_name = service_name_ + style + "If";
  if (style == "CobCl") {
    // Forward declare the client.
    string client_name = service_name_ + "CobClient";
    if (gen_templates_) {
      client_name += "T";
      service_if_name += "T";
      indent(f_header_) <<
        "template <class Protocol_>" << endl;
    }
    indent(f_header_) << "class " << client_name << ";" <<
      endl << endl;
  }

  string extends = "";
  if (tservice->get_extends() != nullptr) {
    extends = " : virtual public " + type_name(tservice->get_extends()) +
      style + "If";
    if (style == "CobCl" && gen_templates_) {
      // TODO(simpkins): If gen_templates_ is enabled, we currently assume all
      // parent services were also generated with templates enabled.
      extends += "T<Protocol_>";
    }
  }

  if (style == "CobCl" && gen_templates_) {
    f_header_ << "template <class Protocol_>" << endl;
  }
  f_header_ <<
    "class " << service_if_name << extends << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << "virtual ~" << service_if_name << "() {}" << endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_header_ <<
      indent() << "virtual " << function_signature(*f_iter, style) << " = 0;" << endl;
  }
  indent_down();
  f_header_ <<
    "};" << endl << endl;

  if (style == "CobCl" && gen_templates_) {
    // generate a backwards-compatible typedef for clients that do not
    // know about the new template-style code
    f_header_ <<
      "typedef " << service_if_name <<
      "<apache::thrift::protocol::TProtocol> " <<
      service_name_ << style << "If;" <<
      endl << endl;
  }
}

/**
 * Generates a service interface factory.
 *
 * @param tservice The service to generate an interface factory for.
 */
void t_cpp_generator::generate_service_interface_factory(t_service* tservice,
                                                         string style) {
  string service_if_name = service_name_ + style + "If";

  // Figure out the name of the upper-most parent class.
  // Getting everything to work out properly with inheritance is annoying.
  // Here's what we're doing for now:
  //
  // - All handlers implement getHandler(), but subclasses use covariant return
  //   types to return their specific service interface class type.  We have to
  //   use raw pointers because of this; shared_ptr<> can't be used for
  //   covariant return types.
  //
  // - Since we're not using shared_ptr<>, we also provide a releaseHandler()
  //   function that must be called to release a pointer to a handler obtained
  //   via getHandler().
  //
  //   releaseHandler() always accepts a pointer to the upper-most parent class
  //   type.  This is necessary since the parent versions of releaseHandler()
  //   may accept any of the parent types, not just the most specific subclass
  //   type.  Implementations can use dynamic_cast to cast the pointer to the
  //   subclass type if desired.
  t_service* base_service = tservice;
  while (base_service->get_extends() != nullptr) {
    base_service = base_service->get_extends();
  }
  string base_if_name = type_name(base_service) + style + "If";

  // Generate the abstract factory class
  string factory_name = service_if_name + "Factory";
  string extends;
  if (tservice->get_extends() != nullptr) {
    extends = " : virtual public " + type_name(tservice->get_extends()) +
      style + "IfFactory";
  }

  f_header_ <<
    "class " << factory_name << extends << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << "typedef " << service_if_name << " Handler;" << endl <<
    endl <<
    indent() << "virtual ~" << factory_name << "() {}" << endl <<
    endl <<
    indent() << "virtual " << service_if_name << "* getHandler(" <<
      "::apache::thrift::server::TConnectionContext* ctx) = 0;" <<
    endl <<
    indent() << "virtual void releaseHandler(" << base_if_name <<
    "* handler) = 0;" << endl;

  indent_down();
  f_header_ <<
    "};" << endl << endl;

  // Generate the singleton factory class
  string singleton_factory_name = service_if_name + "SingletonFactory";
  f_header_ <<
    "class " << singleton_factory_name <<
    " : virtual public " << factory_name << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << singleton_factory_name << "(const std::shared_ptr<" <<
    service_if_name << ">& iface) : iface_(iface) {}" << endl <<
    indent() << "virtual ~" << singleton_factory_name << "() {}" << endl <<
    endl <<
    indent() << "virtual " << service_if_name << "* getHandler(" <<
      "::apache::thrift::server::TConnectionContext*) {" << endl <<
    indent() << "  return iface_.get();" << endl <<
    indent() << "}" << endl <<
    indent() << "virtual void releaseHandler(" << base_if_name <<
    "* handler) {}" << endl;

  f_header_ <<
    endl <<
    " protected:" << endl <<
    indent() << "std::shared_ptr<" << service_if_name << "> iface_;" << endl;

  indent_down();
  f_header_ <<
    "};" << endl << endl;
}

/**
 * Generates a null implementation of the service.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_null(t_service* tservice, string style) {
  string extends = "";
  if (tservice->get_extends() != nullptr) {
    extends = " , virtual public " + type_name(tservice->get_extends()) + style + "Null";
  }
  f_header_ <<
    "class " << service_name_ << style << "Null : virtual public " << service_name_ << style << "If" << extends << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << "virtual ~" << service_name_ << style << "Null() {}" << endl;
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_header_ <<
      indent() << function_signature(*f_iter, style, "", false) << " {" << endl;
    indent_up();

    t_type* returntype = (*f_iter)->get_returntype();
    t_field returnfield(returntype, "_return");

    if (style == "") {
      if (returntype->is_void() || is_complex_type(returntype)) {
        f_header_ << indent() << "return;" << endl;
      } else {
        f_header_ <<
          indent() << declare_field(&returnfield, true) << endl <<
          indent() << "return _return;" << endl;
      }
    } else if (style == "CobSv") {
      if (returntype->is_void()) {
        f_header_ << indent() << "return cob();" << endl;
    } else {
      t_field returnfield(returntype, "_return");
      f_header_ <<
        indent() << declare_field(&returnfield, true) << endl <<
        indent() << "return cob(_return);" << endl;
    }

    } else {
      throw "UNKNOWN STYLE";
    }

    indent_down();
    f_header_ <<
      indent() << "}" << endl;
  }
  indent_down();
  f_header_ <<
    "};" << endl << endl;
}

void t_cpp_generator::generate_function_call(ostream& out, t_function* tfunction, string target, string iface, string arg_prefix) {
  bool first = true;
  t_type* ret_type = get_true_type(tfunction->get_returntype());
  out << indent();
  if (!tfunction->is_oneway() && !ret_type->is_void()) {
    if (is_complex_type(ret_type)) {
      first = false;
      out << iface << "->" << tfunction->get_name() << "(" << target;
    } else {
      out << target << " = " << iface << "->" << tfunction->get_name() << "(";
    }
  } else {
    out << iface << "->" << tfunction->get_name() << "(";
  }
  const std::vector<t_field*>& fields = tfunction->get_arglist()->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      out << ", ";
    }
    out << arg_prefix << (*f_iter)->get_name();
  }
  out << ");" << endl;
}

void t_cpp_generator::generate_service_async_skeleton(t_service* tservice) {
  string svcname = tservice->get_name();

  // Service implementation file includes
  string f_skeleton_name = get_out_dir()+svcname+"_async_server.skeleton.cpp";

  string ns = namespace_prefix(tservice->get_program()->get_namespace("cpp"));

  ofstream f_skeleton;
  f_skeleton.open(f_skeleton_name.c_str());
  record_genfile(f_skeleton_name);
  f_skeleton <<
    autogen_comment() << endl <<
    "// This autogenerated skeleton file illustrates how to build an\n"
    "// asynchronous server. You should copy it to another filename to\n"
    "// avoid overwriting it.\n"
    "\n"
    "#include \"" << get_include_prefix(*get_program()) << svcname << ".h\"\n"<<
    "#include <thrift/lib/cpp/async/TEventServer.h>\n"
    "#include <thrift/lib/cpp/util/TEventServerCreator.h>\n"
    "\n"
    "using apache::thrift::async::TAsyncProcessor;\n"
    "using apache::thrift::async::TEventServer;\n"
    "using apache::thrift::util::TEventServerCreator;\n"
    "\n"
    "using std::shared_ptr;\n"
    "\n";

  if (!ns.empty()) {
    f_skeleton <<
      "using namespace " << string(ns, 0, ns.size()-2) << ";" << endl <<
      endl;
  }

  f_skeleton <<
    "class " << svcname << "AsyncHandler : " <<
      "public " << svcname << "CobSvIf {\n" <<
    " public:\n";
  indent_up();
  f_skeleton <<
    indent() << svcname << "AsyncHandler() {\n" <<
    indent() << "  // Your initialization goes here\n" <<
    indent() << "}\n";
  f_skeleton <<
    indent() << "virtual ~" << service_name_ << "AsyncHandler() {}\n";

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_skeleton <<
      "\n" <<
      indent() << function_signature(*f_iter, "CobSv", "", true) << " {\n";
    indent_up();

    t_type* returntype = (*f_iter)->get_returntype();
    t_field returnfield(returntype, "_return");

    string target = returntype->is_void() ? "" : "_return";
    if (!returntype->is_void()) {
      f_skeleton <<
        indent() << declare_field(&returnfield, true) << endl;
    }
    f_skeleton <<
      indent() << "// Your implementation goes here.\n" <<
      indent() << "// You can also schedule an async operation, and invoke\n" <<
      indent() << "// the callback later once the operation completes,\n" <<
      indent() << "// rather than invoking it before returning here.\n" <<
      indent() << "printf(\"" << (*f_iter)->get_name() << "\\n\");\n" <<
      indent() << "cob(" << target << ");\n";

    scope_down(f_skeleton);
  }
  indent_down();
  f_skeleton <<
    "};\n"
    "\n";

  f_skeleton <<
    indent() << "int main(int argc, char **argv) {\n";
  indent_up();
  f_skeleton <<
    indent() << "int port = 9090;\n" <<
    indent() << "shared_ptr<" << svcname << "AsyncHandler> handler(new " <<
      svcname << "AsyncHandler());\n" <<
    indent() << "shared_ptr<TAsyncProcessor> processor(new " <<
      svcname << "AsyncProcessor(handler));\n" <<
    indent() << "TEventServerCreator serverCreator(processor, port);\n" <<
    indent() << "shared_ptr<TEventServer> server = " <<
      "serverCreator.createEventServer();\n" <<
    indent() << "server->serve();\n" <<
    indent() << "return 0;\n";
  indent_down();
  f_skeleton <<
    "}\n";
}

/**
 * Generates a multiface, which is a single server that just takes a set
 * of objects implementing the interface and calls them all, returning the
 * value of the last one to be called.
 *
 * @param tservice The service to generate a multiserver for.
 */
void t_cpp_generator::generate_service_multiface(t_service* tservice) {
  // Generate the dispatch methods
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  string extends = "";
  string extends_multiface = "";
  if (tservice->get_extends() != nullptr) {
    extends = type_name(tservice->get_extends());
    extends_multiface = ", public " + extends + "Multiface";
  }

  string list_type = string("std::vector<std::shared_ptr<") + service_name_ + "If> >";

  // Generate the header portion
  f_header_ <<
    "class " << service_name_ << "Multiface : " <<
    "virtual public " << service_name_ << "If" <<
    extends_multiface << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << service_name_ << "Multiface(" << list_type << "& ifaces) : ifaces_(ifaces) {" << endl;
  if (!extends.empty()) {
    f_header_ <<
      indent() << "  std::vector<std::shared_ptr<" + service_name_ + "If> >::iterator iter;" << endl <<
      indent() << "  for (iter = ifaces.begin(); iter != ifaces.end(); ++iter) {" << endl <<
      indent() << "    " << extends << "Multiface::add(*iter);" << endl <<
      indent() << "  }" << endl;
  }
  f_header_ <<
    indent() << "}" << endl <<
    indent() << "virtual ~" << service_name_ << "Multiface() {}" << endl;
  indent_down();

  // Protected data members
  f_header_ <<
    " protected:" << endl;
  indent_up();
  f_header_ <<
    indent() << list_type << " ifaces_;" << endl <<
    indent() << service_name_ << "Multiface() {}" << endl <<
    indent() << "void add(std::shared_ptr<" << service_name_ << "If> iface) {" << endl;
  if (!extends.empty()) {
    f_header_ <<
      indent() << "  " << extends << "Multiface::add(iface);" << endl;
  }
  f_header_ <<
    indent() << "  ifaces_.push_back(iface);" << endl <<
    indent() << "}" << endl;
  indent_down();

  f_header_ <<
    indent() << " public:" << endl;
  indent_up();

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arglist = (*f_iter)->get_arglist();
    const vector<t_field*>& args = arglist->get_members();
    vector<t_field*>::const_iterator a_iter;

    string call = string("ifaces_[i]->") + (*f_iter)->get_name() + "(";
    bool first = true;
    if (is_complex_type((*f_iter)->get_returntype())) {
      call += "_return";
      first = false;
    }
    for (a_iter = args.begin(); a_iter != args.end(); ++a_iter) {
      if (first) {
        first = false;
      } else {
        call += ", ";
      }
      call += (*a_iter)->get_name();
    }
    call += ")";

    // If the generated function actually returns a value, then we break
    // out of the loop an iteration early to return the value from the call
    // to the last interface.  Otherwise, we can just call all interfaces
    // from within the loop.
    bool has_ret = !(*f_iter)->get_returntype()->is_void() &&
                   !is_complex_type((*f_iter)->get_returntype());

    f_header_ <<
      indent() << function_signature(*f_iter, "") << " {" << endl;
    indent_up();

    f_header_ <<
      indent() << "uint32_t i;" << endl <<
      indent() << "uint32_t sz = ifaces_.size();" << endl <<
      indent() << "for (i = 0; i < sz" << (has_ret ? " - 1" : "")
               << "; ++i) {" << endl <<
      indent() << "  " << call << ";" << endl <<
      indent() << "}" << endl;

    if (has_ret) {
        f_header_ << indent() << "return " << call << ";" << endl;
    }

    indent_down();
    f_header_ <<
      indent() << "}" << endl <<
      endl;
  }

  indent_down();
  f_header_ <<
    indent() << "};" << endl <<
    endl;
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_cpp_generator::generate_service_client(t_service* tservice, string style) {
  string ifstyle;
  if (style == "Cob") {
    ifstyle = "CobCl";
  }

  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);
  string template_header, template_suffix, short_suffix, protocol_type, _this;
  string const prot_factory_type =
    "apache::thrift::protocol::TProtocolFactory";
  if (gen_templates_) {
    template_header = "template <class Protocol_>\n";
    short_suffix = "T";
    template_suffix = "T<Protocol_>";
    protocol_type = "Protocol_";
    _this = "this->";
  } else {
    protocol_type = "apache::thrift::protocol::TProtocol";
  }
  string prot_ptr = "std::shared_ptr<" + protocol_type + ">";
  string client_suffix = "Client" + template_suffix;
  string if_suffix = "If";
  if (style == "Cob") {
    if_suffix += template_suffix;
  }

  string extends = "";
  string extends_client = "";
  if (tservice->get_extends() != nullptr) {
    // TODO(simpkins): If gen_templates_ is enabled, we currently assume all
    // parent services were also generated with templates enabled.
    extends = type_name(tservice->get_extends());
    extends_client = ", public " + extends + style + client_suffix;
  } else {
    // Client base class
    extends_client = ", virtual public apache::thrift::TClientBase";
  }

  // Generate the header portion
  f_header_ <<
    template_header <<
    "class " << service_name_ << style << "Client" << short_suffix << " : " <<
    "virtual public " << service_name_ << ifstyle << if_suffix <<
    extends_client << " {" << endl <<
    " public:" << endl;

  indent_up();
  if (style != "Cob") {
    f_header_ <<
      indent() << service_name_ << style << "Client" << short_suffix <<
      "(" << prot_ptr << " prot) :" <<
      endl;
    if (extends.empty()) {
      f_header_ <<
        indent() << "  checkSeqid_(true)," << endl <<
        indent() << "  nextSendSequenceId_(1)," << endl <<
        indent() << "  nextRecvSequenceId_(1)," << endl <<
        indent() << "  piprot_(prot)," << endl <<
        indent() << "  poprot_(prot) {" << endl <<
        indent() << "  iprot_ = prot.get();" << endl <<
        indent() << "  oprot_ = prot.get();" << endl <<
        indent() << "  connectionContext_ = std::shared_ptr<TClientBase::ConnContext>(" << endl <<
        indent() << "    new TClientBase::ConnContext(piprot_, poprot_));" << endl <<
        indent() << "}" << endl;
    } else {
      f_header_ <<
        indent() << "  " << extends << style << client_suffix <<
        "(prot, prot) {}" << endl;
    }

    f_header_ <<
      indent() << service_name_ << style << "Client" << short_suffix <<
      "(" << prot_ptr << " iprot, " << prot_ptr << " oprot) :" << endl;
    if (extends.empty()) {
      f_header_ <<
        indent() << "  checkSeqid_(true)," << endl <<
        indent() << "  nextSendSequenceId_(1)," << endl <<
        indent() << "  nextRecvSequenceId_(1)," << endl <<
        indent() << "  piprot_(iprot)," << endl <<
        indent() << "  poprot_(oprot) {" << endl <<
        indent() << "  iprot_ = iprot.get();" << endl <<
        indent() << "  oprot_ = oprot.get();" << endl <<
        indent() << "  connectionContext_ = std::shared_ptr<TClientBase::ConnContext>(" << endl <<
        indent() << "    new TClientBase::ConnContext(piprot_, poprot_));" << endl <<
        indent() << "}" << endl;
    } else {
      f_header_ <<
        indent() << "  " << extends << style << client_suffix <<
        "(iprot, oprot) {}" << endl;
    }

    // Generate getters for the protocols.
    // Note that these are not currently templated for simplicity.
    // TODO(simpkins): should they be templated?
    f_header_ <<
      indent() << "std::shared_ptr<apache::thrift::protocol::TProtocol> getInputProtocol() {" << endl <<
      indent() << "  return " << _this << "piprot_;" << endl <<
      indent() << "}" << endl;

    f_header_ <<
      indent() << "std::shared_ptr<apache::thrift::protocol::TProtocol> getOutputProtocol() {" << endl <<
      indent() << "  return " << _this << "poprot_;" << endl <<
      indent() << "}" << endl;

  } else /* if (style == "Cob") */ {
    /* Generate TProtocolFactory* constructor */
    f_header_ <<
      indent() << service_name_ << style << "Client" << short_suffix << "(" <<
      "std::shared_ptr<apache::thrift::async::TAsyncChannel> channel, " <<
      "apache::thrift::protocol::TProtocolFactory* protocolFactory) :" <<
      endl;
    if (extends.empty()) {
      f_header_ <<
        indent() << "  channel_(channel)," << endl <<
        indent() <<
        "  itrans_(new apache::thrift::transport::TMemoryBuffer())," << endl <<
        indent() <<
        "  otrans_(new apache::thrift::transport::TMemoryBuffer())," << endl <<
        indent() <<
        "  checkSeqid_(true)," << endl <<
        indent() <<
        "  nextSendSequenceId_(1)," << endl <<
        indent() <<
        "  nextRecvSequenceId_(1)," << endl;
      if (gen_templates_) {
        // TProtocolFactory classes return generic TProtocol pointers.
        // We have to dynamic cast to the Protocol_ type we are expecting.
        f_header_ <<
          indent() << "  piprot_(std::dynamic_pointer_cast<Protocol_>(" <<
          "protocolFactory->getProtocol(itrans_)))," << endl <<
          indent() << "  poprot_(std::dynamic_pointer_cast<Protocol_>(" <<
          "protocolFactory->getProtocol(otrans_))) {" << endl;
        // Throw a TException if either dynamic cast failed.
        f_header_ <<
          indent() << "  if (!piprot_ || !poprot_) {" << endl <<
          indent() << "    throw apache::thrift::TLibraryException(\"" <<
          "TProtocolFactory returned unexpected protocol type in " <<
          service_name_ << style << "Client" << short_suffix <<
          " constructor\");" << endl <<
          indent() << "  }" << endl;
      } else {
        f_header_ <<
          indent() << "  piprot_(protocolFactory->getProtocol(itrans_))," <<
          endl <<
          indent() << "  poprot_(protocolFactory->getProtocol(otrans_)) {" <<
          endl;
      }
      f_header_ <<
        indent() << "  iprot_ = piprot_.get();" << endl <<
        indent() << "  oprot_ = poprot_.get();" << endl <<
        indent() << "  connectionContext_ = std::shared_ptr<TClientBase::ConnContext>(" << endl <<
        indent() << "    new TClientBase::ConnContext(channel, piprot_, poprot_));" << endl <<
        indent() << "}" << endl;
    } else {
      f_header_ <<
        indent() << "  " << extends << style << client_suffix <<
        "(channel, protocolFactory) {}" << endl;
    }

    /* Generate TDuplexProtocolFactory* constructor */
    f_header_ <<
      indent() << service_name_ << style << "Client" << short_suffix << "(" <<
      "std::shared_ptr<apache::thrift::async::TAsyncChannel> channel, " <<
      "apache::thrift::protocol::TDuplexProtocolFactory* protocolFactory) :" <<
      endl;
    if (extends.empty()) {
      f_header_ <<
        indent() << "  channel_(channel)," << endl <<
        indent() <<
        "  itrans_(new apache::thrift::transport::TMemoryBuffer())," << endl <<
        indent() <<
        "  otrans_(new apache::thrift::transport::TMemoryBuffer())," << endl <<
        indent() <<
        "  checkSeqid_(true)," << endl <<
        indent() <<
        "  nextSendSequenceId_(1)," << endl <<
        indent() <<
        "  nextRecvSequenceId_(1) {" << endl;

      f_header_ <<
        indent() <<
        "  apache::thrift::transport::TTransportPair tpair = std::make_pair(itrans_, otrans_);" << endl <<
        indent() <<
        "  apache::thrift::protocol::TProtocolPair ppair = protocolFactory->getProtocol(tpair);" << endl;
      if (gen_templates_) {
        // TDuplexProtocolFactory classes return generic TProtocol pointers.
        // We have to dynamic cast to the Protocol_ type we are expecting.
        f_header_ <<
          indent() << "  piprot_ = std::dynamic_pointer_cast<Protocol_>(" <<
          "ppair.first);" << endl <<
          indent() << "  poprot_ = std::dynamic_pointer_cast<Protocol_>(" <<
          "ppair.second);" << endl;
        // Throw a TException if either dynamic cast failed.
        f_header_ <<
          indent() << "  if (!piprot_ || !poprot_) {" << endl <<
          indent() << "    throw apache::thrift::TLibraryException(\"" <<
          "TDuplexProtocolFactory returned unexpected protocol type in " <<
          service_name_ << style << "Client" << short_suffix <<
          " constructor\");" << endl <<
          indent() << "  }" << endl;
      } else {
        f_header_ <<
          indent() << "  piprot_ = ppair.first;" <<
          endl <<
          indent() << "  poprot_ = ppair.second;" <<
          endl;
      }
      f_header_ <<
        indent() <<
        "  iprot_ = piprot_.get();" << endl <<
        indent() <<
        "  oprot_ = poprot_.get();" << endl <<
        indent() <<
        "  connectionContext_ = std::shared_ptr<TClientBase::ConnContext>(" << endl <<
        indent() <<
        "    new TClientBase::ConnContext(channel, piprot_, poprot_));" << endl <<
        indent() <<
        "}" << endl;
    } else {
      f_header_ <<
        indent() << "  " << extends << style << client_suffix <<
        "(channel, protocolFactory) {}" << endl;
    }
  }

  if (style == "Cob") {
    f_header_ <<
      indent() << "std::shared_ptr<apache::thrift::async::TAsyncChannel> getChannel() {" << endl <<
      indent() << "  return " << _this << "channel_;" << endl <<
      indent() << "}" << endl;
    if (!gen_no_client_completion_) {
      f_header_ <<
        indent() << "virtual void completed__(bool success) {}" << endl;
    }
  }

  f_header_ << indent() << "virtual ~" << service_name_
            << style << "Client" << short_suffix << "() {}" << endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(f_header_) << function_signature(*f_iter, ifstyle) << ";" << endl;
    // TODO(dreiss): Use private inheritance to avoid generating thise in cob-style.
    t_function send_function(g_type_void,
        string("send_") + (*f_iter)->get_name(),
        (*f_iter)->get_arglist());
    indent(f_header_) << "virtual " <<
      function_signature(&send_function, "") << ";" << endl;
    if (!(*f_iter)->is_oneway()) {
      t_struct noargs(program_);
      t_function recv_function((*f_iter)->get_returntype(),
          string("recv_") + (*f_iter)->get_name(),
          &noargs);
      indent(f_header_) << "virtual " <<
        function_signature(&recv_function, "") << ";" << endl;
    }
  }
  indent_down();

  if (extends.empty()) {
    indent_up();
    f_header_ <<
      indent() << "apache::thrift::server::TConnectionContext* getConnectionContext() {" << endl <<
      indent() << "  return connectionContext_.get();" << endl <<
      indent() << "}" << endl;

    f_header_ <<
      endl <<
      indent() << "/**" << endl <<
      indent() << " * Disable checking the seqid field in server responses." <<
        endl <<
      indent() << " *" << endl <<
      indent() << " * This should only be used with broken servers that " <<
       "return incorrect seqid values." << endl <<
      indent() << " */" << endl <<
      indent() << "void _disableSequenceIdChecks() {" << endl <<
      indent() << "  checkSeqid_ = false;" << endl <<
      indent() << "}" << endl <<
      endl;

    if (style == "Cob") {
      f_header_ <<
        endl <<
        indent() << "/**" << endl <<
        indent() << " * Increase the send buffer size.  Use this if you " <<
        "plan to have more than one message outstanding." <<
        endl <<
        indent() << " */" << endl <<
        indent() << "void _resizeSendBuffer(uint32_t size) {" << endl <<
        indent() << "  otrans_->getWritePtr(size);" << endl <<
        indent() << "}" << endl <<
        endl;
    }

    indent_down();

    f_header_ <<
      " protected:" << endl;
    indent_up();

    if (style == "Cob") {
      f_header_ <<
        indent() << "std::shared_ptr<apache::thrift::async::TAsyncChannel> channel_;"  << endl <<
        indent() << "std::shared_ptr<apache::thrift::transport::TMemoryBuffer> itrans_;"  << endl <<
        indent() << "std::shared_ptr<apache::thrift::transport::TMemoryBuffer> otrans_;"  << endl;
    }
    f_header_ <<
      indent() << "bool checkSeqid_;"  << endl <<
      indent() << "int32_t nextSendSequenceId_;" << endl <<
      indent() << "int32_t nextRecvSequenceId_;" << endl <<
      indent() << "int32_t getNextSendSequenceId();" << endl <<
      indent() << "int32_t getNextRecvSequenceId();" << endl <<
      indent() << prot_ptr << " piprot_;"  << endl <<
      indent() << prot_ptr << " poprot_;"  << endl <<
      indent() << protocol_type << "* iprot_;"  << endl <<
      indent() << protocol_type << "* oprot_;"  << endl <<
      indent() << "std::shared_ptr<apache::thrift::server::TConnectionContext> connectionContext_;" << endl;

    indent_down();
  }

  indent_up();
  f_header_ << indent() << "virtual std::string getServiceName();" << endl;
  indent_down();

  f_header_ <<
    "};" << endl <<
    endl;

  if (gen_templates_) {
    // Output a backwards compatibility typedef using
    // TProtocol as the template parameter.
    f_header_ <<
      "typedef " << service_name_ << style <<
      "ClientT<apache::thrift::protocol::TProtocol> " <<
      service_name_ << style << "Client;" << endl <<
      endl;
  }

  string scope = service_name_ + style + client_suffix + "::";

  if (extends.empty()) {
    if (gen_templates_) {
      indent(out) << template_header;
    }
    indent(out) <<
      "int32_t " << scope << "getNextSendSequenceId()" << endl;
    scope_up(out);
    out << indent() << "return nextSendSequenceId_++;" << endl;
    scope_down(out);
    out << endl;

    if (gen_templates_) {
      indent(out) << template_header;
    }
    indent(out) <<
      "int32_t " << scope << "getNextRecvSequenceId()" << endl;
    scope_up(out);
    out << indent() << "return nextRecvSequenceId_++;" << endl;
    scope_down(out);
    out << endl;
  }

  if (gen_templates_) {
    indent(out) << template_header;
  }
  indent(out) << "std::string " << scope << "getServiceName() {" << endl;
  scope_up(out);
  indent(out) << "return \"" << tservice->get_name() << "\";" << endl;
  scope_down(out);
  indent(out) << "}" << endl;

  // Generate client method implementations
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    string funname = (*f_iter)->get_name();
    string service_func_name = "\"" + tservice->get_name() + "." +
      (*f_iter)->get_name() + "\"";


    // Open function
    if (gen_templates_) {
      indent(out) << template_header;
    }
    indent(out) <<
      function_signature(*f_iter, ifstyle, scope) << endl;
    scope_up(out);
    indent(out) << "SCOPE_EXIT { this->clearClientContextStack(); };"
                << endl;

    indent(out) << "this->generateClientContextStack(this->getServiceName(), "
                << service_func_name << ", this->getConnectionContext());" << endl
                << endl;

    out <<
      indent() << "try {" << endl;
    indent_up();
    indent(out) << "send_" << funname << "(";

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
        out << ", ";
      }
      out << (*fld_iter)->get_name();
    }
    out << ");" << endl;

    if (style != "Cob") {
      if (!(*f_iter)->is_oneway()) {
        out << indent();
        if (!(*f_iter)->get_returntype()->is_void()) {
          if (is_complex_type((*f_iter)->get_returntype())) {
            out << "recv_" << funname << "(_return);" << endl;
          } else {
            out << "return recv_" << funname << "();" << endl;
          }
        } else {
          out <<
            "recv_" << funname << "();" << endl;
        }
      }
    } else {
      if (!(*f_iter)->is_oneway()) {
        out <<
          indent() << _this << "channel_->sendAndRecvMessage(" <<
          "std::bind(cob, this), " << _this << "otrans_.get(), " <<
          _this << "itrans_.get());" << endl;
      } else {
        out <<
        indent() << _this << "channel_->sendOnewayMessage(" <<
          "std::bind(cob, this), " << _this << "otrans_.get());" << endl;
      }
    }

    indent_down();
    out <<
      indent() <<
      "} catch(apache::thrift::transport::TTransportException& ex) {" << endl;
    indent(out) << "  ::apache::thrift::ContextStack* c = "
                   "this->getClientContextStack();" << endl <<
       indent() << "  if (c) c->handlerError();" << endl;
    indent(out) << "  " << _this << "iprot_->getTransport()->close();"
                << endl <<
      indent() << "  " << _this << "oprot_->getTransport()->close();" << endl <<
      indent() << "  throw;" << endl <<
      indent() << "} catch(apache::thrift::TApplicationException& ex) {"
               << endl <<
      // NOTE: bad sequence id is an unrecoverable exception, so we close
      // the connection here.
      indent() <<
      "  if (ex.getType() =="
      " apache::thrift::TApplicationException::BAD_SEQUENCE_ID) {"
               << endl <<
      indent() << "    ::apache::thrift::ContextStack* c = "
                  "this->getClientContextStack();" << endl <<
      indent() << "    if (c) c->handlerError();" << endl <<
      indent() << "    " << _this << "iprot_->getTransport()->close();"
               << endl <<
      indent() << "    " << _this << "oprot_->getTransport()->close();"
               << endl <<
      indent() << "  }" << endl <<
      indent() << "  throw;" << endl <<
      indent() << "}" << endl;
    scope_down(out);
    out << endl;

    //if (style != "Cob") // TODO(dreiss): Libify the client and don't generate this for cob-style
    if (true) {
      // Function for sending
      t_function send_function(g_type_void,
                               string("send_") + (*f_iter)->get_name(),
                               (*f_iter)->get_arglist());

      // Open the send function
      if (gen_templates_) {
        indent(out) << template_header;
      }
      indent(out) <<
        function_signature(&send_function, "", scope) << endl;
      scope_up(out);

      // Function arguments and results
      string argsname = tservice->get_name() + "_" + (*f_iter)->get_name() + "_pargs";
      string resultname = tservice->get_name() + "_" + (*f_iter)->get_name() + "_presult";

      // We don't want to increment the sequence ID if there is no response
      string sendSequenceId = ((*f_iter)->is_oneway() ?
                               "0" :
                               _this + "getNextSendSequenceId()");

      // Serialize the request
      indent(out) << "apache::thrift::ContextStack* ctx = "
                  << "this->getClientContextStack();" << endl;
      indent(out) << "if (ctx) ctx->preWrite();" << endl;
      out <<
        indent() << _this << "oprot_->writeMessageBegin(\"" <<
        (*f_iter)->get_name() << "\", apache::thrift::protocol::T_CALL, " <<
        sendSequenceId << ");" << endl <<
        endl <<
        indent() << argsname << " args;" << endl;

      for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
        out <<
          indent() << "args." << (*fld_iter)->get_name() << " = &" << (*fld_iter)->get_name() << ";" << endl;
      }

      string bytes = tmp("_bytes");

      out <<
        indent() << "args.write(" << _this << "oprot_);" << endl <<
        endl <<
        indent() << _this << "oprot_->writeMessageEnd();" << endl <<
        indent() << "uint32_t " << bytes << " = " << _this
                 << "oprot_->getTransport()->writeEnd();" << endl;
        if (!(*f_iter)->is_oneway()) {
          out <<
            indent() << _this << "oprot_->getTransport()->flush();" << endl;
        } else {
          out <<
            indent() << _this << "oprot_->getTransport()->onewayFlush();" << endl;
        }

        out <<
          indent() << "if (ctx) ctx->postWrite(" << bytes << ");" << endl <<

        indent() << "return;" << endl;

      scope_down(out);
      out << endl;

      // Generate recv function only if not a oneway function
      if (!(*f_iter)->is_oneway()) {
        t_struct noargs(program_);
        t_function recv_function((*f_iter)->get_returntype(),
                                 string("recv_") + (*f_iter)->get_name(),
                                 &noargs);
        // Open the recv function
        if (gen_templates_) {
          indent(out) << template_header;
        }

        indent(out) <<
          function_signature(&recv_function, "", scope) << endl;
        scope_up(out);

        if (style == "Cob") {
          indent(out) << "// TODO: load ContextStack generated in "
                      << (*f_iter)->get_name() << endl;
          indent(out) << "this->clearClientContextStack();" << endl;
        }
        indent(out) << "apache::thrift::ContextStack* ctx = "
                    << "this->getClientContextStack();" << endl;

        indent(out) << "uint32_t bytes;" << endl;
        out <<
          indent() << "int32_t rseqid = 0;" << endl <<
          indent() << "int32_t eseqid = " << _this
                   << "getNextRecvSequenceId();" << endl <<
          indent() << "std::string fname;" << endl <<
          indent() << "apache::thrift::protocol::TMessageType mtype;" << endl <<
          indent() << "if (ctx) ctx->preRead();" << endl;
        if (style == "Cob" && !gen_no_client_completion_) {
          out <<
            indent() << "bool completed = false;" << endl << endl <<
            indent() << "try {";
          indent_up();
        }
        out << endl <<
          indent() << _this << "iprot_->readMessageBegin(" <<
          "fname, mtype, rseqid);" << endl <<
          indent() << "if (this->checkSeqid_ && " <<
            "rseqid != eseqid) {" << endl <<
          indent() << "  " << _this <<
          "iprot_->skip(apache::thrift::protocol::T_STRUCT);" << endl <<
          indent() << "  " << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << "  " << _this <<
          "iprot_->getTransport()->readEnd();" << endl <<
          indent() <<
          "  throw apache::thrift::TApplicationException(apache::thrift::"
          "TApplicationException::BAD_SEQUENCE_ID);" << endl <<
          indent() << "}" << endl <<
          indent() << "if (mtype == apache::thrift::protocol::T_EXCEPTION) {" << endl <<
          indent() << "  apache::thrift::TApplicationException x;" << endl <<
          indent() << "  x.read(" << _this << "iprot_);" << endl <<
          indent() << "  " << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << "  " << _this << "iprot_->getTransport()->readEnd();" <<
          endl;
        if (style == "Cob" && !gen_no_client_completion_) {
          out <<
            indent() << "  completed = true;" << endl <<
            indent() << "  completed__(true);" << endl;
        }
        out <<
          indent() << "  throw x;" << endl <<
          indent() << "}" << endl <<
          indent() << "if (mtype != apache::thrift::protocol::T_REPLY) {" << endl <<
          indent() << "  " << _this << "iprot_->skip(" <<
          "apache::thrift::protocol::T_STRUCT);" << endl <<
          indent() << "  " << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << "  " << _this << "iprot_->getTransport()->readEnd();" <<
          endl;
        if (style == "Cob" && !gen_no_client_completion_) {
          out <<
            indent() << "  completed = true;" << endl <<
            indent() << "  completed__(false);" << endl;
        }
        out <<
          indent() << "  throw apache::thrift::TApplicationException(apache::thrift::TApplicationException::INVALID_MESSAGE_TYPE);" << endl <<
          indent() << "}" << endl <<
          indent() << "if (fname.compare(\"" << (*f_iter)->get_name() << "\") != 0) {" << endl <<
          indent() << "  " << _this << "iprot_->skip(" <<
          "apache::thrift::protocol::T_STRUCT);" << endl <<
          indent() << "  " << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << "  " << _this << "iprot_->getTransport()->readEnd();" <<
          endl;
        if (style == "Cob" && !gen_no_client_completion_) {
          out <<
            indent() << "  completed = true;" << endl <<
            indent() << "  completed__(false);" << endl;
        }
        out <<
          indent() << "  throw apache::thrift::TApplicationException(apache::thrift::TApplicationException::WRONG_METHOD_NAME);" << endl <<
          indent() << "}" << endl;

        if (!(*f_iter)->get_returntype()->is_void() &&
            !is_complex_type((*f_iter)->get_returntype())) {
          t_field returnfield((*f_iter)->get_returntype(), "_return");
          out <<
            indent() << declare_field(&returnfield) << endl;
        }

        out <<
          indent() << resultname << " result;" << endl;

        if (!(*f_iter)->get_returntype()->is_void()) {
          out <<
            indent() << "result.success = &_return;" << endl;
        }

        out <<
          indent() << "result.read(" << _this << "iprot_);" << endl <<
          indent() << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << "bytes = " << _this
                   << "iprot_->getTransport()->readEnd();" << endl <<
          indent() << "if (ctx) ctx->postRead(bytes);" << endl;

        // Careful, only look for _result if not a void function
        if (!(*f_iter)->get_returntype()->is_void()) {
          if (is_complex_type((*f_iter)->get_returntype())) {
            out <<
              indent() << "if (result.__isset.success) {" << endl <<
              indent() << "  // _return pointer has now been filled" << endl;
            if (style == "Cob" && !gen_no_client_completion_) {
              out <<
                indent() << "  completed = true;" << endl <<
                indent() << "  completed__(true);" << endl;
            }
            out <<
              indent() << "  return;" << endl <<
              indent() << "}" << endl;
          } else {
            out <<
              indent() << "if (result.__isset.success) {" << endl;
            if (style == "Cob" && !gen_no_client_completion_) {
              out <<
                indent() << "  completed = true;" << endl <<
                indent() << "  completed__(true);" << endl;
            }
            out <<
              indent() << "  return _return;" << endl <<
              indent() << "}" << endl;
          }
        }

        t_struct* xs = (*f_iter)->get_xceptions();
        const std::vector<t_field*>& xceptions = xs->get_members();
        vector<t_field*>::const_iterator x_iter;
        for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
          out <<
            indent() << "if (result.__isset." << (*x_iter)->get_name() << ") {" << endl;
          if (style == "Cob" && !gen_no_client_completion_) {
            out <<
              indent() << "  completed = true;" << endl <<
              indent() << "  completed__(true);" << endl;
          }
          out  <<
            indent() << "  throw result." << (*x_iter)->get_name() << ";" << endl <<
            indent() << "}" << endl;
        }

        // We only get here if we are a void function
        if ((*f_iter)->get_returntype()->is_void()) {
          if (style == "Cob" && !gen_no_client_completion_) {
            out <<
              indent() << "completed = true;" << endl <<
              indent() << "completed__(true);" << endl;
          }
          indent(out) <<
            "return;" << endl;
        } else {
          if (style == "Cob" && !gen_no_client_completion_) {
            out <<
              indent() << "completed = true;" << endl <<
              indent() << "completed__(true);" << endl;
          }
          out <<
            indent() << "throw apache::thrift::TApplicationException(apache::thrift::TApplicationException::MISSING_RESULT, \"" << (*f_iter)->get_name() << " failed: unknown result\");" << endl;
        }
        if (style == "Cob" && !gen_no_client_completion_) {
          indent_down();
          out <<
            indent() << "} catch (...) {" << endl <<
            indent() << "  if (!completed) {" << endl <<
            indent() << "    completed__(false);" << endl <<
            indent() << "  }" << endl <<
            indent() << "  throw;" << endl <<
            indent() << "}" << endl;
        }
        // Close function
        scope_down(out);
        out << endl;
      }
    }
  }
}

class ProcessorGenerator {
 public:
  ProcessorGenerator(t_cpp_generator* generator, t_service* service,
                     const string& style);

  void run() {
    generate_class_definition();

    // Generate the dispatchCall() function
    generate_dispatch_call(false);
    if (generator_->gen_templates_) {
      generate_dispatch_call(true);
    }

    // Generate all of the process subfunctions
    generate_process_functions();

    generate_factory();
  }

  void generate_class_definition();
  void generate_dispatch_call(bool template_protocol);
  void generate_process_functions();
  void generate_factory();

 protected:
  std::string type_name(t_type* ttype, int flags=0) {
    return generator_->type_name(ttype, flags);
  }

  std::string indent() {
    return generator_->indent();
  }
  std::ostream& indent(std::ostream &os) {
    return generator_->indent(os);
  }

  void indent_up() {
    generator_->indent_up();
  }
  void indent_down() {
    generator_->indent_down();
  }

  t_cpp_generator* generator_;
  t_service* service_;
  std::ofstream& f_header_;
  std::ofstream& f_out_;
  string service_name_;
  string style_;
  string pstyle_;
  string class_name_;
  string if_name_;
  string factory_class_name_;
  string finish_cob_;
  string finish_cob_decl_;
  string ret_type_;
  string call_context_;
  string cob_arg_;
  string call_context_arg_;
  string call_context_decl_;
  string template_header_;
  string template_suffix_;
  string typename_str_;
  string class_suffix_;
  string extends_;
};

ProcessorGenerator::ProcessorGenerator(t_cpp_generator* generator,
                                       t_service* service,
                                       const string& style)
  : generator_(generator),
    service_(service),
    f_header_(generator->f_header_),
    f_out_(generator->gen_templates_ ?
           generator->f_service_tcc_ : generator->f_service_),
    service_name_(generator->service_name_),
    style_(style) {
  if (style_ == "Cob") {
    pstyle_ = "Async";
    class_name_ = service_name_ + pstyle_ + "Processor";
    if_name_ = service_name_ + "CobSvIf";

    finish_cob_ = "std::function<void(bool ok)> cob, ";
    finish_cob_decl_ = "std::function<void(bool ok)>, ";
    cob_arg_ = "cob, ";
    ret_type_ = "void ";
    call_context_ = ", apache::thrift::server::TConnectionContext* " \
      "connectionContext";
    call_context_arg_ = ", connectionContext";
    call_context_decl_ = ", apache::thrift::server::TConnectionContext*";
  } else {
    class_name_ = service_name_ + "Processor";
    if_name_ = service_name_ + "If";

    ret_type_ = "bool ";
    call_context_ = ", apache::thrift::server::TConnectionContext* " \
      "connectionContext";
    call_context_arg_ = ", connectionContext";
    call_context_decl_ = ", apache::thrift::server::TConnectionContext*";
  }

  factory_class_name_ = class_name_ + "Factory";

  if (generator->gen_templates_) {
    template_header_ = "template <class Protocol_>\n";
    template_suffix_ = "<Protocol_>";
    typename_str_ = "typename ";
    class_name_ += "T";
    factory_class_name_ += "T";
  }

  if (service_->get_extends() != nullptr) {
    extends_ = type_name(service_->get_extends()) + pstyle_ + "Processor";
    if (generator_->gen_templates_) {
      // TODO(simpkins): If gen_templates_ is enabled, we currently assume all
      // parent services were also generated with templates enabled.
      extends_ += "T<Protocol_>";
    }
  }
}

void ProcessorGenerator::generate_class_definition() {
  // Generate the dispatch methods
  vector<t_function*> functions = service_->get_functions();
  vector<t_function*>::iterator f_iter;

  string parent_class;
  if (service_->get_extends() != nullptr) {
    parent_class = extends_;
  } else {
    if (style_ == "Cob") {
      parent_class = "::apache::thrift::async::TAsyncDispatchProcessor";
    } else {
      parent_class = "::apache::thrift::TDispatchProcessor";
    }

    if (generator_->gen_templates_) {
      parent_class += "T<Protocol_>";
    }
  }

  // Generate the header portion
  f_header_ <<
    template_header_ <<
    "class " << class_name_ << " : public " << parent_class << " {" << endl;

  f_header_ <<
    " public:" << endl;
  indent_up();
  f_header_ << indent() << "virtual std::string getServiceName() {" << endl;
  indent_up();
  f_header_ << indent()
            << "return \"" << service_name_ << "\";" << endl;
  indent_down();
  f_header_ << indent()
            << "}" << endl;
  indent_down();

  // Protected data members
  f_header_ <<
    " protected:" << endl;
  indent_up();
  f_header_ <<
    indent() << "std::shared_ptr<" << if_name_ << "> iface_;" << endl;
  f_header_ <<
    indent() << "virtual " << ret_type_ << "dispatchCall(" << finish_cob_ <<
      "apache::thrift::protocol::TProtocol* iprot, " <<
      "apache::thrift::protocol::TProtocol* oprot, " <<
      "const std::string& fname, int32_t seqid" << call_context_ << ");" <<
      endl;
  if (generator_->gen_templates_) {
    f_header_ <<
      indent() << "virtual " << ret_type_ << "dispatchCallTemplated(" <<
        finish_cob_ << "Protocol_* iprot, Protocol_* oprot, " <<
        "const std::string& fname, int32_t seqid" << call_context_ << ");" <<
        endl;
  }
  indent_down();

  // Process function declarations
  f_header_ <<
    " private:" << endl;
  indent_up();

  string pf_type =
    generator_->gen_templates_ ? "ProcessFunctions" : "ProcessFunction";

  // Declare processMap_
  f_header_ <<
    indent() << "typedef  void (" << class_name_ << "::*" <<
      "ProcessFunction)(" << finish_cob_decl_ << "int32_t, " <<
      "apache::thrift::protocol::TProtocol*, " <<
      "apache::thrift::protocol::TProtocol*" << call_context_decl_ << ");" <<
      endl;
  if (generator_->gen_templates_) {
    f_header_ <<
      indent() << "typedef void (" << class_name_ << "::*" <<
        "SpecializedProcessFunction)(" << finish_cob_decl_ << "int32_t, " <<
        "Protocol_*, Protocol_*" << call_context_decl_ << ");" <<
        endl <<
      indent() << "struct ProcessFunctions {" << endl <<
      indent() << "  ProcessFunction generic;" << endl <<
      indent() << "  SpecializedProcessFunction specialized;" << endl <<
      indent() << "  ProcessFunctions(ProcessFunction g, " <<
        "SpecializedProcessFunction s) :" << endl <<
      indent() << "    generic(g)," << endl <<
      indent() << "    specialized(s) {}" << endl <<
      indent() << "  ProcessFunctions() : generic(NULL), specialized(NULL) " <<
        "{}" << endl <<
      indent() << "};" << endl;
  }

  if (!generator_->gen_perfhash_) {
    f_header_ <<
      indent() << "typedef std::map<std::string, " << pf_type << "> " <<
        "ProcessMap;" << endl;
    f_header_ <<
      indent() << "ProcessMap processMap_;" << endl;
  }

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(f_header_) <<
      "void process_" << (*f_iter)->get_name() << "(" << finish_cob_ << "int32_t seqid, apache::thrift::protocol::TProtocol* iprot, apache::thrift::protocol::TProtocol* oprot" << call_context_ << ");" << endl;
    if (generator_->gen_templates_) {
      indent(f_header_) <<
        "void process_" << (*f_iter)->get_name() << "(" << finish_cob_ <<
        "int32_t seqid, Protocol_* iprot, Protocol_* oprot" <<
        call_context_ << ");" << endl;
    }
    if (style_ == "Cob") {
      // XXX Factor this out, even if it is a pain.
      string ret_arg = ((*f_iter)->get_returntype()->is_void()
                        ? ""
                        : ", const " + type_name((*f_iter)->get_returntype()) + "& _return");
      f_header_ <<
        indent() << "void return_" << (*f_iter)->get_name() <<
        "(std::function<void(bool ok)> cob, int32_t seqid, " <<
        "apache::thrift::protocol::TProtocol* oprot, " <<
        "apache::thrift::ContextStack* ctx"
                 << ret_arg << ");" << endl;
      if (generator_->gen_templates_) {
        f_header_ <<
          indent() << "void return_" << (*f_iter)->get_name() <<
          "(std::function<void(bool ok)> cob, int32_t seqid, " <<
          "Protocol_* oprot, apache::thrift::ContextStack*"
                   << " ctx" << ret_arg << ");" << endl;
      }
      // XXX Don't declare throw if it doesn't exist
      f_header_ <<
        indent() << "void throw_" << (*f_iter)->get_name() <<
        "(std::function<void(bool ok)> cob, int32_t seqid, " <<
        "apache::thrift::protocol::TProtocol* oprot, " <<
        "apache::thrift::ContextStack* ctx, " <<
        "const std::exception& ex);" << endl;
      if (generator_->gen_templates_) {
        f_header_ <<
          indent() << "void throw_" << (*f_iter)->get_name() <<
          "(std::function<void(bool ok)> cob, int32_t seqid, " <<
          "Protocol_* oprot, " <<
          "apache::thrift::ContextStack* ctx, " <<
          "const std::exception& ex);" << endl;
      }
    }
  }

  f_header_ <<
    " public:" << endl <<
    indent() << class_name_ <<
    "(std::shared_ptr<" << if_name_ << "> iface) :" << endl;
  if (!extends_.empty()) {
    f_header_ <<
      indent() << "  " << extends_ << "(iface)," << endl;
  }
  f_header_ <<
    indent() << "  iface_(iface) {" << endl;
  indent_up();

  if (!generator_->gen_perfhash_) {
    for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
      f_header_ <<
        indent() << "processMap_[\"" << (*f_iter)->get_name() << "\"] = ";
      if (generator_->gen_templates_) {
        f_header_ << "ProcessFunctions(" << endl;
        if (generator_->gen_templates_only_) {
          indent(f_header_) << "  NULL," << endl;
        } else {
          indent(f_header_) << "  &" << class_name_ << "::process_" <<
            (*f_iter)->get_name() << "," << endl;
        }
        indent(f_header_) << "  &" << class_name_ << "::process_" <<
          (*f_iter)->get_name() << ")";
      } else {
        f_header_ << "&" << class_name_ << "::process_" << (*f_iter)->get_name();
      }
      f_header_ <<
        ";" << endl;
    }
  }

  indent_down();
  f_header_ <<
    indent() << "}" << endl <<
    endl <<
    indent() << "virtual ~" << class_name_ << "() {}" << endl << endl;

  f_header_ <<
    indent() << "std::shared_ptr<std::set<std::string> >"
             << " getProcessFunctions() { " << endl;
  indent_up();
  f_header_ <<
    indent() << "std::shared_ptr<std::set<std::string> >"
             << " rSet(new std::set<std::string>());" << endl;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_header_ <<
      indent() << "rSet->insert(\"" << service_name_ << "." <<
      (*f_iter)->get_name() << "\");" << endl;
  }
  f_header_ <<
    indent() << "return rSet;" << endl;
  indent_down();
  f_header_ <<
    indent() << "}" << endl;


  indent_down();
  f_header_ <<
    "};" << endl << endl;

  if (generator_->gen_templates_) {
    // Generate a backwards compatible typedef, for callers who don't know
    // about the new template-style code.
    //
    // We can't use TProtocol as the template parameter, since ProcessorT
    // provides overloaded versions of most methods, one of which accepts
    // TProtocol pointers, and one which accepts Protocol_ pointers.  This
    // results in a compile error if instantiated with Protocol_ == TProtocol.
    // Therefore, we define TDummyProtocol solely so we can use it as the
    // template parameter here.
    f_header_ <<
      "typedef " << class_name_ <<
      "<apache::thrift::protocol::TDummyProtocol> " <<
      service_name_ << pstyle_ << "Processor;" << endl << endl;
  }
}

void ProcessorGenerator::generate_dispatch_call(bool template_protocol) {
  string protocol = "::apache::thrift::protocol::TProtocol";
  string function_suffix;
  if (template_protocol) {
    protocol = "Protocol_";
    // We call the generic version dispatchCall(), and the specialized
    // version dispatchCallTemplated().  We can't call them both
    // dispatchCall(), since this will cause the compiler to issue a warning if
    // a service that doesn't use templates inherits from a service that does
    // use templates: the compiler complains that the subclass only implements
    // the generic version of dispatchCall(), and hides the templated version.
    // Using different names for the two functions prevents this.
    function_suffix = "Templated";
  }

  f_out_ <<
    template_header_ <<
    ret_type_ << class_name_ <<
    template_suffix_ << "::dispatchCall" << function_suffix << "(" <<
    finish_cob_ << protocol << "* iprot, " << protocol << "* oprot, " <<
    "const std::string& fname, int32_t seqid" << call_context_ << ") {" <<
    endl;
  indent_up();

  string pf_type =
    generator_->gen_templates_ ? "ProcessFunctions" : "ProcessFunction";

  // HOT: member function pointer map
  if (generator_->gen_perfhash_) {
    vector<t_function*>::iterator f_iter;
    vector<t_function*> functions = service_->get_functions();
    f_out_ << indent() << "static const " << pf_type << " pfs[] = {" << endl;
    indent_up();
    for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
      if (generator_->gen_templates_) {
        f_out_ << indent() << "ProcessFunctions(" << endl;
        if (generator_->gen_templates_only_) {
          f_out_ << indent() << "  NULL," << endl;
        } else {
          f_out_ << indent() << "  &" << class_name_ << "::process_" <<
            (*f_iter)->get_name() << "," << endl;
        }
        f_out_ << indent() << "  &" << class_name_ << "::process_" <<
          (*f_iter)->get_name() << ")," << endl;
      } else {
        f_out_ << indent() << "&" << class_name_ << "::process_" <<
          (*f_iter)->get_name() << "," << endl;
      }
    }
    indent_down();
    f_out_ << indent() << "};" << endl;

    f_out_ <<
      indent() << "int idx = " << service_name_ << "_method_lookup(fname);" <<
      endl <<
      indent() << "if (idx == -1) {" << endl;
  } else {
    f_out_ <<
      indent() << typename_str_ << "ProcessMap::iterator pfn;" << endl <<
      indent() << "pfn = processMap_.find(fname);" << endl <<
      indent() << "if (pfn == processMap_.end()) {" << endl;
  }

  // error case
  if (extends_.empty()) {
    f_out_ <<
      indent() << "  iprot->skip(apache::thrift::protocol::T_STRUCT);" << endl <<
      indent() << "  iprot->readMessageEnd();" << endl <<
      indent() << "  iprot->getTransport()->readEnd();" << endl <<
      indent() << "  apache::thrift::TApplicationException x(apache::thrift::TApplicationException::UNKNOWN_METHOD, \"Invalid method name: '\"+fname+\"'\");" << endl <<
      indent() << "  oprot->writeMessageBegin(fname, apache::thrift::protocol::T_EXCEPTION, seqid);" << endl <<
      indent() << "  x.write(oprot);" << endl <<
      indent() << "  oprot->writeMessageEnd();" << endl <<
      indent() << "  oprot->getTransport()->writeEnd();" << endl <<
      indent() << "  oprot->getTransport()->flush();" << endl <<
      indent() << (style_ == "Cob" ? "  return cob(true);" : "  return true;") << endl;
  } else {
    f_out_ <<
      indent() << "  return "
               << extends_ << "::dispatchCall" << function_suffix << "("
               << (style_ == "Cob" ? "cob, " : "")
               << "iprot, oprot, fname, seqid" << call_context_arg_ << ");" << endl;
  }

  // brace ending error case block (which exits non-locally)
  f_out_ <<
    indent() << "}" << endl;

  // normal case
  if (generator_->gen_perfhash_) {
    f_out_ << indent() << "const " << pf_type << "& pf = pfs[idx];" << endl;
  } else {
    f_out_ << indent() << "const " << pf_type << "& pf = pfn->second;" << endl;
  }

  if (template_protocol) {
    f_out_ <<
      indent() << "(this->*(pf.specialized))";
  } else {
    if (generator_->gen_templates_only_) {
      // TODO: This is a null pointer, so nothing good will come from calling
      // it.  Throw an exception instead.
      f_out_ <<
        indent() << "(this->*(pf.generic))";
    } else if (generator_->gen_templates_) {
      f_out_ <<
        indent() << "(this->*(pf.generic))";
    } else {
      f_out_ <<
        indent() << "(this->*pf)";
    }
  }
  f_out_ << "(" << cob_arg_ << "seqid, iprot, oprot" <<
    call_context_arg_ << ");" << endl;

  // TODO(dreiss): return pfn ret?
  if (style_ == "Cob") {
    f_out_ <<
      indent() << "return;" << endl;
  } else {
    f_out_ <<
      indent() << "return true;" << endl;
  }

  indent_down();
  f_out_ <<
    "}" << endl <<
    endl;
}

void ProcessorGenerator::generate_process_functions() {
  vector<t_function*> functions = service_->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    if (generator_->gen_templates_) {
      generator_->generate_process_function(service_, *f_iter, style_, false);
      generator_->generate_process_function(service_, *f_iter, style_, true);
    } else {
      generator_->generate_process_function(service_, *f_iter, style_, false);
    }
  }
}

void ProcessorGenerator::generate_factory() {
  string if_factory_name = if_name_ + "Factory";
  string parent_class;
  if (pstyle_ == "Async") {
    // TAsyncProcessorFactory is in the async namespace
    parent_class = "::apache::thrift::async::TAsyncProcessorFactory";
  } else {
    parent_class = "::apache::thrift::TProcessorFactory";
  }

  // Generate the factory class definition
  f_header_ <<
    template_header_ <<
    "class " << factory_class_name_ <<
      " : public " << parent_class << " {" << endl <<
    " public:" << endl;
  indent_up();

  f_header_ <<
    indent() << factory_class_name_ << "(const ::std::shared_ptr< " <<
      if_factory_name << " >& handlerFactory) :" << endl <<
    indent() << "    handlerFactory_(handlerFactory) {}" << endl <<
    endl <<
    indent() << "::std::shared_ptr< ::apache::thrift::T" << pstyle_ <<
      "Processor > " <<
      "getProcessor(::apache::thrift::server::TConnectionContext* ctx);" <<
      endl;

  f_header_ <<
    endl <<
    " protected:" << endl <<
    indent() << "::std::shared_ptr< " << if_factory_name <<
      " > handlerFactory_;" << endl;

  indent_down();
  f_header_ <<
    "};" << endl << endl;

  // If we are generating templates, output a typedef for the plain
  // factory name.
  if (generator_->gen_templates_) {
    f_header_ <<
      "typedef " << factory_class_name_ <<
      "< ::apache::thrift::protocol::TDummyProtocol > " <<
      service_name_ << pstyle_ << "ProcessorFactory;" << endl << endl;
  }

  // Generate the getProcessor() method
  f_out_ <<
    template_header_ <<
    indent() << "::std::shared_ptr< ::apache::thrift::T" << pstyle_ <<
      "Processor > " <<
      factory_class_name_ << template_suffix_ << "::getProcessor(" <<
      "::apache::thrift::server::TConnectionContext* ctx) {" << endl;
  indent_up();

  f_out_ <<
    indent() << "::apache::thrift::ReleaseHandler< " << if_factory_name <<
      " > cleanup(handlerFactory_);" << endl <<
    indent() << "::std::shared_ptr< " << if_name_ << " > handler(" <<
      "handlerFactory_->getHandler(ctx), cleanup);" << endl <<
    indent() << "::std::shared_ptr< ::apache::thrift::T" << pstyle_ <<
      "Processor > processor(new " << class_name_ << template_suffix_ <<
      "(handler));" << endl <<
    indent() << "return processor;" << endl;

  indent_down();
  f_out_ <<
    indent() << "}" << endl;
}

/**
 * Generates a service processor definition.
 *
 * @param tservice The service to generate a processor for.
 */
void t_cpp_generator::generate_service_processor(t_service* tservice,
                                                 string style) {
  ProcessorGenerator generator(this, tservice, style);
  generator.run();
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_cpp_generator::generate_function_helpers(t_service* tservice,
                                                t_function* tfunction) {
  if (tfunction->is_oneway()) {
    return;
  }

  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);

  t_struct result(program_, tservice->get_name() + "_" + tfunction->get_name() + "_result");
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

  generate_struct_definition(f_header_, &result, false);
  if (!bootstrap_) {
    generate_struct_reflection(f_service_, &result);
  }
  generate_struct_reader(out, &result);
  generate_struct_result_writer(out, &result);

  result.set_name(tservice->get_name() + "_" + tfunction->get_name() + "_presult");
  generate_struct_definition(f_header_, &result, false, true, true, gen_cob_style_);
  if (!bootstrap_) {
    generate_struct_reflection(f_service_, &result);
  }
  generate_struct_reader(out, &result, true);
  if (gen_cob_style_) {
    generate_struct_result_writer(out, &result, true);
  }

}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_cpp_generator::generate_process_function(t_service* tservice,
                                                t_function* tfunction,
                                                string style,
                                                bool specialized) {
  t_struct* arg_struct = tfunction->get_arglist();
  const std::vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator f_iter;

  t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;
  string service_func_name = "\"" + tservice->get_name() + "." +
    tfunction->get_name() + "\"";

  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);

  string prot_type =
    (specialized ? "Protocol_" : "apache::thrift::protocol::TProtocol");
  string class_suffix;
  if (gen_templates_) {
    class_suffix = "T<Protocol_>";
  }

  // I tried to do this as one function.  I really did.  But it was too hard.
  if (style != "Cob") {
    // Open function
    if (gen_templates_) {
      out <<
        indent() << "template <class Protocol_>" << endl;
    }
    out <<
      "void " << tservice->get_name() << "Processor" << class_suffix << "::" <<
      "process_" << tfunction->get_name() << "(int32_t seqid, " <<
      prot_type << "* iprot, " << prot_type <<
      "* oprot, apache::thrift::server::TConnectionContext* "
              << "connectionContext)" << endl;
    scope_up(out);

    string argsname = tservice->get_name() + "_" + tfunction->get_name() +
      "_args";
    string resultname = tservice->get_name() + "_" + tfunction->get_name() +
      "_result";

    out <<
      indent() << "std::unique_ptr<apache::thrift::ContextStack> ctx("
               << "this->getContextStack(this->getServiceName(), "
               << service_func_name << ", connectionContext));" << endl
               << endl <<
      indent() << "if (ctx) ctx->preRead();" << endl <<
      indent() << argsname << " args;" << endl <<
      indent() << "try {" << endl;
    indent_up();
    out <<
      indent() << "args.read(iprot);" << endl;
    indent_down();
    out <<
      indent() << "} catch (const apache::thrift::protocol::"
               << "TProtocolException& e) {" << endl;
    indent_up();
    out <<
      indent() << "apache::thrift::TApplicationException x("
                  << "apache::thrift::TApplicationException::PROTOCOL_ERROR, "
                  << "e.what());" << endl <<
      indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
        "\", apache::thrift::protocol::T_EXCEPTION, seqid);" << endl <<
      indent() << "x.write(oprot);" << endl <<
      indent() << "oprot->writeMessageEnd();" << endl <<
      indent() << "oprot->getTransport()->writeEnd();" << endl <<
      indent() << "oprot->getTransport()->flush();" << endl <<
      indent() << "return;" << endl;
    indent_down();
    out <<
      indent() << "}" << endl <<
      indent() << "iprot->readMessageEnd();" << endl <<
      indent() << "uint32_t bytes = iprot->getTransport()->readEnd();"
               << endl << endl <<
      indent() << "if (ctx) ctx->postRead(bytes);" << endl << endl;

    // Declare result
    if (!tfunction->is_oneway()) {
      out <<
        indent() << resultname << " result;" << endl;
    }

    // Try block for functions with exceptions
    out <<
      indent() << "try {" << endl;
    indent_up();

    // Generate the function call
    bool first = true;
    out << indent();
    if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
      if (is_complex_type(tfunction->get_returntype())) {
        first = false;
        out << "iface_->" << tfunction->get_name() << "(result.success";
      } else {
        out << "result.success = iface_->" << tfunction->get_name() << "(";
      }
    } else {
      out <<
        "iface_->" << tfunction->get_name() << "(";
    }
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      out << "args." << (*f_iter)->get_name();
    }
    out << ");" << endl;

    // Set isset on success field
    if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
      out <<
        indent() << "result.__isset.success = true;" << endl;
    }

    indent_down();
    out << indent() << "}";

    if (!tfunction->is_oneway()) {
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        out << " catch (" << type_name((*x_iter)->get_type()) << " &" <<
          (*x_iter)->get_name() << ") {" << endl;
        if (!tfunction->is_oneway()) {
          indent_up();
          out <<
            indent() << "result." << (*x_iter)->get_name() << " = " <<
              (*x_iter)->get_name() << ";" << endl <<
            indent() << "result.__isset." << (*x_iter)->get_name() <<
              " = true;" << endl;
          indent_down();
          out << indent() << "}";
        } else {
          out << "}";
        }
      }
    }

    out << " catch (const std::exception& e) {" << endl;

    indent_up();
    out <<
      indent() << "if (ctx) ctx->handlerError();" << endl << endl;

    if (!tfunction->is_oneway()) {
      out <<
        endl <<
        indent() << "apache::thrift::TApplicationException x(e.what());" <<
          endl <<
        indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
          "\", apache::thrift::protocol::T_EXCEPTION, seqid);" << endl <<
        indent() << "x.write(oprot);" << endl <<
        indent() << "oprot->writeMessageEnd();" << endl <<
        indent() << "oprot->getTransport()->writeEnd();" << endl <<
        indent() << "oprot->getTransport()->flush();" << endl;
    }
    out << indent() << "return;" << endl;
    indent_down();
    out << indent() << "}" << endl << endl;

    // Shortcut out here for oneway functions
    if (tfunction->is_oneway()) {
      out <<
        indent() << "if (ctx) ctx->asyncComplete();" << endl <<
        indent() << "return;" << endl;
      indent_down();
      out << "}" << endl <<
        endl;
      return;
    }

    // Serialize the result into a struct
    out <<
      indent() << "if (ctx) ctx->preWrite();" << endl <<
      indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
        "\", apache::thrift::protocol::T_REPLY, seqid);" << endl <<
      indent() << "result.write(oprot);" << endl <<
      indent() << "oprot->writeMessageEnd();" << endl <<
      indent() << "bytes = oprot->getTransport()->writeEnd();" << endl <<
      indent() << "oprot->getTransport()->flush();" << endl << endl <<
      indent() << "if (ctx) ctx->postWrite(bytes);" << endl << endl;

    // Close function
    scope_down(out);
    out << endl;
  }

  // Cob style.
  else {
    // Processor entry point.
    // TODO(edhall) update for callContext when TEventServer is ready
    if (gen_templates_) {
      out <<
        indent() << "template <class Protocol_>" << endl;
    }
    out <<
      "void " << tservice->get_name() << "AsyncProcessor" << class_suffix <<
      "::process_" << tfunction->get_name() <<
      "(std::function<void(bool ok)> cob, int32_t seqid, " <<
      prot_type << "* iprot, " << prot_type <<
      "* oprot, apache::thrift::server::TConnectionContext* " <<
      "connectionContext)" << endl;
    scope_up(out);

    // TODO(simpkins): we could try to consoldate this
    // with the non-cob code above
    if (gen_templates_ && !specialized) {
      // If these are instances of Protocol_, instead of any old TProtocol,
      // use the specialized process function instead.
      out <<
        indent() << "Protocol_* _iprot = dynamic_cast<Protocol_*>(iprot);" <<
        endl <<
        indent() << "Protocol_* _oprot = dynamic_cast<Protocol_*>(oprot);" <<
        endl <<
        indent() << "if (_iprot && _oprot) {" << endl <<
        indent() << "  return process_" << tfunction->get_name() <<
        "(cob, seqid, _iprot, _oprot, connectionContext);" << endl <<
        indent() << "}" << endl <<
        indent() << "T_GENERIC_PROTOCOL(this, iprot, _iprot);" << endl <<
        indent() << "T_GENERIC_PROTOCOL(this, oprot, _oprot);" << endl << endl;
    }

    out <<
      indent() << tservice->get_name() + "_" + tfunction->get_name() <<
        "_args args;" << endl <<
      indent() << "std::unique_ptr<apache::thrift::ContextStack> ctx("
               << "this->getContextStack(this->getServiceName(), "
               << service_func_name << ", connectionContext));" << endl
               << endl <<
      indent() << "try {" << endl;
    indent_up();
    out <<
      indent() << "if (ctx) ctx->preRead();" << endl <<
      indent() << "args.read(iprot);" << endl <<
      indent() << "iprot->readMessageEnd();" << endl <<
      indent() << "uint32_t bytes = iprot->getTransport()->readEnd();" <<
        endl <<
      indent() << "if (ctx) ctx->postRead(bytes);" << endl << endl;
    scope_down(out);

    // TODO(dreiss): Handle TExceptions?  Expose to server?
    out <<
      indent() << "catch (const std::exception& exn) {" << endl <<
      indent() << "  if (ctx) ctx->handlerError();" << endl <<
      indent() << "  return cob(false);" << endl <<
      indent() << "}" << endl;

    if (tfunction->is_oneway()) {
      out <<
        indent() << "if (ctx) ctx->asyncComplete();" << endl << endl;
    }

    if (tfunction->is_oneway()) {
      // No return.  Just hand off our cob.
      // TODO(dreiss): Call the cob immediately?
      out <<
        indent() << "iface_->" << tfunction->get_name() << "(" <<
        "std::bind(cob, true)" << endl;
      indent_up(); indent_up();
    } else {
      out <<
        indent() << "apache::thrift::ContextStack *contextStack = "
                 << "ctx.release();" << endl;

      string ret_arg, ret_placeholder;
      if (!tfunction->get_returntype()->is_void()) {
        ret_arg = ", const " + type_name(tfunction->get_returntype()) +
          "& _return";
        ret_placeholder = ", std::placeholders::_1";
      }

      // When gen_templates_ is true, the return_ and throw_ functions are
      // overloaded.  We have to declare pointers to them so that the compiler
      // can resolve the correct overloaded version.
      out <<
        indent() << "void (" << tservice->get_name() << "AsyncProcessor" <<
        class_suffix << "::*return_fn)(std::function<void(bool ok)> " <<
        "cob, int32_t seqid, " << prot_type <<
        "* oprot, apache::thrift::ContextStack* ctx" <<
        ret_arg << ") =" << endl;
      out <<
        indent() << "  &" << tservice->get_name() << "AsyncProcessor" <<
        class_suffix << "::return_" << tfunction->get_name() << ";" << endl;
      if (!xceptions.empty()) {
        out <<
          indent() << "void (" << tservice->get_name() << "AsyncProcessor" <<
          class_suffix << "::*throw_fn)(std::function<void(bool ok)> " <<
          "cob, int32_t seqid, " << prot_type <<
          "* oprot, apache::thrift::ContextStack* ctx, " <<
          "const std::exception& ex) =" << endl;
        out <<
          indent() << "  &" << tservice->get_name() << "AsyncProcessor" <<
          class_suffix << "::throw_" << tfunction->get_name() << ";" << endl;
      }

      out <<
        indent() << "iface_->" << tfunction->get_name() << "(" << endl;
      indent_up(); indent_up();
      out <<
        indent() << "std::bind(return_fn, this, cob, seqid, oprot, "
                 << "contextStack" << ret_placeholder << ")";
      if (!xceptions.empty()) {
        out
          << ',' << endl <<
          indent() << "std::bind(throw_fn, this, cob, seqid, oprot, " <<
          "contextStack, std::placeholders::_1)";
      }
    }

    // XXX Whitespace cleanup.
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      out
                 << ',' << endl <<
        indent() << "args." << (*f_iter)->get_name();
    }
    out << ");" << endl;
    indent_down(); indent_down();
    scope_down(out);
    out << endl;

    // Normal return.
    if (!tfunction->is_oneway()) {
      string ret_arg_decl, ret_arg_name;
      if (!tfunction->get_returntype()->is_void()) {
        ret_arg_decl = ", const " + type_name(tfunction->get_returntype()) +
          "& _return";
        ret_arg_name = ", _return";
      }
      if (gen_templates_) {
        out <<
          indent() << "template <class Protocol_>" << endl;
      }
      out <<
        "void " << tservice->get_name() << "AsyncProcessor" << class_suffix <<
        "::return_" << tfunction->get_name() <<
        "(std::function<void(bool ok)> cob, int32_t seqid, "
                << prot_type << "* oprot, " <<
        "apache::thrift::ContextStack* contextStack"
                << ret_arg_decl << ')' << endl;
      scope_up(out);

      out <<
        indent() << "std::unique_ptr<apache::thrift::ContextStack> "
                 << "ctx(contextStack);" << endl;
      if (gen_templates_ && !specialized) {
        // If oprot is a Protocol_ instance,
        // use the specialized return function instead.
        out <<
          indent() << "Protocol_* _oprot = dynamic_cast<Protocol_*>(oprot);" <<
          endl <<
          indent() << "if (_oprot) {" << endl <<
          indent() << "  return return_" << tfunction->get_name()
                   << "(cob, seqid, _oprot, ctx.release()" << ret_arg_name
                   << ");" << endl <<
          indent() << "}" << endl <<
          indent() << "T_GENERIC_PROTOCOL(this, oprot, _oprot);" <<
          endl << endl;
      }

      out <<
        indent() << tservice->get_name() << "_" << tfunction->get_name() <<
          "_presult result;" << endl;
      if (!tfunction->get_returntype()->is_void()) {
        // The const_cast here is unfortunate, but it would be a pain to avoid,
        // and we only do a write with this struct, which is const-safe.
        out <<
          indent() << "result.success = const_cast<" <<
            type_name(tfunction->get_returntype()) << "*>(&_return);" <<
            endl <<
          indent() << "result.__isset.success = true;" << endl;
      }
      // Serialize the result into a struct
      out <<
        endl <<
        indent() << "if (ctx) ctx->preWrite();" << endl << endl <<
        indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
          "\", apache::thrift::protocol::T_REPLY, seqid);" << endl <<
        indent() << "result.write(oprot);" << endl <<
        indent() << "oprot->writeMessageEnd();" << endl <<
        indent() << "uint32_t bytes = oprot->getTransport()->writeEnd();" <<
          endl <<
        indent() << "oprot->getTransport()->flush();" << endl <<
        indent() << "if (ctx) ctx->postWrite(bytes);" << endl << endl <<
        indent() << "return cob(true);" << endl;
      scope_down(out);
      out << endl;
    }

    // Exception return.
    if (!tfunction->is_oneway() && !xceptions.empty()) {
      if (gen_templates_) {
        out <<
          indent() << "template <class Protocol_>" << endl;
      }
      out <<
        "void " << tservice->get_name() << "AsyncProcessor" << class_suffix <<
        "::throw_" << tfunction->get_name() <<
        "(std::function<void(bool ok)> cob, int32_t seqid, "
                << prot_type << "* oprot, " <<
        "apache::thrift::ContextStack* contextStack, " <<
        "const std::exception& ex)" << endl;
      scope_up(out);

      out <<
        indent() << "std::unique_ptr<apache::thrift::ContextStack> "
                 << "ctx(contextStack);" << endl;
      if (gen_templates_ && !specialized) {
        // If oprot is a Protocol_ instance,
        // use the specialized throw function instead.
        out <<
          indent() << "Protocol_* _oprot = dynamic_cast<Protocol_*>(oprot);" <<
          endl <<
          indent() << "if (_oprot) {" << endl <<
          indent() << "  return throw_" << tfunction->get_name() <<
          "(cob, seqid, _oprot, ctx.release(), ex);" << endl <<
          indent() << "}" << endl <<
          indent() << "T_GENERIC_PROTOCOL(this, oprot, _oprot);" <<
          endl << endl;
      }

      // Determine the type of the exception
      // Note that we perform a dynamic_cast on the pointer type, rather than
      // the reference type, so that we don't have to deal with exceptions
      // thrown by dynamic_cast.
      out <<
        indent() << tservice->get_name() << "_" << tfunction->get_name() <<
        "_result result;" << endl;
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        out << indent();
        if (x_iter != xceptions.begin()) {
          out << "} else ";
        }
        out << "if (dynamic_cast<const " << type_name((*x_iter)->get_type()) <<
          "*>(&ex) != NULL) {" << endl;
        indent_up();
        // We know the dynamic_cast succeeded; Just perform a static_cast here.
        // We perform two casts just because it is easier than generating
        // temporary variables to store the result of the first cast.
        out <<
          indent() << "result." << (*x_iter)->get_name() << " = " <<
            "static_cast<const " << type_name((*x_iter)->get_type()) <<
            "&>(ex);" << endl <<
          indent() << "result.__isset." << (*x_iter)->get_name() <<
            " = true;" << endl;
        indent_down();
      }

      // Handle an undeclared exception type
      out <<
        indent() << "} else {" << endl;
      indent_up();
      out <<
        indent() << "if (ctx) ctx->handlerError();" << endl << endl <<
        indent() << "apache::thrift::TApplicationException x(ex.what());" <<
          endl <<
        indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
          "\", apache::thrift::protocol::T_EXCEPTION, seqid);" << endl <<
        indent() << "x.write(oprot);" << endl <<
        indent() << "oprot->writeMessageEnd();" << endl <<
        indent() << "oprot->getTransport()->writeEnd();" << endl <<
        indent() << "oprot->getTransport()->flush();" << endl <<
        // We pass true to the cob here, since we did successfully write a
        // response, even though it is an exception response.
        // It looks like the argument is currently ignored, anyway.
        indent() << "return cob(true);" << endl;
      scope_down(out);

      // Serialize the result into a struct
      out <<
        indent() << "if (ctx) ctx->preWrite();" << endl << endl <<
        indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
          "\", apache::thrift::protocol::T_REPLY, seqid);" << endl <<
        indent() << "result.write(oprot);" << endl <<
        indent() << "oprot->writeMessageEnd();" << endl <<
        indent() << "uint32_t bytes = oprot->getTransport()->writeEnd();" <<
          endl <<
        indent() << "oprot->getTransport()->flush();" << endl <<
        indent() << "if (ctx) ctx->postWrite(bytes);" << endl <<
        indent() << "return cob(true);" << endl;
      scope_down(out);
      out << endl;
    }
  } // cob style
}

/**
 * Generates a skeleton file of a server
 *
 * @param tservice The service to generate a server for.
 */
void t_cpp_generator::generate_service_skeleton(t_service* tservice) {
  string svcname = tservice->get_name();

  // Service implementation file includes
  string f_skeleton_name = get_out_dir()+svcname+"_server.skeleton.cpp";

  string ns = namespace_prefix(tservice->get_program()->get_namespace("cpp"));

  ofstream f_skeleton;
  f_skeleton.open(f_skeleton_name.c_str());
  record_genfile(f_skeleton_name);
  f_skeleton <<
    autogen_comment() << "\n" <<
    "// This autogenerated skeleton file illustrates how to build a server.\n"
    "// You should copy it to another filename to avoid overwriting it.\n"
    "\n"
    "#include \"" << get_include_prefix(*get_program()) << svcname << ".h\"\n"<<
    "#include <thrift/lib/cpp/async/TEventServer.h>\n"
    "#include <thrift/lib/cpp/server/TConnectionContext.h>\n"
    "#include <thrift/lib/cpp/util/TEventServerCreator.h>\n"
    "\n"
    "using apache::thrift::TProcessor;\n"
    "using apache::thrift::async::TEventServer;\n"
    "using apache::thrift::util::TEventServerCreator;\n"
    "\n"
    "using std::shared_ptr;\n"
    "\n";

  if (!ns.empty()) {
    f_skeleton <<
      "using namespace " << string(ns, 0, ns.size()-2) << ";\n" <<
      "\n";
  }

  f_skeleton <<
    "class " << svcname << "Handler : virtual public " << svcname << "If {\n" <<
    " public:\n";
  indent_up();
  f_skeleton <<
    indent() << svcname << "Handler() {\n" <<
    indent() << "  // Your initialization goes here\n" <<
    indent() << "}\n" <<
    "\n";

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_skeleton <<
      indent() << function_signature(*f_iter, "") << " {\n" <<
      indent() << "  // Your implementation goes here\n" <<
      indent() << "  printf(\"" << (*f_iter)->get_name() << "\\n\");\n";
    t_type* return_type = (*f_iter)->get_returntype();
    if (!return_type->is_void() && !is_complex_type(return_type)) {
      f_skeleton <<
        indent() << "  return " << type_name(return_type) << "();\n";
    }
    f_skeleton <<
      indent() << "}\n" <<
      "\n";
  }

  indent_down();
  f_skeleton <<
    "};\n"
    "\n";

  f_skeleton <<
    indent() << "int main(int argc, char **argv) {\n";
  indent_up();
  f_skeleton <<
    indent() << "int port = 9090;\n" <<
    indent() << "shared_ptr<" << svcname << "Handler> handler(new " <<
      svcname << "Handler());\n" <<
    indent() << "shared_ptr<TProcessor> processor(new " <<
      svcname << "Processor(handler));\n" <<
    indent() << "TEventServerCreator serverCreator(processor, port);\n" <<
    indent() << "shared_ptr<TEventServer> server = " <<
      "serverCreator.createEventServer();\n" <<
    indent() << "server->serve();\n" <<
    indent() << "return 0;\n";
  indent_down();
  f_skeleton <<
    "}\n";

  // Close the files
  f_skeleton.close();
}

void t_cpp_generator::generate_service_perfhash_keywords(t_service* tservice) {
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  int i = 0;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter, ++i) {
    f_service_gperf_ << (*f_iter)->get_name() << ", " << i << endl;
  }
}

/**
 * Deserializes a field of any type.
 */
void t_cpp_generator::generate_deserialize_field(ofstream& out,
                                                 t_field* tfield,
                                                 string prefix,
                                                 string suffix) {
  t_type* f_type = tfield->get_type();
  suffix = get_type_access_suffix(f_type) + suffix;
  t_type* type = get_true_type(f_type);

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " +
      prefix + tfield->get_name();
  }

  string name = prefix + tfield->get_name() + suffix;

  if (type->is_struct() || type->is_xception()) {
    auto pointer = is_reference(tfield);
    generate_deserialize_struct(out, (t_struct*)type, name, pointer);
  } else if (type->is_container()) {
    generate_deserialize_container(out, type, name);
  } else if (type->is_base_type()) {
    indent(out) <<
      "xfer += iprot->";
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "compiler error: cannot serialize void field in a struct: " + name;
      break;
    case t_base_type::TYPE_STRING:
      if (((t_base_type*)type)->is_binary()) {
        out << "readBinary(" << name << ");";
      }
      else {
        out << "readString(" << name << ");";
      }
      break;
    case t_base_type::TYPE_BOOL:
      out << "readBool(" << name << ");";
      break;
    case t_base_type::TYPE_BYTE:
      out << "readByte(" << name << ");";
      break;
    case t_base_type::TYPE_I16:
      out << "readI16(" << name << ");";
      break;
    case t_base_type::TYPE_I32:
      out << "readI32(" << name << ");";
      break;
    case t_base_type::TYPE_I64:
      out << "readI64(" << name << ");";
      break;
    case t_base_type::TYPE_DOUBLE:
      out << "readDouble(" << name << ");";
      break;
    case t_base_type::TYPE_FLOAT:
      out << "readFloat(" << name << ");";
      break;
    default:
      throw "compiler error: no C++ reader for base type " + t_base_type::t_base_name(tbase) + name;
    }
    out <<
      endl;
  } else if (type->is_enum()) {
    string t = tmp("ecast");
    out <<
      indent() << "int32_t " << t << ";" << endl <<
      indent() << "xfer += iprot->readI32(" << t << ");" << endl <<
      indent() << name << " = (" << type_name(type) << ")" << t << ";" << endl;
  } else {
    printf("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
           tfield->get_name().c_str(), type_name(type).c_str());
  }
}

/**
 * Generates an unserializer for a variable. This makes two key assumptions,
 * first that there is a const char* variable named data that points to the
 * buffer for deserialization, and that there is a variable protocol which
 * is a reference to a TProtocol serialization object.
 */
void t_cpp_generator::generate_deserialize_struct(ofstream& out,
                                                  t_struct* tstruct,
                                                  string prefix,
                                                  bool pointer) {
  bool can_throw = type_can_throw(tstruct);
  if (can_throw) {
    indent(out) << "try {" << endl;
    indent_up();
  }
  if (pointer) {
    indent(out) << prefix << " = std::unique_ptr< " << type_name(tstruct) <<
      ">(new " << type_name(tstruct) << ");" << endl;
    indent(out) <<
      "xfer += " << prefix << "->read(iprot);" << endl;
    indent(out) << "if (false) {" << endl;
    for (auto& member : tstruct->get_members()) {
      if (is_reference(member)){
        indent(out) << "} else if (" << prefix << "->" << member->get_name()
                    << ") {" << endl;
      } else if (has_isset(member)) {
        indent(out) << "} else if (" << prefix << "->__isset."
                    << member->get_name() << ") {" << endl;
      }
    }
    indent(out) << "} else { " << prefix << " = nullptr; }" << endl;
  } else {
    indent(out) <<
      "xfer += " << prefix << ".read(iprot);" << endl;
  }
  if (can_throw) {
    indent_down();
    indent(out) << "} catch (const TProtocolException& e) {" << endl;
    indent_up();
    indent(out) << "if (e.getType() != " <<
                     "TProtocolException::MISSING_REQUIRED_FIELD) {" << endl;
    indent_up();
    indent(out) << "throw;" << endl;
    indent_down();
    indent(out) << "}" << endl;
    indent(out) << "exception = std::current_exception();" << endl;
    indent_down();
    indent(out) << "}" << endl;
  }
}

void t_cpp_generator::generate_deserialize_container(ofstream& out,
                                                     t_type* ttype,
                                                     string prefix) {
  scope_up(out);

  string size = tmp("_size");
  string sizeUnknown = tmp("_sizeUnknown");
  string ktype = tmp("_ktype");
  string vtype = tmp("_vtype");
  string etype = tmp("_etype");

  t_container* tcontainer = (t_container*)ttype;
  // One of them at least is != than annotations_.end()
  bool use_push = tcontainer->annotations_.find("cpp.type")
    != tcontainer->annotations_.find("cpp.template");

  indent(out) <<
    prefix << ".clear();" << endl <<
    indent() << "uint32_t " << size << ";" << endl <<
    indent() << "bool " << sizeUnknown << ";" << endl;

  // Declare variables, read header
  if (ttype->is_map()) {
    out <<
      indent() << "apache::thrift::protocol::TType " << ktype << ";" << endl <<
      indent() << "apache::thrift::protocol::TType " << vtype << ";" << endl <<
      indent() << "xfer += iprot->readMapBegin(" <<
        ktype << ", " << vtype << ", " << size << ", " << sizeUnknown << ");" <<
        endl;
  } else if (ttype->is_set()) {
    out <<
      indent() << "apache::thrift::protocol::TType " << etype << ";" << endl <<
      indent() << "xfer += iprot->readSetBegin(" <<
                   etype << ", " << size << ", " << sizeUnknown << ");" << endl;
  } else if (ttype->is_list()) {
    out <<
      indent() << "apache::thrift::protocol::TType " << etype << ";" << endl <<
      indent() << "xfer += iprot->readListBegin(" <<
      etype << ", " << size << ", " << sizeUnknown << ");" << endl;
  }

  out <<
    indent() << "if (!" << sizeUnknown << ") {" << endl;

    indent_up();

    if (ttype->is_list() && !use_push) {
      out << indent() << prefix << ".resize(" << size << ");" << endl;
    }

    // For loop iterates over elements
    string i = tmp("_i");
    out <<
      indent() << "uint32_t " << i << ";" << endl <<
      indent() << "for ("
               << i << " = 0; " << i << " < " << size << "; ++" << i << ")"
               << endl;

      scope_up(out);

      if (ttype->is_map()) {
        generate_deserialize_map_element(out, (t_map*)ttype, prefix);
      } else if (ttype->is_set()) {
        generate_deserialize_set_element(out, (t_set*)ttype, prefix);
      } else if (ttype->is_list()) {
        generate_deserialize_list_element(out, (t_list*)ttype, prefix,
                                          use_push, i);
      }

      scope_down(out);

    indent_down();
  out << indent() << "} else {" << endl;
    indent_up();

      if (ttype->is_map()) {
        out << indent() << "while (iprot->peekMap())" << endl;
      } else if (ttype->is_set()) {
        out << indent() << "while (iprot->peekSet())" << endl;
      } else if (ttype->is_list()) {
        out << indent() << "while (iprot->peekList())" << endl;
      }

      scope_up(out);

      if (ttype->is_map()) {
        generate_deserialize_map_element(out, (t_map*)ttype, prefix);
      } else if (ttype->is_set()) {
        generate_deserialize_set_element(out, (t_set*)ttype, prefix);
      } else if (ttype->is_list()) {
        generate_deserialize_list_element(out, (t_list*)ttype, prefix, true, i);
      }

      scope_down(out);
    indent_down();
  indent(out) << "}" << endl;

  // Read container end
  if (ttype->is_map()) {
    indent(out) << "xfer += iprot->readMapEnd();" << endl;
  } else if (ttype->is_set()) {
    indent(out) << "xfer += iprot->readSetEnd();" << endl;
  } else if (ttype->is_list()) {
    indent(out) << "xfer += iprot->readListEnd();" << endl;
  }

  scope_down(out);
}


/**
 * Generates code to deserialize a map
 */
void t_cpp_generator::generate_deserialize_map_element(ofstream& out,
                                                       t_map* tmap,
                                                       string prefix) {
  string key = tmp("_key");
  string val = tmp("_val");
  t_field fkey(tmap->get_key_type(), key);
  t_field fval(tmap->get_val_type(), val);

  out <<
    indent() << declare_field(&fkey) << endl;

  generate_deserialize_field(out, &fkey);
  indent(out) <<
    declare_field(&fval, false, false, false, true) << " = " <<
    prefix << "[" << key << "];" << endl;

  generate_deserialize_field(out, &fval);
}

void t_cpp_generator::generate_deserialize_set_element(ofstream& out,
                                                       t_set* tset,
                                                       string prefix) {
  string elem = tmp("_elem");
  t_field felem(tset->get_elem_type(), elem);

  indent(out) <<
    declare_field(&felem) << endl;

  generate_deserialize_field(out, &felem);

  indent(out) <<
    prefix << ".insert(" << elem << ");" << endl;
}

void t_cpp_generator::generate_deserialize_list_element(ofstream& out,
                                                        t_list* tlist,
                                                        string prefix,
                                                        bool use_push,
                                                        string index) {
  if (use_push) {
    string elem = tmp("_elem");
    t_field felem(tlist->get_elem_type(), elem);
    indent(out) << declare_field(&felem) << endl;
    generate_deserialize_field(out, &felem);
    indent(out) << prefix << ".push_back(" << elem << ");" << endl;
  } else {
    t_field felem(tlist->get_elem_type(), prefix + "[" + index + "]");
    generate_deserialize_field(out, &felem);
  }
}


/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_cpp_generator::generate_serialize_field(ofstream& out,
                                               t_field* tfield,
                                               string prefix,
                                               string suffix) {
  t_type* f_type = tfield->get_type();
  suffix = get_type_access_suffix(f_type) + suffix;
  t_type* type = get_true_type(f_type);

  string name = prefix + tfield->get_name() + suffix;

  // Do nothing for void types
  if (type->is_void()) {
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " + name;
  }

  if (type->is_struct() || type->is_xception()) {
    auto pointer = is_reference(tfield);
    generate_serialize_struct(out,
                              (t_struct*)type,
                              name, pointer);
  } else if (type->is_container()) {
    generate_serialize_container(out, type, name);
  } else if (type->is_base_type() || type->is_enum()) {

    indent(out) <<
      "xfer += oprot->";

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
        }
        else {
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
        throw "compiler error: no C++ writer for base type " + t_base_type::t_base_name(tbase) + name;
      }
    } else if (type->is_enum()) {
      out << "writeI32((int32_t)" << name << ");";
    }
    out << endl;
  } else {
    printf("DO NOT KNOW HOW TO SERIALIZE FIELD '%s' TYPE '%s'\n",
           name.c_str(),
           type_name(type).c_str());
  }
}

/**
 * Serializes all the members of a struct.
 *
 * @param tstruct The struct to serialize
 * @param prefix  String prefix to attach to all fields
 */
void t_cpp_generator::generate_serialize_struct(ofstream& out,
                                                t_struct* tstruct,
                                                string prefix,
                                               bool pointer) {
  if (pointer) {
    indent(out) << "if (" << prefix << ") {" <<
      "xfer += " << prefix << "->write(oprot); " << endl
                << "} else {" << "oprot->writeStructBegin(\"" <<
      tstruct->get_name() << "\"); oprot->writeStructEnd(); oprot->writeFieldStop();}" << endl;
  } else {
    indent(out) <<
      "xfer += " << prefix << ".write(oprot);" << endl;
  }
}

void t_cpp_generator::generate_serialize_container(ofstream& out,
                                                   t_type* ttype,
                                                   string prefix) {
  scope_up(out);

  if (ttype->is_map()) {
    indent(out) <<
      "xfer += oprot->writeMapBegin(" <<
      type_to_enum(((t_map*)ttype)->get_key_type()) << ", " <<
      type_to_enum(((t_map*)ttype)->get_val_type()) << ", " <<
      prefix << ".size());" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "xfer += oprot->writeSetBegin(" <<
      type_to_enum(((t_set*)ttype)->get_elem_type()) << ", " <<
      prefix << ".size());" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "xfer += oprot->writeListBegin(" <<
      type_to_enum(((t_list*)ttype)->get_elem_type()) << ", " <<
      prefix << ".size());" << endl;
  }

  string iter = tmp("_iter");
  out <<
    indent() << type_name(ttype) << "::const_iterator " << iter << ";" <<
      endl <<
    indent() << "for (" << iter << " = " << prefix  << ".begin(); " << iter <<
      " != " << prefix << ".end(); ++" << iter << ")" << endl;
  scope_up(out);
    if (ttype->is_map()) {
      generate_serialize_map_element(out, (t_map*)ttype, iter);
    } else if (ttype->is_set()) {
      generate_serialize_set_element(out, (t_set*)ttype, iter);
    } else if (ttype->is_list()) {
      generate_serialize_list_element(out, (t_list*)ttype, iter);
    }
  scope_down(out);

  if (ttype->is_map()) {
    indent(out) <<
      "xfer += oprot->writeMapEnd();" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "xfer += oprot->writeSetEnd();" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "xfer += oprot->writeListEnd();" << endl;
  }

  scope_down(out);
}

/**
 * Serializes the members of a map.
 *
 */
void t_cpp_generator::generate_serialize_map_element(ofstream& out,
                                                     t_map* tmap,
                                                     string iter) {
  t_field kfield(tmap->get_key_type(), iter + "->first");
  generate_serialize_field(out, &kfield, "");

  t_field vfield(tmap->get_val_type(), iter + "->second");
  generate_serialize_field(out, &vfield, "");
}

/**
 * Serializes the members of a set.
 */
void t_cpp_generator::generate_serialize_set_element(ofstream& out,
                                                     t_set* tset,
                                                     string iter) {
  t_field efield(tset->get_elem_type(), "(*" + iter + ")");
  generate_serialize_field(out, &efield, "");
}

/**
 * Serializes the members of a list.
 */
void t_cpp_generator::generate_serialize_list_element(ofstream& out,
                                                      t_list* tlist,
                                                      string iter) {
  t_field efield(tlist->get_elem_type(), "(*" + iter + ")");
  generate_serialize_field(out, &efield, "");
}

/**
 * Makes a :: prefix for a namespace
 *
 * @param ns The namepsace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_prefix(string ns, string delimiter) const {
  // Always start with "::", to avoid possible name collisions with
  // other names in one of the current namespaces.
  //
  // We also need a leading space, in case the name is used inside of a
  // template parameter.  "MyTemplate<::foo::Bar>" is not valid C++,
  // since "<:" is an alternative token for "[".
  string result = " " + delimiter;

  if (ns.size() == 0) {
    return result;
  }
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += ns.substr(0, loc);
    result += delimiter;
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += ns + delimiter;
  }
  return result;
}

/**
 * Opens namespace.
 *
 * @param ns The namepsace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_open(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "";
  string separator = "";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += separator;
    result += "namespace ";
    result += ns.substr(0, loc);
    result += " {";
    separator = " ";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += separator + "namespace " + ns + " {";
  }
  return result;
}

/**
 * Closes namespace.
 *
 * @param ns The namepsace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_close(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "}";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += "}";
    ns = ns.substr(loc+1);
  }
  result += " // namespace";
  return result;
}

/**
 * Returns a C++ type name
 *
 * @param ttype The type
 * @return String of the type name, i.e. std::set<type>
 */
string t_cpp_generator::type_name(t_type* ttype, int flags) {
  bool in_typedef = flags & IN_TYPEDEF;
  bool arg = flags & IN_ARG;
  bool chase_typedefs = flags & CHASE_TYPEDEFS;
  bool always_namespace = flags & ALWAYS_NAMESPACE;

  flags &= ~IN_ARG;  // the other flags propagate

  if (chase_typedefs) {
    ttype = get_true_type(ttype);
  }
  if (ttype->is_base_type()) {
    t_base_type* btype = static_cast<t_base_type*>(ttype);
    string bname = base_type_name(btype->get_base());
    std::map<string, string>::iterator it = ttype->annotations_.find("cpp.type");
    if (it != ttype->annotations_.end()) {
      bname = it->second;
    }

    if (arg && (btype->get_base() == t_base_type::TYPE_STRING)) {
      return "const " + bname + "&";
    } else {
      return bname;
    }
  }

  // Check for a custom overloaded C++ name
  if (ttype->is_container()) {
    string cname;

    t_container* tcontainer = (t_container*) ttype;

    std::map<string, string>::iterator it
      = tcontainer->annotations_.find("cpp.type");
    if (it != tcontainer->annotations_.end()) {
      cname = it->second;
    } else {
      it = tcontainer->annotations_.find("cpp.template");
      if (ttype->is_map()) {
        t_map* tmap = (t_map*) ttype;
        if (it != tcontainer->annotations_.end()) {
          cname = it->second;
        } else if (tmap->is_unordered()) {
          cname = "std::unordered_map";
        } else {
          cname = "std::map";
        }
        cname = cname + "<" +
          type_name(tmap->get_key_type(), flags) + ", " +
          type_name(tmap->get_val_type(), flags) + "> ";
      } else if (ttype->is_set()) {
        t_set* tset = (t_set*) ttype;
        if (it != tcontainer->annotations_.end())
          cname = it->second;
        else
          cname = "std::set";
        cname = cname + "<" + type_name(tset->get_elem_type(), flags) + "> ";
      } else if (ttype->is_list()) {
        t_list* tlist = (t_list*) ttype;
        if (it != tcontainer->annotations_.end())
          cname = it->second;
        else
          cname = "std::vector";
        cname = cname + "<" + type_name(tlist->get_elem_type(), flags) + "> ";
      }
    }

    if (arg) {
      return "const " + cname + "&";
    } else {
      return cname;
    }
  }

  string class_prefix;
  if (in_typedef && (ttype->is_struct() || ttype->is_xception())) {
    class_prefix = "class ";
  }

  // Check if it needs to be namespaced
  string pname;
  t_program* program = ttype->get_program();
  if (program != nullptr && (always_namespace || program != program_)) {
    pname =
      class_prefix +
      namespace_prefix(program->get_namespace("cpp")) +
      ttype->get_name();
  } else {
    pname = class_prefix + ttype->get_name();
  }

  if (arg && is_complex_type(ttype)) {
    return "const " + pname + "&";
  } else {
    return pname;
  }
}

/**
 * Returns the C++ type that corresponds to the thrift type.
 *
 * @param tbase The base type
 * @return Explicit C++ type, i.e. "int32_t"
 */
string t_cpp_generator::base_type_name(t_base_type::t_base tbase) {
  switch (tbase) {
  case t_base_type::TYPE_VOID:
    return "void";
  case t_base_type::TYPE_STRING:
    return "std::string";
  case t_base_type::TYPE_BOOL:
    return "bool";
  case t_base_type::TYPE_BYTE:
    return "int8_t";
  case t_base_type::TYPE_I16:
    return "int16_t";
  case t_base_type::TYPE_I32:
    return "int32_t";
  case t_base_type::TYPE_I64:
    return "int64_t";
  case t_base_type::TYPE_DOUBLE:
    return "double";
  case t_base_type::TYPE_FLOAT:
    return "float";
  default:
    throw "compiler error: no C++ base type name for base type " +
          t_base_type::t_base_name(tbase);
  }
}

std::string t_cpp_generator::generate_reflection_initializer_name(t_type* type){
  char buf[21];
  sprintf(buf, "%" PRIu64, type->get_type_id());
  return std::string("reflectionInitializer_") + buf;
}

std::string t_cpp_generator::generate_reflection_datatype(t_type* ttype) {

  auto& ns = reflection_ns_prefix_;

  // Chase typedefs
  ttype = get_true_type(ttype);

  if (ttype->is_base_type()) {
    return "";
  }

  struct TypeInfo {
    uint64_t id;
    std::string name;
  };

  TypeInfo tinfo { ttype->get_type_id(), ttype->get_full_name()};

  std::string initializer = generate_reflection_initializer_name(ttype);
  if (!reflection_hashes_.insert(tinfo.id).second) {
    return initializer;  // we already have it
  }

  TypeInfo mapKeyType;
  TypeInfo valueType;
  struct Field {
    int32_t tag;
    bool isRequired;
    std::string name;
    std::map<std::string, std::string> annotations;
    TypeInfo type;
  };
  std::vector<Field> fields;

  std::map<std::string, std::string> deps;

  auto gen_dep = [&] (t_type* ttype) mutable -> TypeInfo {
    auto initializer = generate_reflection_datatype(ttype);
    std::string name = ttype->get_full_name();
    if (!initializer.empty()) {
      deps[initializer] = name;
    }
    return {ttype->get_type_id(), name};
  };

  std::map<std::string, int32_t> enumValues;

  if (ttype->is_struct() || ttype->is_xception()) {
    t_struct* tstruct = static_cast<t_struct*>(ttype);
    for (auto tfield : tstruct->get_members()) {
      bool isRequired = (tfield->get_req() != t_field::T_OPTIONAL);
      fields.push_back({tfield->get_key(),
                        isRequired,
                        tfield->get_name(),
                        tfield->annotations_,
                        gen_dep(tfield->get_type())});
    }
  } else if (ttype->is_map()) {
    t_map* tmap = static_cast<t_map*>(ttype);
    mapKeyType = gen_dep(tmap->get_key_type());
    valueType = gen_dep(tmap->get_val_type());
  } else if (ttype->is_set()) {
    t_set* tset = static_cast<t_set*>(ttype);
    valueType = gen_dep(tset->get_elem_type());
  } else if (ttype->is_list()) {
    t_list* tlist = static_cast<t_list*>(ttype);
    valueType = gen_dep(tlist->get_elem_type());
  } else if (ttype->is_enum()) {
    t_enum* tenum = static_cast<t_enum*>(ttype);
    for (auto tconst : tenum->get_constants()) {
      enumValues[tconst->get_name()] = tconst->get_value();
    }
  } else {
    throw "compiler error: weird type? " + ttype->get_name();
  }

  // Generate initializer.
  //
  // We only remove duplicates from the same file, so we could end up
  // with duplicate initializers for simple types like map<int32_t, int32_t>,
  // but that's okay, as the initializers all do the same thing (the hashes
  // are the same)

  // Generate forward decls for dependent initializers
  for (auto& p : deps) {
    f_reflection_impl_
      << "void  " << p.first << "(" << ns << "Schema&);"
      << "  // " << p.second << endl;
  }

  f_reflection_impl_ <<
    "// Reflection initializer for " << tinfo.name << endl <<
    "void " << initializer << "(" << ns << "Schema& schema) {" << endl;

  f_reflection_impl_ <<
    "  const uint64_t id = " << tinfo.id << "U;" << endl <<
    "  if (schema.dataTypes.count(id)) return;" << endl;


  f_reflection_impl_ <<
    "  " << ns << "DataType& dt = schema.dataTypes[id];" << endl <<
    "  dt.name = \"" << tinfo.name << "\";" << endl <<
    "  schema.names[dt.name] = id;" << endl;

  if (ttype->is_struct() || ttype->is_xception()) {
    f_reflection_impl_ << "  dt.__isset.fields = true;" << endl;
    int order = 0;
    for (auto& p : fields) {
      f_reflection_impl_ <<
        "  {" << endl <<
        "    " << ns << "StructField& f = dt.fields[" << p.tag << "];" <<
        endl <<
        "    f.isRequired = " << (p.isRequired ? "true" : "false") << ";" <<
        endl <<
        "    f.type = " << p.type.id << "U;" << endl <<
        "    f.name = \"" << escape(p.name) << "\";" << endl <<
        "    f.order = " << order << ";" << endl;
      ++order;

      if (!p.annotations.empty()) {
        f_reflection_impl_ <<
          "    f.__isset.annotations = true;" << endl;
        for (auto& ann : p.annotations) {
          f_reflection_impl_ <<
            "    f.annotations[\"" << escape(ann.first) << "\"] = \"" <<
            escape(ann.second) << "\";" << endl;
        }
      }
      f_reflection_impl_ <<
        "  }" << endl;
    }
  }
  if (ttype->is_map()) {
    f_reflection_impl_
      << "  dt.__isset.mapKeyType = true;" << endl
      << "  dt.mapKeyType = " << mapKeyType.id << "U;" << endl;
  }
  if (ttype->is_map() || ttype->is_list() || ttype->is_set()) {
    f_reflection_impl_
      << "  dt.__isset.valueType = true;" << endl
      << "  dt.valueType = " << valueType.id << "U;" << endl;
  }
  if (ttype->is_enum()) {
    f_reflection_impl_
      << "  dt.__isset.enumValues = true;" << endl;
    for (auto& p : enumValues) {
      f_reflection_impl_
        << "  dt.enumValues[\"" << escape(p.first) << "\"] = " << p.second
        << ";" << endl;
    }
  }

  // Call dependent initializers
  for (auto& p : deps) {
    f_reflection_impl_
      << "  " << p.first << "(schema);" << "  // " << p.second << endl;
  }

  f_reflection_impl_ <<
    "}" << endl <<
    endl;

  return initializer;
}

void t_cpp_generator::generate_struct_reflection(ostream& out,
                                                 t_struct* tstruct) {
  auto initializer = generate_reflection_datatype(tstruct);
  auto& ns = reflection_ns_prefix_;

  f_reflection_ <<
    "void " << initializer << "(" << ns << "Schema& schema);" << endl
    << endl;

  out <<
    "const uint64_t " << tstruct->get_name() << "::_reflection_id;" <<
    endl <<
    "void " << tstruct->get_name() << "::_reflection_register(" <<
    ns << "Schema& schema) {" << endl <<
    "  " << ns_reflection_prefix_ << initializer << "(schema);" << endl <<
    "}" << endl <<
    endl;
}

/**
 * Declares a field, which may include initialization as necessary.
 *
 * @param ttype The type
 * @return Field declaration, i.e. int x = 0;
 */
string t_cpp_generator::declare_field(t_field* tfield, bool init, bool pointer, bool constant, bool reference, bool unique) {
  // TODO(mcslee): do we ever need to initialize the field?
  string result = "";
  if (constant) {
    result += "const ";
  }
  result += type_name(tfield->get_type());
  if (pointer) {
    result += "*";
  }
  if (reference) {
    result += "&";
  }
  if (unique) {
    result = "std::unique_ptr<" + result + ">";
  }
  result += " " + tfield->get_name();
  if (init) {
    t_type* type = get_true_type(tfield->get_type());

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
      case t_base_type::TYPE_STRING:
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
      default:
        throw "compiler error: no C++ initializer for base type " + t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      result += " = (" + type_name(type) + ")0";
    }
  }
  if (!reference) {
    result += ";";
  }
  return result;
}

string t_cpp_generator::declare_frozen_field(const t_field* tfield) {
  string result = "";
  if (is_boolean_type(tfield->get_type())) {
    result += "bool " + tfield->get_name() + " : 1";
  } else {
    result += "typename Freezer<" +
        type_name(tfield->get_type(), ALWAYS_NAMESPACE) +
      ">::FrozenType";
    result += " " + tfield->get_name();
  }

  result += ";";
  return result;
}

/**
 * Renders a function signature of the form 'type name(args)'
 *
 * @param tfunction Function definition
 * @return String of rendered function definition
 */
string t_cpp_generator::function_signature(t_function* tfunction,
                                           string style,
                                           string prefix,
                                           bool name_params) {
  t_type* ttype = tfunction->get_returntype();
  t_struct* arglist = tfunction->get_arglist();
  bool has_xceptions = !tfunction->get_xceptions()->get_members().empty();

  if (style == "") {
    if (is_complex_type(ttype)) {
      return
        "void " + prefix + tfunction->get_name() +
        "(" + type_name(ttype) + (name_params ? "& _return" : "& /* _return */") +
        argument_list(arglist, name_params, true) + ")";
    } else {
      return
        type_name(ttype) + " " + prefix + tfunction->get_name() +
        "(" + argument_list(arglist, name_params) + ")";
    }
  } else if (style.substr(0,3) == "Cob") {
    string cob_type;
    string exn_cob;
    if (style == "CobCl") {
      cob_type = "(" + service_name_ + "CobClient";
      if (gen_templates_) {
        cob_type += "T<Protocol_>";
      }
      cob_type += "* client)";
    } else if (style =="CobSv") {
      cob_type = (ttype->is_void()
                  ? "()"
                  : ("(" + type_name(ttype) + " const& _return)"));
      if (has_xceptions) {
        exn_cob = ", std::function<void(const std::exception& ex)> exn_cob";
      }
    } else {
      throw "UNKNOWN STYLE";
    }

    return
      "void " + prefix + tfunction->get_name() +
      "(std::function<void" + cob_type + "> cob" + exn_cob +
      argument_list(arglist, name_params, true) + ")";
  } else {
    throw "UNKNOWN STYLE";
  }
}

/**
 * Renders a field list
 *
 * @param tstruct The struct definition
 * @return Comma sepearated list of all field names in that struct
 */
string t_cpp_generator::argument_list(t_struct* tstruct, bool name_params, bool start_comma) {
  string result = "";

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = !start_comma;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }
    result += type_name((*f_iter)->get_type(), IN_ARG) + " " +
      (name_params ? (*f_iter)->get_name() : "/* " + (*f_iter)->get_name() + " */");
  }
  return result;
}

/**
 * Converts the parse type to a C++ enum string for the given type.
 *
 * @param type Thrift Type
 * @return String of C++ code to definition of that type constant
 */
string t_cpp_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
      return "apache::thrift::protocol::T_STRING";
    case t_base_type::TYPE_BOOL:
      return "apache::thrift::protocol::T_BOOL";
    case t_base_type::TYPE_BYTE:
      return "apache::thrift::protocol::T_BYTE";
    case t_base_type::TYPE_I16:
      return "apache::thrift::protocol::T_I16";
    case t_base_type::TYPE_I32:
      return "apache::thrift::protocol::T_I32";
    case t_base_type::TYPE_I64:
      return "apache::thrift::protocol::T_I64";
    case t_base_type::TYPE_DOUBLE:
      return "apache::thrift::protocol::T_DOUBLE";
    case t_base_type::TYPE_FLOAT:
      return "apache::thrift::protocol::T_FLOAT";
    }
  } else if (type->is_enum()) {
    return "apache::thrift::protocol::T_I32";
  } else if (type->is_struct()) {
    return "apache::thrift::protocol::T_STRUCT";
  } else if (type->is_xception()) {
    return "apache::thrift::protocol::T_STRUCT";
  } else if (type->is_map()) {
    return "apache::thrift::protocol::T_MAP";
  } else if (type->is_set()) {
    return "apache::thrift::protocol::T_SET";
  } else if (type->is_list()) {
    return "apache::thrift::protocol::T_LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

string t_cpp_generator::get_include_prefix(const t_program& program) const {
  string include_prefix = program.get_include_prefix();
  if (!use_include_prefix_ ||
      (include_prefix.size() > 0 && include_prefix[0] == '/')) {
    // if flag is turned off or this is absolute path, return empty prefix
    return "";
  }

  string::size_type last_slash = string::npos;
  if ((last_slash = include_prefix.rfind("/")) != string::npos) {
    return include_prefix.substr(0, last_slash) + "/" + out_dir_base_ + "/";
  }

  return "";
}

string t_cpp_generator::get_include_guard(const char* suffix) const {

  string ns = namespace_prefix(get_program()->get_namespace("cpp"), "_");

  stringstream ss;
  ss << "#ifndef " << ns << program_name_ << suffix
     << endl
     << "#define " << ns << program_name_ << suffix
     << endl
     << endl;

  return ss.str();
}

THRIFT_REGISTER_GENERATOR(cpp, "C++",
//   bootstrap:       Internal use.
"    cob_style:       Generate \"Continuation OBject\"-style classes as well.\n"
"    enum_strict:     Generate C++11 class enums instead of C-style enums.\n"
"    frozen[=packed]: Enable frozen (mmap-able) structs.\n"
"    frozen2:         Enable frozen2 (versioned, mmap-able) structs.\n"
"    include_prefix:  Use full include paths in generated files.\n"
"    json:            Generate functions to parse JsonEntity to thrift struct.\n"
"    no_client_completion: Omit calls to completion__() in CobClient classes.\n"
"    templates[=only]: Generate templatized read()/write() functions (faster).\n"
"    terse_writes:    Suppress writes for fields holding default values.\n"
);
