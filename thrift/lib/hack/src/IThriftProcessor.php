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
 * Base interface for Thrift processors
 */
interface IThriftProcessor {
  public function getEventHandler(): TProcessorEventHandler;
  public function setEventHandler(TProcessorEventHandler $handler): this;
  public function process(TProtocol $input, TProtocol $output): bool;
}
