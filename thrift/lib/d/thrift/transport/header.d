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

  enum uint HEADER_MAGIC_MASK = 0xFFFF0000;
  enum uint HEADER_FLAGS_MASK = 0x0000FFFF;

  // 16th and 32nd bits must be 0 to differentiate framed vs unframed.
  enum uint HEADER_MAGIC = 0x0FFF0000;

  // HTTP has different magic
  enum uint HTTP_SERVER_MAGIC = 0x504F5354; // 'POST'

  // Note max frame size is slightly less than HTTP_SERVER_MAGIC
  enum uint MAX_FRAME_SIZE = 0x3FFFFFFF;

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
    if (!rBuf_.empty) {
      return super.read(buf);
    }

    if (clientType == ClientType.UnframedDEPRECATED) {
      return transport.read(buf);
    }

    readFrame(buf.length);
    return super.read(buf);
  }

  override void readAll(ubyte[] buf) {
    if (!rBuf_.empty) {
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

  /* Writes the output buffer in header format, or format
   * client responded with (framed, unframed, http)
   */
  override void flush() {
    if (wBuf_.empty) return;
    auto frame = wBuf_;

    // Properly reset the write buffer even some of the protocol operations go
    // wrong.
    scope (exit) {
      wBuf_.length = 0;
      wBuf_.assumeSafeAppend();
    }

    if (clientType == ClientType.Headers) {
      frame = transform(frame);
    }

    if (frame.length > MAX_FRAME_SIZE) {
      import std.conv;
      throw new TTransportException("Attempting to send frame that is " ~
                                    "too large: " ~
                                    to!string(frame.length));
    }

    if (protoId == Protocol.JSON && clientType != ClientType.HTTPServer) {
      throw new TTransportException("Trying to send JSON encoding" ~
                                    " over binary");
    }

    if (clientType == ClientType.Headers) {
      // For now, no transforms require data.
      auto numTransforms = cast(uint) writeTransforms.length;

      // Each varint could be up to 5 in size.
      // Extra 10 for crypto varints.
      import std.array;
      auto transformDataStorage =
        uninitializedArray!(ubyte[])(numTransforms * 5 + 10);
      auto transformData = transformDataStorage;

      uint transformDataSize = 0;
      foreach (trans; writeTransforms) {
        transformDataSize += writeVarint(transformData, trans);
      }

      if (crypto !is null) {
        numTransforms += 1;
        transformDataSize += writeVarint(transformData, Transform.HMAC);

        transformData[0] = 0; // mac size, fixup later
        transformDataSize++;
        transformData = transformData[1 .. $];
      }

      transformData = transformData[0 .. transformDataSize];

      if (identity != "") {
        writeHeaders[IdVersionHeader] = IdVersion;
        writeHeaders[IdentityHeader] = identity;
      }

      auto infoData1 = flushInfoHeaders(Info.PKEYVALUE, writePersistentHeaders);
      auto infoData2 = flushInfoHeaders(Info.KEYVALUE, writeHeaders);

      ubyte[10] headerDataStorage;
      ubyte[] headerData = headerDataStorage[];
      auto headerDataSize = writeVarint(headerData, protoId);
      headerDataSize += writeVarint(headerData, numTransforms);
      headerData = headerDataStorage[0 .. headerDataSize];

      uint headerSize = transformDataSize + headerDataSize +
        cast(uint) (infoData1.length + infoData2.length);
      uint paddingSize = 4 - headerSize % 4;
      headerSize += paddingSize;

      // Allocate buffer for the headers.
      // 14 bytes for sz, magic , flags , seqId , headerSize
      import std.array;
      auto headerBuf = uninitializedArray!(ubyte[])(headerSize + 14);

      IntBuf!uint b = void;
      b.value = hostToNet(cast(uint) (10 + headerSize + frame.length));
      headerBuf[0 .. 4][] = b.bytes;

      IntBuf!ushort s = void;
      s.value = hostToNet(cast(ushort) (HEADER_MAGIC >> 16));
      headerBuf[4 .. 6][] = s.bytes;

      s.value = hostToNet(flags);
      headerBuf[6 .. 8][] = s.bytes;

      b.value = hostToNet(seqId);
      headerBuf[8 .. 12][] = b.bytes;

      s.value = hostToNet(cast(ushort) (headerSize /4));
      headerBuf[12 .. 14][] = s.bytes;

      auto endHeaderData = headerData.length + 14;
      headerBuf[14 .. endHeaderData][] = headerData[];

      auto endTransformData = endHeaderData + transformData.length;
      headerBuf[endHeaderData .. endTransformData][] = transformData[];
      auto macLoc = endTransformData - 1;

      auto endInfoData1 = endTransformData + infoData1.length;
      headerBuf[endTransformData .. endInfoData1][] = infoData1[];

      auto endInfoData2 = endInfoData1 + infoData2.length;
      headerBuf[endInfoData1 .. endInfoData2][] = infoData2[];

      // There are no info headers for this version
      // Pad out the header with 0x00
      for (int i = 0; i < paddingSize; i++) {
        headerBuf[endTransformData + i] = 0;
      }

      ubyte[] mac = null;
      if (crypto !is null) {
        try {
          mac = crypto.mac(headerBuf[4 .. $]);
        } catch (Exception e) {
          throw new THeaderException("Unable to mac data: " ~ e.toString());
        }

        // Update mac size
        headerBuf[macLoc] = cast(ubyte) mac.length;

        // Update the frame size to include mac bytes
        b.value = hostToNet(cast(uint)
          (10 + headerSize + frame.length + mac.length));
        headerBuf[0 .. 4][] = b.bytes;
      }

      transport.write(headerBuf);
      transport.write(frame);

      if (mac.length > 0) {
        transport.write(mac);
      }
    } else if (clientType == ClientType.FramedDEPRECATED) {
      ubyte[4] buf;

      if (frame.length < 4) {
        buf[0 .. frame.length] = frame[];
        frame = buf[0 .. 4];
      }

      transport.write(frame);
    } else if (clientType == ClientType.UnframedDEPRECATED) {
      transport.write(frame);
    } else {
      import std.conv;
      throw new TTransportException("Unknown client type on send: " ~
                                    to!string(clientType));
    }

    transport.flush();
  }

protected:
  override bool readFrame() {
    throw new TTransportException("You must use readFrame(int reqLen)");
  }

  bool readFrame(size_t reqLen) {
    IntBuf!uint b = void;

    transport.readAll(b.bytes);
    uint w1 = netToHost(b.value);

    if ((w1 & VERSION_MASK) == VERSION_1) {
      clientType = ClientType.UnframedDEPRECATED;
      if (reqLen <= 4) {
        rBuf_.length = 4;
        rBuf_[] = b.bytes;
      } else {
        rBuf_.length = reqLen;
        rBuf_[0 .. 4][] = b.bytes;
        transport.readAll(rBuf_[4 .. reqLen]);
      }
    } else if (w1 == HTTP_SERVER_MAGIC) {
      throw new THeaderException("This transport does not support HTTP");
    } else {
      if (w1 - 4 > MAX_FRAME_SIZE) {
        throw new TTransportException("Framed transport frame is too large");
      }

      // Could be framed or header format. Check next word.
      transport.readAll(b.bytes);
      uint w2 = netToHost(b.value);
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
        rBuf_[0 .. 4] = b.bytes;

        // read packet minus version
        transport.readAll(rBuf_[4 .. w1]);
        flags = w2 & HEADER_FLAGS_MASK;

        // read seqId
        b.bytes[] = rBuf_[4 .. 8];
        seqId = netToHost(b.value);

        IntBuf!ushort s = void;
        s.bytes[] = rBuf_[8 .. 10];
        int headerSize = netToHost(s.value);
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

  uint writeVarint(ref ubyte[] buf, uint n) {
    uint i = 0;
    while (true) {
      if ((n & ~0x7F) == 0) {
        buf[i++] = cast(ubyte) n;
        break;
      }

      buf[i++] = cast(ubyte) (n | 0x80);
      n >>= 7;
    }

    buf = buf[i .. $];
    return i;
  }

  uint writeString(ref ubyte[] buf, string str) {
    uint s = writeVarint(buf, cast(uint) str.length);
    uint i = 0;
    while (i < str.length) {
      buf[i] = str[i];
      i++;
    }

    buf = buf[i .. $];
    return s + i;
  }

  string readString(ref ubyte[] frame) {
    int sz = readVarint32Buf(frame);

    ubyte[] data = frame[0 .. sz];
    frame = frame[sz .. $];

    import std.conv;
    return data.to!string();
  }

  uint getMaxHeadersSize(string[string] headers) {
    if (headers.length == 0) {
      return 0;
    }

    uint len = 10; // 5 bytes varint for info header type
                   // 5 bytes varint for info headers count
    foreach (k, v; headers) {
      len += 10; // 5 bytes varint for key size and
                 // 5 bytes varint for value size
      len += k.length;
      len += v.length;
    }

    return len;
  }

  ubyte[] flushInfoHeaders(Info info, string[string] headers) {
    if (headers.length == 0) {
      return [];
    }

    import std.array;
    auto infoData = uninitializedArray!(ubyte[])(getMaxHeadersSize(headers));
    auto infoDataBuf = infoData;

    auto infoDataSize = writeVarint(infoDataBuf, info);
    infoDataSize += writeVarint(infoDataBuf, cast(uint) headers.length);

    foreach (k, v; headers) {
      infoDataSize += writeString(infoDataBuf, k);
      infoDataSize += writeString(infoDataBuf, v);
    }

    return infoData[0 .. infoDataSize];
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

  ubyte[] transform(ubyte[] data) {
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
  ushort flags = 0;

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
