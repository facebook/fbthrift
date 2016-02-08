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
class THandlerShortCircuitException extends Exception {
  const R_SUCCESS = 0;
  const R_EXPECTED_EX = 1;
  const R_UNEXPECTED_EX = 2;
  public $result;
  public $resultType;
  public function __construct($result, $resultType) {
    $this->result = $result;
    $this->resultType = $resultType;
    parent::__construct();
  }
}
