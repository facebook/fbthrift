<?php
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @package thrift.transport
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TApplicationException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/binary/TBinaryProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TFramedTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportSupportsHeaders.php';
class THeaderTransport extends TFramedTransport
  implements TTransportSupportsHeaders {
  const TBINARY_PROTOCOL = 0;
  const TJSON_PROTOCOL = 1;
  const TCOMPACT_PROTOCOL = 2;
  const HEADER_MAGIC = 0x0fff;
  const HTTP_MAGIC = 0x504f5354;
  const MAX_FRAME_SIZE = 0x3fffffff;
  const ZLIB_TRANSFORM = 0x01;
  const HMAC_TRANSFORM = 0x02;
  const SNAPPY_TRANSFORM = 0x03;
  const INFO_KEYVALUE = 0x01;
  const HEADER_CLIENT_TYPE = 0x00;
  const FRAMED_DEPRECATED = 0x01;
  const UNFRAMED_DEPRECATED = 0x02;
  const HTTP_CLIENT_TYPE = 0x03;
  const UNKNOWN_CLIENT_TYPE = 0x04;
  protected $protoId_ = self::TBINARY_PROTOCOL;
  protected $clientType_ = self::HEADER_CLIENT_TYPE;
  protected $supportedProtocols;
  protected $readTrans_;
  protected $writeTrans_;
  protected $seqId_ = 0;
  protected $flags_ = 0;
  protected $readHeaders;
  protected $writeHeaders;
  protected $persistentWriteHeaders;
  const IDENTITY_HEADER = "identity";
  const ID_VERSION_HEADER = "id_version";
  const ID_VERSION = 1;
  protected $identity = null;
  public function __construct($transport = null, $protocols = null) {
    parent::__construct($transport, true, true);
    $this->readTrans_ = new \HH\Vector(array());
    $this->writeTrans_ = new \HH\Vector(array());
    $this->readHeaders = \HH\Map::hacklib_new(array(), array());
    $this->writeHeaders = \HH\Map::hacklib_new(array(), array());
    $this->persistentWriteHeaders = \HH\Map::hacklib_new(array(), array());
    $this->supportedProtocols = new \HH\Set(
      array(
        self::HEADER_CLIENT_TYPE,
        self::FRAMED_DEPRECATED,
        self::UNFRAMED_DEPRECATED,
        self::HTTP_CLIENT_TYPE
      )
    );
    if (\hacklib_cast_as_boolean($protocols)) {
      $this->supportedProtocols->addAll($protocols);
    }
  }
  public function getProtocolID() {
    return $this->protoId_;
  }
  public function setProtocolID($protoId) {
    $this->protoId_ = $protoId;
    return $this;
  }
  public function addTransform($trans_id) {
    $this->writeTrans_[] = $trans_id;
    return $this;
  }
  public function resetProtocol() {
    if ($this->clientType_ === self::HTTP_CLIENT_TYPE) {
      $this->flush();
    }
    $this->clientType_ = self::HEADER_CLIENT_TYPE;
    $this->readFrame(0);
  }
  public function setHeader($str_key, $str_value) {
    $this->writeHeaders[$str_key] = (string) $str_value;
    return $this;
  }
  public function setPersistentHeader($str_key, $str_value) {
    $this->persistentWriteHeaders[$str_key] = (string) $str_value;
    return $this;
  }
  public function getWriteHeaders() {
    return $this->writeHeaders;
  }
  public function getPersistentWriteHeaders() {
    return $this->persistentWriteHeaders;
  }
  public function getHeaders() {
    return $this->readHeaders;
  }
  public function clearHeaders() {
    $this->writeHeaders = \HH\Map::hacklib_new(array(), array());
  }
  public function clearPersistentHeaders() {
    $this->persistentWriteHeaders = \HH\Map::hacklib_new(array(), array());
  }
  public function getPeerIdentity() {
    if (\hacklib_cast_as_boolean(
          $this->readHeaders->contains(self::IDENTITY_HEADER)
        )) {
      if (((int) $this->readHeaders[self::ID_VERSION_HEADER]) ===
          self::ID_VERSION) {
        return $this->readHeaders[self::IDENTITY_HEADER];
      }
    }
    return null;
  }
  public function setIdentity($identity) {
    $this->identity = $identity;
    return $this;
  }
  public function read($len) {
    if ($this->clientType_ === self::UNFRAMED_DEPRECATED) {
      return $this->transport_->readAll($len);
    }
    if (strlen($this->rBuf_) === 0) {
      $this->readFrame($len);
    }
    $out = substr($this->rBuf_, $this->rIndex_, $len);
    $this->rIndex_ += $len;
    if (strlen($this->rBuf_) <= $this->rIndex_) {
      $this->rBuf_ = "";
      $this->rIndex_ = 0;
    }
    return $out;
  }
  private function readFrame($req_sz) {
    $buf = $this->transport_->readAll(4);
    $val = unpack("N", $buf);
    $sz = $val[1];
    if (($sz & TBinaryProtocol::VERSION_MASK) ===
        TBinaryProtocol::VERSION_1) {
      $this->clientType_ = self::UNFRAMED_DEPRECATED;
      if ($req_sz <= 4) {
        $this->rBuf_ = $buf;
      } else {
        $this->rBuf_ = $buf.$this->transport_->readAll($req_sz - 4);
      }
    } else {
      if ($sz === self::HTTP_MAGIC) {
        throw new TTransportException(
          "PHP HeaderTransport does not support HTTP",
          TTransportException::INVALID_CLIENT
        );
      } else {
        $buf2 = $this->transport_->readAll(4);
        $val2 = unpack("N", $buf2);
        $version = $val2[1];
        if (($version & TBinaryProtocol::VERSION_MASK) ===
            TBinaryProtocol::VERSION_1) {
          $this->clientType_ = self::FRAMED_DEPRECATED;
          if (($sz - 4) > self::MAX_FRAME_SIZE) {
            throw new TTransportException(
              "Frame size is too large",
              TTransportException::INVALID_FRAME_SIZE
            );
          }
          $this->rBuf_ = $buf2.$this->transport_->readAll($sz - 4);
        } else {
          if (($version & 0xffff0000) === (self::HEADER_MAGIC << 16)) {
            $this->clientType_ = self::HEADER_CLIENT_TYPE;
            if (($sz - 4) > self::MAX_FRAME_SIZE) {
              throw new TTransportException(
                "Frame size is too large",
                TTransportException::INVALID_FRAME_SIZE
              );
            }
            $this->flags_ = $version & 0x0000ffff;
            $buf3 = $this->transport_->readAll(4);
            $val3 = unpack("N", $buf3);
            $this->seqId_ = $val3[1];
            $buf4 = $this->transport_->readAll(2);
            $val4 = unpack("n", $buf4);
            $header_size = $val4[1];
            $data = $buf.$buf2.$buf3.$buf4;
            $index = strlen($data);
            $data .= $this->transport_->readAll($sz - 10);
            $this->readHeaderFormat($sz - 10, $header_size, $data, $index);
          } else {
            $this->clientType_ = self::UNKNOWN_CLIENT_TYPE;
            throw new TTransportException(
              "Unknown client type",
              TTransportException::INVALID_CLIENT
            );
          }
        }
      }
    }
    if (!\hacklib_cast_as_boolean(
          $this->supportedProtocols->contains($this->clientType_)
        )) {
      throw new TTransportException(
        "Client type not supported on this server",
        TTransportException::INVALID_CLIENT
      );
    }
  }
  protected function readVarint($data, &$index) {
    $result = 0;
    $shift = 0;
    while (true) {
      $x = substr($data, $index++, 1);
      $byte = ord($x);
      $result |= ($byte & 0x7f) << $shift;
      if (($byte >> 7) === 0) {
        return $result;
      }
      $shift += 7;
    }
    throw new TTransportException("THeaderTransport: You shouldn't be here");
  }
  protected function getVarint($data) {
    $out = "";
    while (true) {
      if (($data & (~0x7f)) === 0) {
        $out .= chr($data);
        break;
      } else {
        $out .= chr(($data & 0xff) | 0x80);
        $data = $data >> 7;
      }
    }
    return $out;
  }
  protected function writeString($str) {
    $buf = $this->getVarint(strlen($str));
    $buf .= $str;
    return $buf;
  }
  protected function readString($data, &$index, $limit) {
    $str_sz = $this->readVarint($data, $index);
    if (($str_sz + $index) > $limit) {
      throw new TTransportException(
        "String read too long",
        TTransportException::INVALID_FRAME_SIZE
      );
    }
    $str = substr($data, $index, $str_sz);
    $index += $str_sz;
    return $str;
  }
  protected function readHeaderFormat($sz, $header_size, $data, $index) {
    $this->readTrans_ = new \HH\Vector(array());
    $header_size = $header_size * 4;
    if ($header_size > $sz) {
      throw new TTransportException(
        "Header size is larger than frame",
        TTransportException::INVALID_FRAME_SIZE
      );
    }
    $end_of_header = $index + $header_size;
    $this->protoId_ = $this->readVarint($data, $index);
    $numHeaders = $this->readVarint($data, $index);
    if (($this->protoId_ === 1) &&
        ($this->clientType_ !== self::HTTP_CLIENT_TYPE)) {
      throw new TTransportException(
        "Trying to recv JSON encoding over binary",
        TTransportException::INVALID_CLIENT
      );
    }
    for ($i = 0; $i < $numHeaders; $i++) {
      $transId = $this->readVarint($data, $index);
      switch ($transId) {
        case self::ZLIB_TRANSFORM:
        case self::SNAPPY_TRANSFORM:
          $this->readTrans_[] = $transId;
          break;
        case self::HMAC_TRANSFORM:
          throw new TApplicationException(
            "Hmac transform no longer supported",
            TApplicationException::INVALID_TRANSFORM
          );
        default:
          throw new TApplicationException(
            "Unknown transform in client request",
            TApplicationException::INVALID_TRANSFORM
          );
      }
    }
    $this->readTrans_ = array_reverse($this->readTrans_);
    $this->readHeaders = \HH\Map::hacklib_new(array(), array());
    while ($index < $end_of_header) {
      $infoId = $this->readVarint($data, $index);
      switch ($infoId) {
        case self::INFO_KEYVALUE:
          $num_keys = $this->readVarint($data, $index);
          for ($i = 0; $i < $num_keys; $i++) {
            $strKey = $this->readString($data, $index, $end_of_header);
            $strValue = $this->readString($data, $index, $end_of_header);
            $this->readHeaders[$strKey] = $strValue;
          }
          break;
        default:
          break;
      }
    }
    $this->rBuf_ =
      $this->untransform(substr($data, $end_of_header, $sz - $header_size));
  }
  protected function transform($data) { // UNSAFE
    foreach ($this->writeTrans_ as $trans) {
      switch ($trans) {
        case self::ZLIB_TRANSFORM:
          $data = gzcompress($data);
          break;
        case self::SNAPPY_TRANSFORM:
          $data = sncompress($data);
          break;
        default:
          throw new TTransportException(
            "Unknown transform during send",
            TTransportException::INVALID_TRANSFORM
          );
      }
    }
    return $data;
  }
  protected function untransform($data) { // UNSAFE
    foreach ($this->readTrans_ as $trans) {
      switch ($trans) {
        case self::ZLIB_TRANSFORM:
          $data = gzuncompress($data);
          break;
        case self::SNAPPY_TRANSFORM:
          $data = snuncompress($data);
          break;
        default:
          throw new TApplicationException(
            "Unknown transform during recv",
            TTransportException::INVALID_TRANSFORM
          );
      }
    }
    return $data;
  }
  public function flush() {
    $this->flushImpl(false);
  }
  public function onewayFlush() {
    $this->flushImpl(true);
  }
  private function flushImpl($oneway) {
    if (\hacklib_equals(strlen($this->wBuf_), 0)) {
      if (\hacklib_cast_as_boolean($oneway)) {
        $this->transport_->onewayFlush();
      } else {
        $this->transport_->flush();
      }
      return;
    }
    $out = $this->transform($this->wBuf_);
    $this->wBuf_ = "";
    if (($this->protoId_ === 1) &&
        ($this->clientType_ !== self::HTTP_CLIENT_TYPE)) {
      throw new TTransportException(
        "Trying to send JSON encoding over binary",
        TTransportException::INVALID_CLIENT
      );
    }
    $buf = "";
    if ($this->clientType_ === self::HEADER_CLIENT_TYPE) {
      $transformData = "";
      $num_headers = 0;
      foreach ($this->writeTrans_ as $trans) {
        ++$num_headers;
        $transformData .= $this->getVarint($trans);
      }
      if ($this->identity !== null) {
        $this->writeHeaders[self::ID_VERSION_HEADER] =
          (string) self::ID_VERSION;
        $this->writeHeaders[self::IDENTITY_HEADER] = $this->identity;
      }
      $infoData = "";
      if (\hacklib_cast_as_boolean($this->writeHeaders) ||
          \hacklib_cast_as_boolean($this->persistentWriteHeaders)) {
        $infoData .= $this->getVarint(self::INFO_KEYVALUE);
        $infoData .= $this->getVarint(
          count($this->writeHeaders) + count($this->persistentWriteHeaders)
        );
        foreach ($this->persistentWriteHeaders as $str_key => $str_value) {
          $infoData .= $this->writeString($str_key);
          $infoData .= $this->writeString($str_value);
        }
        foreach ($this->writeHeaders as $str_key => $str_value) {
          $infoData .= $this->writeString($str_key);
          $infoData .= $this->writeString($str_value);
        }
      }
      $this->writeHeaders = \HH\Map::hacklib_new(array(), array());
      $headerData =
        $this->getVarint($this->protoId_).$this->getVarint($num_headers);
      $header_size =
        strlen($transformData) + strlen($infoData) + strlen($headerData);
      $paddingSize = 4 - ($header_size % 4);
      $header_size += $paddingSize;
      $buf = (string) pack("nn", self::HEADER_MAGIC, $this->flags_);
      $buf .= (string) pack("Nn", $this->seqId_, $header_size / 4);
      $buf .= $headerData.$transformData;
      $buf .= $infoData;
      for ($i = 0; $i < $paddingSize; $i++) {
        $buf .= (string) pack("C", "\\0");
      }
      $buf .= $out;
      $buf = ((string) pack("N", strlen($buf))).$buf;
    } else {
      if ($this->clientType_ === self::FRAMED_DEPRECATED) {
        $buf = (string) pack("N", strlen($out));
        $buf .= $out;
      } else {
        if ($this->clientType_ === self::UNFRAMED_DEPRECATED) {
          $buf = $out;
        } else {
          if ($this->clientType_ === self::HTTP_CLIENT_TYPE) {
            throw new TTransportException(
              "HTTP not implemented",
              TTransportException::INVALID_CLIENT
            );
          } else {
            throw new TTransportException(
              "Unknown client type",
              TTransportException::INVALID_CLIENT
            );
          }
        }
      }
    }
    if (strlen($buf) > self::MAX_FRAME_SIZE) {
      throw new TTransportException(
        "Attempting to send oversize frame",
        TTransportException::INVALID_FRAME_SIZE
      );
    }
    $this->transport_->write($buf);
    if (\hacklib_cast_as_boolean($oneway)) {
      $this->transport_->onewayFlush();
    } else {
      $this->transport_->flush();
    }
  }
}
