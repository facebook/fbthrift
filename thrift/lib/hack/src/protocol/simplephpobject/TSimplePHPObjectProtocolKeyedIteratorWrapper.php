<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.simplephpobject
*/

class TSimplePHPObjectProtocolKeyedIteratorWrapper
  implements Iterator<mixed> {
  private bool $key = true;

  public function __construct(private Iterator<mixed> $itr) {}

  public function key(): string {
    invariant_violation('Cannot Access Key');
  }

  public function current(): mixed {
    if ($this->key) {
      // UNSAFE_BLOCK
      return $this->itr->key();
    } else {
      return $this->itr->current();
    }
  }

  public function next(): void {
    $this->key = !$this->key;
    if ($this->key) {
      $this->itr->next();
    }
  }

  public function rewind(): void {
    $this->key = !$this->key;
    if (!$this->key) {
      $this->itr->rewind();
    }
  }

  public function valid(): bool {
    return $this->itr->valid();
  }
}
