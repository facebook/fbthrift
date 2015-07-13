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
 * Base interface for a server.
 *
 *     The attribute serverEventHandler (default: null) receives
 *     callbacks for various events in the server lifecycle.  It should
 *     be set to an instance of TServerEventHandler.
 */
abstract class TServer {
  protected IThriftProcessor $processor;
  protected TServerSocket $serverTransport;
  protected TTransportFactory $transportFactory;
  protected TProtocolFactory $protocolFactory;
  protected ?TServerEventHandler $serverEventHandler;

  public function __construct(
    IThriftProcessor $processor,
    TServerSocket $serverTransport,
    TTransportFactory $transportFactory,
    TProtocolFactory $protocolFactory,
  ) {
    $this->processor = $processor;
    $this->serverTransport = $serverTransport;
    $this->transportFactory = $transportFactory;
    $this->protocolFactory = $protocolFactory;

    $this->serverEventHandler = null;
  }

  protected function _clientBegin(TProtocol $prot): void {
    if ($this->serverEventHandler) {
      $this->serverEventHandler->clientBegin($prot);
    }
  }

  protected function handle(TBufferedTransport $client): bool {
    $trans = $this->transportFactory->getTransport($client);
    $prot = $this->protocolFactory->getProtocol($trans);

    $this->_clientBegin($prot);
    try {
      $this->processor->process($prot, $prot);
    } catch (TTransportException $tx) {
      // ignore
      return false;
    } catch (Exception $x) {
      echo 'Handle caught transport exception: '.$x->getMessage()."\n";
      return false;
    }

    $trans->close();
    return true;
  }

  public abstract function serve(): void;
}
