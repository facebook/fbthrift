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

import dataclasses
import unittest
from collections.abc import Callable, Iterator
from typing import Any

import apache.thrift.dynamic.test.service_catalog_digest_fixture.thrift_services as Fixture
import thrift.lib.python.schema.tests.service_descriptor_test.thrift_services as TestServices
from apache.thrift.dynamic.test.service_catalog_digest_expected_values.thrift_types import (
    DIGEST_CALCULATOR,
    DIGEST_PERFORMED_INTERACTION,
    DIGEST_RICH_DESCRIPTOR,
    DIGEST_RICH_DESCRIPTOR_STRUCTURAL,
)
from thrift.lib.python.schema._digest_common import _Hasher
from thrift.lib.python.schema.schema_registry import SchemaRegistry
from thrift.lib.python.schema.service_catalog_digest import (
    _hash_runtime_bidirectional_stream,
    _hash_runtime_interface,
    _hash_runtime_response,
    _hash_runtime_service,
    _hash_runtime_streaming,
    DigestMode,
    service_catalog_digest,
    SERVICE_CATALOG_DIGEST_VERSION,
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
from thrift.lib.python.schema.service_descriptor_serialization import to_serializable
from thrift.lib.thrift.service_catalog.thrift_types import (
    SerializableFunction,
    SerializableServiceCatalog,
)

_Node = Interaction | Function | Parameter | DeclaredException | Stream | Sink


def _fixture(service: type[Any]) -> ServiceDescriptor:
    return ServiceDescriptor.from_service(service, SchemaRegistry())


# Every fixture service, including one with a plain server stream.
_SERVICES: tuple[type[Any], ...] = (
    Fixture.CalculatorInterface,
    Fixture.CatalogGoldenInterface,
    Fixture.SessionServiceInterface,
    TestServices.TestServiceInterface,
)


def _as_catalog(descriptor: ServiceDescriptor) -> ServiceCatalog:
    return ServiceCatalog(descriptor.type_system, (descriptor,))


def _function_nodes(function: Function) -> Iterator[_Node]:
    yield function
    yield from function.params
    yield from function.exceptions
    if function.stream is not None:
        yield function.stream
        yield from function.stream.exceptions
    if function.sink is not None:
        yield function.sink
        yield from function.sink.client_exceptions
        yield from function.sink.server_exceptions


def _functions(descriptor: ServiceDescriptor) -> Iterator[Function]:
    yield from (f for f in descriptor.functions if not f.is_performs)
    for interaction in descriptor.interactions:
        yield from interaction.functions


def _nodes(descriptor: ServiceDescriptor) -> Iterator[_Node]:
    yield from descriptor.interactions
    for function in _functions(descriptor):
        yield from _function_nodes(function)


def _projection_digest(
    hash_projection: Callable[..., None], *values: Any, mode: DigestMode
) -> bytes:
    h = _Hasher(mode)
    hash_projection(h, *values)
    return h.finalize()


class ServiceCatalogDigestTest(unittest.TestCase):
    def assertGoldenDigest(
        self,
        descriptor: ServiceDescriptor,
        expected: str,
        mode: DigestMode = DigestMode.FULL,
    ) -> None:
        self.assertEqual(service_catalog_digest(descriptor, mode).hex(), expected)
        self.assertEqual(
            service_catalog_digest(_as_catalog(descriptor), mode).hex(), expected
        )

    def test_version_matches_cpp_and_rust(self) -> None:
        self.assertEqual(SERVICE_CATALOG_DIGEST_VERSION, 2)

    def test_golden_calculator_digest(self) -> None:
        self.assertGoldenDigest(
            _fixture(Fixture.CalculatorInterface), DIGEST_CALCULATOR
        )

    def test_golden_rich_descriptor_digest(self) -> None:
        self.assertGoldenDigest(
            _fixture(Fixture.CatalogGoldenInterface), DIGEST_RICH_DESCRIPTOR
        )

    def test_golden_rich_descriptor_structural_digest(self) -> None:
        self.assertGoldenDigest(
            _fixture(Fixture.CatalogGoldenInterface),
            DIGEST_RICH_DESCRIPTOR_STRUCTURAL,
            DigestMode.STRUCTURAL,
        )

    def test_golden_performed_interaction_digest(self) -> None:
        self.assertGoldenDigest(
            _fixture(Fixture.SessionServiceInterface), DIGEST_PERFORMED_INTERACTION
        )

    def test_unnamed_interaction_constructors_all_count(self) -> None:
        descriptor = _fixture(Fixture.SessionServiceInterface)
        (session,) = descriptor.interactions
        second = dataclasses.replace(session, name="Second", uri=f"{session.uri}Second")
        factory = descriptor.get_function_by_name("openSession")
        constructor = descriptor.get_function_by_name("createSession")
        constructors = (
            constructor,
            dataclasses.replace(
                constructor, name="createSecond", created_interaction_uri=second.uri
            ),
        )
        named = dataclasses.replace(
            descriptor,
            functions=(factory, *constructors),
            interactions=(session, second),
        )
        unnamed = dataclasses.replace(
            named,
            functions=(
                factory,
                *(dataclasses.replace(c, name="", uri="") for c in constructors),
            ),
        )

        self.assertEqual(service_catalog_digest(unnamed), service_catalog_digest(named))

    def test_ignores_function_and_parameter_order(self) -> None:
        descriptor = _fixture(Fixture.CalculatorInterface)
        reordered = dataclasses.replace(
            descriptor,
            functions=tuple(
                dataclasses.replace(function, params=function.params[::-1])
                for function in reversed(descriptor.functions)
            ),
        )

        self.assertEqual(
            service_catalog_digest(reordered), service_catalog_digest(descriptor)
        )

    def test_distinguishes_interaction_constructor_from_factory(self) -> None:
        descriptor = _fixture(Fixture.SessionServiceInterface)
        as_factory = dataclasses.replace(
            descriptor,
            functions=tuple(
                dataclasses.replace(function, is_performs=False)
                for function in descriptor.functions
            ),
        )

        self.assertNotEqual(
            service_catalog_digest(as_factory), service_catalog_digest(descriptor)
        )

    def test_structural_mode_ignores_annotations(self) -> None:
        annotated = _fixture(Fixture.CatalogGoldenInterface)
        unannotated = dataclasses.replace(annotated, annotations={})

        self.assertNotEqual(
            service_catalog_digest(unannotated), service_catalog_digest(annotated)
        )
        self.assertEqual(
            service_catalog_digest(unannotated, DigestMode.STRUCTURAL),
            service_catalog_digest(annotated, DigestMode.STRUCTURAL),
        )

    def test_rejects_interaction_missing_from_the_catalog(self) -> None:
        descriptor = dataclasses.replace(
            _fixture(Fixture.SessionServiceInterface), interactions=()
        )

        with self.assertRaisesRegex(ValueError, "not in the catalog"):
            service_catalog_digest(descriptor)

    def test_runtime_nodes_digest_like_their_serialized_form(self) -> None:
        for service in _SERVICES:
            for mode in DigestMode:
                for node in _nodes(_fixture(service)):
                    with self.subTest(
                        service=service.__name__, mode=mode, node=type(node).__name__
                    ):
                        self.assertEqual(
                            service_catalog_digest(node, mode),
                            service_catalog_digest(to_serializable(node), mode),
                        )

    def test_runtime_projections_digest_like_their_serialized_nodes(self) -> None:
        for service in _SERVICES:
            for mode in DigestMode:
                with self.subTest(service=service.__name__, mode=mode):
                    self._check_projections(_fixture(service), mode)

    def _check_projections(
        self, descriptor: ServiceDescriptor, mode: DigestMode
    ) -> None:
        wire = to_serializable(descriptor)
        assert isinstance(wire, SerializableServiceCatalog)
        interface = wire.interfaces[descriptor.service_uri]
        self.assertEqual(
            _projection_digest(_hash_runtime_service, descriptor, mode=mode),
            service_catalog_digest(interface.serviceDef, mode),
        )
        runtime_interfaces: dict[str, ServiceDescriptor | Interaction] = {
            descriptor.service_uri: descriptor,
            **{interaction.uri: interaction for interaction in descriptor.interactions},
        }
        for uri, runtime_interface in runtime_interfaces.items():
            self.assertEqual(
                _projection_digest(
                    _hash_runtime_interface, runtime_interface, mode=mode
                ),
                service_catalog_digest(wire.interfaces[uri], mode),
            )
        for function in _functions(descriptor):
            serialized = to_serializable(function)
            assert isinstance(serialized, SerializableFunction)
            response = serialized.response
            self.assertEqual(
                _projection_digest(_hash_runtime_response, function, mode=mode),
                service_catalog_digest(response, mode),
            )
            if response.streaming is None:
                continue
            self.assertEqual(
                _projection_digest(
                    _hash_runtime_streaming, function.stream, function.sink, mode=mode
                ),
                service_catalog_digest(response.streaming, mode),
            )
            if function.stream is not None and function.sink is not None:
                self.assertEqual(
                    _projection_digest(
                        _hash_runtime_bidirectional_stream,
                        function.stream,
                        function.sink,
                        mode=mode,
                    ),
                    service_catalog_digest(
                        response.streaming.bidirectionalStream, mode
                    ),
                )

    def test_node_digest_ignores_parameter_order(self) -> None:
        function = _fixture(Fixture.CalculatorInterface).get_function_by_name("add")
        reordered = dataclasses.replace(function, params=function.params[::-1])

        self.assertEqual(
            service_catalog_digest(reordered), service_catalog_digest(function)
        )

    def test_interaction_constructor_has_no_node_digest(self) -> None:
        constructor = _fixture(Fixture.SessionServiceInterface).get_function_by_name(
            "createSession"
        )

        with self.assertRaisesRegex(ValueError, "digested as part of its service"):
            service_catalog_digest(constructor)

    def test_rejects_malformed_out_of_band_types_digest(self) -> None:
        catalog = SerializableServiceCatalog(typesDigest=b"short")

        with self.assertRaisesRegex(ValueError, "no valid type system digest"):
            service_catalog_digest(catalog)
