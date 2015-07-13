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
 * Base interface for Thrift clients
 */
interface IThriftClient {
  public function getEventHandler(): TClientEventHandler;
  public function setEventHandler(TClientEventHandler $handler): this;
  public function getAsyncHandler(): TClientAsyncHandler;
  public function setAsyncHandler(TClientAsyncHandler $handler): this;
}
