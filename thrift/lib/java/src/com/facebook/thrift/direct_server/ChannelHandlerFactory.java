/**
 * Copyright 2011 Facebook
 * @author Wei Chen (weichen@fb.com)
 *
 * A interface to provide ChannelHandlers to DirectServer.
 * Extentions of DirectServer uses this class to create ChannelHandler
 * instances of their choice.
 */

package com.facebook.thrift.direct_server;

import java.nio.channels.SocketChannel;

public interface ChannelHandlerFactory {
  ChannelHandler newChannelHandler(SocketChannel ch, SelectorThread th);
}
