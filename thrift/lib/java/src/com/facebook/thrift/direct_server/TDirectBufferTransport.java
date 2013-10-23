/**
 * Copyright 2011 Facebook
 * @author Wei Chen (weichen@fb.com)
 *
 * To achieve the full performance of java direct buffer, we need
 * some tweaks to thrift TTransport and TProtocol classes.
 *
 * Class DirectBufferTransport extends TTransport so that we can
 * take the full benefits of direct buffer.
 */

package com.facebook.thrift.direct_server;

import java.nio.BufferOverflowException;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;

import com.facebook.thrift.transport.TTransport;
import com.facebook.thrift.transport.TTransportException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


/**
 * A TTransport class that support java.nio's direct buffer.
 *
 * We are going to use a single TTransport object for a ChannelHandler.
 * The TTransport object is first used for reading from a socket,
 * and then switch the role to be an output transport object.
 *
 * We need a special TProtocol class which overrides readMessageEnd() to
 * flip the buffer for us.
 */
public class TDirectBufferTransport extends TTransport {

  private static Logger LOG =
      LoggerFactory.getLogger(TDirectBufferTransport.class);

  private static int defaultBytes_ = 1024 * 64;

  private int maxBytes_;

  private ByteBuffer buffer_;

  private ByteBuffer defaultBuffer_;

  private boolean isDirect_;


  /**
   * A class that manages a pool of DirectBufferTransport instances.
   */
  public static class Pool {
    private int idx_;
    private TDirectBufferTransport[] pool_;

    public Pool(int num) {
      // synchronize to assure safe publication of idx_ and pool_
      synchronized (this) {
        idx_ = num - 1;
        pool_ = new TDirectBufferTransport[num];
        for (int i = 0; i < num; ++i) {
          pool_[i] = new TDirectBufferTransport(true);
        }
      }
    }

    public TDirectBufferTransport getTransport() {
      synchronized(this) {
        if (idx_ >= 0) {
          TDirectBufferTransport r = pool_[idx_];
          pool_[idx_] = null;
          --idx_;
          return r;
        }
      }

      return new TDirectBufferTransport(false);
    }

    public void returnTransport(TDirectBufferTransport dbt) {
      dbt.cleanup();
      if (dbt.isDirect()) {
        synchronized(this) {
          ++idx_;
          pool_[idx_] = dbt;
        }
      }

      // Otherwise the transport will be reclaimed by GC.
    }

    public int getUsed() {
      return pool_.length - idx_ - 1;
    }
  };


  // Callers do not need to access constructor, use Pool to get an object.
  private TDirectBufferTransport(boolean useDirect) {
    maxBytes_ = defaultBytes_;
    if (useDirect) {
      defaultBuffer_ = ByteBuffer.allocateDirect(defaultBytes_);
    } else {
      defaultBuffer_ = ByteBuffer.allocate(defaultBytes_);
    }

    buffer_ = defaultBuffer_;
    isDirect_ = useDirect;
  }

  boolean isDirect() {
    return isDirect_;
  }

  // If the default size does not fit, call this to prepare a bigger buffer
  void setMaxBytes(int max) {
    if (maxBytes_ < max) {
      maxBytes_ = max;
      ByteBuffer b = ByteBuffer.allocate(max);
      buffer_.flip();
      b.put(buffer_);
      buffer_ = b;
    }
  }

  // Apps that needs a different default buffer size should call this.
  public static void setDefaultBytes(int bytes) {
    defaultBytes_ = bytes;
  }

  public ByteBuffer buffer() {
    return buffer_;
  }

  // Called when returning the object to pool to clean up oversized buffer
  void cleanup() {
    if (maxBytes_ > defaultBytes_) {
      buffer_ = defaultBuffer_;
      maxBytes_ = defaultBytes_;
    }
    buffer_.clear();
  }

  /**
   * Queries whether the transport is open.
   *
   * @return True if the transport is open.
   */
  public boolean isOpen() {
    return true;
  }

  /**
   * Opens the transport for reading/writing.
   *
   * @throws TTransportException if the transport could not be opened
   */
  public void open() throws TTransportException {
  }

  /**
   * Closes the transport.
   */
  public void close() {
  }

  /**
   * Reads up to len bytes into buffer buf, starting att offset off.
   *
   * @param buf Array to read into
   * @param off Index to start reading at
   * @param len Maximum number of bytes to read
   * @return The number of bytes actually read
   * @throws TTransportException if there was an error reading data
   */
  public int read(byte[] buf, int off, int len) throws TTransportException {
    try {
      buffer_.get(buf, off, len);
    } catch (BufferUnderflowException e) {
      LOG.warn("read buffer underflow", e);
      throw new TTransportException("Buffer underflow");
    }
    return len;
  }

  /**
   * Guarantees that all of len bytes are actually read off the transport.
   *
   * @param buf Array to read into
   * @param off Index to start reading at
   * @param len Maximum number of bytes to read
   * @return The number of bytes actually read, which must be equal to len
   * @throws TTransportException if there was an error reading data
   */
  public int readAll(byte[] buf, int off, int len) throws TTransportException {
    read(buf, off, len);
    return len;
  }

  /**
   * Writes up to len bytes from the buffer.
   *
   * @param buf The output data buffer
   * @param off The offset to start writing from
   * @param len The number of bytes to write
   * @throws TTransportException if there was an error writing data
   */
  public void write(byte[] buf, int off, int len)
    throws TTransportException {

    while (true) {
      try {
        buffer_.put(buf, off, len);
        return;
      } catch (BufferOverflowException e) {
        maxBytes_ = 2 * maxBytes_;
        ByteBuffer b = ByteBuffer.allocate(maxBytes_);
        buffer_.flip();
        b.put(buffer_);
        buffer_ = b;
        continue;
      } catch (Exception e) {
        throw new TTransportException(e.toString());
      }
    }
  }

  /**
   * Access the protocol's underlying buffer directly. If this is not a
   * buffered transport, return null.
   * @return
   */
  public byte[] getBuffer() {
    return null;
  }

  /**
   * Return the index within the underlying buffer that specifies the next spot
   * that should be read from.
   * @return
   */
  public int getBufferPosition() {
    LOG.warn("Unsupported method getBufferPosition()");
    return -1;
  }

  /**
   * Get the number of bytes remaining in the underlying buffer. Returns -1 if
   * this is a non-buffered transport.
   * @return
   */
  public int getBytesRemainingInBuffer() {
    return -1;
  }

  /**
   * Consume len bytes from the underlying buffer.
   * @param len
   */
  public void consumeBuffer(int len) {
    int pos = buffer_.position();
    buffer_.position(pos + len);
  }
};
