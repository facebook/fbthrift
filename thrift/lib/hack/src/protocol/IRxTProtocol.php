<?hh // partial

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
interface IRxTProtocol {

  <<__Rx, __Mutable>>
  public function readStructBegin(inout $name);
  <<__Rx, __Mutable>>
  public function readStructEnd();

  <<__Rx, __Mutable>>
  public function readFieldBegin(
    inout $name,
    inout $field_type,
    inout $field_id,
  );
  <<__Rx, __Mutable>>
  public function readFieldEnd();

  <<__Rx, __Mutable>>
  public function readMapBegin(inout $key_type, inout $val_type, inout $size);
  <<__Rx, __Mutable>>
  public function readMapHasNext(): bool;
  <<__Rx, __Mutable>>
  public function readMapEnd();

  <<__Rx, __Mutable>>
  public function readListBegin(inout $elem_type, inout $size);
  <<__Rx, __Mutable>>
  public function readListHasNext(): bool;
  <<__Rx, __Mutable>>
  public function readListEnd();

  <<__Rx, __Mutable>>
  public function readSetBegin(inout $elem_type, inout $size);
  <<__Rx, __Mutable>>
  public function readSetHasNext(): bool;
  <<__Rx, __Mutable>>
  public function readSetEnd();

  <<__Rx, __Mutable>>
  public function readBool(inout $bool);
  <<__Rx, __Mutable>>
  public function readByte(inout $byte);
  <<__Rx, __Mutable>>
  public function readI16(inout $i16);
  <<__Rx, __Mutable>>
  public function readI32(inout $i32);
  <<__Rx, __Mutable>>
  public function readI64(inout $i64);
  <<__Rx, __Mutable>>
  public function readDouble(inout $dub);
  <<__Rx, __Mutable>>
  public function readFloat(inout $flt);
  <<__Rx, __Mutable>>
  public function readString(inout $str);

  <<__Rx, __Mutable>>
  public function skip($type);
}
