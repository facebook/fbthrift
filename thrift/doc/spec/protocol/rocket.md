---
state: draft
created: 07-18-2022
updated: 07-20-2022
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

### Initial Response

The initial response for a stream is distinct from [streaming responses](#stream-responses) and it must be sent from the server even if there is no initial response type specified in the IDL. The initial response should be sent using the same format as a regular [Request-Response response](#request-response) with the distinction that the Complete flag must only be set if the payload contains an exception.

### Stream Responses

Once a stream has been established, the server can send stream payloads to the client as long as it has credits remaining. Sending a stream payload must consume one credit on the server. To send a stream payload to the client, the server must send a [PAYLOAD](https://rsocket.io/about/protocol/#payload-frame-0x0a) frame of the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - The stream ID of the existing stream on which you would like to send the payload
- Frame type (6 bits)
    - Must be PAYLOAD (0x0A)
- Flags (10 bits)
    - Metadata flag must be set
    - Next flag must be set
    - Follows flag must be set if the frame requires [fragmentation](https://rsocket.io/about/protocol/#fragmentation-and-reassembly)
- Metadata size (24 bit unsigned integer indicating the length of metadata)
- Compact Protocol serialized [StreamPayloadMetadata struct](https://github.com/facebook/fbthrift/blob/main/thrift/lib/thrift/RpcMetadata.thrift)
- Thrift serialized result from [Interface Protocol](interface.md#stream-responses) using the serialization protocol specified in [RequestRpcMetadata struct](https://github.com/facebook/fbthrift/blob/main/thrift/lib/thrift/RpcMetadata.thrift)

### Stream Completion

The server must complete a stream once it is done sending all stream payloads. Completing a stream does not require credits on the server. To send a stream completion to the client, the server must send a [PAYLOAD](https://rsocket.io/about/protocol/#payload-frame-0x0a) frame of the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - The stream ID of the existing stream you would like to complete
- Frame type (6 bits)
    - Must be PAYLOAD (0x0A)
- Flags (10 bits)
    - Complete flag must be set
- Empty metadata and data

### Credit Mechanism

Thrift streaming is flow-controlled using the [RSocket credit mechanism](https://rsocket.io/about/protocol/#reactive-streams-semantics). When a client requests a stream, it sends the server an initial number of credits in the request. The client can send more credits to the server by sending a [REQUEST_N](https://rsocket.io/about/protocol/#request_n-frame-0x08) frame of the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - The stream ID of the existing stream that you would like to send credits for
- Frame type (6 bits)
    - Must be REQUEST_N (0x08)
- Flags (10 bits)
    - No flags should be set
- Request N (32 bits)
    - Unsigned integer representing the initial number of stream payloads to request. Value MUST be > 0. Max value is 2^31 - 1.

Credits are cumulative on the server. The server must not send stream payloads if it has 0 credits until it receives more credits from the client.

### Cancellation

Thrift streaming supports cancellation from the client. Upon receiving the cancellation, the server should stop sending payloads to the client. The client can cancel a stream by sending a [CANCEL](https://rsocket.io/about/protocol/#cancel-frame-0x09) frame of the following format:

- Frame size (24 bit unsigned integer indicating the length of the *entire* frame)
- Stream ID (32 bits)
    - The stream ID of the existing stream that you would like to cancel
- Frame type (6 bits)
    - Must be CANCEL (0x09)
- Flags (10 bits)
    - No flags should be set

TODO:

- Initial exception
- Stream exception

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
