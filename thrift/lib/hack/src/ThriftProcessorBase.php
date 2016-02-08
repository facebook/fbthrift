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

abstract class ThriftProcessorBase implements IThriftProcessor {
  abstract const type TThriftIf as IThriftIf;
  protected TProcessorEventHandler $eventHandler_;

  // This exists so subclasses still using php can still access the handler
  // Once the migration to hack is complete, this field can be removed safely
  protected mixed $handler_;

  public function __construct(
    protected this::TThriftIf $handler,
  ) {
    $this->eventHandler_ = new TProcessorEventHandler();
    $this->handler_ = $handler;
  }

  public function getHandler(): this::TThriftIf {
    return $this->handler;
  }

  public function setEventHandler(TProcessorEventHandler $event_handler): this {
    $this->eventHandler_ = $event_handler;
    return $this;
  }

  public function getEventHandler(): TProcessorEventHandler {
    return $this->eventHandler_;
  }
}
