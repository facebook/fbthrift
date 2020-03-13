/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

package com.facebook.movies;

import com.facebook.nifty.client.NiftyClientChannel;
import com.facebook.nifty.client.NiftyClientConnector;
import com.facebook.nifty.header.client.HeaderClientConnector;
import com.facebook.swift.service.ThriftClientManager;
import com.google.common.net.HostAndPort;
import org.apache.thrift.TException;

public class MoviesClient {
  public static final int PORT = 7777;

  public static void main(String[] args) throws Exception {
    NiftyClientConnector<? extends NiftyClientChannel> connector =
        new HeaderClientConnector(HostAndPort.fromParts("localhost", 7777));

    try (ThriftClientManager clientManager = new ThriftClientManager();
        MoviesService client = clientManager.createClient(connector, MoviesService.class).get()) {
      perform(client);
    }
  }

  private static void perform(MoviesService client) throws TException {
    final String goodTitle = "Life is beautiful";
    final String badTitle = "Dummy movie name";

    MovieInfoRequest movieInfoRequest = new MovieInfoRequest(goodTitle);

    try {
      MovieInfo movieInfo = client.getMovieInfo(movieInfoRequest);
      System.out.println("Good. \"" + goodTitle + "\" found in " + movieInfo.getYear() + ".");
    } catch (MovieNotFoundException e) {
      System.out.println("ERROR! \"" + goodTitle + "\" not found.");
    }

    try {
      client.getMovieInfo(new MovieInfoRequest(badTitle));
      System.out.println("ERROR! \"" + badTitle + "\" found.");
    } catch (MovieNotFoundException e) {
      System.out.println("Good. \"" + badTitle + "\" not found.");
    }
  }
}
