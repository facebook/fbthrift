<?php
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
 *
 * @package thrift
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
abstract class TType {
  const STOP = 0;
  const VOID = 1;
  const BOOL = 2;
  const BYTE = 3;
  const I08 = 3;
  const DOUBLE = 4;
  const I16 = 6;
  const I32 = 8;
  const I64 = 10;
  const STRING = 11;
  const UTF7 = 11;
  const STRUCT = 12;
  const MAP = 13;
  const SET = 14;
  const LST = 15;
  const UTF8 = 16;
  const UTF16 = 17;
  const FLOAT = 19;
}
