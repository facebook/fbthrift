/*
 * Copyright 2004-present Facebook, Inc.
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

#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <folly/io/IOBuf.h>

const std::string kOverloadedErrorCode{"1"};
const std::string kTaskExpiredErrorCode{"2"};
const std::string kProxyTransportExceptionErrorCode{"3"};
const std::string kProxyClientProtocolExceptionErrorCode{"4"};
const std::string kQueueOverloadedErrorCode{"5"};
const std::string kProxyHeaderParseExceptionErrorCode{"6"};
const std::string kProxyAuthExceptionErrorCode{"7"};
const std::string kProxyLookupTransportExceptionErrorCode{"8"};
const std::string kProxyLookupAppExceptionErrorCode{"9"};
const std::string kProxyWhitelistExceptionErrorCode{"10"};
const std::string kProxyClientAppExceptionErrorCode{"11"};
const std::string kProxyProtocolMismatchExceptionErrorCode{"12"};
const std::string kProxyQPSThrottledExceptionErrorCode{"13"};
const std::string kInjectedFailureErrorCode{"14"};
const std::string kServerQueueTimeoutErrorCode{"15"};
const std::string kProxyResponseSizeThrottledExceptionErrorCode{"16"};
const std::string kResponseTooBigErrorCode{"17"};
const std::string kProxyAclCheckExceptionErrorCode{"18"};
const std::string kProxyOverloadedErrorCode{"19"};
