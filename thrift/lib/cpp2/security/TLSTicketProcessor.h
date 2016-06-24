/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>

#include <folly/Optional.h>
#include <thrift/lib/cpp2/util/FilePoller.h>
#include <wangle/ssl/TLSTicketKeySeeds.h>

namespace apache {
namespace thrift {

class TLSTicketProcessor {
 public:
  explicit TLSTicketProcessor(std::string ticketFile);

  ~TLSTicketProcessor();

  void addCallback(std::function<void(wangle::TLSTicketKeySeeds)> callback);

  void fileUpdated() noexcept;

  void stop();

  /**
   * This parses a TLS ticket file with the tickets and returns a
   * TLSTicketKeySeeds structure if the file is valid.
   * The TLS ticket file is formatted as a json blob
   * {
   *   "old": [
   *     "seed1",
   *     ...
   *   ],
   *   "new": [
   *     ...
   *   ],
   *   "current": [
   *     ...
   *   ]
   * }
   * Seeds are aribitrary length secret strings which are used to derive
   * ticket encryption keys.
   */
  static folly::Optional<wangle::TLSTicketKeySeeds> processTLSTickets(
      const std::string& fileName);

  std::string ticketFile_;
  FilePoller poller_;
  std::vector<std::function<void(wangle::TLSTicketKeySeeds)>> callbacks_;
};
}
}
