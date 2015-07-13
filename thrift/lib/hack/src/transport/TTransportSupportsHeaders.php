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

interface TTransportSupportsHeaders {
  public function setHeader(string $str_key, @string $str_value): this;
  public function setPersistentHeader(
    string $str_key,
    @string $str_value,
  ): this;
  public function getWriteHeaders(): Indexish<string, string>;
  public function getPersistentWriteHeaders(): Indexish<string, string>;
  public function getHeaders(): Indexish<string, string>;
  public function clearHeaders(): void;
  public function clearPersistentHeaders(): void;
}
