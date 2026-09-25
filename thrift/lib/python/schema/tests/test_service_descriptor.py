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

import unittest

import thrift.lib.python.schema.tests.schema_registry_legacy_uri.thrift_services as LegacyServices
import thrift.lib.python.schema.tests.service_descriptor_test.thrift_services as TestServices
import thrift.lib.python.schema.tests.service_descriptor_test.thrift_types as TestTypes
from apache.thrift.type.schema.thrift_types import ErrorBlame, ErrorKind, ErrorSafety
from thrift.lib.python.schema.schema_registry import SchemaRegistry
from thrift.lib.python.schema.service_descriptor import (
    FunctionQualifier,
    RpcKind,
    ServiceDescriptor,
)
from thrift.lib.python.schema.type_system import (
    PresenceQualifier,
    Primitive,
    PrimitiveTypeRef,
    StructTypeRef,
)


_SERVICE_URI = "thrift.com/python/schema/service_descriptor_test/TestService"
_BASE_SERVICE_URI = "thrift.com/python/schema/service_descriptor_test/BaseService"
_INTERACTION_URI = "thrift.com/python/schema/service_descriptor_test/TestInteraction"
_ANNOTATION_URI = "thrift.com/python/schema/service_descriptor_test/TestAnnotation"
_REQUEST_URI = "thrift.com/python/schema/service_descriptor_test/Request"


class ServiceDescriptorTest(unittest.TestCase):
    def setUp(self) -> None:
        self.registry = SchemaRegistry()
        self.descriptor = ServiceDescriptor.from_service(
            TestServices.TestServiceInterface,
            self.registry,
        )

    def test_service_identity_and_lookup(self) -> None:
        self.assertEqual(self.descriptor.service_name, "TestService")
        self.assertEqual(self.descriptor.service_uri, _SERVICE_URI)
        self.assertIs(self.descriptor.type_system, self.registry)
        self.assertEqual(
            self.descriptor.get_function_by_name("add").uri,
            f"{_SERVICE_URI}/add",
        )
        self.assertEqual(
            self.descriptor.get_function(f"{_SERVICE_URI}/ping").name,
            "ping",
        )
        with self.assertRaises(KeyError):
            self.descriptor.get_function_by_name("missing")

    def test_inherited_functions_are_flattened_base_first(self) -> None:
        inherited = self.descriptor.functions[0]
        self.assertEqual(inherited.name, "inherited")
        self.assertEqual(inherited.uri, f"{_BASE_SERVICE_URI}/inherited")

    def test_function_types_and_envelopes(self) -> None:
        add = self.descriptor.get_function_by_name("add")
        self.assertEqual([param.name for param in add.params], ["a", "b"])
        self.assertIsInstance(add.params[0].type, PrimitiveTypeRef)
        self.assertEqual(add.params[0].type.primitive, Primitive.I32)
        self.assertIn(_ANNOTATION_URI, add.params[0].annotations)

        request = add.request_envelope()
        self.assertEqual(
            [(field.identity.id, field.identity.name) for field in request.fields],
            [(1, "a"), (2, "b")],
        )
        self.assertTrue(
            all(
                field.presence == PresenceQualifier.UNQUALIFIED
                for field in request.fields
            )
        )

        response = add.response_envelope()
        self.assertEqual(len(response.fields), 1)
        self.assertEqual(response.fields[0].identity.id, 0)
        self.assertEqual(response.fields[0].identity.name, "success")
        self.assertEqual(response.fields[0].presence, PresenceQualifier.OPTIONAL)

    def test_declared_exception_metadata(self) -> None:
        greet = self.descriptor.get_function_by_name("greet")
        self.assertIsInstance(greet.params[0].type, StructTypeRef)
        self.assertEqual(greet.params[0].type.node.uri, _REQUEST_URI)

        exception = greet.exceptions[0]
        self.assertEqual(exception.name, "error")
        self.assertEqual(exception.safety, ErrorSafety.Safe)
        self.assertEqual(exception.kind, ErrorKind.Permanent)
        self.assertEqual(exception.blame, ErrorBlame.Client)
        self.assertIn(_ANNOTATION_URI, exception.annotations)

        response = greet.response_envelope()
        self.assertEqual(
            [(field.identity.id, field.identity.name) for field in response.fields],
            [(0, "success"), (1, "error")],
        )

    def test_qualifiers_and_rpc_kinds(self) -> None:
        expected = {
            "add": (FunctionQualifier.Unspecified, RpcKind.Unary),
            "idempotentCall": (FunctionQualifier.Idempotent, RpcKind.Unary),
            "readOnlyCall": (FunctionQualifier.ReadOnly, RpcKind.Unary),
            "streamNames": (FunctionQualifier.Unspecified, RpcKind.Stream),
            "collectStrings": (FunctionQualifier.Unspecified, RpcKind.Sink),
            "bidiEcho": (
                FunctionQualifier.Unspecified,
                RpcKind.BidirectionalStream,
            ),
            "fireAndForget": (FunctionQualifier.Unspecified, RpcKind.OneWay),
        }
        for name, values in expected.items():
            with self.subTest(name=name):
                function = self.descriptor.get_function_by_name(name)
                self.assertEqual((function.qualifier, function.rpc_kind), values)

    def test_stream_and_sink_shapes(self) -> None:
        stream = self.descriptor.get_function_by_name("streamNames").stream
        self.assertIsNotNone(stream)
        assert stream is not None
        self.assertEqual(stream.payload_type, PrimitiveTypeRef(Primitive.STRING))

        sink = self.descriptor.get_function_by_name("collectStrings").sink
        self.assertIsNotNone(sink)
        assert sink is not None
        self.assertEqual(sink.payload_type, PrimitiveTypeRef(Primitive.STRING))
        self.assertEqual(
            sink.final_response_type,
            PrimitiveTypeRef(Primitive.I32),
        )

        bidi = self.descriptor.get_function_by_name("bidiEcho")
        self.assertIsNotNone(bidi.stream)
        self.assertIsNotNone(bidi.sink)
        assert bidi.sink is not None
        self.assertIsNone(bidi.sink.final_response_type)

    def test_interactions_and_annotations(self) -> None:
        self.assertIn(_ANNOTATION_URI, self.descriptor.annotations)
        interaction = self.descriptor.get_interaction(_INTERACTION_URI)
        self.assertEqual(interaction.name, "TestInteraction")
        self.assertEqual(
            interaction.get_function_by_name("getValue").uri,
            f"{_INTERACTION_URI}/getValue",
        )

        creator = self.descriptor.get_function_by_name("createInteraction")
        self.assertEqual(creator.created_interaction_uri, _INTERACTION_URI)
        performs = next(
            function for function in self.descriptor.functions if function.is_performs
        )
        self.assertEqual(performs.created_interaction_uri, _INTERACTION_URI)

    def test_rejects_uri_less_interaction(self) -> None:
        with self.assertRaisesRegex(ValueError, "LegacyInteraction"):
            ServiceDescriptor.from_service(
                LegacyServices.LegacyServiceInterface,
                SchemaRegistry(),
            )

    def test_rejects_non_service_type(self) -> None:
        with self.assertRaises(TypeError):
            ServiceDescriptor.from_service(TestTypes.Request, SchemaRegistry())
