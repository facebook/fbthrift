/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
module thrift.transport.header;

import thrift.internal.endian;
import thrift.protocol.binary;
import thrift.transport.base;
import thrift.transport.framed;

final class THeaderTransport : TBaseFramedTransport {

  enum HEADER_MAGIC_MASK = 0xFFFF0000;
  enum HEADER_FLAGS_MASK = 0x0000FFFF;

  // 16th and 32nd bits must be 0 to differentiate framed vs unframed.
  enum HEADER_MAGIC = 0x0FFF0000;

  // HTTP has different magic
  enum HTTP_SERVER_MAGIC = 0x504F5354; // 'POST'

  // Note max frame size is slightly less than HTTP_SERVER_MAGIC
  enum MAX_FRAME_SIZE = 0x3FFFFFFF;

  enum Transform : ubyte {
    None = 0x00,
    Zlib = 0x01,
    HMAC = 0x02,
    Snappy = 0x03,
    QLZ = 0x04,
  }

  enum Info {
    KEYVALUE = 1,
    PKEYVALUE = 2,
  }

  enum ClientType : ubyte {
    Headers = 0,
    FramedDEPRECATED = 1,
    UnframedDEPRECATED = 2,
    HTTPServer = 3,
    HTTPClient = 4,
    FramedCompact = 5,
    HeaderSASL = 6,
    HTTPGet = 7,
    Unknown = 8,
  }

  enum Protocol : ubyte {
    Binary = 0,
    JSON = 1,
    Compact = 2,
    Debug = 3,
    Virtual = 4,
  }

  this(TTransport transport) {
    super(transport);
  }

  // this with supported client

  Protocol getProtocol() {
    // Default to binary for all others.
    return (clientType == ClientType.Headers)
      ? protoId
      : Protocol.Binary;
  }

  @property
  void setProtocol(Protocol protoId) {
    this.protoId = protoId;
  }

  // transforms

  void setHeader(string key, string value) {
    writeHeaders[key] = value;
  }

  string[string] getWriteHeaders() {
    return writeHeaders;
  }

  void setPersistentHeader(string key, string value) {
    writePersistentHeaders[key] = value;
  }

  string[string] getWritePersistentHeaders() {
    return writePersistentHeaders;
  }

  string[string] getReadPersistentHeaders() {
    return readPersistentHeaders;
  }

  string[string] getHeaders() {
    return readHeaders;
  }

  void clearHeaders() {
    writeHeaders = null;
  }

  public void clearPersistentHeaders() {
    writePersistentHeaders = null;
  }

  // identity
  string getPeerIdentity() {
    if (auto idPtr = IdentityHeader in readHeaders) {
      if (readHeaders.get(IdVersionHeader, "") == IdVersion) {
        return *idPtr;
      }
    }

    return "";
  }

  void setIdentity(string identity) {
    this.identity = identity;
  }

  // crypto
  override size_t read(ubyte[] buf) {
    if (peek()) {
      return super.read(buf);
    }

    if (clientType == ClientType.UnframedDEPRECATED) {
      return transport.read(buf);
    }

    readFrame(buf.length);
    return super.read(buf);
  }

  override void readAll(ubyte[] buf) {
    if (peek()) {
      super.readAll(buf);
      return;
    }

    if (clientType == ClientType.UnframedDEPRECATED) {
      transport.readAll(buf);
      return;
    }

    readFrame(buf.length);
    super.readAll(buf);
  }

  /**
   * Should be called from THeaderProtocol at the start of every message
   */
  public void _resetProtocol() {
    // Set to anything except unframed
    clientType = ClientType.Headers;
    // Read the header bytes to check which protocol to use
    readFrame(0);
  }

protected:
  override bool readFrame() {
    throw new TTransportException("You must use readFrame(int reqLen)");
  }

  bool readFrame(size_t reqLen) {
    IntBuf!uint b = void;

    transport.readAll(b.bytes);
    uint w1 = b.value;

    if ((w1 & VERSION_MASK) == VERSION_1) {
      clientType = ClientType.UnframedDEPRECATED;
      if (reqLen <= 4) {
        rBuf_.length = 4;
        rBuf_[] = b.bytes;
      } else {
        rBuf_.length = reqLen;
        rBuf_[0 .. 4][] = b.bytes;
        transport.readAll(rBuf_[4 .. reqLen - 4]);
      }
    } else if (w1 == HTTP_SERVER_MAGIC) {
      throw new THeaderException("This transport does not support HTTP");
    } else {
      if (w1 - 4 > MAX_FRAME_SIZE) {
        throw new TTransportException("Framed transport frame is too large");
      }

      // Could be framed or header format. Check next word.
      transport.readAll(b.bytes);
      uint w2 = b.value;
      if ((w2 & VERSION_MASK) == VERSION_1) {
        clientType = ClientType.FramedDEPRECATED;
        rBuf_.length = w1;
        rBuf_[0 .. 4][] = b.bytes;
        transport.readAll(rBuf_[4 .. w1 - 4]);
      } else if ((w2 & HEADER_MAGIC_MASK) == HEADER_MAGIC) {
        clientType = ClientType.Headers;
        if (w1 - 4 < 10) {
          throw new TTransportException("Header transport frame is too small");
        }

        rBuf_.length = w1;
        rBuf_[0 .. 4][] = b.bytes;

        // read packet minus version
        transport.readAll(rBuf_[4 .. w1 - 4]);
        flags = w2 & HEADER_FLAGS_MASK;

        // read seqId
        b.bytes[] = rBuf_[4 .. 8];
        seqId = b.value;

        IntBuf!ushort s = void;
        s.bytes[] = rBuf_[8 .. 10];
        int headerSize = s.value;
        readHeaderFormat(headerSize, rBuf_);
      } else {
        clientType = ClientType.Unknown;
        throw new THeaderException("Unsupported client type");
      }
    }

    return true;
  }

private:

  /**
   * Reads a varint from the buffer.
   * frame.data = buffer to use
   * frame.idx = Offset to data in this case, incremented by size of varint
   */
  int readVarint32Buf(ref ubyte[] frame) {
    int result = 0;
    int shift = 0;

    while (true) {
      ubyte b = frame[0];
      frame = frame[1 .. $];
      result |= cast(int) ((b & 0x7f) << shift);
      if ((b & 0x80) != 0x80) {
        break;
      }
      shift += 7;
    }

    return result;
  }

  string readString(ref ubyte[] frame) {
    int sz = readVarint32Buf(frame);

    ubyte[] data = frame[0 .. sz];
    frame = frame[sz .. $];

    import std.conv;
    return data.to!string();
  }

  void readHeaderFormat(int headerSize, ubyte[] buff) {
    auto frame = buff[10 .. $];

    headerSize = headerSize * 4;
    int endHeader = headerSize + 10;

    if (headerSize > frame.length) {
      throw new TTransportException("Header size is larger than frame");
    }

    import std.conv;
    protoId = readVarint32Buf(frame).to!Protocol();
    int numHeaders = readVarint32Buf(frame);

    // Clear out any previous transforms
    readTransforms.length = numHeaders;
    readTransforms[] = Transform.None;

    if (protoId == Protocol.JSON && clientType != ClientType.HTTPServer) {
      throw new TTransportException("Trying to recv JSON encoding over binary");
    }

    // Read in the headers.  Data for each varies. See
    // doc/HeaderFormat.txt
    int hmacSz = 0;
    for (int i = 0; i < numHeaders; i++) {
      int transId = readVarint32Buf(frame);
      switch (transId) with(Transform) {
        case Zlib :
          readTransforms[i] = Zlib;
          break;

        case Snappy :
          readTransforms[i] = Snappy;
          break;

        case HMAC :
          hmacSz = frame[0];
          frame[0] = 0;
          frame = frame[1 .. $];
          readTransforms[i] = HMAC;
          break;

        default :
          throw new THeaderException("Unknown transform during recv");
      }
    }

    // Read the info section.
    readHeaders.clear();
    while ((frame.ptr - buff.ptr) < endHeader) {
      int infoId = readVarint32Buf(frame);
      switch(infoId) with(Info) {
        case KEYVALUE:
          int numKeys = readVarint32Buf(frame);
          for (int i = 0; i < numKeys; i++) {
            string key = readString(frame);
            string value = readString(frame);
            readHeaders[key] = value;
          }

          break;

        case PKEYVALUE:
          int numKeys = readVarint32Buf(frame);
          for (int i = 0; i < numKeys; i++) {
            string key = readString(frame);
            string value = readString(frame);
            readPersistentHeaders[key] = value;
          }

          break;

        default:
          // Unknown info ID, continue on to reading data.
          break;
      }
    }

    foreach (key, value; readPersistentHeaders) {
      readHeaders[key] = value;
    }

    if (crypto !is null) {
      ubyte[] payload = buff[0 .. $ - hmacSz];
      ubyte[] macBytes = buff[$ - hmacSz .. $];
      try {
        if (!crypto.isValidMac(payload, macBytes)) {
          throw new THeaderException("Mac did not verify");
        }
      } catch (Exception e) {
        throw new THeaderException("Unable to mac data: " ~ e.toString());
      }
    }

    // Read in the data section and limit to data without mac.
    frame = buff[endHeader .. $ - hmacSz];
    rBuf_ = untransform(frame);
  }

  ubyte[] untransform(ubyte[] data) {
    bool hasZlib, hasSnappy;
    foreach (t; readTransforms) {
      switch (t) with(Transform) {
        case Zlib :
          hasZlib = true;
          break;

        case Snappy :
          hasSnappy = true;
          break;

        default:
          continue;
      }
    }

    if (hasZlib) {
      throw new TTransportException("Zlib not implemented");
    } else if (hasSnappy) {
      throw new TTransportException("Snappy not implemented");
    }

    return data;
  }

  enum IdentityHeader = "identity";
  enum IdVersionHeader = "id_version";
  enum IdVersion = "1";

  Protocol protoId = Protocol.Compact;
  ClientType clientType = ClientType.Headers;

  uint seqId = 0;
  uint flags = 0;

  Transform[] writeTransforms;
  Transform[] readTransforms;

  string[string] readHeaders;
  string[string] writeHeaders;

  string[string] readPersistentHeaders;
  string[string] writePersistentHeaders;

  string identity;

  CryptoCallback crypto;
}

/**
 * Wraps given transports into THeaderTransport.
 */
alias THeaderTransportFactory = TWrapperTransportFactory!THeaderTransport;

/**
 * Interface for crypto call backs.
 */
interface CryptoCallback {
  ubyte[] mac(ubyte[] data);
  bool isValidMac(ubyte[] data, ubyte[] mac);
}

/**
 * Application-level exception.
 */
final class THeaderException : TTransportException {

  ///
  this(string msg, string file = __FILE__, size_t line = __LINE__, Throwable next = null) {
    super(msg, file, line, next);
  }

}

unittest {
  import thrift.transport.memory;
  auto buf = new TMemoryBuffer();
  auto writer = new THeaderTransport(buf);

  string frost = "Whose woods these are I think I know";
  ubyte[] testBytes = cast(ubyte[]) frost;

  writer.write(testBytes);
  writer.flush();

  auto reader = new THeaderTransport(buf);
  ubyte[] receivedBytes;
  receivedBytes.length = testBytes.length;
  reader.read(receivedBytes);

  assert(testBytes == receivedBytes);
}
