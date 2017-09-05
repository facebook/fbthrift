/*
 * Copyright 2017-present Facebook, Inc.
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
#include <thrift/lib/cpp2/transport/http2/common/H2ChannelIf.h>

#include <folly/Range.h>
#include <proxygen/lib/utils/Base64.h>

namespace apache {
namespace thrift {

using std::map;
using std::string;
using proxygen::HTTPMessage;

void H2ChannelIf::encodeHeaders(
    const map<string, string>& source,
    HTTPMessage& dest) noexcept {
  auto& msgHeaders = dest.getHeaders();
  for (auto it = source.begin(); it != source.end(); ++it) {
    // Keys in Thrift may not comply with HTTP key format requirements.
    // The following algorithm is implemented at multiple locations and
    // should be kept in sync.
    //
    // If the key does not contain a ":", we assume it is compliant
    // and we use the key and value as is.  In case such a key is not
    // compliant, things will break.
    //
    // If it contains a ":", we encode both the key and the value.  We
    // add a prefix tag ("encode_") to the and encode both the key and
    // the value into the value.
    if (it->first.find(":") != string::npos) {
      auto name = proxygen::Base64::urlEncode(folly::StringPiece(it->first));
      auto value = proxygen::Base64::urlEncode(folly::StringPiece(it->second));
      msgHeaders.rawSet(
          folly::to<std::string>("encode_", name),
          folly::to<std::string>(name, "_", value));
    } else {
      msgHeaders.rawSet(it->first, it->second);
    }
  }
}

void H2ChannelIf::decodeHeaders(
    const HTTPMessage& source,
    map<string, string>& dest) noexcept {
  auto decodeAndCopyKeyValue = [&](const string& key, const string& val) {
    // This decodes key-value pairs that have been encoded using
    // encodeHeaders() or equivalent methods.  If the key starts with
    // "encode_", the value is split at the underscore and then the
    // key and value are decoded from there.  The key is not used
    // because it will get converted to lowercase and therefore the
    // original key cannot be recovered.
    if (key.find("encode_") == 0) {
      auto us = val.find("_");
      if (us == string::npos) {
        LOG(ERROR) << "Encoded value does not contain underscore";
      }
      auto decodedKey = proxygen::Base64::urlDecode(val.substr(0, us));
      auto decodedVal = proxygen::Base64::urlDecode(val.substr(us + 1));
      dest[decodedKey] = decodedVal;
    } else {
      dest[key] = val;
    }
  };
  source.getHeaders().forEach(decodeAndCopyKeyValue);
}

} // namespace thrift
} // namespace apache
