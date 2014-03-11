<?php
// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * Copyright (c) 2006- Facebook
 * Distributed under the Thrift Software License
 *
 * See accompanying file LICENSE or visit the Thrift site at:
 * http://developers.facebook.com/thrift/
 *
 * @package thrift.transport
 */

/** Inherits from THttpClient */

/**
 * Multi-IP HTTP client for Thrift. The client randomly selects and uses a host
 * from its pool.
 *
 * @package thrift.transport
 */
class THttpClientPool extends THttpClient {

  /**
   * Servers belonging to this pool. Array of associative arrays with 'host' and
   * 'port' keys.
   */
  protected $servers_ = array();

  /**
   * Try hosts in order? or randomized?
   *
   * @var bool
   */
  private $randomize_ = true;

  /**
   * The number of times to try to connect to a particular endpoint.
   *
   * @var int
   */
  private $numTries_ = 1;

  /**
   * Make a new multi-IP HTTP client.
   *
   * @param array  $hosts   List of remote hostnames or IPs.
   * @param mixed  $ports   Array of remote ports, or a single
   *                        common port.
   * @param string $uri     URI of the endpoint.
   * @param string $scheme  http or https
   */
  public function __construct($hosts, $ports, $uri='',
                              $scheme='http', $debugHandler = null) {
    parent::__construct(null, null, $uri, $scheme, $debugHandler);

    if (!is_array($ports)) {
      $port = $ports;
      $ports = array();
      foreach ($hosts as $key => $val) {
        $ports[$key] = $port;
      }
    }

    foreach ($hosts as $key => $host) {
      $this->servers_[] = array('host' => $host, 'port' => $ports[$key]);
    }
  }

  /**
   * Add a server to the pool. This function does not prevent you from adding a
   * duplicate server entry.
   *
   * @param string $host  hostname or IP
   * @param int    $port  port
   */
  public function addServer($host, $port) {
    $this->servers_[] = array('host' => $host, 'port' => $port);
  }

  /**
   * Turns random endpoint selection on or off.
   *
   * @param bool $randomize
   */
  public function setRandomize($randomize) {
    $this->randomize_ = $randomize;
  }

  /**
   * Sets how many times to try to connect to an endpoint before giving up on
   * it.
   *
   * @param int $numTries
   */
  public function setNumTries($numTries) {
    $this->numTries_ = $numTries;
  }

  /**
   * Trys to open a connection and send the actual HTTP request to the first
   * available server in the pool.
   */
  public function flush() {
    if ($this->randomize_) {
      shuffle($this->servers_);
    }

    foreach ($this->servers_ as $server) {
      $this->host_ = $server['host'];
      $this->port_ = $server['port'];

      $j = $this->numTries_;
      while ($j > 0) {
        try {
          parent::flush();
          return;
        } catch (TTransportException $e) {
          if ($this->debug_) {
            call_user_func($this->debugHandler_, $e->getMessage());
          }
          --$j;
        }
      }
    }

    $this->host_ = null;
    $this->port_ = null;
    $error = 'THttpClientPool: Could not connect to any of the servers '.
      'in the pool';
    throw new TTransportException($error, TTransportException::NOT_OPEN);
  }
}
