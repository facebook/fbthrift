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

#include <iostream>

#include <folly/init/Init.h>

#include <thrift/lib/cpp/test/gen-cpp/DebugProtocolTest_types.h>
#include <thrift/lib/cpp/protocol/TDebugProtocol.h>

using apache::thrift::ThriftDebugString;

int main(int argc, char **argv) {
  folly::init(&argc, &argv);

  Message m;
  const Message cm;
  MyException e;
  const MyException ce;

  LOG(INFO) << m;
  LOG(INFO) << cm;
  LOG(INFO) << e;
  LOG(INFO) << ce;

  Ooo o;
  o.l.push_back(m);
  o.l.push_back(m);
  o.m["aa"] = m;
  o.m["bbb"] = m;

  LOG(INFO) << ThriftDebugString(o);

  MsgList l;
  l.push_back(m);
  l.push_back(m);

  LOG(INFO) << "A list of structs";
  LOG(INFO) << ThriftDebugString(l);

  LOG(INFO) << "A list of ints";
  IntList il;
  il.push_back(12345678);
  il.push_back(9876543234567);
  LOG(INFO) << ThriftDebugString(il);

  LOG(INFO) << "A list of strings";
  StringList sl;
  sl.push_back("skdskdjskdj");
  sl.push_back("sdhjksdjksdj");
  LOG(INFO) << ThriftDebugString(sl);

  MsgMap mmap;
  mmap["12121"] = m;
  mmap["dkjhdkw"] = m;
  LOG(INFO) << "A map of messages";
  LOG(INFO) << ThriftDebugString(mmap);

  std::unordered_map<std::string, Message> hmap;
  hmap["fddfsd"] = m;
  hmap["34343"] = m;
  LOG(INFO) << "A hash map of messages";
  LOG(INFO) << ThriftDebugString(hmap);

  typedef std::multimap<std::string, Message> m_map_t;
  m_map_t m_map;
  m_map.insert(m_map_t::value_type("fddfsd", m));
  m_map.insert(m_map_t::value_type("34343", m));
  m_map.insert(m_map_t::value_type("34343", m));
  LOG(INFO) << "A multimap of messages";
  LOG(INFO) << ThriftDebugString(m_map);

  MsgSet mset;
  mset.insert("m");
  mset.insert("m");
  mset.insert("a");
  LOG(INFO) << "A set of messages";
  LOG(INFO) << ThriftDebugString(mset);
}
