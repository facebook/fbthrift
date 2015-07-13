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
 * Event handler for thrift clients
 */
class TClientEventHandler {
  public function setClient(IThriftClient $client): void {}
  // Called before the request is sent to the server
  public function preSend(
    string $fn_name,
    mixed $args,
    int $sequence_id,
  ): void {}

  // Called after the request is sent to the server
  public function postSend(
    string $fn_name,
    mixed $args,
    int $sequence_id,
  ): void {}

  // Called if there is an exception writing the data
  public function sendError(
    string $fn_name,
    mixed $args,
    int $sequence_id,
    Exception $ex,
  ): void {}

  // Called before the response is read from the server
  // Exceptions thrown here are caught by clientException and clientError.
  public function preRecv(string $fn_name, ?int $ex_sequence_id): void {}

  // Called after the response is read from the server
  public function postRecv(
    string $fn_name,
    ?int $ex_sequence_id,
    mixed $result,
  ): void {}

  // Called if (and only if) the client threw an expected $exception.
  public function recvException(
    string $fn_name,
    ?int $ex_sequence_id,
    TException $exception,
  ): void {}

  // Called if (and only if) the client threw an unexpected $exception.
  public function recvError(
    string $fn_name,
    ?int $ex_sequence_id,
    Exception $exception,
  ): void {}
}
