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

class TSimpleJSONProtocolMapContext extends TSimpleJSONProtocolContext {
  private bool $first = true;
  private bool $colon = true;

  public function writeStart(): int {
    $this->trans->write('{');
    return 1;
  }

  public function writeSeparator(): int {
    if ($this->first) {
      $this->first = false;
      return 0;
    }

    if ($this->colon) {
      $this->trans->write(':');
    } else {
      $this->trans->write(',');
    }

    $this->colon = !$this->colon;
    return 1;
  }

  public function writeEnd(): int {
    $this->trans->write('}');
    return 1;
  }

  public function readStart(): void {
    $this->skipWhitespace();
    $c = $this->trans->readAll(1);
    if ($c !== '{') {
      throw new TProtocolException(
        'TSimpleJSONProtocol: Expected "{", encountered 0x'.bin2hex($c),
      );
    }
  }

  public function readSeparator(): void {
    if ($this->first) {
      $this->first = false;
      return;
    }

    $this->skipWhitespace();
    $c = $this->trans->readAll(1);
    $target = $this->colon ? ':' : ',';

    if ($c !== $target) {
      throw new TProtocolException(
        'TSimpleJSONProtocol: Expected "'.
        $target.
        '", encountered 0x'.
        bin2hex($c),
      );
    }

    $this->colon = !$this->colon;
  }

  public function readContextOver(): bool {
    $pos = $this->skipWhitespace(false);
    $c = $this->bufTrans->peek(1, $pos);
    if (!$this->first && $c !== ',' && $c !== '}') {
      throw new TProtocolException(
        'TSimpleJSONProtocol: Expected "," or "}", encountered 0x'.
        bin2hex($c),
      );
    }

    return ($c === '}');
  }

  public function readEnd(): void {
    $this->skipWhitespace();
    $c = $this->trans->readAll(1);
    if ($c !== '}') {
      throw new TProtocolException(
        'TSimpleJSONProtocol: Expected "}", encountered 0x'.bin2hex($c),
      );
    }
  }

  public function escapeNum(): bool {
    return $this->colon;
  }
}
