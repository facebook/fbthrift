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

#ifndef T_MSTCH_GENERATOR_H
#define T_MSTCH_GENERATOR_H

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <mstch/mstch.hpp>

#include <boost/filesystem.hpp>

#include <thrift/compiler/generate/t_generator.h>

#include <folly/Optional.h>

class t_mstch_generator : public t_generator {
 public:
  t_mstch_generator(
      t_program* program,
      boost::filesystem::path template_prefix,
      std::map<std::string, std::string> parsed_options);

 protected:
  /**
   *  Directory containing template files for generating code
   */
  boost::filesystem::path template_dir_;

  /**
   * Option pairs specified on command line for influencing generation behavior
   */
  const std::map<std::string, std::string> parsed_options_;

  /**
   * Fetches a particular template from the template map, throwing an error
   * if the template doesn't exist
   */
  const std::string& get_template(const std::string& template_name) const;

  /**
   * Returns the map of (file_name, template_contents) for each template
   * file for this generator
   */
  const std::map<std::string, std::string>& get_template_map() const {
    return this->template_map_;
  }

  /**
   * Render the mstch template with name `tpl_name` in the given context.
   */
  std::string render(const std::string& tpl_name, const mstch::node& context)
      const;

  /**
   * Write an output file with the given contents to a path
   * under the output directory.
   */
  void write_output(boost::filesystem::path path, const std::string& data);

  /**
   * Dump map of command line flags passed to generator
   */
  mstch::map dump_options() const;

  /**
   * Subclasses should call the dump functions to convert elements
   * of the Thrift AST into maps that can be passed into mstch.
   */
  mstch::map dump(const t_program&) const;
  mstch::map dump(const t_struct&, bool shallow = false) const;
  mstch::map dump(const t_field&) const;
  mstch::map dump(const t_type&) const;
  mstch::map dump(const t_enum&) const;
  mstch::map dump(const t_enum_value&) const;
  mstch::map dump(const t_service&) const;
  mstch::map dump(const t_function&) const;
  mstch::map dump(const t_typedef&) const;
  mstch::map dump(const t_const&) const;
  mstch::map dump(const t_const_value&) const;
  mstch::map dump(
      const std::map<t_const_value*, t_const_value*>::value_type&) const;

  using annotation = std::pair<std::string, std::string>;
  mstch::map dump(const annotation&) const;
  mstch::map dump(const string&) const;

  /**
   * Subclasses should override these functions to extend the behavior of
   * the dump functions. These will be passed the map after the default
   * dump has run, and can modify the maps in whichever ways necessary.
   */
  virtual mstch::map extend_program(const t_program&) const;
  virtual mstch::map extend_struct(const t_struct&) const;
  virtual mstch::map extend_field(const t_field&) const;
  virtual mstch::map extend_type(const t_type&) const;
  virtual mstch::map extend_enum(const t_enum&) const;
  virtual mstch::map extend_enum_value(const t_enum_value&) const;
  virtual mstch::map extend_service(const t_service&) const;
  virtual mstch::map extend_function(const t_function&) const;
  virtual mstch::map extend_typedef(const t_typedef&) const;
  virtual mstch::map extend_const(const t_const&) const;
  virtual mstch::map extend_const_value(const t_const_value&) const;
  virtual mstch::map extend_const_value_map_elem(
      const std::map<t_const_value*, t_const_value*>::value_type&) const;
  virtual mstch::map extend_annotation(const annotation&) const;

  template <typename container>
  mstch::array dump_elems(const container& elems) const {
    using T = typename container::value_type;
    mstch::array result{};
    for (auto itr = elems.begin(); itr != elems.end(); ++itr) {
      auto map = this->dump(
          *as_const_pointer<typename std::remove_pointer<T>::type>(*itr));
      map.emplace("first?", itr == elems.begin());
      map.emplace("last?", std::next(itr) == elems.end());
      result.push_back(map);
    }
    return result;
  }

  folly::Optional<std::string> get_option(const std::string& key) const;

 private:
  std::map<std::string, std::string> template_map_;

  void gen_template_map(const boost::filesystem::path& template_prefix);

  /**
   * For every key in the map, prepends a prefix to that key for mstch.
   */
  static mstch::map prepend_prefix(const std::string& prefix, mstch::map map);

  template <typename T>
  static const T* as_const_pointer(const T* x) {
    return x;
  }

  template <typename T>
  static const T* as_const_pointer(const T& x) {
    return std::addressof(x);
  }
};

#endif // T_MSTCH_GENERATOR_H
