<?hh

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.transport
*/

/**
 * Transport exceptions
 *
 * Types deriving from TTransport exception may not be able to
 * translate their custom error into the set of error code
 * supported by TTransportException. For that the $shortMessage
 * facility is provided.
 *
 * @param mixed  $message Message (string) or type-spec (array)
 * @param mixed  $code Code (integer) or values (array)
 * @param string $shortMessage (string)
 */
class TTransportException extends TException {

  const UNKNOWN = 0;
  const NOT_OPEN = 1;
  const ALREADY_OPEN = 2;
  const TIMED_OUT = 3;
  const END_OF_FILE = 4;
  const INVALID_CLIENT = 5;
  const INVALID_FRAME_SIZE = 6;
  const INVALID_TRANSFORM = 7;
  const COULD_NOT_CONNECT = 8;
  const COULD_NOT_READ = 9;
  const COULD_NOT_WRITE = 10;

  protected string $shortMessage;

  public function __construct(
    ?string $message = null,
    int $code = 0,
    string $short_message = '',
  ) {
    $this->shortMessage = $short_message;
    parent::__construct($message, $code);
  }

  public function getShortMessage(): string {
    return $this->shortMessage;
  }
}
