/*
 * Copyright 2016 Facebook, Inc.
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

#ifndef T_CONCAT_GENERATOR_H
#define T_CONCAT_GENERATOR_H

#include <thrift/compiler/generate/t_generator.h>
#include <thrift/compiler/globals.h>
#include <thrift/compiler/parse/t_program.h>

/**
 * String concatenation based generators.
 * This is the older way of generating code, new generators should use
 * a template based approach. See t_mstch_generator.h
 */
class t_concat_generator : public t_generator {
 public:
  explicit t_concat_generator(t_program* program) : t_generator(program) {
    tmp_ = 0;
    indent_ = 0;
    escape_['\n'] = "\\n";
    escape_['\r'] = "\\r";
    escape_['\t'] = "\\t";
    escape_['"'] = "\\\"";
    escape_['\\'] = "\\\\";
  }

  virtual ~t_concat_generator(void) {}

  /**
   * Framework generator method that iterates over all the parts of a program
   * and performs general actions. This is implemented by the base class and
   * should not normally be overwritten in the subclasses.
   */
  virtual void generate_program();

  void generate_docstring_comment(
      std::ofstream& out,
      const std::string& comment_start,
      const std::string& line_prefix,
      const std::string& contents,
      const std::string& comment_end);

  /**
   * Escape string to use one in generated sources.
   */
  std::string escape_string(const std::string& in) const;

  std::string get_escaped_string(const t_const_value* constval) {
    return escape_string(constval->get_string());
  }

 protected:
  /**
   * Optional methods that may be imlemented by subclasses to take necessary
   * steps at the beginning or end of code generation.
   */

  virtual void init_generator() {}
  virtual void close_generator() {}

  virtual void generate_consts(std::vector<t_const*> consts);

  /**
   * Pure virtual methods implemented by the generator subclasses.
   */

  virtual void generate_typedef(t_typedef* ttypedef) = 0;
  virtual void generate_enum(t_enum* tenum) = 0;
  virtual void generate_const(t_const* /*tconst*/) {}
  virtual void generate_forward_declaration(t_struct* /*tstruct*/){}
  virtual void generate_struct(t_struct* tstruct) = 0;
  virtual void generate_service(t_service* tservice) = 0;
  virtual void generate_xception(t_struct* txception) {
    // By default exceptions are the same as structs
    generate_struct(txception);
  }

  /**
   * Method to get the service name, may be overridden
   */
  virtual std::string get_service_name(t_service* tservice) {
    return tservice->get_name();
  }

  /*
   * Get the full thrift typename of a (possibly complex) type.
   */
  virtual std::string thrift_type_name(t_type* ttype) {
    if (ttype->is_base_type()) {
      t_base_type* tbase = (t_base_type*)ttype;
      return t_base_type::t_base_name(tbase->get_base());
    }

    if (ttype->is_container()) {
      if (ttype->is_map()) {
        t_map* tmap = (t_map*)ttype;
        return "map<" + thrift_type_name(tmap->get_key_type()) + ", " +
            thrift_type_name(tmap->get_val_type()) + ">";
      } else if (ttype->is_set()) {
        t_set* tset = (t_set*)ttype;
        return "set<" + thrift_type_name(tset->get_elem_type()) + ">";
      } else if (ttype->is_list()) {
        t_list* tlist = (t_list*)ttype;
        return "list<" + thrift_type_name(tlist->get_elem_type()) + ">";
      }
    }

    // A complex type like struct, typedef, exception.
    std::string full_name = ttype->get_name();

    // Qualify the name with the program name, if the type isn't from this
    // program.
    const t_program* program = ttype->get_program();
    if (program != nullptr && program != program_) {
      full_name = program->get_name() + "." + full_name;
    }
    return full_name;
  }

  /**
   * Generates the structural ID.
   *
   * The structural ID reflects the fields properties (key, name, type,
   * required or not). It is supposedly unique for a given thrift structure.
   *
   * To generate this ID, a hash is created consisting of the following
   * properties: key, name, type, field required or not). The structural ID
   * can be used when deserializing thrift instances in PHP to know if the
   * thrift definition has changed while the object was in the cache. In that
   * case, some properties might not be properly initialized (sets may be
   * null instead of being an empty set) and the object should be carefully
   * handled or deleted.
   *
   * The structural ID is *NOT* unique across different structures. Example:
   * two structures having same fields properties will have the same
   * structural ID.
   */
  std::string generate_structural_id(const vector<t_field*>& members);

  /**
   * Creates a unique temporary variable name, which is just "name" with a
   * number appended to it (i.e. name35)
   */
  std::string tmp(std::string name) {
    std::ostringstream out;
    out << name << tmp_++;
    return out.str();
  }

  /**
   * Get current tmp variable counter value
   */
  int get_tmp_counter() const {
    return tmp_;
  }

  /**
   * Set tmp variable counter value
   */
  void set_tmp_counter(int tmp) {
    tmp_ = tmp;
  }

  /**
   * Get current indentation level.
   */
  int get_indent() const {
    return indent_;
  }

  /**
   * Indentation level modifiers
   */

  void indent_up() {
    ++indent_;
  }

  void indent_down() {
    --indent_;
  }

  /**
   * Get number of spaces to use for each indentation level.
   */
  virtual int get_indent_size() const {
    return 2;
  }

  /**
   * Get line length.
   */
  virtual int get_line_length() const {
    return 80;
  }

  /**
   * Indent by these many levels.
   */
  std::string indent(int levels) {
    return std::string(levels * get_indent_size(), ' ');
  }

  /**
   * Indentation print function
   */
  std::string indent() {
    return indent(get_indent());
  }

  /**
   * Indentation utility wrapper
   */
  std::ostream& indent(std::ostream& os) {
    return os << indent();
  }

  /**
   * Capitalization helpers
   */
  std::string capitalize(std::string in) {
    in[0] = toupper(in[0]);
    return in;
  }
  std::string decapitalize(std::string in) {
    in[0] = tolower(in[0]);
    return in;
  }
  static std::string lowercase(std::string in) {
    for (size_t i = 0; i < in.size(); ++i) {
      in[i] = tolower(in[i]);
    }
    return in;
  }
  std::string uppercase(std::string in) {
    for (size_t i = 0; i < in.size(); ++i) {
      in[i] = toupper(in[i]);
    }
    return in;
  }
  /**
   * Transforms a camel case string to an equivalent one separated by
   * underscores
   * e.g. aMultiWord -> a_multi_word
   *      someName   -> some_name
   *      CamelCase  -> camel_case
   *      name       -> name
   *      Name       -> name
   */
  std::string underscore(std::string in) {
    in[0] = tolower(in[0]);
    for (size_t i = 1; i < in.size(); ++i) {
      if (isupper(in[i])) {
        in[i] = tolower(in[i]);
        in.insert(i, "_");
      }
    }
    return in;
  }
  /**
    * Transforms a string with words separated by underscores to a camel case
    * equivalent. By default, the first letter of the input string is not
    * capitlized.
    * e.g. a_multi_word -> aMultiWord
    *      some_name    ->  someName
    *      name         ->  name
    */
  std::string camelcase(std::string in, bool capitalize_first = false) {
    std::ostringstream out;
    bool underscore = false;

    if (capitalize_first)
      in = capitalize(in);

    for (size_t i = 0; i < in.size(); i++) {
      if (in[i] == '_') {
        underscore = true;
        continue;
      }
      if (underscore) {
        out << (char)toupper(in[i]);
        underscore = false;
        continue;
      }
      out << in[i];
    }

    return out.str();
  }

  bool option_is_specified(
      const std::map<std::string, std::string>& parsed_options,
      const std::string& name) {
    return parsed_options.find(name) != parsed_options.end();
  }

  bool get_option_value(
      const std::map<std::string, std::string>& parsed_options,
      const std::string& name,
      std::string& value) {
    std::map<std::string, std::string>::const_iterator it =
        parsed_options.find(name);
    if (it == parsed_options.end()) {
      return false;
    }
    value = it->second;
    return true;
  }

  bool option_is_set(
      const std::map<std::string, std::string>& parsed_options,
      const std::string& name,
      bool default_value) {
    std::string value;
    if (get_option_value(parsed_options, name, value)) {
      return value == "1" || value == "true";
    }
    return default_value;
  }

 public:
 private:
  void validate_union_members(const t_struct* tstruct);

 protected:
  /**
   * Quick accessor for formatted service name that is currently being
   * generated.
   */
  std::string service_name_;

  /**
   * Map of characters to escape in string literals.
   */
  std::map<char, std::string> escape_;

 private:
  /**
   * Current code indentation level
   */
  int indent_;

  /**
   * Temporary variable counter, for making unique variable names
   */
  int tmp_;
};

#endif // T_CONCAT_GENERATOR_H
