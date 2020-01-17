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
#include <thrift/lib/cpp2/gen/module_metadata_h.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types.h>

namespace apache::thrift::detail::metadata {

using ThriftMetadata = ::apache::thrift::metadata::ThriftMetadata;
using ThriftType = ::apache::thrift::metadata::ThriftType;

class MetadataTypeInterface {
 public:
  virtual void initialize(ThriftType& ty) = 0;
  virtual ~MetadataTypeInterface() {}
};

class Primitive : public MetadataTypeInterface {
 public:
  Primitive(::apache::thrift::metadata::ThriftPrimitiveType base)
      : base_(::std::move(base)) {}
  void initialize(ThriftType& ty) override {
    ty.set_t_primitive(::std::move(base_));
  }

 private:
  ::apache::thrift::metadata::ThriftPrimitiveType base_;
};

class List : public MetadataTypeInterface {
 public:
  List(::std::unique_ptr<MetadataTypeInterface> elemType)
      : elemType_(::std::move(elemType)) {}
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftListType tyList;
    tyList.valueType = ::std::make_unique<ThriftType>();
    elemType_->initialize(*tyList.valueType);
    ty.set_t_list(::std::move(tyList));
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> elemType_;
};

class Set : public MetadataTypeInterface {
 public:
  Set(::std::unique_ptr<MetadataTypeInterface> elemType)
      : elemType_(::std::move(elemType)) {}
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftSetType tySet;
    tySet.valueType = ::std::make_unique<ThriftType>();
    elemType_->initialize(*tySet.valueType);
    ty.set_t_set(::std::move(tySet));
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> elemType_;
};

class Map : public MetadataTypeInterface {
 public:
  Map(::std::unique_ptr<MetadataTypeInterface> keyType,
      ::std::unique_ptr<MetadataTypeInterface> valueType)
      : keyType_(::std::move(keyType)), valueType_(::std::move(valueType)) {}
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftMapType tyMap;
    tyMap.keyType = ::std::make_unique<ThriftType>();
    keyType_->initialize(*tyMap.keyType);
    tyMap.valueType = ::std::make_unique<ThriftType>();
    valueType_->initialize(*tyMap.valueType);
    ty.set_t_map(::std::move(tyMap));
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> keyType_;
  ::std::unique_ptr<MetadataTypeInterface> valueType_;
};

template <typename E>
class Enum : public MetadataTypeInterface {
 public:
  Enum(::std::string name, ThriftMetadata& metadata)
      : name_(::std::move(name)) {
    GeneratedEnumMetadata<E>::genMetadata(metadata);
  }
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftEnumType tyEnum;
    tyEnum.set_name(::std::move(name_));
    ty.set_t_enum(::std::move(tyEnum));
  }

 private:
  ::std::string name_;
};

template <typename S>
class Struct : public MetadataTypeInterface {
 public:
  Struct(::std::string name, ThriftMetadata& metadata)
      : name_(::std::move(name)) {
    GeneratedStructMetadata<S>::genMetadata(metadata);
  }
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftStructType tyStruct;
    tyStruct.set_name(::std::move(name_));
    ty.set_t_struct(::std::move(tyStruct));
  }

 private:
  ::std::string name_;
};

template <typename U>
class Union : public MetadataTypeInterface {
 public:
  Union(::std::string name, ThriftMetadata& metadata)
      : name_(::std::move(name)) {
    GeneratedStructMetadata<U>::genMetadata(metadata);
  }
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftUnionType tyUnion;
    tyUnion.set_name(::std::move(name_));
    ty.set_t_union(::std::move(tyUnion));
  }

 private:
  ::std::string name_;
};

class Typedef : public MetadataTypeInterface {
 public:
  Typedef(
      ::std::string name,
      ::std::unique_ptr<MetadataTypeInterface> underlyingType)
      : name_(std::move(name)), underlyingType_(::std::move(underlyingType)) {}
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftTypedefType tyTypedef;
    tyTypedef.set_name(::std::move(name_));
    tyTypedef.underlyingType = ::std::make_unique<ThriftType>();
    underlyingType_->initialize(*tyTypedef.underlyingType);
    ty.set_t_typedef(::std::move(tyTypedef));
  }

 private:
  ::std::string name_;
  ::std::unique_ptr<MetadataTypeInterface> underlyingType_;
};

class Stream : public MetadataTypeInterface {
 public:
  Stream(
      ::std::unique_ptr<MetadataTypeInterface> elemType,
      ::std::unique_ptr<MetadataTypeInterface> initialResponseType = nullptr)
      : elemType_(::std::move(elemType)),
        initialResponseType_(::std::move(initialResponseType)) {}
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftStreamType tyStream;
    tyStream.elemType = ::std::make_unique<ThriftType>();
    elemType_->initialize(*tyStream.elemType);
    if (initialResponseType_) {
      tyStream.initialResponseType = ::std::make_unique<ThriftType>();
      initialResponseType_->initialize(*tyStream.initialResponseType);
    }
    ty.set_t_stream(::std::move(tyStream));
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
  void initialize(ThriftType& ty) override {
    ::apache::thrift::metadata::ThriftSinkType tySink;
    tySink.elemType = ::std::make_unique<ThriftType>();
    elemType_->initialize(*tySink.elemType);
    tySink.finalResponseType = ::std::make_unique<ThriftType>();
    finalResponseType_->initialize(*tySink.finalResponseType);
    if (initialResponseType_) {
      tySink.initialResponseType = ::std::make_unique<ThriftType>();
      initialResponseType_->initialize(*tySink.initialResponseType);
    }
    ty.set_t_sink(::std::move(tySink));
  }

 private:
  ::std::unique_ptr<MetadataTypeInterface> elemType_;
  ::std::unique_ptr<MetadataTypeInterface> finalResponseType_;
  ::std::unique_ptr<MetadataTypeInterface> initialResponseType_;
};

} // namespace apache::thrift::detail::metadata
