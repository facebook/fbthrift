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
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__;
}
require_once $GLOBALS['THRIFT_ROOT'].'/TClientEventHandler.php';
final class TClientMultiEventHandler extends TClientEventHandler {
  private $handlers;
  public function __construct() {
    $this->handlers = \HH\Map::hacklib_new(array(), array());
  }
  public function addHandler($key, $handler) {
    $this->handlers[$key] = $handler;
    return $this;
  }
  public function getHandler($key) {
    return $this->handlers[$key];
  }
  public function removeHandler($key) {
    $handler = $this->getHandler($key);
    $this->handlers->remove($key);
    return $handler;
  }
  public function preSend($fn_name, $args, $sequence_id) {
    foreach ($this->handlers as $handler) {
      $handler->preSend($fn_name, $args, $sequence_id);
    }
  }
  public function postSend($fn_name, $args, $sequence_id) {
    foreach ($this->handlers as $handler) {
      $handler->postSend($fn_name, $args, $sequence_id);
    }
  }
  public function sendError($fn_name, $args, $sequence_id, $ex) {
    foreach ($this->handlers as $handler) {
      $handler->sendError($fn_name, $args, $sequence_id, $ex);
    }
  }
  public function preRecv($fn_name, $ex_sequence_id) {
    foreach ($this->handlers as $handler) {
      $handler->preRecv($fn_name, $ex_sequence_id);
    }
  }
  public function postRecv($fn_name, $ex_sequence_id, $result) {
    foreach ($this->handlers as $handler) {
      $handler->postRecv($fn_name, $ex_sequence_id, $result);
    }
  }
  public function recvException($fn_name, $ex_sequence_id, $exception) {
    foreach ($this->handlers as $handler) {
      $handler->recvException($fn_name, $ex_sequence_id, $exception);
    }
  }
  public function recvError($fn_name, $ex_sequence_id, $exception) {
    foreach ($this->handlers as $handler) {
      $handler->recvError($fn_name, $ex_sequence_id, $exception);
    }
  }
}
