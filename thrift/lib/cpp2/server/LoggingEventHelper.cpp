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

#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/LoggingEventHelper.h>

namespace apache {
namespace thrift {

namespace {
void logTlsNoPeerCertEvent(const ConnectionLoggingContext& context) {
  DCHECK(context.getWorker() && context.getWorker()->getServer());
  const auto& configSource =
      context.getWorker()->getServer()->metadata().tlsConfigSource;
  if (configSource && *configSource == kDefaultTLSConfigSource) {
    THRIFT_CONNECTION_EVENT(tls.no_peer_cert.config_default).log(context);
  } else {
    THRIFT_CONNECTION_EVENT(tls.no_peer_cert.config_manual).log(context);
  }
}

void logNonTLSEvent(const ConnectionLoggingContext& context) {
  DCHECK(context.getWorker() && context.getWorker()->getServer());
  auto server = context.getWorker()->getServer();
  // There's actually no point in logging these at all as wangle will not set
  // up tls if theres no config.
  if (!server->getSSLConfig() || server->isSSLPolicySet() ||
      (server->isPlaintextAllowedOnLoopback() &&
       context.getPeerAddress()->isLoopbackAddress())) {
    THRIFT_CONNECTION_EVENT(non_tls.manual_policy).log(context);
  } else {
    THRIFT_CONNECTION_EVENT(non_tls).log(context);
  }
}
} // namespace

void logSetupConnectionEventsOnce(
    folly::once_flag& flag, const ConnectionLoggingContext& context) {
  static_cast<void>(folly::try_call_once(flag, [&]() noexcept {
    try {
      if (auto transport = context.getTransport()) {
        const auto& protocol = context.getSecurityProtocol();
        if (protocol == "TLS" || protocol == "Fizz" || protocol == "stopTLS") {
          if (!transport->getPeerCertificate()) {
            logTlsNoPeerCertEvent(context);
          }
        } else {
          logNonTLSEvent(context);
        }
      }
    } catch (...) {
      LOG(ERROR)
          << "Exception thrown during Thrift server connection events logging: "
          << folly::exceptionStr(std::current_exception());
    }
    return true;
  }));
}

} // namespace thrift
} // namespace apache
