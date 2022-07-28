---
state: draft
---

# Interface Protocols

This document describes the layer immediately preceding the transport protocol (eg. [Rocket](rocket.md)). This layer specifies how the request/response data must be serialized and formatted before being wrapped in the underlying transport protocol layer's message format.

## Request

The arguments to an RPC method must be treated as fields of a Thrift struct with an empty name (`“”`). The order of the fields must be the same as the order of the arguments in the IDL. To prepare for sending the request through one of the underlying transport protocols, this unnamed struct should be serialized with one of Thrift’s [data protocols](../data.md).

## Response

### Stream Initial Response

The stream initial response must be treated as a Thrift struct with an empty name (`""`) and either no fields or one field depending on whether there is an initial response type specified in the function signature. If no initial response type is specified, an empty Thrift struct with no fields must be used. If an initial response type is specified, a Thrift struct with one field that matches the type of the initial response in the signature must be used. This Thrift struct must be serialized with one of Thrift’s [data protocols](../data.md). A Thrift envelope must be prepended to the serialized response in some cases (TODO: more details about envelope).

### Stream Responses

A stream response must be treated as a Thrift struct with an empty name (`""`) and one field which matches the type of the stream as specified in the function signature. This Thrift struct must be serialized with one of Thrift’s [data protocols](../data.md). Notably, streaming responses never prepend a Thrift envelope.

## Exception

TODO: TApplicationException

## Underlying Transport Protocols

The Thrift Interface protocol is a higher level abstraction that defines the behavior of various interfaces, however, it must utilize a lower level transport protocol to actually send requests and receive responses.

### Rocket Protocol

[Rocket protocol](rocket.md) is an implementation of the Thrift Interface protocol using the [RSocket protocol](https://rsocket.io/).

### Header Protocol (Deprecated)

Header protocol is a deprecated transport protocol.
