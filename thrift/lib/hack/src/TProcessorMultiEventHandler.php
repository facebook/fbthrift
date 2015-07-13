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

final class TProcessorMultiEventHandler extends TProcessorEventHandler {
  private Map<string, TProcessorEventHandler> $handlers;

  public function __construct() {
    $this->handlers = Map {};
  }

  public function addHandler(
    string $key,
    TProcessorEventHandler $handler,
  ): this {
    $this->handlers[$key] = $handler;
    return $this;
  }

  public function getHandler(string $key): TProcessorEventHandler {
    return $this->handlers[$key];
  }

  public function removeHandler(string $key): TProcessorEventHandler {
    $handler = $this->getHandler($key);
    $this->handlers->remove($key);
    return $handler;
  }

  // Called at the start of processing a handler method
  public function getHandlerContext(string $fn_name): mixed {
    $context = Map {};
    foreach ($this->handlers as $key => $handler) {
      $context[$key] = $handler->getHandlerContext($fn_name);
    }

    return $context;
  }

  // Called before the handler method's argument are read
  public function preRead(
    mixed $handler_context,
    string $fn_name,
    mixed $args,
  ): void {
    invariant($handler_context instanceof Map, 'Context is not a Map');
    foreach ($this->handlers as $key => $handler) {
      $handler->preRead($handler_context->at($key), $fn_name, $args);
    }
  }

  // Called after the handler method's argument are read
  public function postRead(
    mixed $handler_context,
    string $fn_name,
    mixed $args,
  ): void {
    invariant($handler_context instanceof Map, 'Context is not a Map');
    foreach ($this->handlers as $key => $handler) {
      $handler->postRead($handler_context->at($key), $fn_name, $args);
    }
  }

  // Called right before the handler is executed.
  // Exceptions thrown here are caught by handlerException and handlerError.
  public function preExec(
    mixed $handler_context,
    string $fn_name,
    mixed $args,
  ): void {
    invariant($handler_context instanceof Map, 'Context is not a Map');
    foreach ($this->handlers as $key => $handler) {
      $handler->preExec($handler_context->at($key), $fn_name, $args);
    }
  }

  // Called right after the handler is executed.
  // Exceptions thrown here are caught by handlerException and handlerError.
  public function postExec(
    mixed $handler_context,
    string $fn_name,
    mixed $result,
  ): void {
    invariant($handler_context instanceof Map, 'Context is not a Map');
    foreach ($this->handlers as $key => $handler) {
      $handler->postExec($handler_context->at($key), $fn_name, $result);
    }
  }

  // Called before the handler method's $results are written
  public function preWrite(
    mixed $handler_context,
    string $fn_name,
    mixed $result,
  ): void {
    invariant($handler_context instanceof Map, 'Context is not a Map');
    foreach ($this->handlers as $key => $handler) {
      $handler->preWrite($handler_context->at($key), $fn_name, $result);
    }
  }

  // Called after the handler method's $results are written
  public function postWrite(
    mixed $handler_context,
    string $fn_name,
    mixed $result,
  ): void {
    invariant($handler_context instanceof Map, 'Context is not a Map');
    foreach ($this->handlers as $key => $handler) {
      $handler->postWrite($handler_context->at($key), $fn_name, $result);
    }
  }

  // Called if (and only if) the handler threw an expected $exception.
  public function handlerException(
    mixed $handler_context,
    string $fn_name,
    Exception $ex,
  ): void {
    invariant($handler_context instanceof Map, 'Context is not a Map');
    foreach ($this->handlers as $key => $handler) {
      $handler->handlerException($handler_context->at($key), $fn_name, $ex);
    }
  }

  /**
   * Called if (and only if) the handler threw an unexpected $exception.
   *
   * Note that this method is NOT called if the handler threw an
   * exception that is declared in the thrift service specification
   */
  public function handlerError(
    mixed $handler_context,
    string $fn_name,
    Exception $ex,
  ): void {
    invariant($handler_context instanceof Map, 'Context is not a Map');
    foreach ($this->handlers as $key => $handler) {
      $handler->handlerError($handler_context->at($key), $fn_name, $ex);
    }
  }
}
