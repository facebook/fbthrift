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

"""Runtime descriptions of Thrift services and their payload type system."""

from __future__ import annotations

from collections.abc import Mapping, Sequence
from dataclasses import dataclass, field
from types import MappingProxyType
from typing import Any

from apache.thrift.type.schema.thrift_types import ErrorBlame, ErrorKind, ErrorSafety
from thrift.lib.python.schema import syntax_graph as _ast
from thrift.lib.python.schema._record import SerializableRecord
from thrift.lib.python.schema.schema_registry import SchemaRegistry
from thrift.lib.python.schema.type_system import (
    FieldDefinition,
    FieldIdentity,
    PresenceQualifier,
    TypeRef,
    TypeSystem,
)
from thrift.lib.thrift.service_catalog.thrift_types import FunctionQualifier, RpcKind


__all__ = [
    "AnnotationsMap",
    "DeclaredException",
    "ErrorBlame",
    "ErrorKind",
    "ErrorSafety",
    "Function",
    "FunctionQualifier",
    "Interaction",
    "Parameter",
    "RpcKind",
    "RpcStruct",
    "ServiceDescriptor",
    "Sink",
    "Stream",
]


AnnotationsMap = Mapping[str, SerializableRecord]


def _annotations(values: AnnotationsMap) -> AnnotationsMap:
    return MappingProxyType(dict(values))


@dataclass(frozen=True)
class Parameter:
    """A declared function parameter."""

    name: str
    id: int
    type: TypeRef
    annotations: AnnotationsMap = field(default_factory=dict)

    def __post_init__(self) -> None:
        object.__setattr__(self, "annotations", _annotations(self.annotations))


@dataclass(frozen=True)
class DeclaredException:
    """A declared function, stream, or sink exception."""

    name: str
    id: int
    type: TypeRef
    annotations: AnnotationsMap = field(default_factory=dict)
    safety: ErrorSafety = ErrorSafety.Unspecified
    kind: ErrorKind = ErrorKind.Unspecified
    blame: ErrorBlame = ErrorBlame.Unspecified

    def __post_init__(self) -> None:
        object.__setattr__(self, "annotations", _annotations(self.annotations))


@dataclass(frozen=True)
class Stream:
    """The server-streaming portion of an RPC response."""

    payload_type: TypeRef
    exceptions: tuple[DeclaredException, ...] = ()

    def __post_init__(self) -> None:
        object.__setattr__(self, "exceptions", tuple(self.exceptions))


@dataclass(frozen=True)
class Sink:
    """The client-streaming portion of an RPC response."""

    payload_type: TypeRef
    final_response_type: TypeRef | None
    client_exceptions: tuple[DeclaredException, ...] = ()
    server_exceptions: tuple[DeclaredException, ...] = ()

    def __post_init__(self) -> None:
        object.__setattr__(self, "client_exceptions", tuple(self.client_exceptions))
        object.__setattr__(self, "server_exceptions", tuple(self.server_exceptions))


@dataclass(frozen=True)
class RpcStruct:
    """A headerless RPC request or response envelope."""

    fields: tuple[FieldDefinition, ...]

    def __post_init__(self) -> None:
        object.__setattr__(self, "fields", tuple(self.fields))


@dataclass(frozen=True)
class Function:
    """A runtime description of one service or interaction function."""

    name: str
    uri: str
    params: tuple[Parameter, ...]
    response_type: TypeRef | None
    exceptions: tuple[DeclaredException, ...]
    stream: Stream | None
    sink: Sink | None
    qualifier: FunctionQualifier
    rpc_kind: RpcKind
    created_interaction_uri: str | None
    is_performs: bool
    annotations: AnnotationsMap = field(default_factory=dict)
    doc_block: str | None = None

    def __post_init__(self) -> None:
        object.__setattr__(self, "params", tuple(self.params))
        object.__setattr__(self, "exceptions", tuple(self.exceptions))
        object.__setattr__(self, "annotations", _annotations(self.annotations))

    def request_envelope(self) -> RpcStruct:
        return RpcStruct(
            tuple(
                FieldDefinition(
                    identity=FieldIdentity(param.id, param.name),
                    presence=PresenceQualifier.UNQUALIFIED,
                    type=param.type,
                )
                for param in self.params
            )
        )

    def response_envelope(self) -> RpcStruct:
        fields = []
        if self.response_type is not None:
            fields.append(
                FieldDefinition(
                    identity=FieldIdentity(0, "success"),
                    presence=PresenceQualifier.OPTIONAL,
                    type=self.response_type,
                )
            )
        fields.extend(
            FieldDefinition(
                identity=FieldIdentity(exception.id, exception.name),
                presence=PresenceQualifier.OPTIONAL,
                type=exception.type,
            )
            for exception in self.exceptions
        )
        return RpcStruct(tuple(fields))


@dataclass(frozen=True)
class Interaction:
    """A runtime description of a reachable interaction."""

    name: str
    uri: str
    functions: tuple[Function, ...]
    annotations: AnnotationsMap = field(default_factory=dict)
    _functions_by_uri: Mapping[str, Function] = field(
        init=False, repr=False, compare=False
    )
    _functions_by_name: Mapping[str, Function] = field(
        init=False, repr=False, compare=False
    )

    def __post_init__(self) -> None:
        functions = tuple(self.functions)
        object.__setattr__(self, "functions", functions)
        object.__setattr__(self, "annotations", _annotations(self.annotations))
        object.__setattr__(
            self,
            "_functions_by_uri",
            MappingProxyType({function.uri: function for function in functions}),
        )
        object.__setattr__(
            self,
            "_functions_by_name",
            MappingProxyType({function.name: function for function in functions}),
        )

    def get_function(self, uri: str) -> Function:
        try:
            return self._functions_by_uri[uri]
        except KeyError:
            raise KeyError(f"No function with URI {uri!r}") from None

    def get_function_by_name(self, name: str) -> Function:
        try:
            return self._functions_by_name[name]
        except KeyError:
            raise KeyError(f"No function named {name!r}") from None


@dataclass(frozen=True)
class ServiceDescriptor:
    """An immutable runtime view of a generated Thrift service."""

    service_name: str
    service_uri: str
    type_system: TypeSystem
    functions: tuple[Function, ...]
    interactions: tuple[Interaction, ...]
    annotations: AnnotationsMap = field(default_factory=dict)
    _functions_by_uri: Mapping[str, Function] = field(
        init=False, repr=False, compare=False
    )
    _functions_by_name: Mapping[str, Function] = field(
        init=False, repr=False, compare=False
    )
    _interactions_by_uri: Mapping[str, Interaction] = field(
        init=False, repr=False, compare=False
    )

    def __post_init__(self) -> None:
        functions = tuple(self.functions)
        interactions = tuple(self.interactions)
        object.__setattr__(self, "functions", functions)
        object.__setattr__(self, "interactions", interactions)
        object.__setattr__(self, "annotations", _annotations(self.annotations))
        object.__setattr__(
            self,
            "_functions_by_uri",
            MappingProxyType({function.uri: function for function in functions}),
        )
        object.__setattr__(
            self,
            "_functions_by_name",
            MappingProxyType({function.name: function for function in functions}),
        )
        object.__setattr__(
            self,
            "_interactions_by_uri",
            MappingProxyType(
                {interaction.uri: interaction for interaction in interactions}
            ),
        )

    @classmethod
    def from_service(
        cls,
        service: type[Any],
        registry: SchemaRegistry | None = None,
    ) -> ServiceDescriptor:
        registry = SchemaRegistry.get() if registry is None else registry
        node = registry.get_node(service)
        if not isinstance(node, _ast.ServiceNode):
            raise TypeError(f"{service.__name__} is not a Thrift service")
        return _DescriptorBuilder(registry).build_service(node)

    def get_function(self, uri: str) -> Function:
        try:
            return self._functions_by_uri[uri]
        except KeyError:
            raise KeyError(f"No function with URI {uri!r}") from None

    def get_function_by_name(self, name: str) -> Function:
        try:
            return self._functions_by_name[name]
        except KeyError:
            raise KeyError(f"No function named {name!r}") from None

    def get_interaction(self, uri: str) -> Interaction:
        try:
            return self._interactions_by_uri[uri]
        except KeyError:
            raise KeyError(f"No interaction with URI {uri!r}") from None


class _DescriptorBuilder:
    __slots__ = ("_registry",)

    def __init__(self, registry: SchemaRegistry) -> None:
        self._registry = registry

    def build_service(self, service: _ast.ServiceNode) -> ServiceDescriptor:
        interactions: list[Interaction] = []
        self._collect_service_interactions(service, interactions, set())
        functions: list[Function] = []
        self._collect_functions(service, functions)
        return ServiceDescriptor(
            service_name=service.name,
            service_uri=service.uri,
            type_system=self._registry,
            functions=tuple(functions),
            interactions=tuple(interactions),
            annotations=self._convert_annotations(service.annotations),
        )

    def _collect_functions(
        self, service: _ast.ServiceNode, functions: list[Function]
    ) -> None:
        if service.base_service is not None:
            self._collect_functions(service.base_service, functions)
        functions.extend(
            self._build_function(function, service.uri)
            for function in service.functions
        )

    def _collect_service_interactions(
        self,
        service: _ast.ServiceNode,
        interactions: list[Interaction],
        seen_uris: set[str],
    ) -> None:
        if service.base_service is not None:
            self._collect_service_interactions(
                service.base_service, interactions, seen_uris
            )
        for function in service.functions:
            self._collect_created_interaction(function, interactions, seen_uris)

    def _collect_created_interaction(
        self,
        function: _ast.FunctionNode,
        interactions: list[Interaction],
        seen_uris: set[str],
    ) -> None:
        interaction = function.response.interaction
        if interaction is None:
            return
        if not interaction.uri:
            raise ValueError(f"Interaction {interaction.name!r} has no URI")
        if interaction.uri in seen_uris:
            return
        seen_uris.add(interaction.uri)
        result = Interaction(
            name=interaction.name,
            uri=interaction.uri,
            functions=tuple(
                self._build_function(fn, interaction.uri)
                for fn in interaction.functions
            ),
            annotations=self._convert_annotations(interaction.annotations),
        )
        interactions.append(result)
        for nested_function in interaction.functions:
            self._collect_created_interaction(nested_function, interactions, seen_uris)

    def _build_function(
        self, function: _ast.FunctionNode, interface_uri: str
    ) -> Function:
        response = function.response
        return Function(
            name=function.name,
            uri=_function_uri(interface_uri, function.name),
            params=tuple(self._build_parameter(param) for param in function.params),
            response_type=self._convert_optional_type(response.type),
            exceptions=tuple(
                self._build_exception(exception) for exception in function.exceptions
            ),
            stream=self._build_stream(response.stream),
            sink=self._build_sink(response.sink),
            qualifier=function.descriptor_qualifier,
            rpc_kind=function.rpc_kind,
            created_interaction_uri=(
                response.interaction.uri if response.interaction is not None else None
            ),
            is_performs=function.is_performs,
            annotations=self._convert_annotations(function.annotations),
            doc_block=function.doc_block,
        )

    def _build_parameter(self, param: _ast.FunctionParam) -> Parameter:
        return Parameter(
            name=param.name,
            id=param.id,
            type=self._registry.as_type_system_type_ref(param.type),
            annotations=self._convert_annotations(param.annotations),
        )

    def _build_exception(self, exception: _ast.FunctionException) -> DeclaredException:
        return DeclaredException(
            name=exception.name,
            id=exception.id,
            type=self._registry.as_type_system_type_ref(exception.type),
            annotations=self._convert_annotations(exception.annotations),
            safety=exception.safety,
            kind=exception.kind,
            blame=exception.blame,
        )

    def _build_stream(self, stream: _ast.FunctionStream | None) -> Stream | None:
        if stream is None:
            return None
        return Stream(
            payload_type=self._registry.as_type_system_type_ref(stream.payload_type),
            exceptions=tuple(
                self._build_exception(exception) for exception in stream.exceptions
            ),
        )

    def _build_sink(self, sink: _ast.FunctionSink | None) -> Sink | None:
        if sink is None:
            return None
        return Sink(
            payload_type=self._registry.as_type_system_type_ref(sink.payload_type),
            final_response_type=self._convert_optional_type(sink.final_response_type),
            client_exceptions=tuple(
                self._build_exception(exception) for exception in sink.client_exceptions
            ),
            server_exceptions=tuple(
                self._build_exception(exception) for exception in sink.server_exceptions
            ),
        )

    def _convert_optional_type(self, type_ref: _ast.TypeRef | None) -> TypeRef | None:
        if type_ref is None:
            return None
        return self._registry.as_type_system_type_ref(type_ref)

    def _convert_annotations(
        self, annotations: Sequence[_ast.Annotation]
    ) -> AnnotationsMap:
        return self._registry.as_type_system_annotations(annotations)


def _function_uri(interface_uri: str, name: str) -> str:
    return f"{interface_uri}/{name}" if interface_uri else name
