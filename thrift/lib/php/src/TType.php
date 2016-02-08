<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
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
