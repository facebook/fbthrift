/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <vector>

#include <thrift/compiler/ast/ast_visitor.h>
#include <thrift/compiler/ast/diagnostic_context.h>
#include <thrift/compiler/ast/t_program_bundle.h>

namespace apache {
namespace thrift {
namespace compiler {

// Mutators have mutable access to the AST.
using mutator_context = visitor_context;

// An AST mutator is a ast_visitor that collects diagnostics and can
// change the ast.
class ast_mutator
    : public basic_ast_visitor<false, diagnostic_context&, mutator_context&> {
  using base = basic_ast_visitor<false, diagnostic_context&, mutator_context&>;

 public:
  using base::base;

  void mutate(diagnostic_context& ctx, t_program_bundle& bundle) {
    mutator_context mctx;
    for (auto& program : bundle.programs()) {
      operator()(ctx, mctx, program);
    }
  }
};

class ast_mutators {
 public:
  explicit ast_mutators(std::vector<ast_mutator> mutators)
      : mutators_(mutators) {}

  void operator()(diagnostic_context& ctx, t_program_bundle& bundle) {
    for (auto& mutator : mutators_) {
      mutator.mutate(ctx, bundle);
    }
  }

 private:
  std::vector<ast_mutator> mutators_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
