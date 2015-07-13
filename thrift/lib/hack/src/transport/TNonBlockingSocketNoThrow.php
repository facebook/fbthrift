<?hh // strict

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
 * Wrapper class over TNonBlockingSocket that suppresses
 * the exception "operation in progress 115" when the open()
 * is called. This is helpful for classes that don't
 * handle properly this event. Ie. TFramedTransport.
 */
class TNonBlockingSocketNoThrow extends TNonBlockingSocket {
  public function open(): void {
    try {
      parent::open();
    } catch (Exception $e) {
      $op_in_progress =
        (strpos($e->getMessage(), "socket_connect error") !== false) &&
        (strpos($e->getMessage(), "[115]") !== false);
      if (!$op_in_progress) {
        throw $e;
      }
    }
  }
}
