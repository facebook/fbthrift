---
state: draft
created: 07-14-2022
updated: 07-20-2022
---

# Interface Protocol

This document describes the layer immediately preceding the transport protocol (eg. [Rocket](rocket.md), [Header](header.md)). This layer specifies how the request/response data must be serialized and formatted before being wrapped in the underlying transport protocol layer's message format.

## Request

The arguments to an RPC method must be treated as fields of a Thrift struct with an empty name (`“”`). The order of the fields must be the same as the order of the arguments in the IDL. To prepare for sending the request through one of the underlying transport protocols, this unnamed struct should be serialized with one of Thrift’s [data protocols](data).

## Response

### Stream Initial Response

The stream initial response must be treated as a Thrift struct with an empty name (`""`) and either no fields or one field depending on whether there is an initial response type specified in the function signature. If no initial response type is specified, an empty Thrift struct with no fields must be used. If an initial response type is specified, a Thrift struct with one field that matches the type of the initial response in the signature must be used. This Thrift struct must be serialized with one of Thrift’s [data protocols](data). A Thrift envelope must be prepended to the serialized response in some cases (TODO: more details about envelope).

### Stream Responses

A stream response must be treated as a Thrift struct with an empty name (`""`) and one field which matches the type of the stream as specified in the function signature. This Thrift struct must be serialized with one of Thrift’s [data protocols](data). Notably, streaming responses never prepend a Thrift envelope.

## Exception

TODO: TApplicationException
