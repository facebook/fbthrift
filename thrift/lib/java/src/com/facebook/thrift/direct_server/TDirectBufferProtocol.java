/**
 * Copyright 2011 Facebook
 * @author Wei Chen (weichen@fb.com)
 *
 * To achieve the full performance of java direct buffer, we need
 * some tweaks to thrift TTransport and TProtocol classes.
 *
 * Class TDirectBufferProtocol extends TBinaryBufferProtocol class
 * so that we can take the full benefits of direct buffer.
 */

package com.facebook.thrift.direct_server;

import java.nio.BufferOverflowException;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.facebook.thrift.TException;
import com.facebook.thrift.protocol.TBinaryProtocol;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


public class TDirectBufferProtocol extends TBinaryProtocol {
  static private Logger LOG =
      LoggerFactory.getLogger(TDirectBufferProtocol.class);


  TDirectBufferProtocol(TDirectBufferTransport t) {
    super(t);
    t.buffer().order(ByteOrder.BIG_ENDIAN);
  }

  long inputSize() {
    TDirectBufferTransport t = (TDirectBufferTransport)trans_;
    return t.buffer().position();
  }

  long outputSize() {
    TDirectBufferTransport t = (TDirectBufferTransport)trans_;
    return t.buffer().position();
  }

  // Flip the buffer for write.
  @Override public void readMessageEnd() {
    TDirectBufferTransport t = (TDirectBufferTransport)trans_;
    t.buffer().clear();
    t.buffer().position(FramedTransportChannelHandler.FRAME_HEADER_SIZE);
  }

  @Override public void writeMessageEnd() {
    ByteBuffer b = ((TDirectBufferTransport)trans_).buffer();
    int pos = b.position();
    if (pos < FramedTransportChannelHandler.FRAME_HEADER_SIZE) {
      LOG.warn("Write message too small " + Integer.toString(pos));
    } else {
      b.position(0);
      b.putInt(pos - FramedTransportChannelHandler.FRAME_HEADER_SIZE);
      b.position(pos);
    }
  }

  @Override public byte readByte() throws TException {
    try {
      return ((TDirectBufferTransport)trans_).buffer().get();
    } catch (BufferUnderflowException e) {
      return super.readByte();
    }
  }

  @Override public void writeByte(byte b) throws TException {
    try {
      ((TDirectBufferTransport)trans_).buffer().put(b);
    } catch (BufferOverflowException e) {
      super.writeByte(b);
    }
  }

  @Override public void writeI32(int i32) throws TException {
    try {
      ((TDirectBufferTransport)trans_).buffer().putInt(i32);
    } catch (BufferOverflowException e) {
      super.writeI32(i32);
    }
  }

  @Override public int readI32() throws TException {
    try {
      return ((TDirectBufferTransport)trans_).buffer().getInt();
    } catch (BufferUnderflowException e) {
      return super.readI32();
    }
  }
};
