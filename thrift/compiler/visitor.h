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

#include <memory>
#include <vector>

#include <thrift/compiler/parse/t_program.h>
#include <thrift/compiler/parse/t_service.h>

namespace apache { namespace thrift { namespace compiler {

class visitor {
 public:
  virtual ~visitor() = default;

  /***
   *  The entry point for traversal. Call-sites should call this method.
   */
  void traverse(t_program const* program);

  /***
   *  Derived visitor types will generally override these virtual methods.
   *
   *  Return whether or not to visit children AST nodes afterward. For example,
   *  if `visit(t_program const*)` returns `true`, then the visitor will
   *  continue visiting the program's members.
   *
   *  The default implementations of these virtual methods is simply to return
   *  `true`. This allows derived visitor types to implement only the particular
   *  overloads that they specifically need.
   *
   *  Note: These are extension points, not entry points for traversal.
   */
  virtual bool visit(t_program const* program);
  virtual bool visit(t_service const* service);
  virtual bool visit(t_enum const* tenum);

 protected:
  visitor() = default;

  /***
   *  Visitor combinator types will generally override these virutal methods.
   *
   *  General derived visitors should not need to.
   */
  virtual void visit_and_recurse(t_program const* program);
  virtual void visit_and_recurse(t_service const* service);
  virtual void visit_and_recurse(t_enum const* tenum);

  void recurse(t_program const* program);
  void recurse(t_service const* service);
  void recurse(t_enum const* tenum);
};

/***
 *  Useful for splitting up a single large visitor into multiple smaller ones,
 *  each doing its own portion of the work.
 *
 *  Runs the visitors across the AST nodes in lockstep with each other, taking
 *  care to recurse with only those visitors which return true from visit().
 *
 *  Performs a single concurrent traversal will all visitors, rather than a
 *  sequence of traversals with one visitor each. The concurrent traversal will
 *  interleave all the visitor traversals in lockstep.
 */
class interleaved_visitor : public visitor {
 public:
  explicit interleaved_visitor(std::vector<visitor*> visitors);

 protected:
  void visit_and_recurse(t_program const* program) override;
  void visit_and_recurse(t_service const* service) override;
  void visit_and_recurse(t_enum const* tenum) override;

 private:
  template <typename Visitee>
  void visit_and_recurse_gen(Visitee const* visitee);

  std::vector<visitor*> visitors_;
  std::vector<bool> rec_mask_{std::vector<bool>(visitors_.size(), true)};
};

}}}
