/**
 * Copyright 2011 Facebook
 * @author Wei Chen (weichen@fb.com)
 *
 * A generic java TCP socket server that mirrors C++ FiberServer.
 *
 * Fiber server is a C++ thrift server that is implemented directly
 * on top of epoll API. Since epoll APIs are thread safe, Fiber server
 * does not need additional lockings to ensure synchronization and
 * achieves very high throughput under load.
 *
 * The direct socket server consists of one single blocking server socket
 * thread, a number of selector threads, and a thread pool for dispatching
 * tasks.
 *
 * The server socket thread listens on incoming connections,
 * and dispatches the accepted connection to one of selector threads.
 *
 * Each of selector threads runs a selector loop. When a channel becomes
 * active, it can be handled either in the loop or by one of worker
 * threads in the thread pool.
 *
 * If all the channels' activities are handled by selector thread, it likes
 * a FiberServer; if most of them are handled by worker threads, it likes
 * a Java HsHaServer.
 *
 * Different servers can be created by instantiating different ChannelHandler
 * classes. For example, the class FramedTransportChannelHandler makes
 * a DirectServer a thrift server (i.e. TDirectServer), while a class
 * of HttpChannelHandler can make it a http server.
 *
 * Different ChannelHandlers can stack togather to form a mixed server.
 * For example, we can use FramedTransportChannelHandler alone with
 * a HttpChannelHandler to make a thrift server that also serves http
 * requests.
*/

package com.facebook.thrift.direct_server;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.SelectorProvider;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The base class of TDirectServer
 *
 * DirectServer is a TCP socket server. By implementing different ChannelHandler
 * classes, a DirectServer can be made into a thrift server, a web server,
 * or something else.
 */
public class DirectServer {

  public interface LoggingCallback {
    public void onAsyncDrop(Logger logger);
  }

  public static class DoNothing implements Runnable {
    @Override
    public void run() {
    }
  };

  private static final Logger LOG = LoggerFactory.getLogger(DirectServer.class);

  private volatile LoggingCallback loggingCallback =
      new DefaultLoggingCallback();

  // server's listening port
  private final int port_;

  private final int totalSelectorThreads_;

  // runnable which is intended to run when the server is ready to serve traffic
  private final Runnable onServerListens_;

  // parameter to listen().
  private volatile int backlog_;

  // The thread pool used by DirectServer
  private volatile ThreadPoolExecutor executorService_;

  // A factory object that decides how to handle channel events.
  private final ChannelHandlerFactory handlerCreator_;

  // How many synchronous handlers that a single selector thread can have.
  private final int numSyncHandlers_;

  // max number of pending connections that a single selector has to accept.
  private volatile int maxPendingPerSelector_;

  private ServerSocketChannel serverSocketChannel_;
  private Object socketChannelLock_ = new Object();
  private volatile boolean stopping_ = false;

  public DirectServer(int port, ChannelHandlerFactory creator) {
    port_ = port;
    totalSelectorThreads_ = 4;
    backlog_ = 10;
    handlerCreator_ = creator;
    numSyncHandlers_ = 1;

    executorService_ = new ThreadPoolExecutor(
        8, 8, 0, TimeUnit.SECONDS,
        new ArrayBlockingQueue<Runnable>(4),
        new ThreadPoolExecutor.AbortPolicy());

    maxPendingPerSelector_ = 64;
    onServerListens_ = new DoNothing();
  }

  public DirectServer(
    int port,
    ChannelHandlerFactory creator,
    int numSelectors,
    int numSyncHandlers,
    ThreadPoolExecutor service,
    Runnable onServerListens) {

    port_ = port;
    totalSelectorThreads_ = numSelectors;
    backlog_ = 10;
    handlerCreator_ = creator;
    numSyncHandlers_ = numSyncHandlers;

    executorService_ = service;
    maxPendingPerSelector_ = 64;
    onServerListens_ = onServerListens;
  }

  /**
   * Set listening socket's backlog number.
   * Must be called before serve() is called.
   *
   * @param b backlog size of listening queue.
   */
  public void serverSocketBacklog(int b) {
    backlog_ = b;
  }

  /**
   * Set max number of pending incoming connections for a selector.
   * If the pending incoming connections are more than this number,
   * the selector thread will close the oldest pending one.
   *
   * Note that this is different from the max number of connections
   * that a selector thread can handle.
   *
   * @param num max number of incoming connections.
   */
  public void setMaxPendingPerSelector(int num) {
    maxPendingPerSelector_ = num;
  }

  /**
   * Allow users to use their own favorite service executor.
   * Must be called before serve() to take effect.
   *
   * @param service the service executor that caller wants to install.
   */
  public void setExecutor(ThreadPoolExecutor service) {
    executorService_ = service;
  }


  // Selector thread will calls this.
  int maxSyncHandlers() {
    return numSyncHandlers_;
  }

  public LoggingCallback getLoggingCallback() {
    return loggingCallback;
  }

  public void setLoggingCallback(LoggingCallback loggingCallback) {
    this.loggingCallback = loggingCallback;
  }

  public int getLocalPort() {
    synchronized (socketChannelLock_) {
      if (serverSocketChannel_ == null || !serverSocketChannel_.socket().isBound()) {
        throw new IllegalStateException("Server is not yet bound to a local port");
      }
      return serverSocketChannel_.socket().getLocalPort();
    }
  }

  // Now serving, will be blocked until server die.
  public void serve() {
    if (totalSelectorThreads_ <= 0) {
      LOG.warn("totalSelectorThreads_ is not set correctly!");
      return;
    }

    LOG.info("Selector provider is " + SelectorProvider.provider().toString());

    // Start selector threads
    SelectorThread[] threads = new SelectorThread[totalSelectorThreads_];
    for (int i = 0; i < totalSelectorThreads_; ++i) {
      threads[i] = new SelectorThread(
          this, executorService_, maxPendingPerSelector_);
      threads[i].start();
    }

    // Prepare listening socket
    try {
      synchronized (socketChannelLock_) {
        serverSocketChannel_ = ServerSocketChannel.open();
        serverSocketChannel_.socket().setReuseAddress(true);
        InetSocketAddress addr = new InetSocketAddress(port_);
        serverSocketChannel_.socket().bind(addr, backlog_);
      }
    } catch (IOException e) {
      LOG.warn("Error in serving", e);
      return;
    } catch (Exception e) {
      LOG.error("Unexpected exception", e);
      return;
    }

    (new Thread(onServerListens_)).start();

    // An index to selector threads. Titan server uses this to dispatch
    // incoming connection in a round robin fashion.
    int nextThreadIdx = 0;
    // main serving loop.
    while (true) {
      try {
        SocketChannel result = null;
        if (!stopping_) {
          result = serverSocketChannel_.accept();
        } else {
          try {
            serverSocketChannel_.close();
          } catch (Exception e) {
            LOG.error("stop triggerred exception", e);
          }
          break;
        }

        result.configureBlocking(false);
        result.socket().setSoLinger(false, 0);
        result.socket().setTcpNoDelay(true);

        int idx = nextThreadIdx++;
        if (nextThreadIdx >= threads.length) {
          nextThreadIdx = 0;
        }

        SelectorThread thd = threads[idx];
        ChannelHandler handler = handlerCreator_.newChannelHandler(result, thd);

        thd.addHandler(handler);

      } catch (IOException e) {
        LOG.warn("Error in serving", e);
        continue;
      } catch (Exception e) {
        LOG.error("Unexpected exception", e);
        break;
      }
    }

    for (int i = 0; i < threads.length; ++i) {
      threads[i].Stop();
    }

    try {
      Thread.sleep(2000);
    } catch (InterruptedException e) {
    }
  }

  public void stop() {
    stopping_ = true;
  }
};
