

package com.facebook.thrift.direct_server;

import java.io.IOException;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.spi.AbstractSelectableChannel;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ThreadPoolExecutor;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SelectorThread extends Thread {

  private static Logger LOG =
      LoggerFactory.getLogger(SelectorThread.class);

  // Max number of handlers that can be handled in the selector thread
  // on a single select() invocation
  private final int numSyncHandlers_;

  private final DirectServer directServer;

  private volatile Selector selector_;

  // List of finished asynchronous tasks.
  private List<DirectServerTask> pendingTasks_;

  // The max number of connections that a single select can handle
  private final static int NUM_HANDLERS = 512;

  private final ThreadPoolExecutor executorService_;

  // A handler passed from DirectServer
  private final ChannelHandler[] newHandlers_;

  private volatile boolean stopping_ = false;

  SelectorThread(
      DirectServer s,
      ThreadPoolExecutor es,
      int numPendingChannelHandlers) {

    try {
      // TODO remove IO operation out of Ctor
      selector_ = Selector.open();
    } catch (IOException e) {
      LOG.warn("in constructor get", e);
    }

    directServer = s;
    numSyncHandlers_ = s.maxSyncHandlers();
    executorService_ = es;

    newHandlers_ = new ChannelHandler[numPendingChannelHandlers];
  }

  public Selector selector() {
    return selector_;
  }

  public DirectServer.LoggingCallback getLoggingCallback() {
    return directServer.getLoggingCallback();
  }

  // The thread pool worker calls this after the async task is done.
  void notifyExecutorServiceTaskComplete(DirectServerTask t) {
    synchronized(this) {
      if (pendingTasks_ == null) {
        pendingTasks_ = new ArrayList<DirectServerTask>();
      }
      pendingTasks_.add(t);
    }

    selector_.wakeup();
  }

  void addHandler(ChannelHandler h) {
    ChannelHandler discarding = null;
    boolean needNotify = false;

    synchronized(newHandlers_) {
      for (int i = 0; i < newHandlers_.length; ++i) {
        if (newHandlers_[i] != null) {
          continue;
        }
        newHandlers_[i] = h;
        h = null;
        if (i == 0) {
          needNotify = true;
        }
        break;
      }

      // shift newHandlers_ if it is full.
      if (h != null) {
        discarding = newHandlers_[0];
        for (int i = 1; i < newHandlers_.length; ++i) {
          newHandlers_[i - 1] = newHandlers_[i];
        }
        newHandlers_[newHandlers_.length - 1] = h;
      }
    }

    if (discarding == null) {
      if (needNotify) {
        selector_.wakeup();
      }
      return;
    }

    LOG.warn("Discard a pending connection");
    try {
      discarding.close();
    } catch (Exception e) {
      LOG.warn("Closing a channel:", e);
    }
  }

  @Override public void run() {
    ChannelHandler[] handlers = new ChannelHandler[NUM_HANDLERS];

    while (true) {
      try {
        selector_.select();
      } catch (IOException e) {
        LOG.warn("continue with exception", e);
        continue;
      } catch (Exception e) {
        LOG.error("aborting with exception ", e);
        break;
      }

      int numReqs = 0;
      boolean stopping = stopping_;

      // Check if we have any new connections
      synchronized(newHandlers_) {
        for (int i = 0; i < newHandlers_.length; ++i) {
          if (newHandlers_[i] == null) {
            break;
          }

          handlers[numReqs] = newHandlers_[i];
          newHandlers_[i] = null;
          ++numReqs;
        }
      }

      for (int i = 0; i < numReqs; ++i) {
        AbstractSelectableChannel result = handlers[i].channel();
        if (!stopping) {
          try {
            result.register(selector_, SelectionKey.OP_READ, handlers[i]);
          } catch (Exception e) {
            LOG.warn("Fails to register a channel", e);
          }
        } else {
          try {
            handlers[i].close();
            handlers[i] = null;
          } catch (Exception e) {
            LOG.warn("Fails to close a connection", e);
          }
        }
      }

      if (stopping) {
        numReqs = 0;
      }

      // Examin new active connections
      for (Iterator<SelectionKey> it = selector_.selectedKeys().iterator(); it
          .hasNext();) {
        SelectionKey key = it.next();

        // Remove from selected keys set to indicate that it is processed.
        it.remove();

        if (numReqs < NUM_HANDLERS && !stopping) {
          handlers[numReqs] = (ChannelHandler)key.attachment();
          ++numReqs;
        } else {
          // Too many active connections, close some of them.
          ChannelHandler handler = (ChannelHandler)key.attachment();
          key.cancel();
          try {
            handler.close();
          } catch (Exception e) {
            LOG.warn("Fails to close a connection", e);
          }
          LOG.info("Closed a connection to reduce load");
        }
      }


      // If we have too many connections, move some of them to thread pool.
      for (int i = numSyncHandlers_; i < numReqs; ++i) {
        ChannelHandler handler = handlers[i];
        handlers[i] = null;
        if (executorService_ == null) {
          handler.transition(this);
        } else {
          handler.wouldUseThreadPool(true);
          if (!handler.canUseThreadPool()) {
            handler.transition(this);
          }
          if (handler.canUseThreadPool()) {
            new DirectServerTask(this, handler).executingBy(
                executorService_);
          }
        }
      }

      // Handle synchronous requests
      int numSyncReqs = numReqs > numSyncHandlers_ ? numSyncHandlers_ : numReqs;
      for (int i = 0; i < numSyncReqs; ++i) {
        handlers[i].transition(this);
        handlers[i] = null;
      }

      // Processing finished asynchronous tasks.
      if (executorService_ != null) {
        List<DirectServerTask> tasks = null;
        synchronized(this) {
          if (pendingTasks_ != null && pendingTasks_.size() > 0) {
            tasks = pendingTasks_;
            pendingTasks_ = null;
          }
        }

        if (tasks != null) {
          for (DirectServerTask it : tasks) {
            it.handler().transition(this);
          }
        }
      }

      // Back to the beginning of a infinite loop.
    }
  }

  public void Stop() {
    stopping_ = true;
  }
};
