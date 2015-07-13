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
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__;
}
require_once $GLOBALS['THRIFT_ROOT'].'/TProcessorEventHandler.php';
final class TProcessorMultiEventHandler extends TProcessorEventHandler {
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
  public function getHandlerContext($fn_name) {
    $context = \HH\Map::hacklib_new(array(), array());
    foreach ($this->handlers as $key => $handler) {
      $context[$key] = $handler->getHandlerContext($fn_name);
    }
    return $context;
  }
  public function preRead($handler_context, $fn_name, $args) {
    \HH\invariant(
      $handler_context instanceof \HH\Map,
      'Context is not a Map'
    );
    foreach ($this->handlers as $key => $handler) {
      $handler->preRead($handler_context->at($key), $fn_name, $args);
    }
  }
  public function postRead($handler_context, $fn_name, $args) {
    \HH\invariant(
      $handler_context instanceof \HH\Map,
      'Context is not a Map'
    );
    foreach ($this->handlers as $key => $handler) {
      $handler->postRead($handler_context->at($key), $fn_name, $args);
    }
  }
  public function preExec($handler_context, $fn_name, $args) {
    \HH\invariant(
      $handler_context instanceof \HH\Map,
      'Context is not a Map'
    );
    foreach ($this->handlers as $key => $handler) {
      $handler->preExec($handler_context->at($key), $fn_name, $args);
    }
  }
  public function postExec($handler_context, $fn_name, $result) {
    \HH\invariant(
      $handler_context instanceof \HH\Map,
      'Context is not a Map'
    );
    foreach ($this->handlers as $key => $handler) {
      $handler->postExec($handler_context->at($key), $fn_name, $result);
    }
  }
  public function preWrite($handler_context, $fn_name, $result) {
    \HH\invariant(
      $handler_context instanceof \HH\Map,
      'Context is not a Map'
    );
    foreach ($this->handlers as $key => $handler) {
      $handler->preWrite($handler_context->at($key), $fn_name, $result);
    }
  }
  public function postWrite($handler_context, $fn_name, $result) {
    \HH\invariant(
      $handler_context instanceof \HH\Map,
      'Context is not a Map'
    );
    foreach ($this->handlers as $key => $handler) {
      $handler->postWrite($handler_context->at($key), $fn_name, $result);
    }
  }
  public function handlerException($handler_context, $fn_name, $ex) {
    \HH\invariant(
      $handler_context instanceof \HH\Map,
      'Context is not a Map'
    );
    foreach ($this->handlers as $key => $handler) {
      $handler->handlerException($handler_context->at($key), $fn_name, $ex);
    }
  }
  public function handlerError($handler_context, $fn_name, $ex) {
    \HH\invariant(
      $handler_context instanceof \HH\Map,
      'Context is not a Map'
    );
    foreach ($this->handlers as $key => $handler) {
      $handler->handlerError($handler_context->at($key), $fn_name, $ex);
    }
  }
}
