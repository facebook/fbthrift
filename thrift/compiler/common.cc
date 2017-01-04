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

#include <thrift/compiler/common.h>

#ifdef _WIN32
# include <windows.h> /* for GetFullPathName */
#endif

#include <boost/filesystem.hpp>

#include <thrift/compiler/platform.h>

/**
 * Global program tree
 */
t_program* g_program;

/**
 * Global types
 */
t_base_type* g_type_void;
t_base_type* g_type_string;
t_base_type* g_type_binary;
t_base_type* g_type_slist;
t_base_type* g_type_bool;
t_base_type* g_type_byte;
t_base_type* g_type_i16;
t_base_type* g_type_i32;
t_base_type* g_type_i64;
t_base_type* g_type_double;
t_base_type* g_type_float;

/**
 * Global scope
 */
t_scope* g_scope;

/**
 * Parent scope to also parse types
 */
t_scope* g_parent_scope;

/**
 * Prefix for putting types in parent scope
 */
string g_parent_prefix;

/**
 * Parsing pass
 */
PARSE_MODE g_parse_mode;

/**
 * Current directory of file being parsed
 */
string g_curdir;

/**
 * Current file being parsed
 */
string g_curpath;

/**
 * Directory containing template files
 */
string g_template_dir;

/**
 * Search path for inclusions
 */
vector<string> g_incl_searchpath;

/**
 * Should C++ include statements use path prefixes for other thrift-generated
 * header files
 */
bool g_cpp_use_include_prefix = false;

/**
 * Global debug state
 */
int g_debug = 0;

/**
 * Strictness level
 */
int g_strict = 127;

/**
 * Warning level
 */
int g_warn = 1;

/**
 * Verbose output
 */
int g_verbose = 0;

/**
 * Global time string
 */
char* g_time_str;

/**
 * The last parsed doctext comment.
 */
char* g_doctext;

/**
 * The location of the last parsed doctext comment.
 */
int g_doctext_lineno;

/**
 * Whether or not negative field keys are accepted.
 */
int g_allow_neg_field_keys;

/**
 * Whether or not negative enum values.
 */
int g_allow_neg_enum_vals;

/**
 * Whether or not 64-bit constants will generate a warning.
 */
int g_allow_64bit_consts = 0;

std::string compute_absolute_path(const std::string& path) {
  boost::filesystem::path abspath{path};
  try {
    abspath = boost::filesystem::canonical(abspath);
    return abspath.string();
  } catch (const boost::filesystem::filesystem_error& e) {
    failure("Could not find file: %s. Error: %s", path.c_str(), e.what());
  }
}

void yyerror(const char* fmt, ...) {
  va_list args;
  fprintf(stderr,
          "[ERROR:%s:%d] (last token was '%s')\n",
          g_curpath.c_str(),
          yylineno,
          yytext);

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  fprintf(stderr, "\n");
}

void pdebug(const char* fmt, ...) {
  if (g_debug == 0) {
    return;
  }
  va_list args;
  printf("[PARSE:%d] ", yylineno);
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void pverbose(const char* fmt, ...) {
  if (g_verbose == 0) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

void pwarning(int level, const char* fmt, ...) {
  if (g_warn < level) {
    return;
  }
  va_list args;
  fprintf(stderr, "[WARNING:%s:%d] ", g_curpath.c_str(), yylineno);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}

[[noreturn]]
void failure(const char* fmt, ...) {
  va_list args;
  fprintf(stderr, "[FAILURE:%s:%d] ", g_curpath.c_str(), yylineno);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}

string directory_name(string filename) {
  string::size_type slash = filename.rfind("/");
  // No slash, just use the current directory
  if (slash == string::npos) {
    return ".";
  }
  return filename.substr(0, slash);
}

string include_file(string filename) {
  // Absolute path? Just try that
  if (filename[0] == '/') {
    return compute_absolute_path(filename);
  } else { // relative path, start searching
    // new search path with current dir global
    vector<string> sp = g_incl_searchpath;
    sp.insert(sp.begin(), g_curdir);

    // iterate through paths
    vector<string>::iterator it;
    for (it = sp.begin(); it != sp.end(); it++) {
      string sfilename = *(it) + "/" + filename;
      boost::filesystem::path abspath{sfilename};
      try {
        abspath = boost::filesystem::canonical(abspath);
        return abspath.string();
      } catch (const boost::filesystem::filesystem_error& e) {
        pdebug("Could not find: %s. Error: %s", filename.c_str(), e.what());
      }
    }

    // File was not found
    failure("Could not find include filex %s", filename.c_str());
  }

}

void clear_doctext() {
  if (g_doctext != nullptr) {
    pwarning(2, "Uncaptured doctext at on line %d.", g_doctext_lineno);
  }
  free(g_doctext);
  g_doctext = nullptr;
}

char* clean_up_doctext(char* doctext) {
  // Convert to C++ string, and remove Windows's carriage returns.
  string docstring = doctext;
  docstring.erase(
      remove(docstring.begin(), docstring.end(), '\r'),
      docstring.end());

  // Separate into lines.
  vector<string> lines;
  string::size_type pos = string::npos;
  string::size_type last;
  while (true) {
    last = (pos == string::npos) ? 0 : pos+1;
    pos = docstring.find('\n', last);
    if (pos == string::npos) {
      // First bit of cleaning.  If the last line is only whitespace, drop it.
      string::size_type nonwhite = docstring.find_first_not_of(" \t", last);
      if (nonwhite != string::npos) {
        lines.push_back(docstring.substr(last));
      }
      break;
    }
    lines.push_back(docstring.substr(last, pos-last));
  }

  // A very profound docstring.
  if (lines.empty()) {
    return nullptr;
  }

  // Clear leading whitespace from the first line.
  pos = lines.front().find_first_not_of(" \t");
  lines.front().erase(0, pos);

  // If every nonblank line after the first has the same number of spaces/tabs,
  // then a star, remove them.
  bool have_prefix = true;
  bool found_prefix = false;
  string::size_type prefix_len = 0;
  vector<string>::iterator l_iter;
  for (l_iter = lines.begin()+1; l_iter != lines.end(); ++l_iter) {
    if (l_iter->empty()) {
      continue;
    }

    pos = l_iter->find_first_not_of(" \t");
    if (!found_prefix) {
      if (pos != string::npos) {
        if (l_iter->at(pos) == '*') {
          found_prefix = true;
          prefix_len = pos;
        } else {
          have_prefix = false;
          break;
        }
      } else {
        // Whitespace-only line.  Truncate it.
        l_iter->clear();
      }
    } else if (l_iter->size() > pos
        && l_iter->at(pos) == '*'
        && pos == prefix_len) {
      // Business as usual.
    } else if (pos == string::npos) {
      // Whitespace-only line.  Let's truncate it for them.
      l_iter->clear();
    } else {
      // The pattern has been broken.
      have_prefix = false;
      break;
    }
  }

  // If our prefix survived, delete it from every line.
  if (have_prefix) {
    // Get the star too.
    prefix_len++;
    for (l_iter = lines.begin()+1; l_iter != lines.end(); ++l_iter) {
      l_iter->erase(0, prefix_len);
    }
  }

  // Now delete the minimum amount of leading whitespace from each line.
  prefix_len = string::npos;
  for (l_iter = lines.begin()+1; l_iter != lines.end(); ++l_iter) {
    if (l_iter->empty()) {
      continue;
    }
    pos = l_iter->find_first_not_of(" \t");
    if (pos != string::npos
        && (prefix_len == string::npos || pos < prefix_len)) {
      prefix_len = pos;
    }
  }

  // If our prefix survived, delete it from every line.
  if (prefix_len != string::npos) {
    for (l_iter = lines.begin()+1; l_iter != lines.end(); ++l_iter) {
      l_iter->erase(0, prefix_len);
    }
  }

  // Remove trailing whitespace from every line.
  for (l_iter = lines.begin(); l_iter != lines.end(); ++l_iter) {
    pos = l_iter->find_last_not_of(" \t");
    if (pos != string::npos && pos != l_iter->length()-1) {
      l_iter->erase(pos+1);
    }
  }

  // If the first line is empty, remove it.
  // Don't do this earlier because a lot of steps skip the first line.
  if (lines.front().empty()) {
    lines.erase(lines.begin());
  }

  // Now rejoin the lines and copy them back into doctext.
  docstring.clear();
  for (l_iter = lines.begin(); l_iter != lines.end(); ++l_iter) {
    docstring += *l_iter;
    docstring += '\n';
  }

  assert(docstring.length() <= strlen(doctext));
  strcpy(doctext, docstring.c_str());
  return doctext;
}

void dump_docstrings(t_program* program) {
  string progdoc = program->get_doc();
  if (!progdoc.empty()) {
    printf("Whole program doc:\n%s\n", progdoc.c_str());
  }
  const vector<t_typedef*>& typedefs = program->get_typedefs();
  vector<t_typedef*>::const_iterator t_iter;
  for (t_iter = typedefs.begin(); t_iter != typedefs.end(); ++t_iter) {
    t_typedef* td = *t_iter;
    if (td->has_doc()) {
      printf("typedef %s:\n%s\n", td->get_name().c_str(),
             td->get_doc().c_str());
    }
  }
  const vector<t_enum*>& enums = program->get_enums();
  vector<t_enum*>::const_iterator e_iter;
  for (e_iter = enums.begin(); e_iter != enums.end(); ++e_iter) {
    t_enum* en = *e_iter;
    if (en->has_doc()) {
      printf("enum %s:\n%s\n", en->get_name().c_str(), en->get_doc().c_str());
    }
  }
  const vector<t_const*>& consts = program->get_consts();
  vector<t_const*>::const_iterator c_iter;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    t_const* co = *c_iter;
    if (co->has_doc()) {
      printf("const %s:\n%s\n", co->get_name().c_str(), co->get_doc().c_str());
    }
  }
  const vector<t_struct*>& structs = program->get_structs();
  vector<t_struct*>::const_iterator s_iter;
  for (s_iter = structs.begin(); s_iter != structs.end(); ++s_iter) {
    t_struct* st = *s_iter;
    if (st->has_doc()) {
      printf("struct %s:\n%s\n", st->get_name().c_str(), st->get_doc().c_str());
    }
  }
  const vector<t_struct*>& xceptions = program->get_xceptions();
  vector<t_struct*>::const_iterator x_iter;
  for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
    t_struct* xn = *x_iter;
    if (xn->has_doc()) {
      printf("xception %s:\n%s\n", xn->get_name().c_str(),
             xn->get_doc().c_str());
    }
  }
  const vector<t_service*>& services = program->get_services();
  vector<t_service*>::const_iterator v_iter;
  for (v_iter = services.begin(); v_iter != services.end(); ++v_iter) {
    t_service* sv = *v_iter;
    if (sv->has_doc()) {
      printf("service %s:\n%s\n", sv->get_name().c_str(),
             sv->get_doc().c_str());
    }
  }
}

void validate_const_rec(std::string name, t_type* type, t_const_value* value) {
  if (type->is_void()) {
    throw string("type error: cannot declare a void const: " + name);
  }

  auto as_struct = dynamic_cast<t_struct *>(type);
  assert((as_struct != nullptr) == type->is_struct());
  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      if (value->get_type() != t_const_value::CV_STRING) {
        throw string("type error: const \"" + name +
                     "\" was declared as string");
      }
      break;
    case t_base_type::TYPE_BOOL:
      if (value->get_type() != t_const_value::CV_INTEGER) {
        throw string("type error: const \"" + name + "\" was declared as bool");
      }
      break;
    case t_base_type::TYPE_BYTE:
      if (value->get_type() != t_const_value::CV_INTEGER) {
        throw string("type error: const \"" + name + "\" was declared as byte");
      }
      break;
    case t_base_type::TYPE_I16:
      if (value->get_type() != t_const_value::CV_INTEGER) {
        throw string("type error: const \"" + name + "\" was declared as i16");
      }
      break;
    case t_base_type::TYPE_I32:
      if (value->get_type() != t_const_value::CV_INTEGER) {
        throw string("type error: const \"" + name + "\" was declared as i32");
      }
      break;
    case t_base_type::TYPE_I64:
      if (value->get_type() != t_const_value::CV_INTEGER) {
        throw string("type error: const \"" + name + "\" was declared as i64");
      }
      break;
    case t_base_type::TYPE_DOUBLE:
    case t_base_type::TYPE_FLOAT:
      if (value->get_type() != t_const_value::CV_INTEGER &&
          value->get_type() != t_const_value::CV_DOUBLE) {
        throw string("type error: const \"" + name +
                     "\" was declared as double");
      }
      break;
    default:
      throw string("compiler error: no const of base type " +
                   t_base_type::t_base_name(tbase) + name);
    }
  } else if (type->is_enum()) {
    if (value->get_type() != t_const_value::CV_INTEGER) {
      throw string("type error: const \"" + name + "\" was declared as enum");
    }
    const auto as_enum = dynamic_cast<t_enum*>(type);
    assert(as_enum != nullptr);
    const auto enum_val = as_enum->find_value(value->get_integer());
    if (enum_val == nullptr) {
      pwarning(
          0,
          "type error: const \"%s\" was declared as enum \"%s\" with a value"
          " not of that enum",
          name.c_str(),
          type->get_name().c_str());
    }
  } else if (as_struct && as_struct->is_union()) {
    if (value->get_type() != t_const_value::CV_MAP) {
      throw string("type error: const \"" + name + "\" was declared as union");
    }
    auto const &map = value->get_map();
    if (map.size() > 1) {
      throw string("type error: const \"" + name + "\" is a union and can't "
        "have more than one field set");
    }
    if (!map.empty()) {
      if (map.front().first->get_type() != t_const_value::CV_STRING) {
        throw string("type error: const \"" + name + "\" is a union and member "
          "names must be a string");
      }
      auto const &member_name = map.front().first->get_string();
      auto const &member = as_struct->get_member(member_name);
      if (!member) {
        throw string("type error: no member named \"" + member_name + "\" for "
          "union const \"" + name + "\"");
      }
    }
  } else if (type->is_struct() || type->is_xception()) {
    if (value->get_type() != t_const_value::CV_MAP) {
      throw string("type error: const \"" + name + "\" was declared as " +
        "struct/exception");
    }
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;

    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      if (v_iter->first->get_type() != t_const_value::CV_STRING) {
        throw string("type error: " + name + " struct key must be string");
      }
      t_type* field_type = nullptr;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == nullptr) {
        throw string("type error: " + type->get_name() + " has no field " +
          v_iter->first->get_string());
      }

      validate_const_rec(name + "." + v_iter->first->get_string(), field_type,
                         v_iter->second);
    }
  } else if (type->is_map()) {
    t_type* k_type = ((t_map*)type)->get_key_type();
    t_type* v_type = ((t_map*)type)->get_val_type();
    const vector<pair<t_const_value*, t_const_value*>>& val = value->get_map();
    vector<pair<t_const_value*, t_const_value*>>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      validate_const_rec(name + "<key>", k_type, v_iter->first);
      validate_const_rec(name + "<val>", v_type, v_iter->second);
    }
  } else if (type->is_list() || type->is_set()) {
    t_type* e_type;
    if (type->is_list()) {
      e_type = ((t_list*)type)->get_elem_type();
    } else {
      e_type = ((t_set*)type)->get_elem_type();
    }
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      validate_const_rec(name + "<elem>", e_type, *v_iter);
    }
  }
}

void parse(t_program* program,
           t_program* parent_program,
           set<string> already_parsed_paths) {
  // Get scope file path
  string path = program->get_path();

  if (already_parsed_paths.count(path)) {
    failure("Circular dependecy found: file %s is already parsed. ",
            path.c_str());
  } else {
    already_parsed_paths.insert(path);
  }

  // Set current dir global, which is used in the include_file function
  g_curdir = directory_name(path);
  g_curpath = path;

  // Open the file
  yyin = fopen(path.c_str(), "r");
  if (yyin == 0) {
    failure("Could not open input file: \"%s\"", path.c_str());
  }

  // Create new scope and scan for includes
  pverbose("Scanning %s for includes\n", path.c_str());
  g_parse_mode = INCLUDES;
  g_program = program;
  g_scope = program->scope();
  try {
    yylineno = 1;
    if (yyparse() != 0) {
      failure("Parser error during include pass.");
    }
  } catch (const string &x) {
    failure(x.c_str());
  }
  fclose(yyin);

  // Recursively parse all the include programs
  const auto& includes = program->get_includes();
  // Always enable g_allow_neg_field_keys when parsing included files.
  // This way if a thrift file has negative keys, --allow-neg-keys doesn't have
  // to be used by everyone that includes it.
  bool main_allow_neg_keys = g_allow_neg_field_keys;
  bool main_allow_neg_enum_vals = g_allow_neg_enum_vals;
  g_allow_neg_enum_vals = true;
  g_allow_neg_field_keys = true;
  for (auto included_program : includes) {
    parse(included_program, program, already_parsed_paths);
  }
  g_allow_neg_enum_vals = main_allow_neg_enum_vals;
  g_allow_neg_field_keys = main_allow_neg_keys;

  // Parse the program file
  g_parse_mode = PROGRAM;
  g_program = program;
  g_scope = program->scope();
  g_parent_scope = (parent_program != nullptr) ? parent_program->scope() : nullptr;
  g_parent_prefix = program->get_name() + ".";
  g_curpath = path;
  yyin = fopen(path.c_str(), "r");
  if (yyin == 0) {
    failure("Could not open input file: \"%s\"", path.c_str());
  }
  pverbose("Parsing %s for types\n", path.c_str());
  yylineno = 1;
  try {
    if (yyparse() != 0) {
      failure("Parser error during types pass.");
    }
  } catch (const string &x) {
    failure(x.c_str());
  }
  fclose(yyin);
}

void validate_const_type(t_const* c) {
  validate_const_rec(c->get_name(), c->get_type(), c->get_value());
}

void validate_field_value(t_field* field, t_const_value* cv) {
  validate_const_rec(field->get_name(), field->get_type(), cv);
}

/**
 * Get the true type behind a series of typedefs.
 */
const t_type* common_get_true_type(const t_type* type) {
  while (type->is_typedef()) {
    type = (static_cast<const t_typedef*>(type))->get_type();
  }
  return type;
}
t_type* common_get_true_type(t_type* type) {
  return const_cast<t_type*>(
      common_get_true_type(const_cast<const t_type*>(type)));
}

/**
 * Check that all the elements of a throws block are actually exceptions.
 */
bool validate_throws(t_struct* throws) {
  const vector<t_field*>& members = throws->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (!common_get_true_type((*m_iter)->get_type())->is_xception()) {
      return false;
    }
  }
  return true;
}

// TODO: Add a description of the function
void override_annotations(std::map<std::string, std::string>& where,
                          const std::map<std::string, std::string>& from) {
  for (const auto& kvp : from) {
    where[kvp.first] = kvp.second;
  }
}
