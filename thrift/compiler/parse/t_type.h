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

#ifndef T_TYPE_H
#define T_TYPE_H

#include <string>
#include <map>
#include <cstring>
#include <stdint.h>
#include <sstream>
#include <thrift/compiler/parse/t_doc.h>

#include <folly/Bits.h>

class t_program;

struct t_types {
  enum TypeValue {
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

  static const size_t kTypeBits = 5;
  static const uint64_t kTypeMask = (1ULL << kTypeBits) - 1;
};

/**
 * Generic representation of a thrift type. These objects are used by the
 * parser module to build up a tree of object that are all explicitly typed.
 * The generic t_type class exports a variety of useful methods that are
 * used by the code generator to branch based upon different handling for the
 * various types.
 *
 */
class t_type : public t_doc {
 public:
  using TypeValue = t_types::TypeValue;

  virtual ~t_type() {}

  virtual std::string get_full_name() const = 0;
  virtual std::string get_impl_full_name() const = 0;
  virtual TypeValue get_type_value() const = 0;

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
  virtual bool is_service()        const { return false; }
  virtual bool is_binary()         const { return false; }

  virtual uint64_t get_type_id() const;

  // Setters
  virtual void set_name(const std::string& name) { name_ = name; }

  // Getters
  const t_program* get_program() const { return program_; }
  const std::string& get_name() const { return name_; }

  std::map<std::string, std::string> annotations_;

 protected:
  t_type() : program_(nullptr) {}

  explicit t_type(t_program* program) : program_(program) {}

  t_type(t_program* program, std::string name) :
    program_(program),
    name_(std::move(name)) {}

  explicit t_type(std::string name) :
    program_(nullptr),
    name_(std::move(name)) {}

  std::string make_full_name(const char* prefix) const;

  t_program* program_;
  std::string name_;
};


/**
 * Placeholder struct for returning the key and value of an annotation
 * during parsing.
 */
struct t_annotation {
  std::string key;
  std::string val;
};

void override_annotations(std::map<std::string, std::string>& where,
                          const std::map<std::string, std::string>& from);
#endif
