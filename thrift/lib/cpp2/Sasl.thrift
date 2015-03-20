// Copyright (c) 2006- Facebook
// Distributed under the Thrift Software License
//
// See accompanying file LICENSE or visit the Thrift site at:
// http://developers.facebook.com/thrift/

namespace cpp2 apache.thrift.sasl

// Messages sent back and forth as a part of security negotiation.

struct SaslOutcome {
  1: bool success;
  2: optional string additional_data;
}

struct SaslRequest {
  1: optional string response;
  2: optional bool abort;  // TODO or just close
}

struct SaslReply {
  1: optional string challenge;
  2: optional SaslOutcome outcome;
  3: optional string mechanism;
}

struct SaslStart {
  // We don't currently provide a way for the client to discover what
  // mechanisms the server supports, because the implementation only
  // supports one.  If it ever supports more than one, we can add
  // a new optional field to set up discovery.

  1: string mechanism;
  2: optional SaslRequest request;
  // Send a list of additional supported mechanisms. Sorted by preference.
  // If mechanism in 1: is not included, give it lowest preference.
  3: optional list<string> mechanisms;
}

service SaslAuthService {
  // first method client calls to initiate handshake
  SaslReply authFirstRequest(1: SaslStart saslStart);
  // client subsequently calls this method until SaslOutcome.success is true
  SaslReply authNextRequest(1: SaslRequest saslRequest);
}


// gssapi/krb5:
// C: auth request + initial response (token from init)
// S: challenge (token from accept)
// C: empty (or last token)
// S: wrap(bit-mask + max token accepted)
// C: wrap(bit-mask + max token accepted + authz identity)
// S: success indicator

