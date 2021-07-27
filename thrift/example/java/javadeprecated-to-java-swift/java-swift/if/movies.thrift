#!/usr/local/bin/thrift
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

namespace java.swift com.facebook.movies

exception MovieNotFoundException {
  1: string message;
}

struct MovieInfo {
  1: string title;
  2: i32 year;
}

struct MovieInfoRequest {
  1: string title;
}

service MoviesService {
  MovieInfo getMovieInfo(1: MovieInfoRequest request) throws (
    1: MovieNotFoundException e,
  );
}
