<?hh

/*
 * Copyright 2006-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * Base interface for Thrift structs
 */
<<__ConsistentConstruct>>
interface IThriftStruct {

  const type TFieldSpec = ThriftStructFieldSpecImpl;
  // Union of TFieldSpec and TElemSpec. For use in places that operate on both.
  const type TGenericSpec = ThriftStructGenericSpecImpl;
  const type TSpec = KeyedContainer<int, this::TFieldSpec>;
  const type TFieldMap = KeyedContainer<string, int>;

  abstract const this::TSpec SPEC;
  abstract const this::TFieldMap FIELDMAP;
  abstract const int STRUCTURAL_ID;

  <<__Rx>>
  public function __construct();
  public function getName(): string;
  <<__Rx, __AtMostRxAsArgs, __Mutable>>
  public function read(
    <<__OnlyRxIfImpl(IRxTProtocol::class), __Mutable>>
    TProtocol $input,
  ): int;
  public function write(TProtocol $input): int;
}

type ThriftStructFieldSpecImpl = shape(
  'var' => string,
  'type' => TType,
  ?'union' => bool,
  ?'etype' => TType,
  ?'elem' => ThriftStructElemSpecImpl,
  ?'ktype' => TType,
  ?'vtype' => TType,
  ?'key' => ThriftStructElemSpecImpl,
  ?'val' => ThriftStructElemSpecImpl,
  ?'format' => string,
  ?'class' => classname<IThriftStruct>,
  ?'enum' => string, // classname<HH\BuiltinEnum<int>>
);
type ThriftStructElemSpecRecursiveImpl<
  T as ThriftStructElemSpecRecursiveImpl<T>,
> = shape(
  'type' => TType,
  ?'etype' => TType,
  ?'elem' => T,
  ?'ktype' => TType,
  ?'vtype' => TType,
  ?'key' => T,
  ?'val' => T,
  ?'format' => string,
  ?'class' => classname<IThriftStruct>,
  ?'enum' => string, // classname<HH\BuiltinEnum<int>>
);
type ThriftStructGenericSpecRecursiveImpl<
  T as ThriftStructGenericSpecRecursiveImpl<T>,
> = shape(
  ?'var' => string,
  ?'type' => TType,
  ?'union' => bool,
  ?'etype' => TType,
  ?'elem' => T,
  ?'ktype' => TType,
  ?'vtype' => TType,
  ?'key' => T,
  ?'val' => T,
  ?'format' => string,
  ?'class' => classname<IThriftStruct>,
  ?'enum' => string, // classname<HH\BuiltinEnum<int>>
  ...
);
type ThriftStructElemSpecImpl = ThriftStructElemSpecRecursiveImpl<
  /* HH_FIXME[4101] */ /* HH_FIXME[4102] */
  ThriftStructElemSpecRecursiveImpl,
>;
type ThriftStructGenericSpecImpl = ThriftStructGenericSpecRecursiveImpl<
  /* HH_FIXME[4101] */ /* HH_FIXME[4102] */
  ThriftStructGenericSpecRecursiveImpl
>;
