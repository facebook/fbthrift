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

<<__ConsistentConstruct>>
abstract class ThriftClientBase implements IThriftClient {
  protected TProtocol $input_;
  protected TProtocol $output_;
  protected TClientAsyncHandler $asyncHandler_;
  protected TClientEventHandler $eventHandler_;

  protected int $seqid_ = 0;

  final public static function factory(
  ): (string, (function (TProtocol, ?TProtocol): this)) {
    return tuple(
      get_called_class(),
      function(TProtocol $input, ?TProtocol $output) {
        return new static($input, $output);
      },
    );
  }

  public function __construct(TProtocol $input, ?TProtocol $output = null) {
    $this->input_ = $input;
    $this->output_ = $output ?: $input;
    $this->asyncHandler_ = new TClientAsyncHandler();
    $this->eventHandler_ = new TClientEventHandler();
  }

  public function setAsyncHandler(TClientAsyncHandler $async_handler): this {
    $this->asyncHandler_ = $async_handler;
    return $this;
  }

  public function getAsyncHandler(): TClientAsyncHandler {
    return $this->asyncHandler_;
  }

  public function setEventHandler(TClientEventHandler $event_handler): this {
    $this->eventHandler_ = $event_handler;
    return $this;
  }

  public function getEventHandler(): TClientEventHandler {
    return $this->eventHandler_;
  }

  protected function getNextSequenceID(): int {
    $currentseqid = $this->seqid_;
    if ($this->seqid_ >= 0x7fffffff) {
      $this->seqid_ = 0;
    } else {
      $this->seqid_++;
    }
    return $currentseqid;
  }
}
