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
#include "util.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

namespace tutorial { namespace sort {

/**
 * Parse a port number from a string.
 *
 * The port is returned in host byte order.
 * Returns 0 on success, non-zero on failure.
 */
int util_parse_port(const char* value, uint16_t* port) {
  char* endptr;
  long int port_val = strtol(value, &endptr, 0);
  if (endptr == value || *endptr != '\0') {
    // port is not an integer
    return 1;
  }
  if (port_val < 0 || port_val > 0xffff) {
    // illegal port number
    return 2;
  }

  *port = port_val;
  return 0;
}

/**
 * Parse a hostname and port from a string.
 *
 * The port is returned in host byte order.
 * Returns 0 on success, non-zero if the string cannot be parsed.
 *
 * e.g.
 *   "server.example.com:1234" --> host="server.example.com", port=1234
 *   server.example.com --> host="server.example.com", port is unmodified
 *   :1234 --> host is unmodified, port=1234
 */
int util_parse_host_port(const char* value, std::string* host, uint16_t* port) {
  const char* colon = strchr(value, ':');
  if (colon == NULL) {
    // No port specified.  Assign host, and return with port unmodified.
    host->assign(value);
    return 0;
  }

  // Parse the port number
  int ret = util_parse_port(colon + 1, port);
  if (ret != 0) {
    return ret;
  }

  // Assign host only if the supplied hostname is non-empty
  if (colon != value) {
    host->assign(value, colon - value);
  }

  return 0;
}

/**
 * Resolve a hostname/address to an IP address.
 *
 * Returns 0 on success and non-zero on error.
 * On failure, gai_strerror() may be used to convert the returned
 * error code to a human readable string.
 */
int util_resolve_host(const std::string& host, std::string* ip) {
  // Set up arguments for getaddrinfo()
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_ADDRCONFIG;

  // Call getaddrinfo() to resolve the name
  struct addrinfo* addr;
  int error = getaddrinfo(host.c_str(), NULL, &hints, &addr);
  if (error != 0) {
    return error;
  }

  // Convert the numeric IP back to a string,
  // since thrift wants addresses in string form
  char addr_buf[256];
  if (addr->ai_family == AF_INET) {
    const struct sockaddr_in* in_addr =
      reinterpret_cast<const struct sockaddr_in*>(addr->ai_addr);
    if (inet_ntop(addr->ai_family, &in_addr->sin_addr,
                  addr_buf, sizeof(addr_buf)) == NULL) {
      // inet_ntop() stores the error code in errno;
      // EAI_SYSTEM means check errno
      return EAI_SYSTEM;
    }
  } else if (addr->ai_family == AF_INET6) {
    const struct sockaddr_in6* in6_addr =
      reinterpret_cast<const struct sockaddr_in6*>(addr->ai_addr);
    if (inet_ntop(addr->ai_family, &in6_addr->sin6_addr,
                  addr_buf, sizeof(addr_buf)) == NULL) {
      // inet_ntop() stores the error code in errno;
      // EAI_SYSTEM means check errno
      return EAI_SYSTEM;
    }
  } else {
    return EAI_FAMILY;
  }

  // Free the addrinfo list allocated by getaddrinfo
  freeaddrinfo(addr);

  // Set *ip and return
  ip->assign(addr_buf);
  return 0;
}

}} // tutorial::sort
