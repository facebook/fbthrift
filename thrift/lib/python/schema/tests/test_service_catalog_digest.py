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
from typing import Any

import apache.thrift.dynamic.test.service_catalog_digest_fixture.thrift_services as Fixture
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
    SERVICE_CATALOG_DIGEST_VERSION,
)
from thrift.lib.python.schema.service_descriptor import (
    ServiceCatalog,
    ServiceDescriptor,
)
from thrift.lib.thrift.service_catalog.thrift_types import SerializableServiceCatalog


def _fixture(service: type[Any]) -> ServiceDescriptor:
    return ServiceDescriptor.from_service(service, SchemaRegistry())


def _as_catalog(descriptor: ServiceDescriptor) -> ServiceCatalog:
    return ServiceCatalog(descriptor.type_system, (descriptor,))


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

    def test_rejects_malformed_out_of_band_types_digest(self) -> None:
        catalog = SerializableServiceCatalog(typesDigest=b"short")

        with self.assertRaisesRegex(ValueError, "no valid type system digest"):
            service_catalog_digest(catalog)
