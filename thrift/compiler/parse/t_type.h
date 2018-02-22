/*
 * Copyright 2017-present Facebook, Inc.
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

#pragma once

#include <cstring>
#include <stdint.h>
#include <map>
#include <string>
#include <sstream>

#include <thrift/compiler/parse/t_annotated.h>

class t_program;

/*
 * All the thrift supported types
 *
 * @kTypeBits - TODO: add description
 * @kTypeMask - TODO: add description
 */
struct t_types {
  enum struct TypeValue {
    TYPE_VOID,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_BYTE,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_DOUBLE,
    TYPE_ENUM,
    TYPE_LIST,
    TYPE_SET,
    TYPE_MAP,
    TYPE_STRUCT,
    TYPE_SERVICE,
    TYPE_PROGRAM,
    TYPE_FLOAT,
    TYPE_STREAM,
  };

  static constexpr size_t kTypeBits = 5;
  static constexpr uint64_t kTypeMask = (1ULL << kTypeBits) - 1;
};

/**
 * class t_type
 *
 * Generic representation of a thrift type. These objects are used by the
 * parser module to build up a tree of object that are all explicitly typed.
 * The generic t_type class exports a variety of useful methods that are
 * used by the code generator to branch based upon different handling for the
 * various types.
 *
 */
class t_type : public t_annotated {
 public:

  /**
   * Simplify access to thrift's TypeValues
   */
  using TypeValue = t_types::TypeValue;

  virtual ~t_type() {}

  /**
   * t_type abstract methods
   */
  virtual std::string get_full_name() const = 0;
  virtual std::string get_impl_full_name() const = 0;
  virtual TypeValue get_type_value() const = 0;

  /**
   * Default returns for every thrift type
   */
  virtual bool is_void()           const { return false; }
  virtual bool is_base_type()      const { return false; }
  virtual bool is_string()         const { return false; }
  virtual bool is_bool()           const { return false; }
  virtual bool is_byte()           const { return false; }
  virtual bool is_i16()            const { return false; }
  virtual bool is_i32()            const { return false; }
  virtual bool is_i64()            const { return false; }
  virtual bool is_float()          const { return false; }
  virtual bool is_double()         const { return false; }
  virtual bool is_floating_point() const { return false; }
  virtual bool is_typedef()        const { return false; }
  virtual bool is_enum()           const { return false; }
  virtual bool is_struct()         const { return false; }
  virtual bool is_xception()       const { return false; }
  virtual bool is_container()      const { return false; }
  virtual bool is_list()           const { return false; }
  virtual bool is_set()            const { return false; }
  virtual bool is_map()            const { return false; }
  virtual bool is_stream()         const { return false; }
  virtual bool is_pubsub_stream()  const { return false; }
  virtual bool is_service()        const { return false; }
  virtual bool is_binary()         const { return false; }

  /**
   * Create a unique hash number based on t_type's properties.
   */
  virtual uint64_t get_type_id() const;

  /**
   * t_type setters
   */
  virtual void set_name(const std::string& name) { name_ = name; }

  /**
   * t_type getters
   */
  const t_program* get_program() const { return program_; }
  const std::string& get_name() const { return name_; }

 protected:

  /**
   * Default constructor for t_type
   *
   * A t_type object can't be initialized by itself. The constructors
   * are protected and only t_type's children can initialize it.
   */
  t_type() {}

  /**
   * Constructor for t_type
   *
   * @param program - An entire thrift program
   */
  explicit t_type(t_program* program) : program_(program) {}

  /**
   * Constructor for t_type
   *
   * @param name - The symbolic name of the thrift type
   */
  explicit t_type(std::string name) : name_(std::move(name)) {}

  /**
   * Constructor for t_type
   *
   * @param program - An entire thrift program
   * @param name    - The symbolic name of the thrift type
   */
  t_type(t_program* program, std::string name) :
    program_(program),
    name_(std::move(name)) {}

  /**
   * Returns a string in the format "prefix program_name.type_name"
   *
   * @param prefix - A string to add before the program name / type name
   */
  std::string make_full_name(const char* prefix) const;

  t_program* program_{nullptr};
  std::string name_;
};
