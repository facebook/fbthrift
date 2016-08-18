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

class t_mstch_generator : public t_generator {
 public:
  t_mstch_generator(
      t_program* program,
      boost::filesystem::path template_prefix);

 protected:
  /**
   *  Directory containing template files for generating code
   */
  boost::filesystem::path template_dir_;

  /**
   * Fetches a particular template from the template map, throwing an error
   * if the template doesn't exist
   */
  const std::string& get_template(const std::string& template_name) const {
    auto itr = this->template_map_.find(template_name);
    if (itr == this->template_map_.end()) {
      std::ostringstream err;
      err << "Could not find template \"" << template_name << "\"";
      throw std::runtime_error{err.str()};
    }
    return itr->second;
  }

  /**
   * Returns the map of (file_name, template_contents) for each template
   * file for this generator
   */
  const std::map<std::string, std::string>& get_template_map() const {
    return this->template_map_;
  }

  /**
   * Write an output file with the given contents to a path
   * under the output directory.
   */
  void write_output(boost::filesystem::path path, const std::string& data) {
    path = boost::filesystem::path{this->get_out_dir()} / path;
    boost::filesystem::create_directories(path.parent_path());
    std::ofstream ofs{path.string()};
    ofs << data;
    this->record_genfile(path.string());
  }

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

  template <typename T>
  mstch::array dump_vector(const std::vector<T>& elems) const {
    mstch::array result{};
    for (typename std::vector<T>::size_type i = 0; i < elems.size(); i++) {
      auto map = this->dump(
          *as_const_pointer<typename std::remove_pointer<T>::type>(elems[i]));
      map.emplace("first?", i == 0);
      map.emplace("last?", i == elems.size() - 1);
      result.push_back(map);
    }
    return result;
  }

 private:
  std::map<std::string, std::string> template_map_;

  void gen_template_map(const boost::filesystem::path& template_prefix);

  /**
   * For every key in the map, prepends a prefix to that key for mstch.
   */
  static mstch::map prepend_prefix(const std::string& prefix, mstch::map map) {
    mstch::map res{};
    for (auto& pair : map) {
      res.emplace(prefix + ":" + pair.first, std::move(pair.second));
    }
    return res;
  }

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
