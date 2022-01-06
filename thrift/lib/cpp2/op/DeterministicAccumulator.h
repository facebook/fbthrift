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

#include <cstdint>
#include <optional>
#include <stack>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include <folly/ScopeGuard.h>
#include <folly/io/IOBuf.h>
#include <folly/lang/Exception.h>

namespace apache::thrift::op {

/**
 * A deterministic stack-context based accumulator that guarantees the
 * accumulated results to be stable and consistent across different languages
 * and implementations. Accumulators know how to combine individual thrift
 * primitives and know how to handle ordered/unordered elements.
 *
 * Ordered Elements:
 *   OrderedContext wraps a single Hasher instance that can combine a sequence
 *   of bytes. Each time we start combining an ordered collection of elements we
 *   need to create a new OrderedContext object on context stack. All subsequent
 *   combine operations will use that context to accumulate the hash.
 *   OrderedContext creation can be elided by accumulating into the previous
 *   OrderedContext on the stack. It is only possible if the previous context on
 *   the stack was also OrderedContext. For example: list<list<i64>>.
 *
 * Unordered Elements:
 *   UnorderedContext wraps a number of Hasher instances that need to be ordered
 *   before reducing them into one Hasher. Hasher implementations define the way
 *   to order Hasher instances. For example: set<i64>.
 *
 * Examples:
 *   struct MyData {
 *     1: list<i64> f1 = [1, 2, 3];
 *     2: set<i64>  f2 = {4, 5, 6};
 *   }
 *   MyData data;
 *   DeterministicAccumulator<MyHasherGenerator> acc;
 *   acc.beginUnordered();             // struct data begin
 *    acc.beginOrdered();              // field  f1   begin
 *     acc.combine(TType::T_LIST);     // field  f1   type
 *     acc.combine(1);                 // field  f1   id
 *     acc.beginOrdered();             // list   f1   begin
 *      acc.combine(TType::T_I64);     // list   f1   type
 *      acc.combine(3);                // list   f1   size
 *      acc.beginOrdered();            // list   f1   data begin
 *       acc.combine(1);               // f1[0]
 *       acc.combine(2);               // f1[1]
 *       acc.combine(3);               // f1[2]
 *      acc.endOrdered();              // list   f1   data end
 *     acc.endOrdered();               // list   f1   end
 *    acc.endOrdered();                // field  f1   end
 *    acc.beginOrdered();              // field  f2   begin
 *     acc.combine(TType::T_SET);      // field  f2   type
 *     acc.combine(2);                 // field  f2   id
 *     acc.beginOrdered();             // set    f2   begin
 *      acc.combine(TType::T_STRING);  // set    f2   type
 *      acc.combine(3);                // set    f2   size
 *      acc.beginUnordered();          // set    f2   data begin
 *       acc.combine(4);               // f2[0]
 *       acc.combine(5);               // f2[1]
 *       acc.combine(6);               // f2[2]
 *      acc.endUnordered();            // set    f2   data end
 *     acc.endOrdered();               // set    f2   end
 *    acc.endOrdered();                // field  f2   end
 *   acc.endUnordered();               // struct data end
 */
template <typename HasherGenerator>
class DeterministicAccumulator {
 private:
  using Hasher = decltype(std::declval<HasherGenerator>()());

 public:
  DeterministicAccumulator() = default;
  explicit DeterministicAccumulator(HasherGenerator generator)
      : generator_(std::move(generator)) {}

  Hasher& result() { return result_.value(); }
  const Hasher& result() const { return result_.value(); }

  template <typename T>
  void combine(const T& val) {
    next().combine(val);
  }
  void beginOrdered();
  void endOrdered();
  void beginUnordered() { context_.emplace(UnorderedContext{}); }
  void endUnordered();

 private:
  struct OrderedContext {
    Hasher hasher;
    // Number of ordered collections that this context represents.
    // Each time we open a new ordered context and we elide the creation,
    // this count gets bumped. We decrease it each time we leave an ordered
    // context (finish an ordered collection of elements).
    size_t count;
  };
  using UnorderedContext = std::vector<Hasher>;
  using Context = std::variant<OrderedContext, UnorderedContext>;
  enum ContextType { Ordered, Unordered };

  HasherGenerator generator_;
  std::optional<Hasher> result_;
  std::stack<Context> context_;

  template <ContextType I>
  constexpr auto* contextIf() noexcept {
    return !context_.empty() ? std::get_if<I>(&context_.top()) : nullptr;
  }

  template <ContextType I>
  constexpr auto& context() {
    if (auto* ctx = contextIf<I>()) {
      return *ctx;
    }
    folly::throw_exception<std::logic_error>("ordering context missmatch");
  }

  // Returns the hasher to use for the next value.
  Hasher& next();
  void exitContext(Hasher result);
};

// Creates a deterministic accumulator using the provided hasher.
template <class Hasher>
FOLLY_NODISCARD auto makeDeterministicAccumulator() {
  return DeterministicAccumulator([]() { return Hasher{}; });
}

template <class Accumulator>
FOLLY_NODISCARD auto makeOrderedHashGuard(Accumulator& accumulator) {
  accumulator.beginOrdered();
  return folly::makeGuard([&] { accumulator.endOrdered(); });
}

template <class Accumulator>
FOLLY_NODISCARD auto makeUnorderedHashGuard(Accumulator& accumulator) {
  accumulator.beginUnordered();
  return folly::makeGuard([&] { accumulator.endUnordered(); });
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::beginOrdered() {
  if (auto* ctx = contextIf<Ordered>()) {
    ++ctx->count;
  } else {
    context_.emplace(OrderedContext{.hasher = generator_(), .count = 1});
  }
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::endOrdered() {
  auto& ctx = context<Ordered>();
  if (--ctx.count == 0) {
    exitContext(std::move(ctx).hasher);
  }
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::endUnordered() {
  auto& ctx = context<Unordered>();
  std::sort(ctx.begin(), ctx.end());
  auto result = generator_();
  for (const auto& hasher : ctx) {
    result.combine(hasher);
  }
  exitContext(std::move(result));
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::exitContext(Hasher result) {
  context_.pop();
  result.finalize();
  if (auto* ctx = contextIf<Ordered>()) {
    ctx->hasher.combine(result);
  } else if (auto* ctx = contextIf<Unordered>()) {
    ctx->emplace_back(std::move(result));
  } else {
    result_.emplace(std::move(result));
  }
}

template <typename HasherGenerator>
auto DeterministicAccumulator<HasherGenerator>::next() -> Hasher& {
  if (auto* ctx = contextIf<Ordered>()) {
    return ctx->hasher;
  }
  auto& ctx = context<Unordered>();
  ctx.emplace_back(generator_());
  return ctx.back();
}

} // namespace apache::thrift::op
