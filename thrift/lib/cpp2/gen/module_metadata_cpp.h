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

#include <array>
#include <memory>
#include <vector>

#include <thrift/lib/cpp2/gen/module_metadata_h.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types.h>

namespace apache::thrift::detail::md {

using ThriftMetadata = ::apache::thrift::metadata::ThriftMetadata;
using ThriftType = ::apache::thrift::metadata::ThriftType;
using ThriftConstValue = ::apache::thrift::metadata::ThriftConstValue;
using ThriftConstValuePair = ::apache::thrift::metadata::ThriftConstValuePair;
using ThriftConstStruct = ::apache::thrift::metadata::ThriftConstStruct;
using ThriftStructType = ::apache::thrift::metadata::ThriftStructType;

class MetadataTypeInterface {
 public:
  /**
   * writeAndGenType() performs two things:
   * 1. Ensures the type is present in metadata.
   * 2. Populates ThriftType datastruct so that the caller can look
   *    up the type inside metadata.
   */
  virtual void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) = 0;
  virtual ~MetadataTypeInterface() {}
};

/**
 * @brief Used in module metadata templates
 */
struct EncodedThriftField {
  int32_t id;
  const char* name;
  bool is_optional;
  std::unique_ptr<MetadataTypeInterface> metadata_type_interface;
  std::vector<ThriftConstStruct> structured_annotations;
};

class Primitive : public MetadataTypeInterface {
 public:
  Primitive(::apache::thrift::metadata::ThriftPrimitiveType base)
      : base_(base) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata&) override {
    ty.t_primitive() = base_;
  }

 private:
  ::apache::thrift::metadata::ThriftPrimitiveType base_;
};

class List : public MetadataTypeInterface {
 public:
  List(::std::unique_ptr<MetadataTypeInterface> elemType)
      : elemType_(::std::move(elemType)) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    ::apache::thrift::metadata::ThriftListType tyList;
    tyList.valueType() = ::std::make_unique<ThriftType>();
    elemType_->writeAndGenType(*tyList.valueType(), metadata);
    ty.t_list() = ::std::move(tyList);
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> elemType_;
};

class Set : public MetadataTypeInterface {
 public:
  Set(::std::unique_ptr<MetadataTypeInterface> elemType)
      : elemType_(::std::move(elemType)) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    ::apache::thrift::metadata::ThriftSetType tySet;
    tySet.valueType() = ::std::make_unique<ThriftType>();
    elemType_->writeAndGenType(*tySet.valueType(), metadata);
    ty.t_set() = ::std::move(tySet);
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> elemType_;
};

class Map : public MetadataTypeInterface {
 public:
  Map(::std::unique_ptr<MetadataTypeInterface> keyType,
      ::std::unique_ptr<MetadataTypeInterface> valueType)
      : keyType_(::std::move(keyType)), valueType_(::std::move(valueType)) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    ::apache::thrift::metadata::ThriftMapType tyMap;
    tyMap.keyType() = ::std::make_unique<ThriftType>();
    keyType_->writeAndGenType(*tyMap.keyType(), metadata);
    tyMap.valueType() = ::std::make_unique<ThriftType>();
    valueType_->writeAndGenType(*tyMap.valueType(), metadata);
    ty.t_map() = ::std::move(tyMap);
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> keyType_;
  ::std::unique_ptr<MetadataTypeInterface> valueType_;
};

template <typename E>
class Enum : public MetadataTypeInterface {
 public:
  Enum(const char* name) : name_(name) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    EnumMetadata<E>::gen(metadata);
    ::apache::thrift::metadata::ThriftEnumType tyEnum;
    tyEnum.name() = name_;
    ty.t_enum() = ::std::move(tyEnum);
  }

 private:
  const char* name_;
};

template <typename S>
class Struct : public MetadataTypeInterface {
 public:
  Struct(const char* name) : name_(name) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    StructMetadata<S>::gen(metadata);
    ::apache::thrift::metadata::ThriftStructType tyStruct;
    tyStruct.name() = name_;
    ty.t_struct() = ::std::move(tyStruct);
  }

 private:
  const char* name_;
};

template <typename U>
class Union : public MetadataTypeInterface {
 public:
  Union(const char* name) : name_(name) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    StructMetadata<U>::gen(metadata);
    ::apache::thrift::metadata::ThriftUnionType tyUnion;
    tyUnion.name() = name_;
    ty.t_union() = ::std::move(tyUnion);
  }

 private:
  const char* name_;
};

class Typedef : public MetadataTypeInterface {
 public:
  Typedef(
      const char* name,
      ::std::unique_ptr<MetadataTypeInterface> underlyingType,
      ::std::vector<ThriftConstStruct> structured_annotations)
      : name_(name),
        underlyingType_(::std::move(underlyingType)),
        structured_annotations_(::std::move(structured_annotations)) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    ::apache::thrift::metadata::ThriftTypedefType tyTypedef;
    tyTypedef.name() = name_;
    tyTypedef.underlyingType() = ::std::make_unique<ThriftType>();
    tyTypedef.structured_annotations() = structured_annotations_;
    underlyingType_->writeAndGenType(*tyTypedef.underlyingType(), metadata);
    ty.t_typedef() = ::std::move(tyTypedef);
  }

 private:
  const char* name_;
  ::std::unique_ptr<MetadataTypeInterface> underlyingType_;
  ::std::vector<ThriftConstStruct> structured_annotations_;
};

class Stream : public MetadataTypeInterface {
 public:
  Stream(
      ::std::unique_ptr<MetadataTypeInterface> elemType,
      ::std::unique_ptr<MetadataTypeInterface> initialResponseType = nullptr)
      : elemType_(::std::move(elemType)),
        initialResponseType_(::std::move(initialResponseType)) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    ::apache::thrift::metadata::ThriftStreamType tyStream;
    tyStream.elemType() = ::std::make_unique<ThriftType>();
    elemType_->writeAndGenType(*tyStream.elemType(), metadata);
    if (initialResponseType_) {
      tyStream.initialResponseType() = ::std::make_unique<ThriftType>();
      initialResponseType_->writeAndGenType(
          *tyStream.initialResponseType(), metadata);
    }
    ty.t_stream() = ::std::move(tyStream);
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> elemType_;
  ::std::unique_ptr<MetadataTypeInterface> initialResponseType_;
};

class Sink : public MetadataTypeInterface {
 public:
  Sink(
      ::std::unique_ptr<MetadataTypeInterface> elemType,
      ::std::unique_ptr<MetadataTypeInterface> finalResponseType,
      ::std::unique_ptr<MetadataTypeInterface> initialResponseType = nullptr)
      : elemType_(::std::move(elemType)),
        finalResponseType_(::std::move(finalResponseType)),
        initialResponseType_(::std::move(initialResponseType)) {}
  void writeAndGenType(ThriftType& ty, ThriftMetadata& metadata) override {
    ::apache::thrift::metadata::ThriftSinkType tySink;
    tySink.elemType() = ::std::make_unique<ThriftType>();
    elemType_->writeAndGenType(*tySink.elemType(), metadata);
    tySink.finalResponseType() = ::std::make_unique<ThriftType>();
    finalResponseType_->writeAndGenType(*tySink.finalResponseType(), metadata);
    if (initialResponseType_) {
      tySink.initialResponseType() = ::std::make_unique<ThriftType>();
      initialResponseType_->writeAndGenType(
          *tySink.initialResponseType(), metadata);
    }
    ty.t_sink() = ::std::move(tySink);
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> elemType_;
  ::std::unique_ptr<MetadataTypeInterface> finalResponseType_;
  ::std::unique_ptr<MetadataTypeInterface> initialResponseType_;
};

ThriftConstValue cvBool(bool value);
ThriftConstValue cvInteger(int64_t value);
ThriftConstValue cvDouble(double value);
ThriftConstValue cvString(const char* value);

ThriftConstValue cvMap(std::vector<ThriftConstValuePair>&& value);
ThriftConstValue cvList(std::vector<ThriftConstValue>&& value);
ThriftConstValue cvStruct(
    const char* name, std::map<std::string, ThriftConstValue>&& fields);

ThriftConstValuePair cvPair(ThriftConstValue&& key, ThriftConstValue&& value);

} // namespace apache::thrift::detail::md
