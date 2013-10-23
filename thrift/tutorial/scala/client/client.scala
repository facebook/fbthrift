// Copyright 2011-present Facebook. All Rights Reserved.

package com.facebook.swift.scalaexample.client

import com.facebook.swift.service._
import com.facebook.nifty.client._
import com.google.common.net.HostAndPort.fromParts

@ThriftService
trait PingService {
  @ThriftMethod
  def ping(): String
}

object client {
  def main(args: Array[String]) {
    val clientManager = new ThriftClientManager()
    val client = new ThriftClient[PingService](clientManager, classOf[PingService])
    val connector = new FramedClientConnector(fromParts("localhost", 4567))
    val pingService = client.open(connector).get()

    println(pingService.ping())
  }
}
