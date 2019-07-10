<?hh

/*
 * Copyright 2006-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * Server that can run in non-blocking mode
 */
class TNonBlockingServer extends TServer {
  protected int $clientIdx = 0;
  protected darray<int, TBufferedTransport> $clients = darray[];

  public function __construct(
    IThriftProcessor $processor,
    TServerSocket $serverTransport,
    TTransportFactory $transportFactory,
    TProtocolFactory $protocolFactory,
  ) {
    parent::__construct(
      $processor,
      $serverTransport,
      $transportFactory,
      $protocolFactory,
    );
  }

  /**
   * Because our server is non-blocking, don't close this socket
   * until we need to.
   *
   * @return bool true if we should keep the client alive
   */
  <<__Override>>
  protected function handle(TBufferedTransport $client): bool {
    $trans = $this->transportFactory->getTransport($client);
    $prot = $this->protocolFactory->getProtocol($trans);

    $this->_clientBegin($prot);
    try {
      // First check the transport is readable to avoid
      // blocking on read
      if (!($trans is TTransportStatus) || $trans->isReadable()) {
        $this->processor->process($prot, $prot);
      }
    } catch (Exception $x) {
      $md = $client->getMetaData();
      if (PHPism_FIXME::nullableTruthyCheck($md['timed_out'])) {
        // keep waiting for the client to send more requests
      } else if (PHPism_FIXME::nullableTruthyCheck($md['eof'])) {
        invariant($trans is TTransport, 'Need to make Hack happy');
        $trans->close();
        return false;
      } else {
        echo 'Handle caught transport exception: '.$x->getMessage()."\n";
      }
    }
    return true;
  }

  protected function processExistingClients(): void {
    foreach ($this->clients as $i => $client) {
      if (!$this->handle($client)) {
        // remove the client from our list of open clients if
        // our handler reports that the client is no longer alive
        unset($this->clients[$i]);
      }
    }
  }

  /*
   * This method should be called repeately on idle to listen and
   * process an request. If there is no pending request, it will
   * return;
   */
  <<__Override>>
  public function serve(): void {
    $this->serverTransport->listen();
    $this->process();
  }

  public function process(): void {
    // 0 timeout is non-blocking
    $client = $this->serverTransport->accept(0);
    if ($client) {
      $this->clients[$this->clientIdx] = $client;
      $this->clientIdx++;
    }

    $this->processExistingClients();
  }
}
