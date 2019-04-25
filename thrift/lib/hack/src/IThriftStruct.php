<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift
*/

/**
 * Base interface for Thrift structs
 */
<<__ConsistentConstruct>>
interface IThriftStruct {

  const type TFieldSpec = shape(
    'var' => string,
    'type' => TType,
    ?'union' => bool,
    ?'etype' => TType,
    ?'elem' => this::TElemSpec,
    ?'ktype' => TType,
    ?'vtype' => TType,
    ?'key' => this::TElemSpec,
    ?'val' => this::TElemSpec,
    ?'format' => string,
    ?'class' => string,
    ?'enum' => string,
  );
  const type TElemSpec = shape(
    'type' => TType,
    ?'etype' => TType,
    ?'elem' => mixed, // this::TElemSpec once hack supports recursive types
    ?'ktype' => TType,
    ?'vtype' => TType,
    ?'key' => mixed, // this::TElemSpec once hack supports recursive types
    ?'val' => mixed, // this::TElemSpec once hack supports recursive types
    ?'format' => string,
    ?'class' => string,
    ?'enum' => string,
  );

  abstract const dict<int, this::TFieldSpec> SPEC;
  abstract const dict<string, int> FIELDMAP;
  abstract const int STRUCTURAL_ID;

  <<__Rx>>
  public function __construct();
  public function getName(): string;
  public function read(TProtocol $input): int;
  public function write(TProtocol $input): int;
}
