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

import apache.thrift.dynamic.test.service_catalog_digest_fixture.thrift_services as DigestFixture
import thrift.lib.python.schema.tests.service_descriptor_test.thrift_services as TestServices
from apache.thrift.dynamic.test.service_catalog_digest_expected_values.thrift_types import (
    DIGEST_CALCULATOR,
    DIGEST_PERFORMED_INTERACTION,
    DIGEST_RICH_DESCRIPTOR,
    DIGEST_RICH_DESCRIPTOR_STRUCTURAL,
)
from thrift.lib.python.schema.schema_registry import SchemaRegistry
from thrift.lib.python.schema.service_catalog_digest import (
    DigestMode,
    service_catalog_digest,
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
from thrift.lib.python.schema.service_descriptor_serialization import (
    from_serializable,
    to_serializable,
)
from thrift.lib.python.schema.type_system_digest import type_system_digest
from thrift.lib.thrift.service_catalog.thrift_types import (
    SerializableRpcInterfaceDefinition,
    SerializableServiceCatalog,
)
from thrift.python.serializer import deserialize, serialize


_SERVICE_URI = "thrift.com/python/schema/service_descriptor_test/TestService"
_BASE_SERVICE_URI = "thrift.com/python/schema/service_descriptor_test/BaseService"
_INTERACTION_URI = "thrift.com/python/schema/service_descriptor_test/TestInteraction"
_REQUEST_URI = "thrift.com/python/schema/service_descriptor_test/Request"


class ServiceDescriptorSerializationTest(unittest.TestCase):
    def setUp(self) -> None:
        self.registry = SchemaRegistry()
        self.descriptor = ServiceDescriptor.from_service(
            TestServices.TestServiceInterface,
            self.registry,
        )

    def test_descriptor_compact_round_trip(self) -> None:
        wire = to_serializable(self.descriptor)
        assert isinstance(wire, SerializableServiceCatalog)
        self.assertEqual(len(wire.typesDigest), 32)
        self.assertCountEqual(wire.interfaces, [_SERVICE_URI, _INTERACTION_URI])

        decoded = deserialize(SerializableServiceCatalog, serialize(wire))
        descriptor = from_serializable(ServiceDescriptor, decoded, uri=_SERVICE_URI)

        self.assertEqual(descriptor.service_name, "TestService")
        self.assertEqual(descriptor.service_uri, _SERVICE_URI)
        self.assertIsNotNone(descriptor.type_system.get_user_defined_type(_REQUEST_URI))
        self.assertEqual(
            [interaction.uri for interaction in descriptor.interactions],
            [_INTERACTION_URI],
        )
        rebuilt = to_serializable(descriptor)
        assert isinstance(rebuilt, SerializableServiceCatalog)
        self.assertEqual(rebuilt.interfaces, wire.interfaces)
        self.assertEqual(rebuilt.typesDigest, wire.typesDigest)

    def test_service_definition_flattens_inherited_functions(self) -> None:
        wire = to_serializable(self.descriptor)
        assert isinstance(wire, SerializableServiceCatalog)
        service = wire.interfaces[_SERVICE_URI].serviceDef

        self.assertIsNone(service.baseService)
        self.assertEqual(
            [function.name for function in service.functions],
            [
                function.name
                for function in self.descriptor.functions
                if not function.is_performs
            ],
        )
        self.assertIn("inherited", [function.name for function in service.functions])

    def test_interaction_constructors_round_trip_as_performed_interactions(
        self,
    ) -> None:
        wire = to_serializable(self.descriptor)
        assert isinstance(wire, SerializableServiceCatalog)
        service = wire.interfaces[_SERVICE_URI].serviceDef

        self.assertEqual(set(service.performedInteractions), {_INTERACTION_URI})
        self.assertNotIn(
            "createTestInteraction", [function.name for function in service.functions]
        )

        rebuilt = from_serializable(ServiceDescriptor, wire, uri=_SERVICE_URI)
        factory = rebuilt.get_function_by_name("createInteraction")
        (constructor,) = (
            function for function in rebuilt.functions if function.is_performs
        )

        self.assertFalse(factory.is_performs)
        self.assertEqual(factory.created_interaction_uri, _INTERACTION_URI)
        self.assertEqual(constructor.name, "")
        self.assertEqual(constructor.uri, "")
        self.assertEqual(constructor.created_interaction_uri, _INTERACTION_URI)
        self.assertEqual(constructor.params, ())
        self.assertIsNone(constructor.response_type)

    def test_unknown_performed_interaction_is_rejected(self) -> None:
        wire = to_serializable(self.descriptor)
        assert isinstance(wire, SerializableServiceCatalog)
        service = wire.interfaces[_SERVICE_URI].serviceDef
        interfaces = dict(wire.interfaces)
        interfaces[_SERVICE_URI] = SerializableRpcInterfaceDefinition(
            serviceDef=service(performedInteractions={"test.dev/MissingInteraction"})
        )
        corrupted = wire(interfaces=interfaces)

        with self.assertRaisesRegex(ValueError, "not in the catalog"):
            from_serializable(ServiceDescriptor, corrupted, uri=_SERVICE_URI)

    def test_serialized_fixtures_match_golden_digests(self) -> None:
        cases = (
            (DigestFixture.CalculatorInterface, DigestMode.FULL, DIGEST_CALCULATOR),
            (
                DigestFixture.CatalogGoldenInterface,
                DigestMode.FULL,
                DIGEST_RICH_DESCRIPTOR,
            ),
            (
                DigestFixture.CatalogGoldenInterface,
                DigestMode.STRUCTURAL,
                DIGEST_RICH_DESCRIPTOR_STRUCTURAL,
            ),
            (
                DigestFixture.SessionServiceInterface,
                DigestMode.FULL,
                DIGEST_PERFORMED_INTERACTION,
            ),
        )
        for service, mode, expected in cases:
            with self.subTest(service=service.__name__, mode=mode):
                wire = to_serializable(
                    ServiceDescriptor.from_service(service, SchemaRegistry())
                )
                assert isinstance(wire, SerializableServiceCatalog)
                types = wire.types
                assert types is not None
                out_of_band = wire(
                    types=None, typesDigest=type_system_digest(types, mode)
                )

                self.assertEqual(service_catalog_digest(wire, mode).hex(), expected)
                self.assertEqual(
                    service_catalog_digest(out_of_band, mode).hex(), expected
                )

    def test_serialized_form_digests_like_the_runtime_nodes(self) -> None:
        catalog = ServiceCatalog.from_service(TestServices.TestServiceInterface)

        for mode in DigestMode:
            for runtime in (self.descriptor, catalog):
                with self.subTest(mode=mode, runtime=type(runtime).__name__):
                    wire = to_serializable(runtime)
                    assert isinstance(wire, SerializableServiceCatalog)
                    self.assertEqual(
                        service_catalog_digest(wire, mode),
                        service_catalog_digest(runtime, mode),
                    )

    def test_catalog_round_trip(self) -> None:
        catalog = ServiceCatalog.from_service(TestServices.TestServiceInterface)
        wire = to_serializable(catalog)
        assert isinstance(wire, SerializableServiceCatalog)
        self.assertCountEqual(
            wire.interfaces,
            [_BASE_SERVICE_URI, _SERVICE_URI, _INTERACTION_URI],
        )

        rebuilt = from_serializable(ServiceCatalog, wire)

        self.assertCountEqual(rebuilt.service_uris, catalog.service_uris)
        self.assertIs(
            rebuilt.get_service_or_throw(_SERVICE_URI).type_system,
            rebuilt.type_system,
        )
        self.assertEqual(
            rebuilt.get_service_or_throw(_BASE_SERVICE_URI).interactions, ()
        )
        rewired = to_serializable(rebuilt)
        assert isinstance(rewired, SerializableServiceCatalog)
        self.assertEqual(rewired.interfaces, wire.interfaces)

    def test_runtime_node_round_trips(self) -> None:
        greet = self.descriptor.get_function_by_name("greet")
        parameter = from_serializable(
            Parameter,
            to_serializable(greet.params[0]),
            type_system=self.registry,
        )
        exception = from_serializable(
            DeclaredException,
            to_serializable(greet.exceptions[0]),
            type_system=self.registry,
        )
        stream = self.descriptor.get_function_by_name("streamNames").stream
        sink = self.descriptor.get_function_by_name("collectStrings").sink
        assert stream is not None
        assert sink is not None
        rebuilt_stream = from_serializable(
            Stream,
            to_serializable(stream),
            type_system=self.registry,
        )
        rebuilt_sink = from_serializable(
            Sink,
            to_serializable(sink),
            type_system=self.registry,
        )
        function = from_serializable(
            Function,
            to_serializable(greet),
            type_system=self.registry,
            uri=_SERVICE_URI,
        )
        interaction = self.descriptor.get_interaction(_INTERACTION_URI)
        rebuilt_interaction = from_serializable(
            Interaction,
            to_serializable(interaction),
            type_system=self.registry,
            uri=_INTERACTION_URI,
        )

        self.assertEqual(parameter, greet.params[0])
        self.assertEqual(exception, greet.exceptions[0])
        self.assertEqual(rebuilt_stream, stream)
        self.assertEqual(rebuilt_sink, sink)
        self.assertEqual(function.name, greet.name)
        self.assertEqual(function.params, greet.params)
        self.assertEqual(rebuilt_interaction.name, interaction.name)
        self.assertEqual(
            to_serializable(rebuilt_interaction),
            to_serializable(interaction),
        )

    def test_bidirectional_stream_round_trips(self) -> None:
        bidi = self.descriptor.get_function_by_name("bidiEcho")
        assert bidi.stream is not None
        assert bidi.sink is not None

        rebuilt = from_serializable(
            Function,
            to_serializable(bidi),
            type_system=self.registry,
            uri=_SERVICE_URI,
        )

        self.assertEqual(rebuilt.rpc_kind, bidi.rpc_kind)
        self.assertEqual(rebuilt.stream, bidi.stream)
        self.assertEqual(rebuilt.sink, bidi.sink)

    def test_serialization_requires_created_interactions(self) -> None:
        descriptor = dataclasses.replace(self.descriptor, interactions=())

        with self.assertRaisesRegex(ValueError, "not in the catalog"):
            to_serializable(descriptor)

    def test_descriptor_deserialization_requires_a_service_uri(self) -> None:
        wire = to_serializable(self.descriptor)
        assert isinstance(wire, SerializableServiceCatalog)

        with self.assertRaisesRegex(ValueError, "Service URI not found"):
            from_serializable(ServiceDescriptor, wire, uri="missing")
        with self.assertRaisesRegex(ValueError, "does not point to a service"):
            from_serializable(ServiceDescriptor, wire, uri=_INTERACTION_URI)

    def test_deserialization_requires_inline_types(self) -> None:
        wire = to_serializable(self.descriptor)
        assert isinstance(wire, SerializableServiceCatalog)
        without_types = SerializableServiceCatalog(
            types=None,
            interfaces=wire.interfaces,
            typesDigest=wire.typesDigest,
        )

        with self.assertRaisesRegex(ValueError, "no inline types"):
            from_serializable(ServiceCatalog, without_types)
