---
state: draft
created: 07-14-2022
updated: 07-18-2022
---

# Interface Protocol

This document describes the layer immediately preceding the transport protocol (eg. [Rocket](rocket.md), [Header](header.md)). This layer specifies how the request/response data must be serialized and formatted before being wrapped in the underlying transport protocol layer's message format.

## Request

The arguments to an RPC method must be treated as fields of a Thrift struct with an empty name (`“”`). The order of the fields must be the same as the order of the arguments in the IDL. To prepare for sending the request through one of the underlying transport protocols, this unnamed struct should be serialized with one of Thrift’s [data protocols](data).

## Response

## Exception

TODO: TApplicationException
