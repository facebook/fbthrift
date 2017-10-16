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

namespace cpp2 apache.thrift

enum ProtocolId {
  // The values must match those in thrift/lib/cpp/protocol/TProtocolTypes.h
  BINARY = 0,
  COMPACT = 2,
  FROZEN2 = 6,
}

enum RpcKind {
  SINGLE_REQUEST_SINGLE_RESPONSE = 0,
  SINGLE_REQUEST_NO_RESPONSE = 1,
  STREAMING_REQUEST_SINGLE_RESPONSE = 2,
  STREAMING_REQUEST_NO_RESPONSE = 3,
  SINGLE_REQUEST_STREAMING_RESPONSE = 4,
  STREAMING_REQUEST_STREAMING_RESPONSE = 5,
}

enum RpcPriority {
  HIGH_IMPORTANT = 0,
  HIGH = 1,
  IMPORTANT = 2,
  NORMAL = 3,
  BEST_EFFORT = 4,
  // This should be the immediately after the last enumerator.
  N_PRIORITIES = 5,
}

// RPC metadata sent from the client to the server.  The lifetime of
// objects of this type starts at the call to the generated client
// code, and ends at the generated server code.
struct RequestRpcMetadata {
  // The protocol using which the RPC payload has been serialized.
  1: optional ProtocolId protocol;
  // The name of the RPC function.  It is assigned in generated client
  // code and passed all the way to the ThriftProcessor object on the
  // server.
  2: optional string name;
  // The kind of RPC.  It is assigned in generated client code and
  // passed all the way to the ThriftProcessor object on the server.
  3: optional RpcKind kind;
  // A sequence id to disambiguate multiple RPCs that take place on
  // the same channel object (so only used by channels that support
  // multiple RPCs).  Assigned by the channel on the client side and
  // passed all the way to the ThriftProcessor object on the server.
  // Then returned back in the ResponseRpcMetadata.
  4: optional i32 seqId;
  // The amount of time that a client should wait for a response from
  // the server.  This value is passed to the server so it can choose
  // to time out also.  May be passed in the call to the generated
  // client code to override the default client timeout.
  5: optional i32 clientTimeoutMs;
  // The maximum amount of time that a request should wait at the
  // server before it is handled.  May be passed in the call to the
  // generated client code to override the default server queue time
  // out.  This value is used only by the server.
  6: optional i32 queueTimeoutMs;
  // May be passed in the call to the generated client code to
  // override the default priority of this RPC on the server side.
  // This value is used only by the server.
  7: optional RpcPriority priority;
  // A string to string map that can be populated in the call to the
  // generated client code and further populated by plugins on the
  // client side before it is finally sent over to the server.
  // Any frequently used key-value pair in this map should be replaced
  // by a field in this struct.
  8: optional map<string, string> otherMetadata;
}

// RPC metadata sent from the server to the client.  The lifetime of
// objects of this type starts at the generated server code and ends
// as a return value to the application code that initiated the RPC on
// the client side.
struct ResponseRpcMetadata {
  // The protocol using which the RPC payload has been serialized.
  1: optional ProtocolId protocol;
  // The same sequence id that was passed to the server from the
  // client in RequestRpcMetadata.  This is returned to the client so
  // the channel can match the response to the correct request.  See
  // additional comments in RequestRpcMetadata above.
  2: optional i32 seqId;
  // A string to string map that can be populated by the server
  // handler and further populated by plugins on the server side
  // before it is finally sent back to the client.
  // Any frequently used key-value pair in this map should be replaced
  // by a field in this struct.
  3: optional map<string, string> otherMetadata;
}
