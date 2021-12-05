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

#include <cstdint>
#include <optional>
#include <stack>
#include <utility>
#include <variant>
#include <vector>

#include <folly/Range.h>
#include <folly/io/IOBuf.h>

namespace apache {
namespace thrift {
namespace hash {

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
 *   acc.unorderedElementsBegin();     // struct data begin
 *    acc.orderedElementsBegin();      // field  f1   begin
 *     acc.combine(TType::T_LIST);     // field  f1   type
 *     acc.combine(1);                 // field  f1   id
 *     acc.orderedElementsBegin();     // list   f1   begin
 *      acc.combine(TType::T_I64);     // list   f1   type
 *      acc.combine(3);                // list   f1   size
 *      acc.orderedElementsBegin();    // list   f1   data begin
 *       acc.combine(1);               // f1[0]
 *       acc.combine(2);               // f1[1]
 *       acc.combine(3);               // f1[2]
 *      acc.orderedElementsEnd();      // list   f1   data end
 *     acc.orderedElementsEnd();       // list   f1   end
 *    acc.orderedElementsEnd();        // field  f1   end
 *    acc.orderedElementsBegin();      // field  f2   begin
 *     acc.combine(TType::T_SET);      // field  f2   type
 *     acc.combine(2);                 // field  f2   id
 *     acc.orderedElementsBegin();     // set    f2   begin
 *      acc.combine(TType::T_STRING);  // set    f2   type
 *      acc.combine(3);                // set    f2   size
 *      acc.unorderedElementsBegin();  // set    f2   data begin
 *       acc.combine(4);               // f2[0]
 *       acc.combine(5);               // f2[1]
 *       acc.combine(6);               // f2[2]
 *      acc.unorderedElementsEnd();    // set    f2   data end
 *     acc.orderedElementsEnd();       // set    f2   end
 *    acc.orderedElementsEnd();        // field  f2   end
 *   acc.unorderedElementsEnd();       // struct data end
 */
template <typename HasherGenerator>
class DeterministicAccumulator {
 private:
  using Hasher = decltype(std::declval<HasherGenerator>()());

 public:
  inline explicit DeterministicAccumulator(HasherGenerator generator = {});

  inline operator Hasher&&() &&;
  auto getResult() && { return std::move(result_)->getResult(); }

  inline void combine(bool value);
  inline void combine(std::int8_t value);
  inline void combine(std::int16_t value);
  inline void combine(std::int32_t value);
  inline void combine(std::int64_t value);
  inline void combine(float value);
  inline void combine(double value);
  inline void combine(const folly::IOBuf& value);
  inline void combine(folly::ByteRange value);
  inline void orderedElementsBegin();
  inline void orderedElementsEnd();
  inline void unorderedElementsBegin();
  inline void unorderedElementsEnd();

 private:
  struct OrderedContext {
    Hasher hasher;
    // Number of ordered collections that this context represents.
    // Each time we open a new ordered context and we elide the creation,
    // this count gets bumped. We decrease it each time we leave an ordered
    // context (finish an ordered collection of elements).
    std::size_t count;
  };
  using UnorderedContext = std::vector<Hasher>;
  using Context = std::variant<OrderedContext, UnorderedContext>;

  template <typename ContextType>
  inline ContextType* getContext();
  inline void exitContext(Hasher result);
  inline auto& lastHasher();

  HasherGenerator generator_;
  std::optional<Hasher> result_;
  std::stack<Context> context_;
};

} // namespace hash
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/hash/DeterministicAccumulator-inl.h>
