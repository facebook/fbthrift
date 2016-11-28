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

#pragma once

#include <thrift/compiler/parse/t_program.h>
#include <thrift/compiler/parse/t_service.h>

namespace apache { namespace thrift { namespace compiler {

class visitor {
 public:
  virtual ~visitor() = default;

  void traverse(t_program const* program);

 protected:
  visitor() = default;

  /***
   *  Derived visitor types will override these virtual methods.
   *
   *  Return whether or not to visit children AST nodes afterward. For example,
   *  if `visit(t_program const*)` returns `true`, then the visitor will
   *  continue visiting the program's members.
   *
   *  The default implementations of these virtual methods is simply to return
   *  `true`. This allows derived visitor types to implement only the particular
   *  overloads that they specifically need.
   */
  virtual bool visit(t_program const* program);
  virtual bool visit(t_service const* service);

 private:
  void visit_(t_program const* program);
  void visit_(t_service const* service);
};

}}}
