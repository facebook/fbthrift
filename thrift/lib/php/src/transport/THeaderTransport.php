<?php
// Copyright 2004-present Facebook. All Rights Reserved.


/**
 * Header transport. Writes and reads data with
 *
 * THeaderTransport (any protocol)
 * or TFramedTransport format
 * or unframed TBinaryProtocol.
 * (or TBinary over HTTP if someone wants to implement it)
 *
 * @package thrift.transport
 */
class THeaderTransport extends TFramedTransport {
  const TBINARY_PROTOCOL = 0;
  const TJSON_PROTOCOL = 1;
  const TCOMPACT_PROTOCOL = 2;

  const HEADER_MAGIC = 0x0fff;
  const HTTP_SERVER_MAGIC = 0x504f5354; // POST
  const MAX_FRAME_SIZE = 0x3fffffff;

  // Transforms
  const ZLIB_TRANSFORM = 0x01;
  const HMAC_TRANSFORM = 0x02;
  const SNAPPY_TRANSFORM = 0x03;

  // Infos
  const INFO_KEYVALUE = 0x01;

  // Client types
  const HEADER_CLIENT_TYPE = 0x00;
  const FRAMED_DEPRECATED = 0x01;
  const UNFRAMED_DEPRECATED = 0x02;
  const HTTP_CLIENT_TYPE = 0x03;
  const UNKNOWN_CLIENT_TYPE = 0x04;

  // default to binary
  protected $protoId_ = self::TBINARY_PROTOCOL;
  protected $clientType_ = self::HEADER_CLIENT_TYPE;
  protected $supportedProtocols = array(self::HEADER_CLIENT_TYPE => true,
                                        self::FRAMED_DEPRECATED => false,
                                        self::UNFRAMED_DEPRECATED => false,
                                        self::HTTP_CLIENT_TYPE => false,
                                       );
  protected $readTrans_;
  protected $writeTrans_ = array();
  protected $seqId_ = 0;
  protected $flags_ = 0;

  protected $readHeaders = array();
  protected $writeHeaders = array();
  protected $persistentWriteHeaders = array();

  const IDENTITY_HEADER = "identity";
  const ID_VERSION_HEADER = "id_version";
  const ID_VERSION = 1;
  protected $identity = null;

  protected $hmacFunc = null;
  protected $verifyFunc = null;

  /**
   * Constructor.
   *
   * @param TTransport $transport Underlying transport
   */
  public function __construct($transport=null, $protocols=null) {
    parent::__construct($transport, true, true);
    if ($protocols) {
      foreach ($protocols as $protocol) {
        $this->supportedProtocols[$protocol] = true;
      }
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

  public function setHmac($hmac_func, $verify_func) {
    $this->hmacFunc = $hmac_func;
    $this->verifyFunc = $verify_func;
    return $this;
  }

  public function setHeader($str_key, $str_value) {
    $this->writeHeaders[$str_key] = $str_value;
    return $this;
  }

  public function setPersistentHeader($str_key, $str_value) {
    $this->persistentWriteHeaders[$str_key] = $str_value;
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
    $this->writeHeaders = array();
  }

  public function clearPersistentHeaders() {
    $this->persistentWriteHeaders = array();
  }

  public function getPeerIdentity() {
    if (isset($this->readHeaders[self::IDENTITY_HEADER])) {
      if ((int)$this->readHeaders[self::ID_VERSION_HEADER] ===
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

  /**
   * Reads from the buffer. When more data is required reads another entire
   * chunk and serve future reads out of that.
   *
   * @param int $len How much data
   */
  public function read($len) {
    if ($this->clientType_ === self::UNFRAMED_DEPRECATED) {
       return $this->transport_->readAll($len);
    }

    if (strlen($this->rBuf_) === 0) {
      $this->readFrame($len);
    }

    // Return substr
    $out = substr($this->rBuf_, $this->rIndex_, $len);
    $this->rIndex_ += $len;

    if (strlen($this->rBuf_) <= $this->rIndex_) {
      $this->rBuf_ = '';
      $this->rIndex_ = 0;
    }
    return $out;
  }

  /**
   * Reads a chunk of data into the internal read buffer.
   */
  private function readFrame($req_sz) {
    $buf = $this->transport_->readAll(4);
    $val = unpack('N', $buf);
    $sz = $val[1];

    if (($sz & TBinaryProtocol::VERSION_MASK) === TBinaryProtocol::VERSION_1) {
      $this->clientType_ = self::UNFRAMED_DEPRECATED;
      if ($req_sz <= 4) {
        $this->rBuf_ = $buf;
      } else {
        $this->rBuf_ = $buf . $this->transport_->readAll($req_sz - 4);
      }
    } else if ($sz === self::HTTP_SERVER_MAGIC) {
      throw new TTransportException('PHP HeaderTransport does not support HTTP',
                                    TTransportException::INVALID_CLIENT);
    } else {
      // Either header format or framed. Check next byte
      $buf2 = $this->transport_->readAll(4);
      $val2 = unpack('N', $buf2);
      $version = $val2[1];
      if (($version & TBinaryProtocol::VERSION_MASK) ===
          TBinaryProtocol::VERSION_1) {
        $this->clientType_ = self::FRAMED_DEPRECATED;
        if ($sz - 4 > self::MAX_FRAME_SIZE) {
          throw new TTransportException('Frame size is too large',
                                       TTransportException::INVALID_FRAME_SIZE);
        }
        $this->rBuf_ = $buf2 . $this->transport_->readAll($sz - 4);
      } else if (($version & 0xffff0000) === (self::HEADER_MAGIC << 16)) {
        $this->clientType_ = self::HEADER_CLIENT_TYPE;
        if ($sz - 4 > self::MAX_FRAME_SIZE) {
          throw new TTransportException('Frame size is too large',
                                       TTransportException::INVALID_FRAME_SIZE);
        }
        $this->flags_ = ($version & 0x0000ffff);
        // read seqId
        $buf3 = $this->transport_->readAll(4);
        $val3 = unpack('N', $buf3);
        $this->seqId_ = $val3[1];
        // read header_size
        $buf4 = $this->transport_->readAll(2);
        $val4 = unpack('n', $buf4);
        $header_size = $val4[1];

        $data = $buf . $buf2 . $buf3 . $buf4;
        $index = strlen($data);

        $data .= $this->transport_->readAll($sz - 10);
        $this->readHeaderFormat($sz - 10, $header_size, $data, $index);
      } else {
        $this->clientType_ = self::UNKNOWN_CLIENT_TYPE;
        throw new TTransportException('Unknown client type',
                                      TTransportException::INVALID_CLIENT);
      }
    }

    if (!$this->supportedProtocols[$this->clientType_]) {
      throw new TTransportException('Client type not supported on this server',
                                    TTransportException::INVALID_CLIENT);
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
  }

  protected function getVarint($data) {
    $out = "";
    while (true) {
      if (($data & ~0x7f) === 0) {
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
    if ($str_sz + $index > $limit) {
      throw new TTransportException('String read too long',
                                    TTransportException::INVALID_FRAME_SIZE);
    }

    $str = substr($data, $index, $str_sz);
    $index += $str_sz;
    return $str;
  }

  protected function readHeaderFormat($sz, $header_size, $data, $index) {
    $this->readTrans_ = array();

    $header_size = $header_size * 4;
    if ($header_size > $sz) {
      throw new TTransportException('Header size is larger than frame',
                                    TTransportException::INVALID_FRAME_SIZE);
    }
    $end_of_header = $index + $header_size;

    $this->protoId_ = $this->readVarint($data, $index);
    $numHeaders = $this->readVarint($data, $index);
    if ($this->protoId_ === 1 &&
        $this->clientType_ !== self::HTTP_CLIENT_TYPE) {
      throw new TTransportException('Trying to recv JSON encoding over binary',
                                TTransportException::INVALID_CLIENT);
    }

    // Read in the headers.  Data for each header varies.
    $hmac_sz = 0;
    for ($i = 0; $i < $numHeaders; $i++) {
      $transId = $this->readVarint($data, $index);
      switch ($transId) {
        case self::ZLIB_TRANSFORM:
        case self::SNAPPY_TRANSFORM:
          $this->readTrans_[] = $transId;
          break;

        case self::HMAC_TRANSFORM:
          $hmac_sz = ord($data[$index]);
          $data[$index] = pack('C', '\0');
          ++$index;
          break;

        default:
          throw new TApplicationException(
                     'Unknown transform in client request',
                      TApplicationException::INVALID_TRANSFORM);
      }
    }
    // Make sure that the read transforms are applied in the reverse order
    // from when the data was written.
    $this->readTrans_ = array_reverse($this->readTrans_);

    // Checking hmac should always happen last
    if (isset($this->verifyFunc)) {
      $hmac = substr($data, -$hmac_sz);
      if (!call_user_func($this->verifyFunc,
                          substr($data, 4, strlen($data) - $hmac_sz - 4),
                          $hmac)) {
        throw new TApplicationException(
          'HMAC did not verify',
          TApplicationException::INVALID_TRANSFORM);
      }
    }

    // Read the info headers
    $this->readHeaders = array();
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
          // End of infos
          break;
      }
    }

    $this->rBuf_ = $this->untransform(substr($data,
                                             $end_of_header,
                                             $sz - $header_size));
  }

  protected function transform($data) {
    // UNSAFE: the only way we know that these possibly iterated compressors
    // will produce reeasonable results is by complicated promises of the way
    // thrift sets things up, which Hack has no way to understand.
    foreach ($this->writeTrans_ as $trans) {
      switch ($trans) {
        case self::ZLIB_TRANSFORM:
          $data = gzcompress($data);
          break;

        case self::SNAPPY_TRANSFORM:
          $data = sncompress($data);
          break;

        default:
          throw new TTransportException('Unknown transform during send',
                                        TTransportException::INVALID_TRANSFORM);
      }
    }
    return $data;
  }

  protected function untransform($data) {
    // UNSAFE: the only way we know that these possibly iterated compressors
    // will produce reeasonable results is by complicated promises of the way
    // thrift sets things up, which Hack has no way to understand.
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
                      'Unknown transform during recv',
                      TTransportException::INVALID_TRANSFORM);
      }
    }
    return $data;
  }

  /**
   * Writes the output buffer in header format, or format
   * client responded with (framed, unframed, http)
   */
  public function flush() {
    if (strlen($this->wBuf_) == 0) {
      return $this->transport_->flush();
    }

    $out = $this->transform($this->wBuf_);

    // Note that we clear the internal wBuf_ prior to the underlying write
    // to ensure we're in a sane state (i.e. internal buffer cleaned)
    // if the underlying write throws up an exception
    $this->wBuf_ = '';

    if ($this->protoId_ === 1 &&
        $this->clientType_ !== self::HTTP_CLIENT_TYPE) {
      throw new TTransportException('Trying to send JSON encoding over binary',
                                    TTransportException::INVALID_CLIENT);
    }

    $buf = '';
    if ($this->clientType_ === self::HEADER_CLIENT_TYPE) {
      $transformData = '';

      $num_headers = 0;
      // For now, no transform requires data.
      foreach ($this->writeTrans_ as $trans) {
        ++$num_headers;
        $transformData .= $this->getVarint($trans);
      }

      if (isset($this->hmacFunc)) {
        $num_headers++;
        $transformData .= $this->getVarint(self::HMAC_TRANSFORM);

        // Append hmac size, updated later.
        $transformData .= pack('C','\0');
      }

      // Add in special flags.
      if (isset($this->identity)) {
        $this->writeHeaders[self::ID_VERSION_HEADER] = self::ID_VERSION;
        $this->writeHeaders[self::IDENTITY_HEADER] = $this->identity;
      }

      $infoData = '';
      if ($this->writeHeaders || $this->persistentWriteHeaders) {
        $infoData .= $this->getVarint(self::INFO_KEYVALUE);
        $infoData .= $this->getVarint(count($this->writeHeaders) +
                                      count($this->persistentWriteHeaders));
        foreach ($this->persistentWriteHeaders as $str_key => $str_value) {
          $infoData .= $this->writeString($str_key);
          $infoData .= $this->writeString($str_value);
        }
        foreach ($this->writeHeaders as $str_key => $str_value) {
          $infoData .= $this->writeString($str_key);
          $infoData .= $this->writeString($str_value);
        }
      }
      $this->writeHeaders = array();

      $headerData = $this->getVarint($this->protoId_) .
        $this->getVarint($num_headers);
      $header_size = strlen($transformData) + strlen($infoData) +
        strlen($headerData);
      $paddingSize = 4 - ($header_size % 4);
      $header_size += $paddingSize;

      $buf = pack('nn', self::HEADER_MAGIC, $this->flags_);
      $buf .= pack('Nn', $this->seqId_, $header_size / 4);

      $buf .= $headerData . $transformData;
      $hmac_loc = strlen($buf) - 1; // Update with hmac size.
      $buf .= $infoData;

      // Pad out the header with 0x00
      for ($i = 0; $i < $paddingSize; $i++) {
        $buf .= pack('C','\0');
      }

      // Append the data
      $buf .= $out;

      if (isset($this->hmacFunc)) {
        $hmac = call_user_func($this->hmacFunc, $buf);
        $buf[$hmac_loc] = pack('C', strlen($hmac));
        $buf .= $hmac;
      }

      // Prepend the size.
      $buf = pack('N', strlen($buf)) . $buf;
    } else if ($this->clientType_ === self::FRAMED_DEPRECATED) {
      $buf = pack('N', strlen($out));
      $buf .= $out;
    } else if ($this->clientType_ === self::UNFRAMED_DEPRECATED) {
      $buf = $out;
    } else if ($this->clientType_ === self::HTTP_CLIENT_TYPE) {
      throw new TTransportException('HTTP not implemented',
                                    TTransportException::INVALID_CLIENT);
    } else {
      throw new TTransportException('Unknown client type',
                                    TTransportException::INVALID_CLIENT);
    }

    if (strlen($buf) > self::MAX_FRAME_SIZE) {
      throw new TTransportException('Attempting to send oversize frame',
                                    TTransportException::INVALID_FRAME_SIZE);
    }

    $this->transport_->write($buf);
    $this->transport_->flush();
  }
}
