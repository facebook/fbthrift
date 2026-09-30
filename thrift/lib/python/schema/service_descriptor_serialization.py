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

"""Conversion between runtime service metadata and its canonical wire form.

As in C++, a service is written with its inherited functions flattened into its
own definition, and function responses and streaming unions are flattened into
the runtime ``Function`` model rather than represented by separate wrapper types.
"""

from __future__ import annotations

from collections.abc import Iterable, Mapping, Sequence
from typing import cast, TypeVar

from apache.thrift.type.schema.thrift_types import ErrorBlame, ErrorKind, ErrorSafety
from apache.thrift.type_system.type_id.thrift_types import TypeId
from apache.thrift.type_system.type_system.thrift_types import (
    FieldIdentity as WireFieldIdentity,
)
from thrift.lib.python.schema._digest_common import is_standard_annotation
from thrift.lib.python.schema._record import SerializableRecord
from thrift.lib.python.schema._serializable import (
    annotations_from_wire,
    build_serializable_type_system,
    resolve_type_id,
    to_type_id,
    to_wire_annotations,
)
from thrift.lib.python.schema._serializable_builder import (
    from_serializable as type_system_from_serializable,
)
from thrift.lib.python.schema.service_descriptor import (
    DeclaredException,
    Function,
    FunctionQualifier,
    Interaction,
    Parameter,
    RpcKind,
    ServiceCatalog,
    ServiceDescriptor,
    Sink,
    Stream,
)
from thrift.lib.python.schema.type_system import (
    _type_ref_uris,
    PruneOptions,
    TypeRef,
    TypeSystem,
)
from thrift.lib.python.schema.type_system_digest import type_system_digest
from thrift.lib.thrift.service_catalog.thrift_types import (
    ExceptionBlame as WireExceptionBlame,
    ExceptionKind as WireExceptionKind,
    ExceptionSafety as WireExceptionSafety,
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

__all__ = ["from_serializable", "to_serializable"]


_RuntimeNode = (
    ServiceCatalog
    | ServiceDescriptor
    | Interaction
    | Function
    | Parameter
    | DeclaredException
    | Stream
    | Sink
)
_SerializableNode = (
    SerializableServiceCatalog
    | SerializableInteractionDefinition
    | SerializableFunction
    | SerializableParameter
    | SerializableException
    | SerializableStream
    | SerializableSink
)
_RuntimeNodeT = TypeVar("_RuntimeNodeT", bound=_RuntimeNode)


def to_serializable(value: _RuntimeNode) -> _SerializableNode:
    """Convert a runtime service node to its corresponding wire node.

    Catalogs and service descriptors both become a ``SerializableServiceCatalog``
    carrying the types their functions reference.
    """
    if isinstance(value, ServiceCatalog):
        return _services_to_serializable(value.type_system, value.services)
    if isinstance(value, ServiceDescriptor):
        return _services_to_serializable(value.type_system, (value,))
    if isinstance(value, Interaction):
        return _interaction_to_serializable(value)
    if isinstance(value, Function):
        return _function_to_serializable(value)
    if isinstance(value, Parameter):
        return _parameter_to_serializable(value)
    if isinstance(value, DeclaredException):
        return _exception_to_serializable(value)
    if isinstance(value, Stream):
        return _stream_to_serializable(value)
    if isinstance(value, Sink):
        return _sink_to_serializable(value)
    raise TypeError(f"Unsupported runtime service node: {type(value).__name__}")


def from_serializable(
    runtime_type: type[_RuntimeNodeT],
    value: _SerializableNode,
    *,
    type_system: TypeSystem | None = None,
    uri: str | None = None,
) -> _RuntimeNodeT:
    """Convert a wire node to the requested runtime service node type.

    Catalogs and service descriptors are rebuilt over the catalog's inline types,
    and ``uri`` selects the service to rebuild as a ``ServiceDescriptor``. Other
    nodes resolve their types against ``type_system``; for an ``Interaction`` or
    ``Function``, ``uri`` is the URI of the interface it belongs to.
    """
    if runtime_type is ServiceCatalog and isinstance(value, SerializableServiceCatalog):
        return cast(_RuntimeNodeT, _catalog_from_serializable(value))
    if runtime_type is ServiceDescriptor and isinstance(
        value, SerializableServiceCatalog
    ):
        return cast(
            _RuntimeNodeT, _descriptor_from_serializable(value, _require_uri(uri))
        )
    resolved_types = _require_type_system(type_system)
    if runtime_type is Interaction and isinstance(
        value, SerializableInteractionDefinition
    ):
        return cast(
            _RuntimeNodeT,
            _interaction_from_serializable(value, _require_uri(uri), resolved_types),
        )
    if runtime_type is Function and isinstance(value, SerializableFunction):
        return cast(
            _RuntimeNodeT,
            _function_from_serializable(value, _require_uri(uri), resolved_types),
        )
    if runtime_type is Parameter and isinstance(value, SerializableParameter):
        return cast(_RuntimeNodeT, _parameter_from_serializable(value, resolved_types))
    if runtime_type is DeclaredException and isinstance(value, SerializableException):
        return cast(_RuntimeNodeT, _exception_from_serializable(value, resolved_types))
    if runtime_type is Stream and isinstance(value, SerializableStream):
        return cast(_RuntimeNodeT, _stream_from_serializable(value, resolved_types))
    if runtime_type is Sink and isinstance(value, SerializableSink):
        return cast(_RuntimeNodeT, _sink_from_serializable(value, resolved_types))
    raise TypeError(f"Cannot convert {type(value).__name__} to {runtime_type.__name__}")


def _services_to_serializable(
    type_system: TypeSystem,
    services: Sequence[ServiceDescriptor],
) -> SerializableServiceCatalog:
    interactions: dict[str, Interaction] = {}
    roots: set[str] = set()
    for service in services:
        if not service.service_uri:
            raise ValueError(f"Service {service.service_name!r} has no URI")
        declared = {
            interaction.uri: interaction for interaction in service.interactions
        }
        for interaction in _reachable_interactions(service.functions, declared):
            interactions.setdefault(interaction.uri, interaction)
        _add_interface_roots(roots, service.annotations, service.functions)
    for interaction in interactions.values():
        _add_interface_roots(roots, interaction.annotations, interaction.functions)

    types = build_serializable_type_system(
        type_system,
        sorted(roots),
        PruneOptions(include_source_info=False),
    )
    interfaces = {
        service.service_uri: SerializableRpcInterfaceDefinition(
            serviceDef=_service_to_serializable(service)
        )
        for service in services
    }
    for uri, interaction in interactions.items():
        interfaces[uri] = SerializableRpcInterfaceDefinition(
            interactionDef=_interaction_to_serializable(interaction)
        )
    return SerializableServiceCatalog(
        types=types,
        interfaces=interfaces,
        typesDigest=type_system_digest(types),
    )


def _service_to_serializable(
    service: ServiceDescriptor,
) -> SerializableServiceDefinition:
    return SerializableServiceDefinition(
        functions=[
            _function_to_serializable(fn)
            for fn in service.functions
            if not fn.is_performs
        ],
        performedInteractions={
            _performed_interaction(fn) for fn in service.functions if fn.is_performs
        },
        annotations=to_wire_annotations(service.annotations),
    )


def _performed_interaction(constructor: Function) -> str:
    if constructor.created_interaction_uri is None:
        raise ValueError(
            f"Interaction constructor {constructor.name!r} does not name its "
            "interaction"
        )
    return constructor.created_interaction_uri


def _interaction_to_serializable(
    interaction: Interaction,
) -> SerializableInteractionDefinition:
    return SerializableInteractionDefinition(
        functions=[_function_to_serializable(fn) for fn in interaction.functions],
        annotations=to_wire_annotations(interaction.annotations),
    )


def _catalog_from_serializable(value: SerializableServiceCatalog) -> ServiceCatalog:
    type_system = _inline_type_system(value)
    interactions = _interactions_from_serializable(value, type_system)
    return ServiceCatalog(
        type_system,
        [
            _service_from_serializable(
                definition.serviceDef, uri, type_system, interactions
            )
            for uri, definition in value.interfaces.items()
            if definition.type == SerializableRpcInterfaceDefinition.Type.serviceDef
        ],
    )


def _descriptor_from_serializable(
    value: SerializableServiceCatalog, uri: str
) -> ServiceDescriptor:
    definition = value.interfaces.get(uri)
    if definition is None:
        raise ValueError(f"Service URI not found: {uri!r}")
    if definition.type != SerializableRpcInterfaceDefinition.Type.serviceDef:
        raise ValueError(f"URI does not point to a service: {uri!r}")
    type_system = _inline_type_system(value)
    return _service_from_serializable(
        definition.serviceDef,
        uri,
        type_system,
        _interactions_from_serializable(value, type_system),
    )


def _inline_type_system(value: SerializableServiceCatalog) -> TypeSystem:
    if value.types is None:
        raise ValueError(
            "SerializableServiceCatalog has no inline types to rebuild from"
        )
    return type_system_from_serializable(value.types)


def _interactions_from_serializable(
    value: SerializableServiceCatalog,
    type_system: TypeSystem,
) -> dict[str, Interaction]:
    interactions: dict[str, Interaction] = {}
    for uri, definition in value.interfaces.items():
        if definition.type == SerializableRpcInterfaceDefinition.Type.interactionDef:
            interactions[uri] = _interaction_from_serializable(
                definition.interactionDef, uri, type_system
            )
        elif definition.type != SerializableRpcInterfaceDefinition.Type.serviceDef:
            raise ValueError(f"RPC interface definition for {uri!r} is empty")
    return interactions


def _service_from_serializable(
    value: SerializableServiceDefinition,
    uri: str,
    type_system: TypeSystem,
    interactions: Mapping[str, Interaction],
) -> ServiceDescriptor:
    functions = tuple(
        _function_from_serializable(fn, uri, type_system) for fn in value.functions
    ) + tuple(
        _interaction_constructor(interaction_uri)
        for interaction_uri in sorted(value.performedInteractions)
    )
    return ServiceDescriptor(
        service_name=_name_from_uri(uri),
        service_uri=uri,
        type_system=type_system,
        functions=functions,
        interactions=_reachable_interactions(functions, interactions),
        annotations=annotations_from_wire(
            value.annotations, type_system.get_user_defined_type
        ),
    )


# The empty name marks the constructor as rebuilt rather than declared in IDL.
def _interaction_constructor(interaction_uri: str) -> Function:
    return Function(
        name="",
        uri="",
        params=(),
        response_type=None,
        exceptions=(),
        stream=None,
        sink=None,
        qualifier=FunctionQualifier.Unspecified,
        rpc_kind=RpcKind.Unary,
        created_interaction_uri=interaction_uri,
        is_performs=True,
    )


def _interaction_from_serializable(
    value: SerializableInteractionDefinition,
    uri: str,
    type_system: TypeSystem,
) -> Interaction:
    return Interaction(
        name=_name_from_uri(uri),
        uri=uri,
        functions=tuple(
            _function_from_serializable(fn, uri, type_system) for fn in value.functions
        ),
        annotations=annotations_from_wire(
            value.annotations, type_system.get_user_defined_type
        ),
    )


def _reachable_interactions(
    functions: Iterable[Function],
    interactions: Mapping[str, Interaction],
) -> tuple[Interaction, ...]:
    reached: dict[str, Interaction] = {}

    def visit(pending: Iterable[Function]) -> None:
        for function in pending:
            uri = function.created_interaction_uri
            if uri is None or uri in reached:
                continue
            interaction = interactions.get(uri)
            if interaction is None:
                raise ValueError(
                    f"Function {function.name!r} creates interaction {uri!r}, "
                    "which is not in the catalog"
                )
            reached[uri] = interaction
            visit(interaction.functions)

    visit(functions)
    return tuple(reached.values())


# Wire interfaces are keyed by URI only; like C++, names are recovered from it.
def _name_from_uri(uri: str) -> str:
    return uri.rsplit("/", 1)[-1]


def _function_uri(interface_uri: str, name: str) -> str:
    return f"{interface_uri}/{name}" if interface_uri else name


def _function_to_serializable(function: Function) -> SerializableFunction:
    """Convert a function and its flattened response to wire structs."""
    return SerializableFunction(
        name=function.name,
        qualifier=function.qualifier,
        params=[_parameter_to_serializable(parameter) for parameter in function.params],
        response=_to_serializable_response(function),
        exceptions=[_exception_to_serializable(ex) for ex in function.exceptions],
        rpcKind=function.rpc_kind,
        annotations=to_wire_annotations(function.annotations),
    )


def _to_serializable_response(function: Function) -> SerializableFunctionResponse:
    streaming = None
    if function.stream is not None and function.sink is not None:
        streaming = SerializableStreamingResponse(
            bidirectionalStream=SerializableBidirectionalStream(
                sinkPayloadType=to_type_id(function.sink.payload_type),
                streamPayloadType=to_type_id(function.stream.payload_type),
                sinkExceptions=[
                    _exception_to_serializable(exception)
                    for exception in function.sink.client_exceptions
                ],
                streamExceptions=[
                    _exception_to_serializable(exception)
                    for exception in function.stream.exceptions
                ],
            )
        )
    elif function.stream is not None:
        streaming = SerializableStreamingResponse(
            serverStream=_stream_to_serializable(function.stream)
        )
    elif function.sink is not None:
        streaming = SerializableStreamingResponse(
            clientSink=_sink_to_serializable(function.sink)
        )
    return SerializableFunctionResponse(
        initialResponseType=_optional_type_id(function.response_type),
        streaming=streaming,
        createsInteraction=function.created_interaction_uri,
    )


def _parameter_to_serializable(parameter: Parameter) -> SerializableParameter:
    """Convert a runtime parameter to its canonical wire representation."""
    return SerializableParameter(
        identity=WireFieldIdentity(id=parameter.id, name=parameter.name),
        type=to_type_id(parameter.type),
        annotations=to_wire_annotations(parameter.annotations),
    )


def _exception_to_serializable(
    exception: DeclaredException,
) -> SerializableException:
    """Convert a declared exception to its canonical wire representation."""
    return SerializableException(
        identity=WireFieldIdentity(id=exception.id, name=exception.name),
        type=to_type_id(exception.type),
        annotations=to_wire_annotations(exception.annotations),
        safety=_SAFETY_TO_WIRE[exception.safety],
        kind=_KIND_TO_WIRE[exception.kind],
        blame=_BLAME_TO_WIRE[exception.blame],
    )


def _stream_to_serializable(stream: Stream) -> SerializableStream:
    """Convert a runtime server stream to its canonical wire representation."""
    return SerializableStream(
        payloadType=to_type_id(stream.payload_type),
        exceptions=[_exception_to_serializable(ex) for ex in stream.exceptions],
    )


def _sink_to_serializable(sink: Sink) -> SerializableSink:
    """Convert a runtime client sink to its canonical wire representation."""
    return SerializableSink(
        payloadType=to_type_id(sink.payload_type),
        finalResponseType=_optional_type_id(sink.final_response_type),
        clientExceptions=[
            _exception_to_serializable(ex) for ex in sink.client_exceptions
        ],
        serverExceptions=[
            _exception_to_serializable(ex) for ex in sink.server_exceptions
        ],
    )


def _function_from_serializable(
    function: SerializableFunction,
    interface_uri: str,
    type_system: TypeSystem,
) -> Function:
    """Rebuild a runtime function from its canonical wire representation."""
    response = function.response
    stream, sink = _from_serializable_streaming(response.streaming, type_system)
    return Function(
        name=function.name,
        uri=_function_uri(interface_uri, function.name),
        params=tuple(
            _parameter_from_serializable(parameter, type_system)
            for parameter in function.params
        ),
        response_type=(
            resolve_type_id(response.initialResponseType, type_system)
            if response.initialResponseType is not None
            else None
        ),
        exceptions=tuple(
            _exception_from_serializable(exception, type_system)
            for exception in function.exceptions
        ),
        stream=stream,
        sink=sink,
        qualifier=function.qualifier,
        rpc_kind=function.rpcKind,
        created_interaction_uri=response.createsInteraction,
        is_performs=False,
        annotations=annotations_from_wire(
            function.annotations,
            type_system.get_user_defined_type,
        ),
    )


def _from_serializable_streaming(
    streaming: SerializableStreamingResponse | None,
    type_system: TypeSystem,
) -> tuple[Stream | None, Sink | None]:
    if streaming is None:
        return None, None
    if streaming.type == SerializableStreamingResponse.Type.bidirectionalStream:
        value = streaming.bidirectionalStream
        return (
            Stream(
                payload_type=resolve_type_id(value.streamPayloadType, type_system),
                exceptions=tuple(
                    _exception_from_serializable(exception, type_system)
                    for exception in value.streamExceptions
                ),
            ),
            Sink(
                payload_type=resolve_type_id(value.sinkPayloadType, type_system),
                final_response_type=None,
                client_exceptions=tuple(
                    _exception_from_serializable(exception, type_system)
                    for exception in value.sinkExceptions
                ),
            ),
        )
    if streaming.type == SerializableStreamingResponse.Type.serverStream:
        return _stream_from_serializable(streaming.serverStream, type_system), None
    if streaming.type == SerializableStreamingResponse.Type.clientSink:
        return None, _sink_from_serializable(streaming.clientSink, type_system)
    return None, None


def _stream_from_serializable(
    stream: SerializableStream,
    type_system: TypeSystem,
) -> Stream:
    """Rebuild a runtime server stream from its canonical wire representation."""
    return Stream(
        payload_type=resolve_type_id(stream.payloadType, type_system),
        exceptions=tuple(
            _exception_from_serializable(exception, type_system)
            for exception in stream.exceptions
        ),
    )


def _sink_from_serializable(
    sink: SerializableSink,
    type_system: TypeSystem,
) -> Sink:
    """Rebuild a runtime client sink from its canonical wire representation."""
    return Sink(
        payload_type=resolve_type_id(sink.payloadType, type_system),
        final_response_type=(
            resolve_type_id(sink.finalResponseType, type_system)
            if sink.finalResponseType is not None
            else None
        ),
        client_exceptions=tuple(
            _exception_from_serializable(exception, type_system)
            for exception in sink.clientExceptions
        ),
        server_exceptions=tuple(
            _exception_from_serializable(exception, type_system)
            for exception in sink.serverExceptions
        ),
    )


def _parameter_from_serializable(
    parameter: SerializableParameter,
    type_system: TypeSystem,
) -> Parameter:
    """Rebuild a runtime parameter from its canonical wire representation."""
    return Parameter(
        name=parameter.identity.name,
        id=parameter.identity.id,
        type=resolve_type_id(parameter.type, type_system),
        annotations=annotations_from_wire(
            parameter.annotations,
            type_system.get_user_defined_type,
        ),
    )


def _exception_from_serializable(
    exception: SerializableException,
    type_system: TypeSystem,
) -> DeclaredException:
    """Rebuild a declared exception from its canonical wire representation."""
    return DeclaredException(
        name=exception.identity.name,
        id=exception.identity.id,
        type=resolve_type_id(exception.type, type_system),
        annotations=annotations_from_wire(
            exception.annotations,
            type_system.get_user_defined_type,
        ),
        safety=_SAFETY_FROM_WIRE.get(exception.safety, ErrorSafety.Unspecified),
        kind=_KIND_FROM_WIRE.get(exception.kind, ErrorKind.Unspecified),
        blame=_BLAME_FROM_WIRE.get(exception.blame, ErrorBlame.Unspecified),
    )


def _optional_type_id(type_ref: TypeRef | None) -> TypeId | None:
    return to_type_id(type_ref) if type_ref is not None else None


def _add_interface_roots(
    roots: set[str],
    annotations: Mapping[str, SerializableRecord],
    functions: Iterable[Function],
) -> None:
    _add_annotation_roots(roots, annotations)
    for function in functions:
        _add_function_roots(roots, function)


def _add_function_roots(roots: set[str], function: Function) -> None:
    _add_annotation_roots(roots, function.annotations)
    for parameter in function.params:
        _add_type_roots(roots, parameter.type)
        _add_annotation_roots(roots, parameter.annotations)
    if function.response_type is not None:
        _add_type_roots(roots, function.response_type)
    for exception in function.exceptions:
        _add_exception_roots(roots, exception)
    if function.stream is not None:
        _add_type_roots(roots, function.stream.payload_type)
        for exception in function.stream.exceptions:
            _add_exception_roots(roots, exception)
    if function.sink is not None:
        _add_type_roots(roots, function.sink.payload_type)
        if function.sink.final_response_type is not None:
            _add_type_roots(roots, function.sink.final_response_type)
        for exception in function.sink.client_exceptions:
            _add_exception_roots(roots, exception)
        for exception in function.sink.server_exceptions:
            _add_exception_roots(roots, exception)


def _add_exception_roots(roots: set[str], exception: DeclaredException) -> None:
    _add_type_roots(roots, exception.type)
    _add_annotation_roots(roots, exception.annotations)


def _add_type_roots(roots: set[str], type_ref: TypeRef) -> None:
    roots.update(_type_ref_uris(type_ref))


def _add_annotation_roots(
    roots: set[str], annotations: Mapping[str, SerializableRecord]
) -> None:
    roots.update(uri for uri in annotations if not is_standard_annotation(uri))


def _require_type_system(value: TypeSystem | None) -> TypeSystem:
    if value is None:
        raise TypeError("type_system is required for this conversion")
    return value


def _require_uri(value: str | None) -> str:
    if value is None:
        raise TypeError("uri is required for this conversion")
    return value


_SAFETY_TO_WIRE = {
    ErrorSafety.Unspecified: WireExceptionSafety.Unspecified,
    ErrorSafety.Safe: WireExceptionSafety.Safe,
}
_KIND_TO_WIRE = {
    ErrorKind.Unspecified: WireExceptionKind.Unspecified,
    ErrorKind.Transient: WireExceptionKind.Transient,
    ErrorKind.Stateful: WireExceptionKind.Stateful,
    ErrorKind.Permanent: WireExceptionKind.Permanent,
}
_BLAME_TO_WIRE = {
    ErrorBlame.Unspecified: WireExceptionBlame.Unspecified,
    ErrorBlame.Server: WireExceptionBlame.Server,
    ErrorBlame.Client: WireExceptionBlame.Client,
}
_SAFETY_FROM_WIRE = {value: key for key, value in _SAFETY_TO_WIRE.items()}
_KIND_FROM_WIRE = {value: key for key, value in _KIND_TO_WIRE.items()}
_BLAME_FROM_WIRE = {value: key for key, value in _BLAME_TO_WIRE.items()}
