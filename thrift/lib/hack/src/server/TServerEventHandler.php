<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.server
*/

/**
 * Event handler base class.  Override selected methods on this class
 * to implement custom event handling
 */
abstract class TServerEventHandler {

  /**
   * Called before a new client request is handled
   */
  public abstract function clientBegin(TProtocol $prot): void;
}
