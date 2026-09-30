# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Canonical SHA-256 digest of a Thrift service catalog.

Byte-identical to the other `ServiceCatalogDigest`, meaning they can be used across platforms
and across languages. The digests can be calculated from either the runtime representation, e.g. `ServiceDescriptor`
or the appropriate serialized representation, e.g. `SerializableServiceDescriptor`, as they are
compatible.
"""

from __future__ import annotations

from collections.abc import Callable, Iterable, Mapping, Sequence
from typing import TypeVar

from apache.thrift.type.schema.thrift_types import ErrorBlame, ErrorKind, ErrorSafety
from apache.thrift.type_system.type_id.thrift_types import TypeId
from thrift.lib.python.schema._digest_common import (
    _Hasher,
    DigestMode,
    is_standard_annotation,
)
from thrift.lib.python.schema._record import SerializableRecord
from thrift.lib.python.schema._serializable import _export_referenced_uris
from thrift.lib.python.schema.runtime_digest import (
    _hash_annotations_native,
    _hash_type_id_native,
    runtime_type_system_digest,
)
from thrift.lib.python.schema.service_descriptor import (
    DeclaredException,
    Function,
    Interaction,
    Parameter,
    ServiceCatalog,
    ServiceDescriptor,
    Sink,
    Stream,
)
from thrift.lib.python.schema.type_system import (
    _collect_closure,
    _type_ref_uris,
    IndexedTypeSystem,
    TypeRef,
    TypeSystem,
)
from thrift.lib.python.schema.type_system_digest import (
    _hash_annotations,
    _hash_type_id,
    type_system_digest,
)
from thrift.lib.thrift.service_catalog.thrift_types import (
    ExceptionBlame,
    ExceptionKind,
    ExceptionSafety,
    SerializableBidirectionalStream,
    SerializableException,
    SerializableFunction,
    SerializableFunctionResponse,
    SerializableInteractionDefinition,
    SerializableParameter,
    SerializableRpcInterfaceDefinition,
    SerializableServiceCatalog,
    SerializableServiceDefinition,
    SerializableSink,
    SerializableStream,
    SerializableStreamingResponse,
)

__all__ = ["DigestMode", "SERVICE_CATALOG_DIGEST_VERSION", "service_catalog_digest"]


# Bumped only for backwards-incompatible changes to the digest format; must
# match `kServiceCatalogDigestVersion` in C++ and Rust.
SERVICE_CATALOG_DIGEST_VERSION = 2

_TYPE_SYSTEM_DIGEST_SIZE = 32

_T = TypeVar("_T")
_K = TypeVar("_K", bytes, int)

# Wire-union arm -> thrift field id, the discriminant the digest hashes.
_SERVICE_DEF_FIELD_ID = 1
_INTERACTION_DEF_FIELD_ID = 2
_SERVER_STREAM_FIELD_ID = 1
_CLIENT_SINK_FIELD_ID = 2
_BIDIRECTIONAL_STREAM_FIELD_ID = 3

_INTERFACE_FIELD_ID: dict[SerializableRpcInterfaceDefinition.Type, int] = {
    SerializableRpcInterfaceDefinition.Type.serviceDef: _SERVICE_DEF_FIELD_ID,
    SerializableRpcInterfaceDefinition.Type.interactionDef: _INTERACTION_DEF_FIELD_ID,
}
_STREAMING_FIELD_ID: dict[SerializableStreamingResponse.Type, int] = {
    SerializableStreamingResponse.Type.serverStream: _SERVER_STREAM_FIELD_ID,
    SerializableStreamingResponse.Type.clientSink: _CLIENT_SINK_FIELD_ID,
    SerializableStreamingResponse.Type.bidirectionalStream: _BIDIRECTIONAL_STREAM_FIELD_ID,
}

# Runtime exception classifications hash as their serialized counterparts.
_SAFETY_TO_WIRE: dict[ErrorSafety, ExceptionSafety] = {
    ErrorSafety.Unspecified: ExceptionSafety.Unspecified,
    ErrorSafety.Safe: ExceptionSafety.Safe,
}
_KIND_TO_WIRE: dict[ErrorKind, ExceptionKind] = {
    ErrorKind.Unspecified: ExceptionKind.Unspecified,
    ErrorKind.Transient: ExceptionKind.Transient,
    ErrorKind.Stateful: ExceptionKind.Stateful,
    ErrorKind.Permanent: ExceptionKind.Permanent,
}
_BLAME_TO_WIRE: dict[ErrorBlame, ExceptionBlame] = {
    ErrorBlame.Unspecified: ExceptionBlame.Unspecified,
    ErrorBlame.Server: ExceptionBlame.Server,
    ErrorBlame.Client: ExceptionBlame.Client,
}


def service_catalog_digest(
    value: SerializableServiceCatalog | ServiceCatalog | ServiceDescriptor,
    mode: DigestMode = DigestMode.FULL,
) -> bytes:
    """The canonical 32-byte SHA-256 digest of a service catalog.

    A ``ServiceDescriptor`` hashes as the catalog of its service and the
    interactions it creates. The serialized and runtime representations of the
    same catalog hash equally, and inline ``types`` and an out-of-band
    ``typesDigest`` hash equivalently.
    """
    h = _Hasher(mode)
    h.hash_u8(SERVICE_CATALOG_DIGEST_VERSION)
    if isinstance(value, SerializableServiceCatalog):
        _hash_serialized_catalog(h, value, mode)
    elif isinstance(value, ServiceCatalog):
        _hash_runtime_services(h, value.type_system, value.services, mode)
    else:
        _hash_runtime_services(h, value.type_system, (value,), mode)
    return h.finalize()


# Serialized catalogs


def _hash_serialized_catalog(
    h: _Hasher, catalog: SerializableServiceCatalog, mode: DigestMode
) -> None:
    if catalog.types is not None:
        h.update(type_system_digest(catalog.types, mode))
    else:
        digest = bytes(catalog.typesDigest)
        if len(digest) != _TYPE_SYSTEM_DIGEST_SIZE:
            raise ValueError(
                "SerializableServiceCatalog has no valid type system digest"
            )
        h.update(digest)
    for uri in sorted(catalog.interfaces, key=_utf8):
        h.hash_str(uri)
        _hash_serialized_interface(h, catalog.interfaces[uri])


def _hash_serialized_interface(
    h: _Hasher, definition: SerializableRpcInterfaceDefinition
) -> None:
    field_id = _INTERFACE_FIELD_ID.get(definition.type)
    if field_id is None:
        raise ValueError("Cannot digest an empty RPC interface definition")
    h.hash_i32(field_id)
    if definition.type == SerializableRpcInterfaceDefinition.Type.serviceDef:
        _hash_serialized_service(h, definition.serviceDef)
    else:
        _hash_serialized_interaction(h, definition.interactionDef)


def _hash_serialized_service(
    h: _Hasher, service: SerializableServiceDefinition
) -> None:
    for function in _first_by_key(service.functions, _serialized_function_name):
        _hash_serialized_function(h, function)
    h.hash_bool(service.baseService is not None)
    if service.baseService is not None:
        h.hash_str(service.baseService)
    if service.performedInteractions:
        _hash_presorted(h, sorted(service.performedInteractions, key=_utf8))
    _hash_annotations(h, service.annotations)


def _hash_serialized_interaction(
    h: _Hasher, interaction: SerializableInteractionDefinition
) -> None:
    for function in _first_by_key(interaction.functions, _serialized_function_name):
        _hash_serialized_function(h, function)
    _hash_annotations(h, interaction.annotations)


def _hash_serialized_function(h: _Hasher, function: SerializableFunction) -> None:
    h.hash_str(function.name)
    h.hash_i32(function.qualifier.value)
    for parameter in _first_by_key(function.params, _serialized_parameter_id):
        h.hash_i16(parameter.identity.id)
        h.hash_str(parameter.identity.name)
        _hash_type_id(h, parameter.type)
        _hash_annotations(h, parameter.annotations)
    _hash_serialized_response(h, function.response)
    _hash_serialized_exceptions(h, function.exceptions)
    h.hash_i32(function.rpcKind.value)
    _hash_annotations(h, function.annotations)


def _hash_serialized_response(
    h: _Hasher, response: SerializableFunctionResponse
) -> None:
    _hash_optional_type_id(h, response.initialResponseType)
    streaming = response.streaming
    h.hash_bool(streaming is not None)
    if streaming is not None:
        field_id = _STREAMING_FIELD_ID.get(streaming.type)
        if field_id is None:
            raise ValueError("Cannot digest an empty streaming response")
        h.hash_i32(field_id)
        if streaming.type == SerializableStreamingResponse.Type.serverStream:
            _hash_serialized_stream(h, streaming.serverStream)
        elif streaming.type == SerializableStreamingResponse.Type.clientSink:
            _hash_serialized_sink(h, streaming.clientSink)
        else:
            _hash_serialized_bidirectional_stream(h, streaming.bidirectionalStream)
    _hash_optional_str(h, response.createsInteraction)


def _hash_serialized_stream(h: _Hasher, stream: SerializableStream) -> None:
    _hash_type_id(h, stream.payloadType)
    _hash_serialized_exceptions(h, stream.exceptions)


def _hash_serialized_sink(h: _Hasher, sink: SerializableSink) -> None:
    _hash_type_id(h, sink.payloadType)
    _hash_optional_type_id(h, sink.finalResponseType)
    _hash_serialized_exceptions(h, sink.clientExceptions)
    _hash_serialized_exceptions(h, sink.serverExceptions)


def _hash_serialized_bidirectional_stream(
    h: _Hasher, bidi: SerializableBidirectionalStream
) -> None:
    _hash_type_id(h, bidi.sinkPayloadType)
    _hash_type_id(h, bidi.streamPayloadType)
    _hash_serialized_exceptions(h, bidi.sinkExceptions)
    _hash_serialized_exceptions(h, bidi.streamExceptions)


def _hash_serialized_exceptions(
    h: _Hasher, exceptions: Iterable[SerializableException]
) -> None:
    for exception in _first_by_key(exceptions, _serialized_exception_id):
        h.hash_i16(exception.identity.id)
        h.hash_str(exception.identity.name)
        _hash_type_id(h, exception.type)
        _hash_annotations(h, exception.annotations)
        h.hash_i32(exception.safety.value)
        h.hash_i32(exception.kind.value)
        h.hash_i32(exception.blame.value)


def _hash_optional_type_id(h: _Hasher, type_id: TypeId | None) -> None:
    h.hash_bool(type_id is not None)
    if type_id is not None:
        _hash_type_id(h, type_id)


# Runtime catalogs and service descriptors


def _hash_runtime_services(
    h: _Hasher,
    type_system: TypeSystem,
    services: Sequence[ServiceDescriptor],
    mode: DigestMode,
) -> None:
    interactions: dict[str, Interaction] = {}
    for service in services:
        for interaction in service.interactions:
            interactions.setdefault(interaction.uri, interaction)
        _validate_created_interactions(service.functions, service.interactions)
        for interaction in service.interactions:
            _validate_created_interactions(interaction.functions, service.interactions)
    h.update(_runtime_types_digest(type_system, services, interactions, mode))

    interfaces: dict[str, ServiceDescriptor | Interaction] = {
        service.service_uri: service for service in services
    }
    interfaces.update(interactions)
    for uri in sorted(interfaces, key=_utf8):
        h.hash_str(uri)
        interface = interfaces[uri]
        if isinstance(interface, ServiceDescriptor):
            h.hash_i32(_SERVICE_DEF_FIELD_ID)
            _hash_runtime_service(h, interface)
        else:
            h.hash_i32(_INTERACTION_DEF_FIELD_ID)
            _hash_runtime_functions(h, interface.functions)
            _hash_annotations_native(h, interface.annotations)


def _validate_created_interactions(
    functions: Iterable[Function], interactions: Iterable[Interaction]
) -> None:
    known = {interaction.uri for interaction in interactions}
    for function in functions:
        uri = function.created_interaction_uri
        if uri is not None and uri not in known:
            raise ValueError(
                f"Function {function.name!r} creates interaction {uri!r}, "
                "which is not in the catalog"
            )


# The type universe is the closure of every type the interfaces reference,
# matching what C++ bundles when it serializes the same descriptor.
def _runtime_types_digest(
    type_system: TypeSystem,
    services: Iterable[ServiceDescriptor],
    interactions: Mapping[str, Interaction],
    mode: DigestMode,
) -> bytes:
    roots: set[str] = set()
    for service in services:
        _add_interface_roots(roots, service.annotations, service.functions)
    for interaction in interactions.values():
        _add_interface_roots(roots, interaction.annotations, interaction.functions)
    closure = _collect_closure(type_system, sorted(roots), _export_referenced_uris)
    return runtime_type_system_digest(IndexedTypeSystem(closure), mode)


def _hash_runtime_service(h: _Hasher, service: ServiceDescriptor) -> None:
    # Collected separately: rebuilt constructors all share the empty name, which
    # the name-keyed walk would deduplicate.
    performed_interactions = [
        _performed_interaction(function)
        for function in service.functions
        if function.is_performs
    ]
    _hash_runtime_functions(
        h, (function for function in service.functions if not function.is_performs)
    )
    h.hash_bool(False)
    if performed_interactions:
        _hash_presorted(h, sorted(performed_interactions, key=_utf8))
    _hash_annotations_native(h, service.annotations)


def _performed_interaction(constructor: Function) -> str:
    if constructor.created_interaction_uri is None:
        raise ValueError(
            f"Interaction constructor {constructor.name!r} does not name its "
            "interaction"
        )
    return constructor.created_interaction_uri


def _hash_runtime_functions(h: _Hasher, functions: Iterable[Function]) -> None:
    for function in _first_by_key(functions, _runtime_function_name):
        _hash_runtime_function(h, function)


def _hash_runtime_function(h: _Hasher, function: Function) -> None:
    h.hash_str(function.name)
    h.hash_i32(function.qualifier.value)
    for parameter in _first_by_key(function.params, _runtime_parameter_id):
        h.hash_i16(parameter.id)
        h.hash_str(parameter.name)
        _hash_type_id_native(h, parameter.type)
        _hash_annotations_native(h, parameter.annotations)
    _hash_runtime_response(h, function)
    _hash_runtime_exceptions(h, function.exceptions)
    h.hash_i32(function.rpc_kind.value)
    _hash_annotations_native(h, function.annotations)


def _hash_runtime_response(h: _Hasher, function: Function) -> None:
    _hash_optional_type_ref(h, function.response_type)
    stream, sink = function.stream, function.sink
    h.hash_bool(stream is not None or sink is not None)
    if stream is not None and sink is not None:
        h.hash_i32(_BIDIRECTIONAL_STREAM_FIELD_ID)
        _hash_type_id_native(h, sink.payload_type)
        _hash_type_id_native(h, stream.payload_type)
        _hash_runtime_exceptions(h, sink.client_exceptions)
        _hash_runtime_exceptions(h, stream.exceptions)
    elif stream is not None:
        h.hash_i32(_SERVER_STREAM_FIELD_ID)
        _hash_runtime_stream(h, stream)
    elif sink is not None:
        h.hash_i32(_CLIENT_SINK_FIELD_ID)
        _hash_runtime_sink(h, sink)
    _hash_optional_str(h, function.created_interaction_uri)


def _hash_runtime_stream(h: _Hasher, stream: Stream) -> None:
    _hash_type_id_native(h, stream.payload_type)
    _hash_runtime_exceptions(h, stream.exceptions)


def _hash_runtime_sink(h: _Hasher, sink: Sink) -> None:
    _hash_type_id_native(h, sink.payload_type)
    _hash_optional_type_ref(h, sink.final_response_type)
    _hash_runtime_exceptions(h, sink.client_exceptions)
    _hash_runtime_exceptions(h, sink.server_exceptions)


def _hash_runtime_exceptions(
    h: _Hasher, exceptions: Iterable[DeclaredException]
) -> None:
    for exception in _first_by_key(exceptions, _runtime_exception_id):
        h.hash_i16(exception.id)
        h.hash_str(exception.name)
        _hash_type_id_native(h, exception.type)
        _hash_annotations_native(h, exception.annotations)
        h.hash_i32(_SAFETY_TO_WIRE[exception.safety].value)
        h.hash_i32(_KIND_TO_WIRE[exception.kind].value)
        h.hash_i32(_BLAME_TO_WIRE[exception.blame].value)


def _hash_optional_type_ref(h: _Hasher, type_ref: TypeRef | None) -> None:
    h.hash_bool(type_ref is not None)
    if type_ref is not None:
        _hash_type_id_native(h, type_ref)


def _add_interface_roots(
    roots: set[str],
    annotations: Mapping[str, SerializableRecord],
    functions: Iterable[Function],
) -> None:
    _add_annotation_roots(roots, annotations)
    for function in functions:
        _add_annotation_roots(roots, function.annotations)
        for parameter in function.params:
            roots.update(_type_ref_uris(parameter.type))
            _add_annotation_roots(roots, parameter.annotations)
        if function.response_type is not None:
            roots.update(_type_ref_uris(function.response_type))
        _add_exception_roots(roots, function.exceptions)
        if function.stream is not None:
            roots.update(_type_ref_uris(function.stream.payload_type))
            _add_exception_roots(roots, function.stream.exceptions)
        if function.sink is not None:
            roots.update(_type_ref_uris(function.sink.payload_type))
            if function.sink.final_response_type is not None:
                roots.update(_type_ref_uris(function.sink.final_response_type))
            _add_exception_roots(roots, function.sink.client_exceptions)
            _add_exception_roots(roots, function.sink.server_exceptions)


def _add_exception_roots(
    roots: set[str], exceptions: Iterable[DeclaredException]
) -> None:
    for exception in exceptions:
        roots.update(_type_ref_uris(exception.type))
        _add_annotation_roots(roots, exception.annotations)


def _add_annotation_roots(
    roots: set[str], annotations: Mapping[str, SerializableRecord]
) -> None:
    roots.update(uri for uri in annotations if not is_standard_annotation(uri))


# Shared helpers


def _hash_presorted(h: _Hasher, values: Sequence[str]) -> None:
    h.hash_u32(len(values))
    for value in values:
        h.hash_str(value)


def _hash_optional_str(h: _Hasher, value: str | None) -> None:
    h.hash_bool(value is not None)
    if value is not None:
        h.hash_str(value)


# Mirrors C++ `forEachSortedByKey`: elements sharing a key keep the first one,
# and no count prefix is hashed.
def _first_by_key(items: Iterable[_T], key: Callable[[_T], _K]) -> list[_T]:
    by_key: dict[_K, _T] = {}
    for item in items:
        by_key.setdefault(key(item), item)
    return [by_key[k] for k in sorted(by_key)]


def _utf8(value: str) -> bytes:
    return value.encode("utf-8")


def _serialized_function_name(function: SerializableFunction) -> bytes:
    return _utf8(function.name)


def _serialized_parameter_id(parameter: SerializableParameter) -> int:
    return parameter.identity.id


def _serialized_exception_id(exception: SerializableException) -> int:
    return exception.identity.id


def _runtime_function_name(function: Function) -> bytes:
    return _utf8(function.name)


def _runtime_parameter_id(parameter: Parameter) -> int:
    return parameter.id


def _runtime_exception_id(exception: DeclaredException) -> int:
    return exception.id
