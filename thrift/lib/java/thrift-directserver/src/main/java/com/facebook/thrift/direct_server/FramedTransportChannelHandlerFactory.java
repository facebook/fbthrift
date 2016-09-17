
package com.facebook.thrift.direct_server;

import java.nio.channels.SocketChannel;


public class FramedTransportChannelHandlerFactory
  implements ChannelHandlerFactory {

  private static volatile int numPooledTransports_ = 16;

  private static volatile TDirectBufferTransport.Pool pool_ = null;

  private final TDirectServer server_;


  public FramedTransportChannelHandlerFactory(TDirectServer s) {
    synchronized (FramedTransportChannelHandlerFactory.class) {
      if (pool_ == null) {
        pool_ = new TDirectBufferTransport.Pool(numPooledTransports_);
      }
    }
    server_ = s;
  }

  /**
   * Adjust the number of pooled transports. This should be called before any of
   * the handler factory is created.
   *
   * @param num
   *          the new number of pooled transports.
   */
  public static void numPooledTransports(int num) {
    numPooledTransports_ = num;
  }

  public static TDirectBufferTransport.Pool getPool() {
    return pool_;
  }

  /**
   * Create a new channel handler instance.
   *
   * @param ch
   *          the socket channel that the handler is to work on
   * @param th
   *          the selector thread that manages this handler.
   * @return a new channel handler instance.
   */
  @Override public ChannelHandler newChannelHandler(
    SocketChannel ch, SelectorThread th) {
    return new FramedTransportChannelHandler(
        ch, th, pool_, server_.getProcessor());
  }
};
