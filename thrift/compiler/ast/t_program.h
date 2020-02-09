/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_doc.h>
#include <thrift/compiler/ast/t_enum.h>
#include <thrift/compiler/ast/t_include.h>
#include <thrift/compiler/ast/t_list.h>
#include <thrift/compiler/ast/t_map.h>
#include <thrift/compiler/ast/t_scope.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/ast/t_set.h>
#include <thrift/compiler/ast/t_sink.h>
#include <thrift/compiler/ast/t_stream.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_typedef.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * class t_program
 *
 * Top level class representing an entire thrift program. A program consists
 * fundamentally of the following:
 *
 *   Typedefs
 *   Enumerations
 *   Constants
 *   Structs
 *   Exceptions
 *   Services
 *
 * The program module also contains the definitions of the base types.
 */
class t_program : public t_doc {
 public:
  /**
   * Constructor for t_program
   *
   * @param path - A *.thrift file path.
   */
  explicit t_program(std::string path) : path_(std::move(path)) {}

  /**
   * Set program elements
   */
  void add_typedef(std::unique_ptr<t_typedef> td) {
    typedefs_raw_.push_back(td.get());
    typedefs_.push_back(std::move(td));
  }
  void add_enum(std::unique_ptr<t_enum> te) {
    enums_raw_.push_back(te.get());
    enums_.push_back(std::move(te));
  }
  void add_const(std::unique_ptr<t_const> tc) {
    consts_raw_.push_back(tc.get());
    consts_.push_back(std::move(tc));
  }
  void add_struct(std::unique_ptr<t_struct> ts) {
    objects_.push_back(ts.get());
    structs_raw_.push_back(ts.get());
    structs_.push_back(std::move(ts));
  }
  void add_xception(std::unique_ptr<t_struct> tx) {
    objects_.push_back(tx.get());
    xceptions_raw_.push_back(tx.get());
    xceptions_.push_back(std::move(tx));
  }
  void add_service(std::unique_ptr<t_service> ts) {
    services_raw_.push_back(ts.get());
    services_.push_back(std::move(ts));
  }

  void add_placeholder_typedef(std::unique_ptr<t_typedef> ptd) {
    assert(!ptd->is_defined());
    placeholder_typedefs_raw_.push_back(ptd.get());
    placeholder_typedefs_.push_back(std::move(ptd));
  }

  void add_unnamed_typedef(std::unique_ptr<t_typedef> td) {
    unnamed_typedefs_.push_back(std::move(td));
  }

  void add_unnamed_type(std::unique_ptr<t_type> ut) {
    unnamed_types_.push_back(std::move(ut));
  }

  /**
   * Get program elements
   */
  const std::vector<t_typedef*>& get_typedefs() const {
    return typedefs_raw_;
  }
  const std::vector<t_enum*>& get_enums() const {
    return enums_raw_;
  }
  const std::vector<t_const*>& get_consts() const {
    return consts_raw_;
  }
  const std::vector<t_struct*>& get_structs() const {
    return structs_raw_;
  }
  const std::vector<t_struct*>& get_xceptions() const {
    return xceptions_raw_;
  }
  const std::vector<t_struct*>& get_objects() const {
    return objects_;
  }
  const std::vector<t_service*>& get_services() const {
    return services_raw_;
  }
  const std::vector<t_typedef*>& get_placeholder_typedefs() const {
    return placeholder_typedefs_raw_;
  }

  /**
   * t_program setters
   */
  void add_cpp_include(std::string path) {
    cpp_includes_.push_back(std::move(path));
  }

  // Only used in py_frontend.tcc
  void set_namespace(std::string name) {
    namespace_ = std::move(name);
  }

  /**
   * Language neutral namespace/packaging
   *
   * @param language - The target language (i.e. py, cpp) to generate code
   * @param name_space - //TODO add definition of name_space
   */
  void set_namespace(std::string language, std::string name_space) {
    namespaces_.emplace(language, name_space);
  }

  /**
   * t_program getters
   */
  const std::string& get_path() const {
    return path_;
  }

  const std::string& get_name() const {
    return name_;
  }

  const std::string& get_namespace() const {
    return namespace_;
  }

  const std::string& get_include_prefix() const {
    return include_prefix_;
  }

  /**
   * Returns a list of includes that the program contains. Each include is of
   * type t_include*, and contains information about the program included, as
   * well as the location of the include statement.
   */
  const std::vector<t_include*>& get_includes() const {
    return includes_raw_;
  }

  /**
   * Returns a list of programs that are included by this program.
   */
  std::vector<t_program*> get_included_programs() const {
    std::vector<t_program*> included_programs;
    for (auto const& include : includes_) {
      included_programs.push_back(include->get_program());
    }
    return included_programs;
  }

  t_scope* scope() const {
    return scope_.get();
  }

  // Only used in py_frontend.tcc
  const std::map<std::string, std::string>& get_namespaces() const {
    return namespaces_;
  }

  // Only used in t_cpp_generator
  const std::vector<std::string>& get_cpp_includes() const {
    return cpp_includes_;
  }

  /**
   * Outputs a reference to the namespace corresponding to the
   * key(language) in the namespaces_ map.
   *
   * @param language - The target language (i.e. py, cpp) to generate code
   */
  const std::string& get_namespace(const std::string& language) const;

  /**
   * This creates a new program for every thrift file in an
   * include statement and sets their include_prefix by parsing
   * the directory in which they were included from
   *
   * @param path         - A full thrift file path
   * @param include_site - A full or relative thrift file path
   * @param lineno       - The line number of the include statement
   */
  std::unique_ptr<t_program>
  add_include(std::string path, std::string include_site, int lineno);

  void add_include(std::unique_ptr<t_include> include) {
    includes_raw_.push_back(include.get());
    includes_.push_back(std::move(include));
  }

  /**
   * This sets the directory path of the current thrift program,
   * adding checks to format it into a correct directory path
   *
   * @param include_prefix - The directory path of a thrift include statement
   */
  void set_include_prefix(std::string include_prefix);

  /**
   *  Obtains the name of a thrift file from the full file path
   *
   * @param path - A *.thrift file path
   */
  std::string compute_name_from_file_path(std::string path);

 private:
  /**
   * Components to generate code for
   */
  std::vector<std::unique_ptr<t_typedef>> typedefs_;
  std::vector<std::unique_ptr<t_enum>> enums_;
  std::vector<std::unique_ptr<t_const>> consts_;
  std::vector<t_struct*> objects_; // objects_ is non-owning since it's simply
                                   // structs_ + xceptions_
  std::vector<std::unique_ptr<t_struct>> structs_;
  std::vector<std::unique_ptr<t_struct>> xceptions_;
  std::vector<std::unique_ptr<t_service>> services_;
  std::vector<std::unique_ptr<t_include>> includes_;

  /**
   * A place to store unnamed types so that they can be kept alive for the
   * duration of the program's lifetime, and subsequently destroyed.
   */
  std::vector<std::unique_ptr<t_typedef>> placeholder_typedefs_;
  std::vector<std::unique_ptr<t_typedef>> unnamed_typedefs_;
  std::vector<std::unique_ptr<t_type>> unnamed_types_;

  std::vector<t_typedef*> typedefs_raw_;
  std::vector<t_enum*> enums_raw_;
  std::vector<t_const*> consts_raw_;
  std::vector<t_struct*> structs_raw_;
  std::vector<t_struct*> xceptions_raw_;
  std::vector<t_service*> services_raw_;
  std::vector<t_include*> includes_raw_;

  std::vector<t_typedef*> placeholder_typedefs_raw_;

  std::string path_; // initialized in ctor init-list
  std::string name_{compute_name_from_file_path(path_)};
  std::string namespace_;
  std::string include_prefix_;
  std::map<std::string, std::string> namespaces_;
  std::vector<std::string> cpp_includes_;
  std::unique_ptr<t_scope> scope_{new t_scope{}};
};

} // namespace compiler
} // namespace thrift
} // namespace apache
