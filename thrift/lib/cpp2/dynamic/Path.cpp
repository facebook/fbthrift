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

#include <thrift/lib/cpp2/dynamic/Path.h>
#include <thrift/lib/cpp2/dynamic/Serialization.h>
#include <thrift/lib/cpp2/dynamic/TypeId.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>

#include <fmt/format.h>
#include <folly/ExceptionString.h>
#include <folly/Overload.h>
#include <folly/io/IOBufQueue.h>
#include <folly/lang/Exception.h>

namespace apache::thrift::dynamic {

namespace detail {

std::string typeDisplayName(type_system::TypeRef type) {
  auto uriToName = [](std::string_view uri) {
    auto lastSlash = uri.find_last_of('/');
    if (lastSlash == std::string_view::npos) {
      return std::string(uri);
    } else {
      return std::string(uri.substr(lastSlash + 1));
    }
  };

  switch (type.kind()) {
    case type_system::TypeRef::Kind::BOOL:
      return "bool";
    case type_system::TypeRef::Kind::BYTE:
      return "byte";
    case type_system::TypeRef::Kind::I16:
      return "i16";
    case type_system::TypeRef::Kind::I32:
      return "i32";
    case type_system::TypeRef::Kind::I64:
      return "i64";
    case type_system::TypeRef::Kind::FLOAT:
      return "float";
    case type_system::TypeRef::Kind::DOUBLE:
      return "double";
    case type_system::TypeRef::Kind::STRING:
      return "string";
    case type_system::TypeRef::Kind::BINARY:
      return "binary";
    case type_system::TypeRef::Kind::ANY:
      return "any";
    case type_system::TypeRef::Kind::LIST:
      return fmt::format(
          "list<{}>", typeDisplayName(type.asList().elementType()));
    case type_system::TypeRef::Kind::SET:
      return fmt::format(
          "set<{}>", typeDisplayName(type.asSet().elementType()));
    case type_system::TypeRef::Kind::MAP:
      return fmt::format(
          "map<{}, {}>",
          typeDisplayName(type.asMap().keyType()),
          typeDisplayName(type.asMap().valueType()));
    case type_system::TypeRef::Kind::STRUCT:
      return uriToName(type.asStruct().uri());
    case type_system::TypeRef::Kind::UNION:
      return uriToName(type.asUnion().uri());
    case type_system::TypeRef::Kind::ENUM:
      return uriToName(type.asEnum().uri());
    case type_system::TypeRef::Kind::OPAQUE_ALIAS:
      return uriToName(type.asOpaqueAlias().uri());
  }
  return "unknown";
}

namespace {

std::string toSimpleJSON(const DynamicConstRef& value) {
  folly::IOBufQueue queue;
  SimpleJSONProtocolWriter writer;
  writer.setOutput(&queue);
  serializeValue(writer, value);
  return queue.move()->to<std::string>();
}

struct PathParser {
  std::string_view path;
  const type_system::TypeSystem* ts = nullptr;

  [[noreturn]] void invalid(
      std::string_view what, bool withCurrentException = false) {
    if (withCurrentException) {
      throw InvalidPathAccessError(
          fmt::format(
              "Invalid {} ({}): {}",
              what,
              folly::exceptionStr(std::current_exception()),
              path));
    }
    throw InvalidPathAccessError(fmt::format("Invalid {}: {}", what, path));
  }

  std::string_view consumeTo(char terminator, std::string_view error) {
    size_t bracketDepth = 1;
    for (size_t pos = 0; pos < path.size(); ++pos) {
      switch (path[pos]) {
        case '<':
          ++bracketDepth;
          break;
        case '>':
          --bracketDepth;
          if (bracketDepth == 0) {
            if (terminator != '>') {
              invalid(error);
            }
            auto ret = path.substr(0, pos);
            path.remove_prefix(pos + 1);
            return ret;
          }
          break;
        case ',':
          if (terminator == ',' && bracketDepth == 1) {
            auto ret = path.substr(0, pos);
            if (pos + 1 < path.size() && path[pos + 1] == ' ') {
              ++pos;
            }
            path.remove_prefix(pos + 1);
            return ret;
          }
          break;
        default:
          break;
      }
    }
    invalid(error);
  }

  bool tryConsume(std::string_view prefix) {
    if (path.starts_with(prefix)) {
      path.remove_prefix(prefix.length());
      return true;
    }
    return false;
  }

  bool tryConsumePrimitive(std::string_view name) {
    if (!path.starts_with(name) ||
        (path.size() > name.size() &&
         std::string_view{",[{}]>"}.find(path[name.size()]) ==
             std::string_view::npos)) {
      return false;
    }
    path.remove_prefix(name.size());
    return true;
  }

  type_system::TypeId parseTypeName() {
    if (tryConsumePrimitive("bool")) {
      return type_system::TypeIds::Bool;
    } else if (tryConsumePrimitive("byte")) {
      return type_system::TypeIds::Byte;
    } else if (tryConsumePrimitive("i16")) {
      return type_system::TypeIds::I16;
    } else if (tryConsumePrimitive("i32")) {
      return type_system::TypeIds::I32;
    } else if (tryConsumePrimitive("i64")) {
      return type_system::TypeIds::I64;
    } else if (tryConsumePrimitive("float")) {
      return type_system::TypeIds::Float;
    } else if (tryConsumePrimitive("double")) {
      return type_system::TypeIds::Double;
    } else if (tryConsumePrimitive("string")) {
      return type_system::TypeIds::String;
    } else if (tryConsumePrimitive("binary")) {
      return type_system::TypeIds::Binary;
    } else if (tryConsumePrimitive("any")) {
      return type_system::TypeIds::Any;
    } else if (tryConsume("list<")) {
      auto elemType = parseTypeName(
          consumeTo('>', "container name"), "container element type");
      return type_system::TypeIds::list(elemType);
    } else if (tryConsume("set<")) {
      auto elemType = parseTypeName(
          consumeTo('>', "container name"), "container element type");
      return type_system::TypeIds::set(elemType);
    } else if (tryConsume("map<")) {
      auto keyType = parseTypeName(consumeTo(',', "map name"), "map key type");
      auto valType =
          parseTypeName(consumeTo('>', "map name"), "map value type");
      return type_system::TypeIds::map(keyType, valType);
    } else {
      // URI: skip over domain name, then look for start of the next component
      // or end of the current Any traversal.
      auto slashPos = path.find('/');
      auto selectorPos = path.find_first_of("[{");
      if (slashPos == std::string_view::npos || selectorPos < slashPos) {
        // Raw type name (generated in display mode).
        slashPos = 0;
      }
      auto end = path.find_first_of(".[]{", slashPos);
      auto uri = path.substr(0, end);
      if (uri.empty()) {
        invalid("type name");
      }
      path.remove_prefix(uri.size());
      return type_system::TypeIds::uri(uri);
    }
  }

  type_system::TypeId parseTypeName(
      std::string_view typeName, std::string_view error) {
    PathParser parser{typeName};
    auto typeId = parser.parseTypeName();
    if (!parser.path.empty()) {
      parser.invalid(error);
    }
    return typeId;
  }

  std::string_view parseIdentifier() {
    size_t pos = 0;
    for (; pos < path.size() &&
         (std::isalnum(static_cast<unsigned char>(path[pos])) ||
          path[pos] == '_');
         ++pos) {
    }
    if (pos == 0) {
      invalid("identifier");
    }
    auto ret = path.substr(0, pos);
    path.remove_prefix(pos);
    return ret;
  }

  void skipTypeName() {
    if (path.empty()) {
      return;
    }
    // Type name may be omitted, in which case path starts with a traversal.
    if (std::string_view(".[{").find(path[0]) != std::string_view::npos) {
      return;
    }
    std::ignore = parseTypeName();
  }

  DynamicValue parseValue(type_system::TypeRef type) {
    SimpleJSONProtocolReader reader;
    auto buf = folly::IOBuf::wrapBufferAsValue(path.data(), path.size());
    reader.setInput(&buf);
    try {
      auto ret = deserializeValue(reader, type);
      path.remove_prefix(reader.getCursorPosition());
      return ret;
    } catch (const type_system::InvalidTypeError&) {
      throw;
    } catch (const std::runtime_error&) {
      invalid("value literal for expected type", /*withCurrentException=*/true);
    } catch (const TProtocolException&) {
      invalid("serialized value literal", /*withCurrentException=*/true);
    } catch (const std::out_of_range&) {
      invalid("serialized value literal", /*withCurrentException=*/true);
    }
  }

  using NextComponent = std::pair<Path::Component, type_system::TypeRef>;
  NextComponent parseNext(type_system::TypeRef type) {
    type = type.trueType();
    return type.matchKind(
        [&](type_system::TypeRef::KindConstant<
            type_system::TypeRef::Kind::ANY>) {
          if (!tryConsume("[")) {
            invalid("any traversal");
          }
          const auto end = path.find(']');
          if (end == std::string_view::npos) {
            invalid("any traversal");
          }
          auto typeId =
              parseTypeName(path.substr(0, end), "any traversal type");
          path.remove_prefix(end + 1);
          type = DCHECK_NOTNULL(ts)->resolveTypeId(typeId);
          return NextComponent{Path::AnyType(type), type};
        },
        [&](type_system::TypeRef::KindConstant<
            type_system::TypeRef::Kind::LIST>) {
          if (!tryConsume("[")) {
            invalid("list traversal");
          }
          size_t index;
          const auto* begin = path.data();
          const auto* end = begin + path.size();
          auto [ptr, error] = std::from_chars(begin, end, index);
          if (error != std::errc{}) {
            invalid("list traversal");
          }
          path.remove_prefix(ptr - begin);
          if (!tryConsume("]")) {
            invalid("list traversal");
          }
          return NextComponent{
              Path::ListElement(index), type.asListUnchecked().elementType()};
        },
        [&](type_system::TypeRef::KindConstant<
            type_system::TypeRef::Kind::SET>) {
          if (!tryConsume("{")) {
            invalid("set traversal");
          }
          auto elementType = type.asSetUnchecked().elementType();
          auto value = parseValue(elementType);
          if (!tryConsume("}")) {
            invalid("set traversal");
          }
          return NextComponent{Path::SetElement(std::move(value)), elementType};
        },
        [&](type_system::TypeRef::KindConstant<
            type_system::TypeRef::Kind::MAP>) {
          auto keyType = type.asMapUnchecked().keyType();
          auto valueType = type.asMapUnchecked().valueType();
          if (tryConsume("{")) {
            auto key = parseValue(keyType);
            if (!tryConsume("}")) {
              invalid("map key traversal");
            }
            return NextComponent{Path::MapKey(std::move(key)), keyType};
          } else if (tryConsume("[")) {
            auto key = parseValue(keyType);
            if (!tryConsume("]")) {
              invalid("map value traversal");
            }
            return NextComponent{Path::MapValue(std::move(key)), valueType};
          } else {
            invalid("map traversal");
          }
        },
        [&]<type_system::TypeRef::Kind Kind>(
            type_system::TypeRef::KindConstant<Kind>) {
          if (Kind != type_system::TypeRef::Kind::STRUCT &&
              Kind != type_system::TypeRef::Kind::UNION) {
            invalid("traversal target");
          }

          if (!tryConsume(".")) {
            invalid("structured traversal");
          }

          const auto& structuredType =
              Kind == type_system::TypeRef::Kind::STRUCT
              ? static_cast<const type_system::StructuredNode&>(
                    type.asStructUnchecked())
              : type.asUnionUnchecked();
          auto fieldName = parseIdentifier();
          auto fieldHandle = structuredType.fieldHandleFor(fieldName);
          if (!fieldHandle.valid()) {
            invalid(
                fmt::format(
                    "field name `{}` in traversal of structured type `{}`",
                    fieldName,
                    structuredType.debugName()));
          }
          const auto& field = structuredType.at(fieldHandle);
          return NextComponent{
              Path::FieldAccess(type, fieldHandle), field.type()};
        });
  }
};

} // namespace
} // namespace detail

// Path implementation

Path::Path(type_system::TypeRef rootType) : rootType_(std::move(rootType)) {}

void Path::push(Component component) {
  components_.push_back(std::move(component));
}

void Path::pop() {
  components_.pop_back();
}

std::string Path::toString() const {
  std::string result = detail::typeDisplayName(rootType_);

  for (const auto& component : components_) {
    folly::variant_match(
        component,
        [&](const FieldAccess& f) {
          result += fmt::format(".{}", f.fieldName());
        },
        [&](const ListElement& l) { result += fmt::format("[{}]", l.index()); },
        [&](const SetElement& s) {
          result += fmt::format("{{{}}}", detail::toSimpleJSON(s.value()));
        },
        [&](const MapKey& m) {
          result += fmt::format("{{{}}}", detail::toSimpleJSON(m.key()));
        },
        [&](const MapValue& m) {
          result += fmt::format("[{}]", detail::toSimpleJSON(m.key()));
        },
        [&](const AnyType& a) {
          result += fmt::format("[{}]", a.type().id().name());
        });
  }

  return result;
}

PathBuilder PathBuilder::fromString(
    const type_system::TypeSystem& typeSystem,
    type_system::TypeRef type,
    std::string_view path) {
  detail::PathParser parser{path, &typeSystem};
  PathBuilder builder(type);
  parser.skipTypeName();
  while (!parser.path.empty()) {
    auto [component, nextType] = parser.parseNext(type);
    type = nextType;
    builder.path_.push(std::move(component));
    builder.typeStack_.push_back(type);
  }
  return builder;
}

// PathBuilder implementation

PathBuilder::ScopeGuard::ScopeGuard(PathBuilder* builder, bool popComponent)
    : builder_(builder), popComponent_(popComponent) {}

PathBuilder::ScopeGuard::~ScopeGuard() {
  if (builder_) {
    builder_->pop(popComponent_);
  }
}

PathBuilder::PathBuilder(type_system::TypeRef rootType) : path_(rootType) {
  typeStack_.push_back(rootType);
}

namespace {
template <typename T>
decltype(auto) printable(T t) {
  if constexpr (requires { t.ordinal; }) {
    return t.ordinal;
  } else if constexpr (std::is_enum_v<T>) {
    return static_cast<std::underlying_type_t<T>>(t);
  } else {
    return t;
  }
}
} // namespace

template <typename T>
PathBuilder::ScopeGuard PathBuilder::enterFieldImpl(T id) {
  const auto& current = currentType();

  if (!current.isStructured()) {
    folly::throw_exception<InvalidPathAccessError>(fmt::format(
        "cannot access field '{}' on non-structured type '{}'",
        printable(id),
        detail::typeDisplayName(current)));
  }

  const auto& structured = current.asStructured();
  auto handle = [&] {
    if constexpr (std::is_same_v<T, type_system::FastFieldHandle>) {
      return id;
    } else {
      return structured.fieldHandleFor(id);
    }
  }();

  if (!handle.valid()) {
    folly::throw_exception<InvalidPathAccessError>(fmt::format(
        "field '{}' does not exist on type '{}'",
        printable(id),
        detail::typeDisplayName(current)));
  }

  const auto& field = structured.at(handle);
  typeStack_.push_back(field.type());
  path_.push(Path::FieldAccess{current, handle});

  return ScopeGuard(this);
}

PathBuilder::ScopeGuard PathBuilder::enterField(std::string_view handle) {
  return enterFieldImpl(handle);
}

PathBuilder::ScopeGuard PathBuilder::enterField(type::FieldId handle) {
  return enterFieldImpl(handle);
}

PathBuilder::ScopeGuard PathBuilder::enterField(
    type_system::FastFieldHandle handle) {
  return enterFieldImpl(handle);
}

PathBuilder::ScopeGuard PathBuilder::enterListElement(std::size_t index) {
  const auto& current = currentType();

  if (!current.isList()) {
    folly::throw_exception<InvalidPathAccessError>(fmt::format(
        "cannot access list element on non-list type '{}'",
        detail::typeDisplayName(current)));
  }

  typeStack_.push_back(current.asList().elementType());
  path_.push(Path::ListElement{index});

  return ScopeGuard(this);
}

PathBuilder::ScopeGuard PathBuilder::enterAnyType(
    type_system::TypeRef knownType) {
  const auto& current = currentType();

  if (!current.isAny()) {
    folly::throw_exception<InvalidPathAccessError>(fmt::format(
        "cannot access any type on non-any type '{}'",
        detail::typeDisplayName(current)));
  }

  typeStack_.push_back(knownType);
  path_.push(Path::AnyType{knownType});

  return ScopeGuard(this);
}

PathBuilder::ScopeGuard PathBuilder::enterTypeContext(
    type_system::TypeRef type) {
  typeStack_.push_back(type);
  return ScopeGuard(this, false);
}

void PathBuilder::pop(bool component) {
  if (component) {
    path_.pop();
  }
  typeStack_.pop_back();
}

} // namespace apache::thrift::dynamic
