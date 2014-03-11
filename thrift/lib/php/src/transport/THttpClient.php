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

/**
 * HTTP client for Thrift
 *
 * @package thrift.transport
 */
class THttpClient extends TTransport implements IThriftRemoteConn {

  /**
   * The host to connect to
   *
   * @var string
   */
  protected $host_;

  /**
   * The port to connect on
   *
   * @var int
   */
  protected $port_;

  /**
   * The URI to request
   *
   * @var string
   */
  protected $uri_;

  /**
   * The scheme to use for the request, i.e. http, https
   *
   * @var string
   */
  protected $scheme_;

  /**
   * Buffer for the HTTP request data
   *
   * @var string
   */
  protected $buf_;

  /**
   * Input data stream.
   *
   * @var resource
   */
  protected $data_;

  /**
   * Error message
   *
   * @var string
   */
  protected $errstr_;

  /**
   * Read timeout
   *
   * @var float
   */
  protected $timeout_;

  /**
   * Custom HTTP headers
   *
   * @var array
   */
  protected $custom_headers_;

  /**
   * Debugging on?
   *
   * @var bool
   */
  protected $debug_ = false;

  /**
   * Debug handler
   *
   * @var mixed
   */
  protected $debugHandler_ = null;

  /**
   * Specifies the maximum number of bytes to read
   * at once from internal stream.
   */
  protected $maxReadChunkSize_ = null;

  /**
   * Make a new HTTP client.
   *
   * @param string $host
   * @param int    $port
   * @param string $uri
   */
  public function __construct($host, $port=80, $uri='', $scheme = 'http',
                              $debugHandler = null) {
    if ($uri && ($uri[0] != '/')) {
      $uri = '/'.$uri;
    }
    $this->scheme_ = $scheme;
    $this->host_ = $host;
    $this->port_ = $port;
    $this->uri_ = $uri;
    $this->buf_ = '';
    $this->data_ = '';
    $this->errstr_ = '';
    $this->timeout_ = null;
    $this->custom_headers_ = null;
    $this->debugHandler_ = $debugHandler ?: 'error_log';
  }

  /**
   * Sets the internal max read chunk size.
   * null for no limit (default).
   */
  public function setMaxReadChunkSize($maxReadChunkSize) {
    $this->maxReadChunkSize_ = $maxReadChunkSize;
  }

  /**
   * Set read timeout
   *
   * @param float $timeout
   */
  public function setTimeoutSecs($timeout) {
    $this->timeout_ = $timeout;
  }

  /**
   * Set custom headers
   *
   * @param array of key value pairs of custom headers
   */
  public function setCustomHeaders($custom_headers) {
    $this->custom_headers_ = $custom_headers;
  }

  /**
   * Get custom headers
   *
   * @return array key value pairs of custom headers
   */
  public function getCustomHeaders() {
    return $this->custom_headers_;
  }

  /**
   * Get the remote host that this transport is connected to.
   *
   * @return string host
   */
  public function getHost() {
    return $this->host_;
  }

  /**
   * Get the remote port that this transport is connected to.
   *
   * @return int port
   */
  public function getPort() {
    return $this->port_;
  }

  /**
   * Get the scheme used to connect to the remote server.
   *
   * @return string scheme
   */
  public function getScheme() {
    return $this->scheme_;
  }

  /**
   * Sets debugging output on or off
   *
   * @param bool $debug
   */
  public function setDebug($debug) {
    $this->debug_ = $debug;
  }

  /**
   * Whether this transport is open.
   *
   * @return boolean true if open
   */
  public function isOpen() {
    return true;
  }

  /**
   * Open the transport for reading/writing
   *
   * @throws TTransportException if cannot open
   */
  public function open() {}

  /**
   * Close the transport.
   */
  public function close() {
    $this->data_ = '';
  }

  /**
   * Read some data into the array.
   *
   * @param int    $len How much to read
   * @return string The data that has been read
   * @throws TTransportException if cannot read any more data
   */
  public function read($len) {
    if ($this->maxReadChunkSize_ !== null) {
      $len = min($len, $this->maxReadChunkSize_);
    }

    $t_start = microtime(true);
    if (strlen($this->data_) < $len) {
      $data = $this->data_;
      $this->data_ = '';
    } else {
      $data = substr($this->data_, 0, $len);
      $this->data_ = substr($this->data_, $len);
    }
    $t_delta = microtime(true) - $t_start;
    ProfilingCounters::incrCount(IProfilingCounters::THRIFT_READ_COUNT);
    ProfilingCounters::incrCount(
      IProfilingCounters::THRIFT_READ_BYTES, strlen($data));
    ProfilingCounters::incrDuration(
      IProfilingCounters::THRIFT_READ_DURATION, $t_delta);

    if ($data === FALSE || $data === '') {
      throw new TTransportException('THttpClient: Could not read '.$len.
                                    ' bytes from '.$this->host_.
                                    ':'.$this->port_.'/'.$this->uri_.
                                    " Reason: ".$this->errstr_,
                                    TTransportException::UNKNOWN);
    }
    return $data;
  }

  /**
   * Writes some data into the pending buffer
   *
   * @param string $buf  The data to write
   * @throws TTransportException if writing fails
   */
  public function write($buf) {
    $this->buf_ .= $buf;
  }

  /**
   * Opens and sends the actual request over the HTTP connection
   *
   * @throws TTransportException if a writing error occurs
   */
  public function flush() {
    // God, PHP really has some esoteric ways of doing simple things.
    $host = $this->host_.($this->port_ != 80 ? ':'.$this->port_ : '');
    $user_agent = 'PHP/THttpClient';
    $script = urlencode(basename($_SERVER['SCRIPT_FILENAME']));
    if ($script) {
      $user_agent .= ' ('.$script.')';
    }

    $curl = curl_init($this->scheme_.'://'.$host.$this->uri_);

    $headers = array('Host: '.$host,
                     'Accept: application/x-thrift',
                     'User-Agent: '.$user_agent,
                     'Content-Type: application/x-thrift',
                     'Content-Length: '.strlen($this->buf_));


    if (is_array($this->custom_headers_)) {
      foreach ($this->custom_headers_ as $header => $value) {
        $headers[] = $header.': '.$value;
      }
    }
    curl_setopt($curl, CURLOPT_PROXY, '');
    curl_setopt($curl, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($curl, CURLOPT_POST, true);
    curl_setopt($curl, CURLOPT_MAXREDIRS, 1);
    curl_setopt($curl, CURLOPT_FOLLOWLOCATION, true);
    curl_setopt($curl, CURLOPT_POSTFIELDS, $this->buf_);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($curl, CURLOPT_BINARYTRANSFER, true);

    curl_setopt($curl, CURLOPT_TIMEOUT,
                $this->timeout_);
    $this->buf_ = '';

    $t_start = microtime(true);
    $this->data_ = curl_exec($curl);

    $t_delta = microtime(true) - $t_start;
    ProfilingCounters::incrCount(IProfilingCounters::THRIFT_FLUSH_COUNT);
    ProfilingCounters::incrDuration(
      IProfilingCounters::THRIFT_FLUSH_DURATION, $t_delta);

    $this->errstr_ = curl_error($curl);
    curl_close($curl);

    // Connect failed?
    if ($this->data_ === false) {
      $error = 'THttpClient: Could not connect to '.$host.$this->uri_;
      throw new TTransportException($error, TTransportException::NOT_OPEN);
    }
  }

  public function isReadable() {
    return (bool) $this->data_;
  }

  public function isWritable() {
    return true;
  }

  public function getRecvTimeout() { return 0; }
  public function setRecvTimeout($timeout) { /* do nothing */ }
}
