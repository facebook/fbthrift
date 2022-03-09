/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.meta.thrift.example.ping.server;

import com.facebook.swift.service.ThriftServerConfig;
import com.facebook.thrift.example.ping.PingService;
import com.facebook.thrift.example.ping.PingServiceRpcServerHandler;
import com.facebook.thrift.legacy.server.LegacyServerTransport;
import com.facebook.thrift.legacy.server.LegacyServerTransportFactory;
import java.net.SocketAddress;
import java.util.Collections;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PingServer {
  private static final Logger LOG = LoggerFactory.getLogger(PingServer.class);

  private static PingServerConfig parseArgs(String[] args) {
    Options options = new Options();

    Option portOption =
        Option.builder("p")
            .longOpt("port")
            .desc("port of PingService server (default to 7777)")
            .type(Integer.class)
            .hasArg(true)
            .numberOfArgs(1)
            .required(false)
            .build();
    options.addOption(portOption);

    Option transportOption =
        Option.builder("t")
            .longOpt("transport")
            .desc("thrift transport (header or rsocket, default to header)")
            .type(String.class)
            .hasArg(true)
            .numberOfArgs(1)
            .required(false)
            .build();
    options.addOption(transportOption);

    CommandLineParser parser = new DefaultParser();
    HelpFormatter formatter = new HelpFormatter();
    CommandLine cmd;

    try {
      cmd = parser.parse(options, args);
    } catch (ParseException e) {
      LOG.error("Error parsing args", e);
      formatter.printHelp("scribe_cat", options);

      System.exit(1);
      return null;
    }

    PingServerConfig.Builder configBuilder = new PingServerConfig.Builder();
    if (cmd.hasOption("port")) {
      configBuilder.setPort(Integer.parseInt(cmd.getOptionValue("port")));
    }
    if (cmd.hasOption("transport")) {
      configBuilder.setTransport(cmd.getOptionValue("transport"));
    }

    return configBuilder.build();
  }

  public static void main(String[] args) {
    PingServerConfig config = parseArgs(args);

    PingService pingService = new PingImpl();

    PingServiceRpcServerHandler serverHandler =
        new PingServiceRpcServerHandler(pingService, Collections.emptyList());

    LOG.info("starting server");
    if ("header".equals(config.getTransport())) {
      LegacyServerTransportFactory transportFactory =
          new LegacyServerTransportFactory(new ThriftServerConfig().setPort(config.getPort()));
      LegacyServerTransport transport =
          transportFactory.createServerTransport(serverHandler).block();
      SocketAddress address = transport.getAddress();

      LOG.info("server started at -> " + address.toString());
      transport.onClose().block();
    } else {
      throw new UnsupportedOperationException("Need to Support RSocket");
    }
  }
}
