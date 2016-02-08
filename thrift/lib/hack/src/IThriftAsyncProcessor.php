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

interface IThriftAsyncProcessor extends IThriftProcessor {
  public function processAsync(
    TProtocol $input,
    TProtocol $output,
  ): Awaitable<bool>;
}
