/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <cmath>
#include <iostream>
#include <string>

#include <thrift/lib/cpp/test/gen-cpp/Paths_types.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>

int main(int argc, char **argv) {
  Path1 p1; // list of pairs, 5005 bytes
  Path2 p2; // pair of lists, 2009 bytes

  for (int i = 0; i < 1000; ++i) {
    int x = 60 * cos(i * 0.01);
    int y = 60 * sin(i * 0.01);
    Point p;
    p.x = x;
    p.y = y;
    p1.points.push_back(p);
    p2.xs.push_back(x);
    p2.ys.push_back(y);
  }

  apache::thrift::util::ThriftSerializerCompact<> serializer;
  std::string serialized;

  serializer.serialize(p1, &serialized);
  LOG(INFO) << "Path1: " << serialized.size() << " bytes";

  serializer.serialize(p2, &serialized);
  LOG(INFO) << "Path2 " << serialized.size() << " bytes";
}
