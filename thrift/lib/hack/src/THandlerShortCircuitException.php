<?hh

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift
*/

class THandlerShortCircuitException extends Exception {
  const int R_SUCCESS = 0;
  const int R_EXPECTED_EX = 1;
  const int R_UNEXPECTED_EX = 2;

  public function __construct(public $result, public int $resultType) {
    parent::__construct();
  }
}
