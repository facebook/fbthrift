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

import java.util.HashMap;
import java.util.Map;

public class MoviesServiceHandler implements MoviesService {
  private Map<String, MovieInfo> database;

  public MoviesServiceHandler() {
    database = new HashMap<>();
    MovieInfo movieInfo = new MovieInfo("Life is beautiful", 1997);
    database.put(movieInfo.getTitle(), movieInfo);
  }

  @Override
  public MovieInfo getMovieInfo(MovieInfoRequest request) throws MovieNotFoundException {
    MovieInfo info = database.get(request.getTitle());
    if (info != null) return info;
    throw new MovieNotFoundException(request.getTitle());
  }

  @Override
  public void close() {}
}
