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

include "thrift/annotation/scope.thrift"

namespace cpp2 facebook.thrift.annotation.thrift
namespace py3 facebook.thrift.annotation.thrift
namespace php facebook_thrift_annotation_thrift
namespace java.swift com.facebook.thrift.annotation.thrift
namespace java com.facebook.thrift.annotation.thrift_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.thrift
namespace go thrift.annotation.thrift
namespace py thrift.annotation.thrift

// Indicates changes that break compatibility
@scope.Struct
@scope.Union
@scope.Exception
struct RequiresBackwardCompatibility {
  1: bool field_name = false;
} (
  thrift.uri = "facebook.com/thrift/annotation/thrift/RequiresBackwardCompatibility",
)
