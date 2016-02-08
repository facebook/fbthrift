<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.transport
*/

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftRemoteConn.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
class THttpClient extends TTransport implements IThriftRemoteConn {
  protected $host_;
  protected $port_;
  protected $uri_;
  protected $scheme_;
  protected $buf_;
  protected $data_;
  protected $errstr_;
  protected $timeout_;
  protected $custom_headers_;
  protected $debug_ = false;
  protected $debugHandler_;
  protected $maxReadChunkSize_ = null;
  protected $caInfo_;
  public function __construct(
    $host,
    $port = 80,
    $uri = "",
    $scheme = "http",
    $debugHandler = null
  ) {
    if (\hacklib_cast_as_boolean($uri) && \hacklib_not_equals($uri[0], "/")) {
      $uri = "/".$uri;
    }
    $this->scheme_ = $scheme;
    $this->host_ = $host;
    $this->port_ = $port;
    $this->uri_ = $uri;
    $this->buf_ = "";
    $this->data_ = "";
    $this->errstr_ = "";
    $this->timeout_ = null;
    $this->custom_headers_ = null;
    $this->debugHandler_ =
      \hacklib_cast_as_boolean($debugHandler) ?: fun("error_log");
  }
  public function setMaxReadChunkSize($maxReadChunkSize) {
    $this->maxReadChunkSize_ = $maxReadChunkSize;
  }
  public function setCAInfo($path_to_ca_bundle) {
    $this->caInfo_ = $path_to_ca_bundle;
  }
  public function setTimeoutSecs($timeout) {
    $this->timeout_ = $timeout;
  }
  public function setCustomHeaders($custom_headers) {
    $this->custom_headers_ = $custom_headers;
  }
  public function getCustomHeaders() {
    return $this->custom_headers_;
  }
  public function getHost() {
    return $this->host_;
  }
  public function getPort() {
    return $this->port_;
  }
  public function getScheme() {
    return $this->scheme_;
  }
  public function setDebug($debug) {
    $this->debug_ = $debug;
  }
  public function isOpen() {
    return true;
  }
  public function open() {}
  public function close() {
    $this->data_ = "";
  }
  public function read($len) {
    if ($this->maxReadChunkSize_ !== null) {
      $len = min($len, $this->maxReadChunkSize_);
    }
    if (strlen($this->data_) < $len) {
      $data = $this->data_;
      $this->data_ = "";
    } else {
      $data = substr($this->data_, 0, $len);
      $this->data_ = substr($this->data_, $len);
    }
    if (($data === false) || ($data === "")) {
      throw new TTransportException(
        "THttpClient: Could not read ".
        $len.
        " bytes from ".
        $this->host_.
        ":".
        $this->port_.
        "/".
        $this->uri_.
        " Reason: ".
        $this->errstr_,
        TTransportException::UNKNOWN
      );
    }
    return $data;
  }
  public function write($buf) {
    $this->buf_ .= $buf;
  }
  public function flush() {
    $host = $this->host_.":".$this->port_;
    $user_agent = "PHP/THttpClient";
    $script =
      (string) urlencode(basename($_SERVER[\hacklib_id("SCRIPT_FILENAME")]));
    if (\hacklib_cast_as_boolean($script)) {
      $user_agent .= " (".$script.")";
    }
    $curl = curl_init($this->scheme_."://".$host.$this->uri_);
    $headers = array(
      "Host: ".$host,
      "Accept: application/x-thrift",
      "User-Agent: ".$user_agent,
      "Content-Type: application/x-thrift",
      "Content-Length: ".((string) strlen($this->buf_))
    );
    if ($this->custom_headers_ !== null) {
      foreach ($this->custom_headers_ as $header => $value) {
        $headers[] = $header.": ".$value;
      }
    }
    curl_setopt($curl, CURLOPT_PROXY, "");
    curl_setopt($curl, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($curl, CURLOPT_POST, true);
    curl_setopt($curl, CURLOPT_MAXREDIRS, 1);
    curl_setopt($curl, CURLOPT_FOLLOWLOCATION, true);
    curl_setopt($curl, CURLOPT_POSTFIELDS, $this->buf_);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($curl, CURLOPT_BINARYTRANSFER, true);
    curl_setopt($curl, CURLOPT_TIMEOUT, $this->timeout_);
    if ($this->caInfo_ !== null) {
      curl_setopt($curl, CURLOPT_CAINFO, $this->caInfo_);
    }
    $this->buf_ = "";
    $this->data_ = curl_exec($curl);
    $this->errstr_ = curl_error($curl);
    curl_close($curl);
    if ($this->data_ === false) {
      $error = "THttpClient: Could not connect to ".$host.$this->uri_;
      throw new TTransportException($error, TTransportException::NOT_OPEN);
    }
  }
  public function isReadable() {
    return (bool) \hacklib_cast_as_boolean($this->data_);
  }
  public function isWritable() {
    return true;
  }
  public function getRecvTimeout() {
    return 0;
  }
  public function setRecvTimeout($timeout) {}
}
