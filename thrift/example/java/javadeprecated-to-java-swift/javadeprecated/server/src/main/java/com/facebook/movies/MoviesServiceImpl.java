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

import com.facebook.nifty.core.NettyServerTransport;
import com.facebook.nifty.core.ThriftServerDef;
import com.facebook.nifty.core.fbcode.FacebookThriftServerDefBuilder;
import com.facebook.thrift.TProcessor;
import java.util.Map;
import java.util.concurrent.CountDownLatch;

public class MoviesServiceImpl implements MoviesService.Iface {
  private final int PORT = 7777;
  private Map<String, MovieInfo> database;
  private final CountDownLatch shutdownSignal = new CountDownLatch(1);

  public MoviesServiceImpl() {
    database = new java.util.HashMap<>();
    MovieInfo movieInfo = new MovieInfo("Life is beautiful", 1997);
    database.put(movieInfo.getTitle(), movieInfo);
  }

  @Override
  public MovieInfo getMovieInfo(MovieInfoRequest request) throws MovieNotFoundException {
    MovieInfo info = database.get(request.getTitle());
    if (info != null) return info;
    throw new MovieNotFoundException(request.getTitle());
  }

  private void run() throws InterruptedException {
    TProcessor processor = new MoviesService.Processor(this);
    ThriftServerDef def =
        new FacebookThriftServerDefBuilder().withProcessor(processor).listen(PORT).build();
    NettyServerTransport server = new NettyServerTransport(def);

    try {
      server.start();
      shutdownSignal.await();
    } finally {
      server.stop();
    }
  }

  public static void main(String[] args) throws Exception {
    MoviesServiceImpl moviesService = new MoviesServiceImpl();
    moviesService.run();
  }
}
