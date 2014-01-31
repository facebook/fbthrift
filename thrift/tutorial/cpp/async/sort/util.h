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
#ifndef THRIFT_TUTORIAL_ASYNC_UTIL_H
#define THRIFT_TUTORIAL_ASYNC_UTIL_H

#include <stdint.h>
#include <string>

namespace tutorial { namespace sort {

int util_parse_port(const char* value, uint16_t* port);
int util_parse_host_port(const char* value, std::string* host, uint16_t* port);
int util_resolve_host(const std::string& host, std::string* ip);

}} // tutorial::sort

#endif // THRIFT_TUTORIAL_ASYNC_UTIL_H
