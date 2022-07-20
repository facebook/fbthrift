---
state: draft
created: 07-18-2022
updated: 07-18-2022
---

# Rocket Protocol

This document describes the Rocket transport protocol and how it is used by Thrift to achieve the [Interface Types](../definition/interface.md).

## Request-Response

With an already established connection, the client must send a [REQUEST_RESPONSE](https://rsocket.io/about/protocol/#request_response-frame-0x04) frame of the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - A new stream ID should be generated for the request. It must comply with the requirements of [RSocket Stream Identifiers](https://rsocket.io/about/protocol/#stream-identifiers)
- Frame type (6 bits)
    - Must be REQUEST_RESPONSE (0x04)
- Flags (10 bits)
    - Metadata flag must be set
    - Follows flag must be set if the frame requires [fragmentation](https://rsocket.io/about/protocol/#fragmentation-and-reassembly)
- Metadata size (24 bit unsigned integer indicating the length of metadata)
- Compact Protocol serialized [RequestRpcMetadata struct](https://github.com/facebook/fbthrift/blob/main/thrift/lib/thrift/RpcMetadata.thrift). This includes information such as:
    - Method name
    - Thrift serialization protocol
    - RPC kind (SINGLE_REQUEST_SINGLE_RESPONSE)
- Thrift serialized arguments from [Interface Protocol](interface.md)

The Thrift server should then perform the method specified in the RequestRpcMetadata method name field. Once the result is ready, the server should send a [PAYLOAD](https://rsocket.io/about/protocol/#payload-frame-0x0a) frame containing the result in the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - This Stream ID should be the same value as the request Stream ID
- Frame type (6 bits)
    - Must be PAYLOAD (0x0A)
- Flags (10 bits)
    - Metadata flag must be set
    - Follows flag must be set if the frame requires [fragmentation](https://rsocket.io/about/protocol/#fragmentation-and-reassembly)
    - Complete flag must be set
    - Next flag must be set
- Metadata size (24 bit unsigned integer indicating the length of metadata)
- Compact Protocol serialized [ResponseRpcMetadata struct](https://github.com/facebook/fbthrift/blob/main/thrift/lib/thrift/RpcMetadata.thrift)
- Thrift serialized result from [Interface Protocol](interface.md)

## Oneway Request (request no response)

With an already established connection, the client must send a [REQUEST_FNF](https://rsocket.io/about/protocol/#request_fnf-fire-n-forget-frame-0x05) frame of the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - A new stream ID should be generated for the request. It must comply with the requirements of [RSocket Stream Identifiers](https://rsocket.io/about/protocol/#stream-identifiers)
- Frame type (6 bits)
    - Must be REQUEST_FNF (0x05)
- Flags (10 bits)
    - Metadata flag must be set
    - Follows flag must be set if the frame requires [fragmentation](https://rsocket.io/about/protocol/#fragmentation-and-reassembly)
- Metadata size (24 bit unsigned integer indicating the length of metadata)
- Compact Protocol serialized [RequestRpcMetadata struct](https://github.com/facebook/fbthrift/blob/main/thrift/lib/thrift/RpcMetadata.thrift). This includes information such as:
    - Method name
    - Thrift serialization protocol
    - RPC kind (SINGLE_REQUEST_NO_RESPONSE)
- Thrift serialized arguments from [Interface Protocol](interface.md)

## Stream

With an already established connection, the client must send a [REQUEST_STREAM](https://rsocket.io/about/protocol/#request_stream-frame-0x06) frame of the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - A new stream ID should be generated for the request. It must comply with the requirements of [RSocket Stream Identifiers](https://rsocket.io/about/protocol/#stream-identifiers)
- Frame type (6 bits)
    - Must be REQUEST_STREAM (0x06)
- Flags (10 bits)
    - Metadata flag must be set
    - Follows flag must be set if the frame requires [fragmentation](https://rsocket.io/about/protocol/#fragmentation-and-reassembly)
- Initial request N (32 bits)
    - Unsigned integer representing the initial number of stream payloads to request. Value MUST be > 0. Max value is 2^31 - 1.
- Metadata size (24 bit unsigned integer indicating the length of metadata)
- Compact Protocol serialized [RequestRpcMetadata struct](https://github.com/facebook/fbthrift/blob/main/thrift/lib/thrift/RpcMetadata.thrift). This includes information such as:
    - Method name
    - Thrift serialization protocol
    - RPC kind (SINGLE_REQUEST_STREAMING_RESPONSE)
- Thrift serialized arguments from [Interface Protocol](interface.md)

TODO:

- Initial response sequence
- Stream responses
- Credits
- Cancellation

## Sink

## Interactions

### Factory Functions

Factory function requests should be sent the same way as their non-interaction counterpart with the `RequestRpcMetadata.createInteraction` field filled out (Note: `RequestRpcMetadata.interactionId` must not be set).

The `InteractionCreate` struct must contain a unique interaction ID as well as the name of the interaction (as defined in the IDL).

### Subsequent Requests

All subsequent requests should be sent the same way as their non-interaction counterpart with two important distinctions:

1. `RequestRpcMetadata.interactionId` must be set to the Interaction ID that was created by the Interaction factory function (Note: `RequestRpcMetadata.createInteraction` must not be set)
2. The request must be sent on the same connection as the original factory function request

### Termination

An interaction can only be terminated by the client. If the client has already sent the factory function request to the server before terminating the interaction, it should send a termination signal to the server. Once an interaction is terminated, the client must not send any more requests with that interaction ID.

To send a termination signal, the client must send a [METADATA_PUSH](https://rsocket.io/about/protocol/#metadata_push-frame-0x0c) frame of the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - Must be 0
- Frame type (6 bits)
    - Must be METADATA_PUSH (0x0C)
- Flags (10 bits)
    - Metadata flag must be set
- Compact Protocol serialized [ClientPushMetadata](https://github.com/facebook/fbthrift/blob/main/thrift/lib/thrift/RpcMetadata.thrift) union. This includes information such as:
    - `interactionTerminate` field must be set
    - The `InteractionTerminate` struct must contain the interaction ID of the interaction being terminated
