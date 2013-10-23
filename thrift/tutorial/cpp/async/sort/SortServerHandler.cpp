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
#include "thrift/tutorial/cpp/async/sort/SortServerHandler.h"

namespace tutorial { namespace sort {

void SortServerHandler::sort(SortReturnCob cob, SortErrorCob errcobb,
                             const IntVector &values) {
  // The sort can be performed "immediately"--there are no blocking operations,
  // this is just a CPU computation.  Therefore we can just perform the
  // operation and invoke the callback object immedaitely
  IntVector sorted(values.begin(), values.end());
  std::sort(sorted.begin(), sorted.end());
  cob(sorted);
}

}} // tutorial::sort
