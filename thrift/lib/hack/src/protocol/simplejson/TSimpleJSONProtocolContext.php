<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.simplejson
*/

class TSimpleJSONProtocolContext {
  final public function __construct(
    protected TTransport $trans,
    protected IThriftBufferedTransport $bufTrans,
  ) {}

  public function writeStart(): int {
    return 0;
  }

  public function writeSeparator(): int {
    return 0;
  }

  public function writeEnd(): int {
    return 0;
  }

  public function readStart(): void {
    // Do nothing
  }

  public function readSeparator(): void {
    // Do nothing
  }

  public function readContextOver(): bool {
    return true;
  }

  public function readEnd(): void {
    // Do nothing
  }

  public function escapeNum(): bool {
    return false;
  }

  protected function skipWhitespace(bool $skip = true): int {
    $count = 0;
    $reading = true;
    while ($reading) {
      $byte = $this->bufTrans->peek(1, $count);
      switch ($byte) {
        case ' ':
        case "\t":
        case "\n":
        case "\r":
          $count++;
          break;
        default:
          $reading = false;
          break;
      }
    }
    if ($skip) {
      $this->trans->readAll($count);
    }
    return $count;
  }
}
