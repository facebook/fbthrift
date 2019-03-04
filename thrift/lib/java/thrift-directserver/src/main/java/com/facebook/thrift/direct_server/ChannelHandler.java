/**
 * Copyright 2011 Facebook
 *
 * <p>The interface that can be implemented as extentions to DirectServer. For example,
 * FramedTransportChannelHandler classes extends DirectServer to be a thrift server.
 *
 * @author Wei Chen (weichen@fb.com)
 */
package com.facebook.thrift.direct_server;

import java.io.IOException;
import java.nio.channels.SelectionKey;
import java.nio.channels.spi.AbstractSelectableChannel;

public interface ChannelHandler {
  SelectionKey key();

  AbstractSelectableChannel channel();

  void wouldUseThreadPool(boolean useThreadPool);

  boolean canUseThreadPool();

  void transition(SelectorThread s);

  void close() throws IOException;

  void markError();
}
