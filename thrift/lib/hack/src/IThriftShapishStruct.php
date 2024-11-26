<?hh
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
 *
 */

// @oss-enable: use namespace FlibSL\{C, Math, Str, Vec};

/**
 * Interface for Thrift structs that support conversions to shapes
 */
<<Oncalls('thrift')>> // @oss-disable
interface IThriftShapishStruct extends IThriftStruct {
  abstract const type TShape as shape(...);
}

<<Oncalls('thrift')>> // @oss-disable
interface IThriftShapishSyncStruct extends IThriftShapishStruct {
  abstract const type TShape as shape(...);

  public function __toShape()[]: this::TShape;
  public static function __fromShape(this::TShape $shape)[]: this;

}

/**
 * Interface for Thrift structs that support async conversions to shapes
 *
 * Similar to IThriftAsyncStruct, this is used for structs that directly or
 * indirectly have a field that is wrapped using FieldWrapper annotation.
 *
 */
<<Oncalls('thrift')>> // @oss-disable
interface IThriftShapishAsyncStruct extends IThriftShapishStruct {

  abstract const type TShape as shape(...);

  public function __genToShape(): Awaitable<this::TShape>;
  public static function __genFromShape(this::TShape $shape): Awaitable<this>;
}
