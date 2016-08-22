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

#ifndef T_FUNCTION_H
#define T_FUNCTION_H

#include <string>
#include <thrift/compiler/parse/t_type.h>
#include <thrift/compiler/parse/t_struct.h>
#include <thrift/compiler/parse/t_doc.h>

extern int yylineno;

/**
 * Representation of a function. Key parts are return type, function name,
 * optional modifiers, and an argument list, which is implemented as a thrift
 * struct.
 *
 */
class t_function : public t_doc {
 public:
  t_function(t_type* returntype,
             std::string name,
             t_struct* arglist,
             t_type* annotations=nullptr,
             bool oneway=false) :
    returntype_(returntype),
    name_(name),
    arglist_(arglist),
    annotations_(annotations),
    oneway_(oneway) {
    xceptions_ = new t_struct(nullptr);

    if (oneway_) {
      if (returntype_ == nullptr || !returntype_->is_void()) {
          throw std::string("Oneway methods must have void return type.");
      }
    }
  }

  t_function(t_type* returntype,
             std::string name,
             t_struct* arglist,
             t_struct* xceptions,
             t_type* annotations=nullptr,
             bool oneway=false) :
    returntype_(returntype),
    name_(name),
    arglist_(arglist),
    xceptions_(xceptions),
    annotations_(annotations),
    oneway_(oneway) {
    if (oneway_) {
      if (!xceptions_->get_members().empty()) {
        throw std::string("Oneway methods can't throw exceptions.");
      }

      if (returntype_ == nullptr || !returntype_->is_void()) {
          throw std::string("Oneway methods must have void return type.");
      }
    }
  }

  ~t_function() {}

  t_type* get_returntype() const {
    return returntype_;
  }

  const std::string& get_name() const {
    return name_;
  }

  t_struct* get_arglist() const {
    return arglist_;
  }

  t_struct* get_xceptions() const {
    return xceptions_;
  }

  t_type* get_annotations() const {
      return annotations_;
  }

  bool is_oneway() const {
    return oneway_;
  }

 private:
  friend class t_service;

  t_type* returntype_;
  std::string name_;
  t_struct* arglist_;
  t_struct* xceptions_;
  t_type* annotations_;
  bool oneway_;
  int lineno_{yylineno};
};

#endif
