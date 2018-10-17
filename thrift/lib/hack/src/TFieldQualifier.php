<?hh // strict

/**
* Copyright (c) 2018- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift
*/

/**
 * Enum representing allowed qualifiers in thrift field
 * This matched the definition in `e_req` (thrift/compiler/ast/t_field.h)
 */
enum TFieldQualifier: int {
  T_REQUIRED = 0;
  T_OPTIONAL = 1;
  T_OPT_IN_REQ_OUT = 2;
}
