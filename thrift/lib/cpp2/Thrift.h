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

#ifndef THRIFT_CPP2_H_
#define THRIFT_CPP2_H_

#include <thrift/lib/cpp/Thrift.h>

namespace apache { namespace thrift {

enum FragileConstructor {
  FRAGILE,
};

/**
 * Class template (specialized for each type in generated code) that allows
 * access to write / read / serializedSize / serializedSizeZC functions in
 * a generic way.
 *
 * For native Cpp2 structs, one could call the corresponding methods
 * directly, but structs generated in compatibility mode (ie. typedef'ed
 * to the Thrift1 version) don't have them; they are defined as free
 * functions named <type>_read, <type>_write, etc, so they can't be accessed
 * generically (because the type name is part of the function name).
 *
 * Cpp2Ops bridges to either struct methods (for native Cpp2 structs)
 * or the corresponding free functions (for structs in compatibility mode).
 */
template <class T>
class Cpp2Ops {
 public:
  template <class P>
  static uint32_t write(P*, const T*);

  template <class P>
  static uint32_t read(P*, T*);

  template <class P>
  static uint32_t serializedSize(P*, const T*);

  template <class P>
  static uint32_t serializedSizeZC(P*, const T*);
};

}} // apache::thrift

#endif // #ifndef THRIFT_CPP2_H_
