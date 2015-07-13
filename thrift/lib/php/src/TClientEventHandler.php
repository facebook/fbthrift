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
class TClientEventHandler {
  public function setClient($client) {}
  public function preSend($fn_name, $args, $sequence_id) {}
  public function postSend($fn_name, $args, $sequence_id) {}
  public function sendError($fn_name, $args, $sequence_id, $ex) {}
  public function preRecv($fn_name, $ex_sequence_id) {}
  public function postRecv($fn_name, $ex_sequence_id, $result) {}
  public function recvException($fn_name, $ex_sequence_id, $exception) {}
  public function recvError($fn_name, $ex_sequence_id, $exception) {}
}
