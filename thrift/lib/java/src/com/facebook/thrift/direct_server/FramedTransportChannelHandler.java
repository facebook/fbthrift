
package com.facebook.thrift.direct_server;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.SocketChannel;

import com.facebook.thrift.TProcessor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

// Non Thread Safe
public class FramedTransportChannelHandler implements ChannelHandler {

  private static final Logger LOG =
      LoggerFactory.getLogger(FramedTransportChannelHandler.class);

  private final SocketChannel chan_;

  private final SelectorThread thd_;

  private final TDirectBufferTransport.Pool pool_;

  private TDirectBufferTransport trans_;

  private int status_;

  private int frameSize_;

  private final TProcessor processor_;

  private SelectionKey key_;

  private boolean wouldUseThreadPool_;

  // Defining channel status.
  private static final int STATUS_ERR = -1;
  private static final int STATUS_INIT = 0;
  private static final int STATUS_READING_SIZE = 1;
  private static final int STATUS_READING_PAYLOAD = 2;
  private static final int STATUS_PROCESSING = 3;
  private static final int STATUS_WRITING = 4;

  static final int FRAME_HEADER_SIZE = 4;

  private TDirectBufferProtocol proto_;


  FramedTransportChannelHandler(
    SocketChannel c,
    SelectorThread th,
    TDirectBufferTransport.Pool p,
    TProcessor tproc) {

    chan_ = c;
    thd_ = th;
    pool_ = p;
    status_ = STATUS_INIT;
    frameSize_ = 0;
    key_ = null;
    wouldUseThreadPool_ = false;
    processor_ = tproc;
  }

  @Override public SocketChannel channel() {
    return chan_;
  }

  @Override public SelectionKey key() {
    if (key_ == null) {
      try {
        key_ = chan_.keyFor(thd_.selector());
      } catch (Exception e) {
        LOG.error("Fails to get key", e);
        key_ = null;
      }
    }

    return key_;
  }

  @Override public boolean canUseThreadPool() {
    return status_ == STATUS_PROCESSING;
  }

  @Override public void wouldUseThreadPool(boolean useThreadPool) {
    wouldUseThreadPool_ = useThreadPool;
  }

  // Plan to close the handler, next transition() call will clean it up.
  @Override public void markError() {
    status_ = STATUS_ERR;
  }

  @Override public void transition(SelectorThread s) {
    if (trans_ == null) {
      trans_ = pool_.getTransport();
      proto_ = new TDirectBufferProtocol(trans_);
    }
    while (true) {
      switch (status_) {
      case STATUS_INIT:
      try {
        trans_.buffer().clear();
        trans_.buffer().limit(FRAME_HEADER_SIZE);
        key().interestOps(SelectionKey.OP_READ);
        status_ = STATUS_READING_SIZE;
      } catch (Exception e) {
        LOG.error("Fails to set interested ops", e);
        status_ = STATUS_ERR;
        break;
      }

      case STATUS_READING_SIZE:
      try {
        ByteBuffer buffer = trans_.buffer();
        int nreads = chan_.read(buffer);
        if (nreads < 0) {
          // Probably the channel was closed.
          status_ = STATUS_ERR;
          break;
        } else if (buffer.position() == FRAME_HEADER_SIZE) {
          status_ = STATUS_READING_PAYLOAD;
          int pos = buffer.position();
          buffer.flip();
          frameSize_ = buffer.getInt();

          if (frameSize_ < 0) {
            status_ = STATUS_ERR;
            LOG.error("Negative frame size " + frameSize_ +
                " probably implies wrong buffer format");
            break;
          }

          buffer.position(pos);
          trans_.setMaxBytes(frameSize_ + FRAME_HEADER_SIZE);
          buffer = trans_.buffer();
          buffer.limit(frameSize_ + FRAME_HEADER_SIZE);
          continue;
        } else if (buffer.position() > FRAME_HEADER_SIZE) {
          LOG.error("position is too big " + buffer.position());
          status_ = STATUS_ERR;
          continue;
        } else if (nreads == 0 || buffer.position() < FRAME_HEADER_SIZE) {
          return;
        } else {
          LOG.error("Something seriously wrong in reading header " + nreads);
          status_ = STATUS_ERR;
          break;
        }
      } catch (IOException e) {
        // LOG.warn("Having exception", e);
        status_ = STATUS_ERR;
        break;
      } catch (Exception e) {
        LOG.error("Having unexpected exception", e);
        status_ = STATUS_ERR;
        break;
      }

      case STATUS_READING_PAYLOAD:
      try {
        ByteBuffer buffer = trans_.buffer();
        int nreads = chan_.read(buffer);
        if (buffer.position() == frameSize_ + FRAME_HEADER_SIZE) {
          key().interestOps(0);
          status_ = STATUS_PROCESSING;
          trans_.buffer().position(FRAME_HEADER_SIZE);
          if (!wouldUseThreadPool_) {
            break;
          } else {
            return;
          }
        } else if (nreads < 0) {
          status_ = STATUS_ERR;
          break;
        } else if (buffer.position() < frameSize_ + FRAME_HEADER_SIZE) {
          return;
        } else {
          LOG.warn("Something seriously wrong in reading paylaod");
          status_ = STATUS_ERR;
          break;
        }
      } catch (IOException e) {
        LOG.warn("Having exception", e);
        status_ = STATUS_ERR;
        break;
      } catch (Exception e) {
        LOG.error("Having unexpected exception", e);
        status_ = STATUS_ERR;
        break;
      }

      case STATUS_PROCESSING:
      try {
        processor_.process(proto_, proto_,
            new DirectTConnectionContext(chan_, proto_));
      } catch (Exception e) {
        LOG.warn("exception while processing", e);
      } finally {
        status_ = STATUS_WRITING;
        trans_.buffer().flip();
        if (wouldUseThreadPool_) {
          return;
        }
      }

      case STATUS_WRITING:
      try {
        ByteBuffer buffer = trans_.buffer();
        int nwritten = chan_.write(buffer);
        if (nwritten < 0) {
          status_ = STATUS_ERR;
          break;
        } else if (buffer.position() == buffer.limit()) {
          status_ = STATUS_INIT;
          continue;
        } else if (key().interestOps() != SelectionKey.OP_WRITE) {
          key().interestOps(SelectionKey.OP_WRITE);
          return;
        } else {
          // It is possible that we cannot write the entire outgoing
          // payload to wire in a single call. In this case,
          // wait until underlying socket becomes ready for write again.
          return;
        }
      } catch (Exception e) {
        LOG.warn("Fails to write", e);
        status_ =STATUS_ERR;
        break;
      }

      case STATUS_ERR:
      try {
        SelectionKey key = key();
        if (key != null) {
          key.cancel();
          close();
        } else {
          LOG.warn("Fails to get SelectionKey!");
        }
      } catch (Exception e) {
        LOG.warn("Fail to handle error case:", e);
      }
      return;

      default:
        LOG.warn("Undefined status " + status_);
        break;
      }
    }
  }

  @Override
  public void close() throws IOException {
    if (trans_ != null) {
      pool_.returnTransport(trans_);
    }
    chan_.close();
  }

};
