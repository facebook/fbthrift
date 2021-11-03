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

#include <stdexcept>

#include <folly/lang/Exception.h>

namespace apache {
namespace thrift {
namespace hash {

template <typename HasherGenerator>
DeterministicAccumulator<HasherGenerator>::DeterministicAccumulator(
    HasherGenerator generator)
    : generator_{std::move(generator)} {}

template <typename HasherGenerator>
DeterministicAccumulator<HasherGenerator>::operator Hasher&&() && {
  return std::move(result_).value();
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(bool value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(std::int8_t value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(std::int16_t value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(std::int32_t value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(std::int64_t value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(float value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(double value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(
    const folly::IOBuf& value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::combine(
    folly::ByteRange value) {
  lastHasher().combine(value);
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::orderedElementsBegin() {
  if (auto* context = getContext<OrderedContext>()) {
    ++context->count;
  } else {
    context_.emplace(OrderedContext{.hasher = generator_(), .count = 1});
  }
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::orderedElementsEnd() {
  if (auto* context = getContext<OrderedContext>()) {
    if (--context->count == 0) {
      exitContext(std::move(*context).hasher);
    }
  } else {
    folly::throw_exception<std::logic_error>(
        "Accumulator cannot close ordered elements "
        "without opening them first");
  }
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::unorderedElementsBegin() {
  context_.emplace(UnorderedContext{});
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::unorderedElementsEnd() {
  if (auto* context = getContext<UnorderedContext>()) {
    std::sort(context->begin(), context->end());
    auto result = generator_();
    for (const auto& hasher : *context) {
      result.combine(hasher);
    }
    exitContext(std::move(result));
  } else {
    folly::throw_exception<std::logic_error>(
        "Accumulator cannot close unordered elements "
        "without opening them first");
  }
}

template <typename HasherGenerator>
template <typename ContextType>
ContextType* DeterministicAccumulator<HasherGenerator>::getContext() {
  return context_.empty() ? nullptr : std::get_if<ContextType>(&context_.top());
}

template <typename HasherGenerator>
void DeterministicAccumulator<HasherGenerator>::exitContext(Hasher result) {
  context_.pop();
  result.finalize();
  if (auto* context = getContext<OrderedContext>()) {
    context->hasher.combine(result);
  } else if (auto* context = getContext<UnorderedContext>()) {
    context->emplace_back(std::move(result));
  } else {
    result_.emplace(std::move(result));
  }
}

template <typename HasherGenerator>
auto& DeterministicAccumulator<HasherGenerator>::lastHasher() {
  if (auto* context = getContext<OrderedContext>()) {
    return context->hasher;
  } else if (auto* context = getContext<UnorderedContext>()) {
    context->emplace_back(generator_());
    return context->back();
  } else {
    folly::throw_exception<std::logic_error>(
        "Accumulator cannot combine values "
        "without declaring elements type first");
  }
}

} // namespace hash
} // namespace thrift
} // namespace apache
