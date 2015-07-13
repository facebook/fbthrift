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

require_once ($GLOBALS['HACKLIB_ROOT']);
class TProcessorEventHandler {
  public function getHandlerContext($fn_name) {
    return null;
  }
  public function preRead($handler_context, $fn_name, $args) {}
  public function postRead($handler_context, $fn_name, $args) {}
  public function preExec($handler_context, $fn_name, $args) {}
  public function postExec($handler_context, $fn_name, $result) {}
  public function preWrite($handler_context, $fn_name, $result) {}
  public function postWrite($handler_context, $fn_name, $result) {}
  public function handlerException($handler_context, $fn_name, $exception) {}
  public function handlerError($handler_context, $fn_name, $exception) {}
}
