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

include "thrift/annotation/scope.thrift"

namespace cpp2 facebook.thrift.annotation.api
namespace py3 facebook.thrift.annotation.api
namespace php facebook_thrift_annotation_api
namespace java2 com.facebook.thrift.annotation.api
namespace java.swift com.facebook.thrift.annotation.api
namespace java com.facebook.thrift.annotation.api_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.api
namespace go thrift.annotation.api
namespace py thrift.annotation.api

// Indicates a field will be ignored if sent as input.
//
// For example, life-cycle timestamps fields like `createTime` and `modifyTime`
// should always be Output only, as they are set as side-effects of CRUD operations.
@scope.Field
struct OutputOnly {}

// Indicates a field cannot be changed after creations.
//
// For example, the name or id of a resource can never be changed after
// creation.
@scope.Field
struct Immutable {}
