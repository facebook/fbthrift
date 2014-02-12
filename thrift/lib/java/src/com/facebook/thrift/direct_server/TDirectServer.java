/**
 * Copyright 2011 Facebook
 * @author Wei Chen (weichen@fb.com)
 *
 * A thrift server class that is built on top of DirectServer.
 */

package com.facebook.thrift.direct_server;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import com.facebook.thrift.TProcessor;
import com.facebook.thrift.TProcessorFactory;
import com.facebook.thrift.protocol.TProtocolFactory;
import com.facebook.thrift.server.TServer;
import com.facebook.thrift.transport.TServerTransport;
import com.facebook.thrift.transport.TTransportFactory;


public class TDirectServer extends TServer {

  private final DirectServer directServer_;


  /**
   * Create a TDirectServer instance.
   *
   * Only call this directly if you know what you're doing, otherwise, call
   * either asFiberServer() or asHsHaServer() to get hold of an instance.
   *
   * @param port The server port number
   * @param numSelectors How many selector threads the server needs
   * @param numThreads How many worker threads in the thread pool
   * @param maxPending The max number of tasks in thread pool task queue
   * @param p The TProcessor instance
   * @param onServerListens Runnable to be run when server is listening on port
   */
  public TDirectServer(
    int port,
    int numSelectors,
    int numThreads,
    int maxPending,
    int numSyncHandlers,
    TProcessor p,
    Runnable onServerListens) {

    super(new TProcessorFactory(p), (TServerTransport)null,
          (TTransportFactory)null, (TProtocolFactory)null);

    ThreadPoolExecutor executorService = null;
    if (numThreads > 0) {
        executorService = new ThreadPoolExecutor(
            numThreads, numThreads, 0, TimeUnit.SECONDS,
            new ArrayBlockingQueue<Runnable>(maxPending),
            new ThreadPoolExecutor.AbortPolicy());
    }

    directServer_ = new DirectServer(
        port, new FramedTransportChannelHandlerFactory(this),
        numSelectors, numSyncHandlers, executorService, onServerListens);
  }

  /**
   * Helper constructor to create a TDirectServer that do not need an extra
   * hook to know when the TDirectServer is ready to serve traffic
   */
  public TDirectServer(
    int port,
    int numSelectors,
    int numThreads,
    int maxPending,
    int numSyncHandlers,
    TProcessor p) {

    this(port, numSelectors, numThreads, maxPending, numSyncHandlers,
      p, new DirectServer.DoNothing());
  }

  /**
   * Create a TDirectServer in fiber server mode.
   *
   * Fiber server mode is for services that requires high throughput
   * and that each request has very short latency.
   *
   * @param port the port the server is listening on
   * @param numSelectors the number of selector threads
   * @param p the processor instance
   * @return a TDirectServer instance in fiber server mode
   */
  static public final TDirectServer asFiberServer(
      int port, int numSelectors, TProcessor p) {
    return new TDirectServer(port, numSelectors, 0, 0, 64, p);
  }

  /**
   * Create a TDirectServer in HsHaServer mode
   *
   * HsHaServer mode is for services that individual requests can take
   * long time to be served, so that we should not handle requests
   * in selector thread.
   *
   * @param port the port the server is listening on
   * @param numThreads the number of threads in the thread pool
   * @param maxPending the max pending tasks in task queue
   * @param p the processor instance
   * @return a TDirectServer instance in HsHaServer mode
   */
  static public final TDirectServer asHsHaServer(
      int port, int numThreads, int maxPending, TProcessor p) {
    // We have a listener thread, plus 3 selector threads.
    return new TDirectServer(port, 3, numThreads, maxPending, 0, p);
  }

  // Expose DirectServer so that callers can tune parameters
  public DirectServer directServer() {
    return directServer_;
  }

  TProcessor getProcessor() {
    return processorFactory_.getProcessor(null);
  }

  // Main serving loop, will block until the server quit.
  @Override public void serve() {
    directServer_.serve();
  }

  @Override
  public void stop() {
    directServer_.stop();
  }
};
