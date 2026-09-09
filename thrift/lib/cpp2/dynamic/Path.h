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

#include <thrift/lib/cpp2/dynamic/DynamicValue.h>
#include <thrift/lib/cpp2/dynamic/ValueConversion.h>

#include <fmt/core.h>
#include <folly/lang/Exception.h>

#include <deque>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace apache::thrift::dynamic {

namespace detail {
/**
 * Get a display name for a TypeRef.
 */
std::string typeDisplayName(type_system::TypeRef type);

} // namespace detail

/**
 * Exception thrown when a path access is invalid for the current type.
 */
class InvalidPathAccessError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/**
 * Represents a path through a Thrift structure as a sequence of access
 * operations.
 *
 * Path format when converted to string:
 *   <root><component>*
 *
 * Where each component is one of:
 *   .fieldName     - struct/union field access
 *   [index]        - list element access
 *   {<value>}      - set element or map key
 *   [<value>]      - map value access by key or any type access
 *
 * The <value> is serialized using Thrift's SimpleJSON protocol.
 *
 * Example paths:
 *   MyStruct.users["alice"].scores[0]
 *   MyStruct.tags{"important"}
 *   MyStruct.userData[meta.com/ads/TargetingFeatures].age
 */
class Path {
 public:
  class FieldAccess final {
   public:
    FieldAccess(
        type_system::TypeRef structuredType,
        type_system::FastFieldHandle fieldHandle)
        : structuredType_(structuredType), fieldHandle_(fieldHandle) {}

    type_system::TypeRef structuredType() const { return structuredType_; }
    type_system::FastFieldHandle fieldHandle() const { return fieldHandle_; }
    type::FieldId fieldId() const { return field().identity().id(); }
    std::string_view fieldName() const { return field().identity().name(); }
    type_system::TypeRef fieldType() const { return field().type(); }

   private:
    const type_system::FieldDefinition& field() const {
      return structuredType_.asStructured().at(fieldHandle_);
    }

    type_system::TypeRef structuredType_;
    type_system::FastFieldHandle fieldHandle_;
  };

  class ListElement final {
   public:
    explicit ListElement(std::size_t index) : index_(index) {}
    std::size_t index() const { return index_; }

   private:
    std::size_t index_;
  };

  class SetElement final {
   public:
    explicit SetElement(DynamicValue value) : value_(std::move(value)) {}
    DynamicConstRef value() const { return value_; }

   private:
    DynamicValue value_;
  };

  class MapKey final {
   public:
    explicit MapKey(DynamicValue key) : key_(std::move(key)) {}
    DynamicConstRef key() const { return key_; }

   private:
    DynamicValue key_;
  };

  class MapValue final {
   public:
    explicit MapValue(DynamicValue key) : key_(std::move(key)) {}
    DynamicConstRef key() const { return key_; }

   private:
    DynamicValue key_;
  };

  class AnyType final {
   public:
    explicit AnyType(type_system::TypeRef type) : type_(type) {}
    type_system::TypeRef type() const { return type_; }

   private:
    type_system::TypeRef type_;
  };

  using Component = std::
      variant<FieldAccess, ListElement, SetElement, MapKey, MapValue, AnyType>;

  /**
   * Returns the current path as a human- and machine-readable string.
   * The string uniquely determines a Path, but one Path may have multiple
   * string representations.
   */
  std::string toString() const;

  /**
   * Returns the root type.
   */
  type_system::TypeRef rootType() const { return rootType_; }

  /**
   * Returns the path components.
   */
  std::span<const Component> components() const& { return components_; }
  std::span<const Component> components() const&& = delete;

  auto begin() const& { return components_.begin(); }
  auto end() const& { return components_.end(); }
  std::vector<Component>::const_iterator begin() const&& = delete;
  std::vector<Component>::const_iterator end() const&& = delete;
  bool empty() const { return components_.empty(); }
  std::size_t size() const { return components_.size(); }

 private:
  /**
   * Construct a Path with a root type.
   */
  explicit Path(type_system::TypeRef rootType);

  /**
   * Add a component to the path.
   */
  void push(Component component);

  /**
   * Remove the last component from the path.
   */
  void pop();

  type_system::TypeRef rootType_;
  std::vector<Component> components_;

  friend class PathBuilder;
  friend class DynamicRef;
  friend class DynamicConstRef;
};

/**
 * A builder for creating Path objects with type validation.
 *
 * Provides scope guards for entering fields, list elements, map keys/values,
 * etc., which automatically pop the path component when destroyed.
 *
 * This class validates that all accesses are valid for the current type.
 * If an invalid access is attempted, an InvalidPathAccessError is thrown.
 */
class PathBuilder {
 public:
  class ScopeGuard {
   public:
    ~ScopeGuard();

    ScopeGuard(ScopeGuard&& other) noexcept = delete;
    ScopeGuard& operator=(ScopeGuard&& other) noexcept = delete;
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

   private:
    friend class PathBuilder;
    explicit ScopeGuard(PathBuilder* builder, bool popComponent = true);

    PathBuilder* builder_;
    bool popComponent_;
  };

  /**
   * Construct a PathBuilder with a root type.
   */
  explicit PathBuilder(type_system::TypeRef rootType);

  /**
   * Enter a struct field with the given name.
   * Returns a scope guard that pops this path component on destruction.
   *
   * Throws InvalidPathAccessError if the current type is not a struct/union
   * or does not have a field with the given name.
   */
  [[nodiscard]] ScopeGuard enterField(std::string_view fieldName);
  [[nodiscard]] ScopeGuard enterField(type::FieldId fieldId);
  [[nodiscard]] ScopeGuard enterField(type_system::FastFieldHandle fieldHandle);

  /**
   * Enter a list element at the given index.
   * Returns a scope guard that pops this path component on destruction.
   *
   * Throws InvalidPathAccessError if the current type is not a list.
   */
  [[nodiscard]] ScopeGuard enterListElement(std::size_t index);

  /**
   * Enter a set element with the given value.
   * Returns a scope guard that pops this path component on destruction.
   *
   * Throws InvalidPathAccessError if the current type is not a set.
   */
  template <typename T>
  [[nodiscard]] ScopeGuard enterSetElement(const T& value) {
    const auto& current = currentType();
    if (!current.isSet()) {
      folly::throw_exception<InvalidPathAccessError>(fmt::format(
          "cannot access set element on non-set type '{}'",
          detail::typeDisplayName(current)));
    }
    const auto elementType = current.asSet().elementType();
    auto encoded = makeSelector(value, elementType);
    typeStack_.push_back(elementType);
    path_.push(Path::SetElement{std::move(encoded)});
    return ScopeGuard(this);
  }

  /**
   * Enter the given map key.
   * Returns a scope guard that pops this path component on destruction.
   *
   * Throws InvalidPathAccessError if the current type is not a map.
   */
  template <typename T>
  [[nodiscard]] ScopeGuard enterMapKey(const T& key) {
    const auto& current = currentType();
    if (!current.isMap()) {
      folly::throw_exception<InvalidPathAccessError>(fmt::format(
          "cannot access map key on non-map type '{}'",
          detail::typeDisplayName(current)));
    }
    const auto keyType = current.asMap().keyType();
    auto encoded = makeSelector(key, keyType);
    typeStack_.push_back(keyType);
    path_.push(Path::MapKey{std::move(encoded)});
    return ScopeGuard(this);
  }

  /**
   * Enter a map value with the given key.
   * Returns a scope guard that pops this path component on destruction.
   *
   * Throws InvalidPathAccessError if the current type is not a map.
   */
  template <typename T>
  [[nodiscard]] ScopeGuard enterMapValue(const T& key) {
    const auto& current = currentType();
    if (!current.isMap()) {
      folly::throw_exception<InvalidPathAccessError>(fmt::format(
          "cannot access map value on non-map type '{}'",
          detail::typeDisplayName(current)));
    }
    auto encoded = makeSelector(key, current.asMap().keyType());
    typeStack_.push_back(current.asMap().valueType());
    path_.push(Path::MapValue{std::move(encoded)});
    return ScopeGuard(this);
  }

  /**
   * Enter an any type with the given known inner type.
   * Returns a scope guard that pops this path component on destruction.
   *
   * Throws InvalidPathAccessError if the current type is not an any type.
   */
  [[nodiscard]] ScopeGuard enterAnyType(type_system::TypeRef knownType);

  /** Advance the type context without adding a path component. */
  [[nodiscard]] ScopeGuard enterTypeContext(type_system::TypeRef type);

  /**
   * Returns the current path as a string.
   */
  std::string toString() const { return path_.toString(); }

  /**
   * Initialize a PathBuilder from a path string.
   * The path string may begin with a type name for human friendliness, which is
   * ignored.
   *
   * Throws InvalidPathAccessError if the string is not compatible with the
   * supplied type.
   * Throws InvalidTypeError if a type URI does not resolve.
   */
  static PathBuilder fromString(
      const type_system::TypeSystem& typeSystem,
      type_system::TypeRef type,
      std::string_view path);

  /**
   * Returns a copy of the current path.
   */
  Path path() const& { return path_; }
  Path path() && { return std::move(path_); }

  /**
   * Returns the current type at this path location.
   */
  type_system::TypeRef currentType() const { return typeStack_.back(); }

 private:
  template <typename T>
  static DynamicValue makeSelector(
      const T& value, type_system::TypeRef expectedType) {
    try {
      return toDynamicValue(value, expectedType);
    } catch (const std::exception& error) {
      folly::throw_exception<InvalidPathAccessError>(error.what());
    }
  }

  void pop(bool component);

  template <typename Handle>
  [[nodiscard]] ScopeGuard enterFieldImpl(Handle handle);

  Path path_;
  std::deque<type_system::TypeRef> typeStack_;
};

} // namespace apache::thrift::dynamic
