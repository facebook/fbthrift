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
module thrift.protocol.header;

import thrift.protocol.base;
import thrift.transport.base;
import thrift.transport.header;

/**
 * D implementation of the Header protocol.
 */
final class THeaderProtocol : TProtocol {

  alias Protocol = THeaderTransport.Protocol;

  this(TTransport trans) {
    trans_ = new THeaderTransport(trans);
    resetProtocol();
  }

  override {
    THeaderTransport transport() @property {
      return trans_;
    }

    void writeMessageBegin(TMessage message) {
      proto.writeMessageBegin(message);
    }

    void writeMessageEnd() {
      proto.writeMessageEnd();
    }

    void writeStructBegin(TStruct tstruct) {
      proto.writeStructBegin(tstruct);
    }

    void writeStructEnd() {
      proto.writeStructEnd();
    }

    void writeFieldBegin(TField field) {
      proto.writeFieldBegin(field);
    }

    void writeFieldEnd() {
      proto.writeFieldEnd();
    }

    void writeFieldStop() {
      proto.writeFieldStop();
    }

    void writeMapBegin(TMap map) {
      proto.writeMapBegin(map);
    }

    void writeMapEnd() {
      proto.writeMapEnd();
    }

    void writeListBegin(TList list) {
      proto.writeListBegin(list);
    }

    void writeListEnd() {
      proto.writeListEnd();
    }

    void writeSetBegin(TSet set) {
      proto.writeSetBegin(set);
    }

    void writeSetEnd() {
      proto.writeSetEnd();
    }

    void writeBool(bool b) {
      proto.writeBool(b);
    }

    void writeByte(byte b) {
      proto.writeByte(b);
    }

    void writeI16(short i16) {
      proto.writeI16(i16);
    }

    void writeI32(int i32) {
      proto.writeI32(i32);
    }

    void writeI64(long i64) {
      proto.writeI64(i64);
    }

    void writeDouble(double dub) {
      proto.writeDouble(dub);
    }

    void writeString(string str) {
      proto.writeString(str);
    }

    void writeBinary(ubyte[] buf) {
      proto.writeBinary(buf);
    }

    TMessage readMessageBegin() {
      // Read the next frame, and change protocols if needed
      try {
        trans_._resetProtocol();
        resetProtocol();
      } catch (THeaderException e) {
        // THeaderExceptions are exceptions we want thrown back to the endpoint
        // Such as unknown transforms or protocols. Endpoint may retry
        notifyEndpoint(e.toString());
      }
      return proto.readMessageBegin();
    }

    void readMessageEnd() {
      proto.readMessageEnd();
    }

    TStruct readStructBegin() {
      return proto.readStructBegin();
    }

    void readStructEnd() {
      proto.readStructEnd();
    }

    TField readFieldBegin() {
      return proto.readFieldBegin();
    }

    void readFieldEnd() {
      proto.readFieldEnd();
    }

    TMap readMapBegin() {
      return proto.readMapBegin();
    }

    void readMapEnd() {
      proto.readMapEnd();
    }

    TList readListBegin() {
      return proto.readListBegin();
    }

    void readListEnd() {
      proto.readListEnd();
    }

    TSet readSetBegin() {
      return proto.readSetBegin();
    }

    void readSetEnd() {
      proto.readSetEnd();
    }

    bool readBool() {
      return proto.readBool();
    }

    byte readByte() {
      return proto.readByte();
    }

    short readI16() {
      return proto.readI16();
    }

    int readI32() {
      return proto.readI32();
    }

    long readI64() {
      return proto.readI64();
    }

    double readDouble() {
      return proto.readDouble();
    }

    string readString() {
      return proto.readString();
    }

    ubyte[] readBinary() {
      return proto.readBinary();
    }

    void reset() {}
  }

  void resetProtocol() {
    // We guarantee trans_ to be of type THeaderTransport, parent class does not
    if (proto !is null && trans_.getProtocol() == protoId) {
      return;
    }

    protoId = trans_.getProtocol();
    switch (protoId) with(Protocol) {
      case Binary:
        import thrift.protocol.binary;
        proto = tBinaryProtocol(trans_, 0, 0, true, true);
        break;

      case Compact:
        import thrift.protocol.compact;
        proto = tCompactProtocol(trans_);
        break;

      default:
        import std.conv;
        throw new TProtocolException("Unknown protocol id: " ~ (cast(uint) protoId).to!string());
    }
  }

private:

  /**
   * Helper method to throw an error back to the endpoint
   */
  void notifyEndpoint(string msg) {
    if (proto is null) {
      return;
    }

    writeMessageBegin(TMessage("", TMessageType.EXCEPTION, 0));
    TApplicationException ex = new TApplicationException(msg);
    ex.write(this);
    writeMessageEnd();
    trans_.flush();
  }

  THeaderTransport trans_;

  TProtocol proto;
  Protocol protoId;

}
