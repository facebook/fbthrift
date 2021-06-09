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

namespace cpp2 apache.thrift
namespace java.swift org.apache.thrift
namespace java javadeprecated.org.apache.thrift
namespace php Thrift_RpcMetadata
namespace py thrift.lib.thrift.RpcMetadata
namespace py.asyncio thrift.lib.thrift.asyncio.RpcMetadata
namespace py3 thrift.lib.thrift
namespace go thrift.lib.thrift.RpcMetadata

cpp_include "thrift/lib/cpp2/util/ManagedStringView.h"
cpp_include "thrift/lib/thrift/RpcMetadata_extra.h"

enum ProtocolId {
  // The values must match those in thrift/lib/cpp/protocol/TProtocolTypes.h
  BINARY = 0,
  COMPACT = 2,
// Deprecated.
// FROZEN2 = 6,
}

enum RpcKind {
  SINGLE_REQUEST_SINGLE_RESPONSE = 0,
  SINGLE_REQUEST_NO_RESPONSE = 1,
  STREAMING_REQUEST_SINGLE_RESPONSE = 2,
  STREAMING_REQUEST_NO_RESPONSE = 3,
  SINGLE_REQUEST_STREAMING_RESPONSE = 4,
  STREAMING_REQUEST_STREAMING_RESPONSE = 5,
  SINK = 6,
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

// Represent the bit position of the compression algorithm that is negotiated
// in TLS handshake
enum CompressionAlgorithm {
  NONE = 0,
  ZLIB = 1,
  ZSTD = 2,
}

// re-definition of the same enums from
// thrift/compiler/ast/t_exception.h
enum ErrorKind {
  UNSPECIFIED = 0,
  TRANSIENT = 1,
  STATEFUL = 2,
  PERMANENT = 3,
}

enum ErrorBlame {
  UNSPECIFIED = 0,
  SERVER = 1,
  CLIENT = 2,
}

enum ErrorSafety {
  UNSPECIFIED = 0,
  SAFE = 1,
}

enum FunctionQualifier {
  Unspecified = 0,
  OneWay = 1,
  Idempotent = 2,
  ReadOnly = 3,
}

struct ZlibCompressionCodecConfig {
}

struct ZstdCompressionCodecConfig {
}

union CodecConfig {
  1: ZlibCompressionCodecConfig zlibConfig;
  2: ZstdCompressionCodecConfig zstdConfig;
}

// Represent the compression config user set
struct CompressionConfig {
  1: optional CodecConfig codecConfig;
  2: optional i64 compressionSizeLimit;
}

// A TLS extension used for thrift parameters negotiation during TLS handshake.
// All fields should be optional.
// For fields which are conceptually list<bool>, i64 bitmap is frequently preferred.
struct NegotiationParameters {
  // nth (zero-based) least significant bit set if CompressionAlgorithm = n + 1
  // is accepted. For example, 0b10 means ZSTD is accepted.
  1: optional i64 (cpp.type = "std::uint64_t") compressionAlgos;
  2: optional bool useStopTLS;
}

// String type optimized for generated code
typedef string (
  cpp.type = "::apache::thrift::ManagedStringViewWithConversions",
) ManagedStringViewField

struct InteractionCreate {
  // Must be > 0
  1: i64 interactionId;
  2: ManagedStringViewField interactionName;
}
struct InteractionTerminate {
  1: i64 interactionId;
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
  2: optional ManagedStringViewField name;
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
  // 9: Deprecated
  // 10: Deprecated
  // The CRC32C of the RPC message.
  11: optional i32 (cpp.type = "std::uint32_t") crc32c;
  // 12: Deprecated
  13: optional string loadMetric;
  // The CompressionAlgorithm used to compress requests (if any)
  14: optional CompressionAlgorithm compression;
  // Requested compression policy for responses
  15: optional CompressionConfig compressionConfig;
  // Enables TILES
  // Use interactionCreate for new interactions and this for existing
  16: optional i64 interactionId;
  // Id must be unique per connection
  17: optional InteractionCreate interactionCreate;
  18: optional string clientId;
  19: optional string serviceTraceMeta;
}

struct ErrorClassification {
  1: optional ErrorKind kind;
  2: optional ErrorBlame blame;
  3: optional ErrorSafety safety;
}

struct PayloadResponseMetadata {
}

struct PayloadDeclaredExceptionMetadata {
  1: optional ErrorClassification errorClassification;
}

struct PayloadProxyExceptionMetadata {
}

struct PayloadProxiedExceptionMetadata {
}

struct PayloadAppClientExceptionMetadata {
}

struct PayloadAppServerExceptionMetadata {
}

struct PayloadAppUnknownExceptionMetdata {
  // We only use the blame field for now
  1: optional ErrorClassification errorClassification;
}

union PayloadExceptionMetadata {
  1: PayloadDeclaredExceptionMetadata declaredException;
  2: PayloadProxyExceptionMetadata proxyException;
  // Deprecated
  // replaced by PayloadProxyExceptionMetadata + ProxiedPayloadMetadata
  3: PayloadProxiedExceptionMetadata DEPRECATED_proxiedException;
  // replaced by PayloadAppUnknownExceptionMetdata
  4: PayloadAppClientExceptionMetadata DEPRECATED_appClientException;
  // replaced by PayloadAppUnknownExceptionMetdata
  5: PayloadAppServerExceptionMetadata DEPRECATED_appServerException;
  6: PayloadAppUnknownExceptionMetdata appUnknownException;
}

struct PayloadExceptionMetadataBase {
  1: optional string name_utf8;
  2: optional string what_utf8;
  3: optional PayloadExceptionMetadata metadata;
}

union PayloadMetadata {
  1: PayloadResponseMetadata responseMetadata;
  2: PayloadExceptionMetadataBase exceptionMetadata;
}

struct ProxiedPayloadMetadata {
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
  // Server load. Returned to client if loadMetric was set in RequestRpcMetadata
  4: optional i64 load;
  // The CRC32C of the RPC response.
  5: optional i32 (cpp.type = "std::uint32_t") crc32c;
  // The CompressionAlgorithm used to compress responses (if any)
  6: optional CompressionAlgorithm compression;
  // Additional metadata for the response payload
  7: optional PayloadMetadata payloadMetadata;
  // Additional metadata for the response payload (if proxied)
  8: optional ProxiedPayloadMetadata proxiedPayloadMetadata;
}

enum ResponseRpcErrorCategory {
  // Server failed processing the request.
  // Server may have started processing the request.
  INTERNAL_ERROR = 0,
  // Server didn't process the request because the request was invalid.
  INVALID_REQUEST = 1,
  // Server didn't process the request because it didn't have the resources.
  // Request can be safely retried to a different server, or the same server
  // later.
  LOADSHEDDING = 2,
  // Server didn't process request because it was shutting down.
  // Request can be safely retried to a different server. Request should not
  // be retried to the same server.
  SHUTDOWN = 3,
}

enum ResponseRpcErrorCode {
  UNKNOWN = 0,
  OVERLOAD = 1,
  TASK_EXPIRED = 2,
  QUEUE_OVERLOADED = 3,
  SHUTDOWN = 4,
  INJECTED_FAILURE = 5,
  REQUEST_PARSING_FAILURE = 6,
  QUEUE_TIMEOUT = 7,
  RESPONSE_TOO_BIG = 8,
  WRONG_RPC_KIND = 9,
  UNKNOWN_METHOD = 10,
  CHECKSUM_MISMATCH = 11,
  INTERRUPTION = 12,
  APP_OVERLOAD = 13,
  UNKNOWN_INTERACTION_ID = 14,
  INTERACTION_CONSTRUCTOR_ERROR = 15,
}

struct ResponseRpcError {
  1: optional string name_utf8;
  2: optional string what_utf8;
  3: optional ResponseRpcErrorCategory category;
  4: optional ResponseRpcErrorCode code;
  // Server load. Returned to client if loadMetric was set in RequestRpcMetadata
  5: optional i64 load;
}

enum StreamRpcErrorCode {
  UNKNOWN = 0,
  CREDIT_TIMEOUT = 1,
  CHUNK_TIMEOUT = 2,
}

struct StreamRpcError {
  1: optional string name_utf8;
  2: optional string what_utf8;
  4: optional StreamRpcErrorCode code;
}

struct StreamPayloadMetadata {
  // The CompressionAlgorithm used to compress responses (if any)
  1: optional CompressionAlgorithm compression;
  // A string to string map that can be populated by the server
  // handler and further populated by plugins on the server side
  // before it is finally sent back to the client.
  // Any frequently used key-value pair in this map should be replaced
  // by a field in this struct.
  2: optional map<string, string> otherMetadata;
  3: optional PayloadMetadata payloadMetadata;
}

// Setup metadata sent from the client to the server at the time
// of initial connection.
enum InterfaceKind {
  USER = 0,
  DEBUGGING = 1,
  MONITORING = 2,
  PROFILING = 3,
}

// The key is 32-bit, using a 64-bit constant here to make it work with signed
// types.
const i64 kRocketProtocolKey = 0xf09f9a80;

struct ClientMetadata {
  1: optional string agent;
  2: optional string hostname;
  3: optional map<string, string> otherMetadata;
}

struct RequestSetupMetadata {
  1: optional map<string, binary> (
    cpp.template = "apache::thrift::MetadataOpaqueMap",
  ) opaque;
  // Indicates client wants to use admin interface
  2: optional InterfaceKind interfaceKind;
  // Min Rocket protocol version supported by the client
  3: optional i32 minVersion;
  // Max Rocket protocol version supported by the client
  4: optional i32 maxVersion;
  // The DSCP and SO_MARK values that the server should apply to its end of
  // the connection
  5: optional i32 dscpToReflect;
  6: optional i32 markToReflect;
  9: optional ClientMetadata clientMetadata;
}

struct SetupResponse {
  // the version that server picked
  1: optional i32 version;
}

struct StreamHeadersPush {
  1: optional i32 streamId;
  2: optional HeadersPayloadContent headersPayloadContent;
}

enum DrainCompleteCode {
  EXCEEDED_INGRESS_MEM_LIMIT = 1,
}

struct DrainCompletePush {
  1: optional DrainCompleteCode drainCompleteCode;
}

union ServerPushMetadata {
  1: SetupResponse setupResponse;
  2: StreamHeadersPush streamHeadersPush;
  3: DrainCompletePush drainCompletePush;
}

union ClientPushMetadata {
  1: InteractionTerminate interactionTerminate;
  2: StreamHeadersPush streamHeadersPush;
}

struct HeadersPayloadContent {
  // A string to string map that can be populated by the server
  // handler and further populated by plugins on the server side
  // before it is finally sent back to the client.
  // Any frequently used key-value pair in this map should be replaced
  // by a field in this struct.
  1: optional map<string, string> otherMetadata;
}

struct HeadersPayloadMetadata {
  // The CompressionAlgorithm used to compress responses (if any)
  1: optional CompressionAlgorithm compression;
}
