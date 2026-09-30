/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/dynamic/ServiceCatalogDigest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/dynamic/DynamicValue.h>
#include <thrift/lib/cpp2/dynamic/ServiceDescriptorBuilder.h>
#include <thrift/lib/cpp2/dynamic/ServiceDescriptorSerialization.h>
#include <thrift/lib/cpp2/dynamic/SyntaxGraphServiceDescriptor.h>
#include <thrift/lib/cpp2/dynamic/TypeSystemBuilder.h>
#include <thrift/lib/cpp2/dynamic/test/gen-cpp2/Calculator.h>
#include <thrift/lib/cpp2/dynamic/test/gen-cpp2/service_catalog_digest_expected_values_constants.h>
#include <thrift/lib/cpp2/schema/SyntaxGraph.h>

namespace apache::thrift::dynamic {
namespace {

using def = type_system::TypeSystemBuilder::DefinitionHelper;
namespace expected =
    apache::thrift::dynamic::test::service_catalog_digest_expected_values::
        service_catalog_digest_expected_values_constants;

std::string toHex(const ServiceCatalogDigest& digest) {
  constexpr char kHex[] = "0123456789abcdef";
  std::string result;
  result.reserve(digest.size() * 2);
  for (auto byte : digest) {
    const auto value = static_cast<std::uint8_t>(byte);
    result.push_back(kHex[value >> 4]);
    result.push_back(kHex[value & 0x0f]);
  }
  return result;
}

std::string toDigestBytes(const type_system::TypeSystemDigest& digest) {
  return std::string(
      reinterpret_cast<const char*>(digest.data()), digest.size());
}

std::shared_ptr<const type_system::TypeSystem> makeTypeSystem() {
  return std::shared_ptr<const type_system::TypeSystem>(
      type_system::TypeSystemBuilder{}.build());
}

std::shared_ptr<const type_system::TypeSystem> makeAnnotationTypeSystem() {
  type_system::TypeSystemBuilder builder;
  builder.addType(
      "facebook.com/thrift/service_catalog_digest_test/Annotation",
      def::Struct({
          def::Field(
              def::Identity(1, "label"),
              def::Optional,
              type_system::TypeIds::String),
      }));
  return std::shared_ptr<const type_system::TypeSystem>(
      std::move(builder).build());
}

std::shared_ptr<const type_system::TypeSystem> makeRichAnnotationTypeSystem() {
  type_system::TypeSystemBuilder builder;
  builder.addType(
      "facebook.com/thrift/service_catalog_digest_test/RichAnnotation",
      def::Struct({
          def::Field(
              def::Identity(1, "label"),
              def::Optional,
              type_system::TypeIds::String),
          def::Field(
              def::Identity(2, "enabled"),
              def::Optional,
              type_system::TypeIds::Bool),
          def::Field(
              def::Identity(3, "byteValue"),
              def::Optional,
              type_system::TypeIds::Byte),
          def::Field(
              def::Identity(4, "i16Value"),
              def::Optional,
              type_system::TypeIds::I16),
          def::Field(
              def::Identity(5, "i32Value"),
              def::Optional,
              type_system::TypeIds::I32),
          def::Field(
              def::Identity(6, "i64Value"),
              def::Optional,
              type_system::TypeIds::I64),
          def::Field(
              def::Identity(7, "floatValue"),
              def::Optional,
              type_system::TypeIds::Float),
          def::Field(
              def::Identity(8, "doubleValue"),
              def::Optional,
              type_system::TypeIds::Double),
          def::Field(
              def::Identity(9, "data"),
              def::Optional,
              type_system::TypeIds::Binary),
          def::Field(
              def::Identity(10, "tags"),
              def::Optional,
              type_system::TypeIds::list(type_system::TypeIds::String)),
          def::Field(
              def::Identity(11, "levels"),
              def::Optional,
              type_system::TypeIds::set(type_system::TypeIds::I32)),
          def::Field(
              def::Identity(12, "weights"),
              def::Optional,
              type_system::TypeIds::map(
                  type_system::TypeIds::String, type_system::TypeIds::I64)),
      }));
  builder.addType(
      "facebook.com/thrift/service_catalog_digest_test/AppError",
      def::Struct({}));
  return std::shared_ptr<const type_system::TypeSystem>(
      std::move(builder).build());
}

DynamicValue makeAnnotation(
    const type_system::TypeSystem& typeSystem, std::string_view label) {
  auto value = DynamicValue::makeDefault(typeSystem.UserDefined(
      "facebook.com/thrift/service_catalog_digest_test/Annotation"));
  value.asStruct().setField("label", DynamicValue::makeString(label));
  return value;
}

DynamicValue makeRichAnnotation(const type_system::TypeSystem& typeSystem) {
  auto value = DynamicValue::makeDefault(typeSystem.UserDefined(
      "facebook.com/thrift/service_catalog_digest_test/RichAnnotation"));
  auto& fields = value.asStruct();
  fields.setField("label", DynamicValue::makeString("runtime value"));
  fields.setField("enabled", DynamicValue::makeBool(true));
  fields.setField("byteValue", DynamicValue::makeByte(-7));
  fields.setField("i16Value", DynamicValue::makeI16(-1234));
  fields.setField("i32Value", DynamicValue::makeI32(123456));
  fields.setField("i64Value", DynamicValue::makeI64(1234567890123));
  fields.setField("floatValue", DynamicValue::makeFloat(1.25f));
  fields.setField("doubleValue", DynamicValue::makeDouble(-2.5));
  fields.setField(
      "data", DynamicValue::makeBinary(folly::IOBuf::copyBuffer("bin data")));

  auto tags = DynamicValue::makeDefault(
      typeSystem.ListOf(type_system::TypeSystem::String()));
  tags.asList().push_back(DynamicValue::makeString("alpha"));
  tags.asList().push_back(DynamicValue::makeString("beta"));
  fields.setField("tags", std::move(tags));

  auto levels = DynamicValue::makeDefault(
      typeSystem.SetOf(type_system::TypeSystem::I32()));
  levels.asSet().insert(DynamicValue::makeI32(2));
  levels.asSet().insert(DynamicValue::makeI32(1));
  fields.setField("levels", std::move(levels));

  auto weights = DynamicValue::makeDefault(typeSystem.MapOf(
      type_system::TypeSystem::String(), type_system::TypeSystem::I64()));
  weights.asMap().insert(
      DynamicValue::makeString("left"), DynamicValue::makeI64(10));
  weights.asMap().insert(
      DynamicValue::makeString("right"), DynamicValue::makeI64(20));
  fields.setField("weights", std::move(weights));

  return value;
}

std::unique_ptr<ServiceDescriptor> makeCalculator() {
  ServiceDescriptorBuilder builder(
      makeTypeSystem(),
      "Calculator",
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  builder.addFunction("subtract")
      .addParam("left", FieldId{1}, type_system::TypeSystem::I32())
      .addParam("right", FieldId{2}, type_system::TypeSystem::I32())
      .setResponseType(type_system::TypeSystem::I32());
  builder.addFunction("add")
      .addParam("left", FieldId{1}, type_system::TypeSystem::I32())
      .addParam("right", FieldId{2}, type_system::TypeSystem::I32())
      .setResponseType(type_system::TypeSystem::I32());
  return builder.build();
}

std::unique_ptr<ServiceDescriptor> makeAnnotatedCalculator(
    std::string_view label) {
  auto typeSystem = makeAnnotationTypeSystem();
  ServiceDescriptorBuilder builder(
      typeSystem,
      "Calculator",
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  builder.addServiceAnnotation(makeAnnotation(*typeSystem, label));
  builder.addFunction("get")
      .addAnnotation(makeAnnotation(*typeSystem, label))
      .setResponseType(type_system::TypeSystem::I32());
  return builder.build();
}

std::unique_ptr<ServiceDescriptor> makeAnnotatedSessionCalculator(
    std::string_view label) {
  auto typeSystem = makeAnnotationTypeSystem();
  ServiceDescriptorBuilder builder(
      typeSystem,
      "Calculator",
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  builder.addServiceAnnotation(makeAnnotation(*typeSystem, label));
  builder.addFunction("makeSession")
      .addParam(
          "seed",
          FieldId{1},
          type_system::TypeSystem::I32(),
          {makeAnnotation(*typeSystem, label)})
      .addAnnotation(makeAnnotation(*typeSystem, label))
      .setCreatedInteractionUri(
          "facebook.com/thrift/service_catalog_digest_test/CalculatorSession");

  auto& interaction =
      builder
          .addInteraction(
              "CalculatorSession",
              "facebook.com/thrift/service_catalog_digest_test/CalculatorSession")
          .addAnnotation(makeAnnotation(*typeSystem, label));
  interaction.addFunction("get")
      .addAnnotation(makeAnnotation(*typeSystem, label))
      .setResponseType(type_system::TypeSystem::I32());
  return builder.build();
}

std::unique_ptr<ServiceDescriptor> makeRichDescriptor() {
  auto typeSystem = makeRichAnnotationTypeSystem();
  ServiceDescriptorBuilder builder(
      typeSystem,
      "CatalogGolden",
      "facebook.com/thrift/service_catalog_digest_test/CatalogGolden");
  builder.addServiceAnnotation(makeRichAnnotation(*typeSystem));
  builder.addFunction("makeSession")
      .addParam(
          "seed",
          FieldId{1},
          type_system::TypeSystem::I32(),
          {makeRichAnnotation(*typeSystem)})
      .addAnnotation(makeRichAnnotation(*typeSystem))
      .setQualifier(FunctionQualifier::Idempotent)
      .setCreatedInteractionUri(
          "facebook.com/thrift/service_catalog_digest_test/CatalogGoldenSession");
  builder.addFunction("observe")
      .addAnnotation(makeRichAnnotation(*typeSystem))
      .setBidirectionalStream(
          type_system::TypeSystem::I32(), type_system::TypeSystem::String());
  builder.addFunction("upload")
      .setSink(
          type_system::TypeSystem::I32(), type_system::TypeSystem::String())
      .addException(
          ServiceDescriptor::Exception{
              .name = "appError",
              .id = FieldId{1},
              .type = typeSystem->UserDefined(
                  "facebook.com/thrift/service_catalog_digest_test/AppError"),
              .annotations = {},
              .safety = type::ErrorSafety::Safe,
              .kind = type::ErrorKind::Transient,
              .blame = type::ErrorBlame::Server,
          });
  builder.addFunction("notify").setOneWay();

  auto& interaction = builder.addInteraction(
      "CatalogGoldenSession",
      "facebook.com/thrift/service_catalog_digest_test/CatalogGoldenSession");
  interaction.addAnnotation(makeRichAnnotation(*typeSystem));
  interaction.addFunction("get")
      .addAnnotation(makeRichAnnotation(*typeSystem))
      .setQualifier(FunctionQualifier::ReadOnly)
      .setResponseType(type_system::TypeSystem::I64());
  return builder.build();
}

// The fixture services are the IDL the golden values are computed from; each
// golden is also checked against the equivalent builder-made descriptor.
std::unique_ptr<ServiceDescriptor> loadFixtureService(std::string_view name) {
  auto schema = apache::thrift::ServiceHandler<
                    test::service_catalog_digest_fixture::Calculator>{}
                    .getServiceSchema();
  CHECK(schema.has_value());
  auto graph = std::make_shared<const syntax_graph::SyntaxGraph>(
      syntax_graph::SyntaxGraph::fromSchema(
          apache::thrift::type::Schema(schema->schema)));
  for (const auto program : graph->programs()) {
    for (const auto definition : program->definitions()) {
      if (definition->isService() &&
          definition->asService().definition().name() == name) {
        return std::make_unique<SyntaxGraphServiceDescriptor>(
            graph, definition->asService());
      }
    }
  }
  throw std::invalid_argument(
      "Fixture service not found: " + std::string(name));
}

std::unique_ptr<ServiceDescriptor> makePerformedInteractionDescriptor() {
  ServiceDescriptorBuilder builder(
      makeTypeSystem(),
      "SessionService",
      "facebook.com/thrift/service_catalog_digest_test/SessionService");
  builder.addFunction("openSession")
      .setCreatedInteractionUri(
          "facebook.com/thrift/service_catalog_digest_test/Session");
  builder.addFunction("createSession")
      .setCreatedInteractionUri(
          "facebook.com/thrift/service_catalog_digest_test/Session")
      .setIsPerforms(true);
  builder
      .addInteraction(
          "Session", "facebook.com/thrift/service_catalog_digest_test/Session")
      .addFunction("get")
      .setResponseType(type_system::TypeSystem::I32());
  return builder.build();
}

const ServiceDescriptor& requireDescriptor(
    const std::unique_ptr<ServiceDescriptor>& descriptor) {
  return *CHECK_NOTNULL(descriptor.get());
}

std::string_view requireExpectedDigest(const char* digest) {
  return CHECK_NOTNULL(digest);
}

void expectGoldenDigest(
    const ServiceDescriptor& descriptor,
    std::string_view serviceUri,
    std::string_view expectedDigest,
    type_system::DigestMode mode = type_system::DigestMode::Full) {
  ServiceCatalogHasher hasher{mode};
  auto catalog = toSerializable(descriptor, serviceUri);
  auto outOfBandCatalog = catalog;
  if (mode != type_system::DigestMode::Full) {
    type_system::TypeSystemHasher typeHasher{mode};
    outOfBandCatalog.typesDigest() =
        toDigestBytes(typeHasher(*catalog.types()));
  }
  outOfBandCatalog.types_ref().reset();

  const auto descriptorDigest = hasher(descriptor, serviceUri);
  const auto inlineCatalogDigest = hasher(catalog);
  const auto outOfBandCatalogDigest = hasher(outOfBandCatalog);

  EXPECT_EQ(descriptorDigest, inlineCatalogDigest);
  EXPECT_EQ(descriptorDigest, outOfBandCatalogDigest);
  EXPECT_EQ(toHex(descriptorDigest), expectedDigest);
}

TEST(ServiceCatalogDigestTest, VersionConstantExists) {
  EXPECT_EQ(kServiceCatalogDigestVersion, 2);
}

TEST(ServiceCatalogDigestTest, DistinguishesInteractionConstructorFromFactory) {
  auto makeService = [](bool isPerforms) {
    ServiceDescriptorBuilder builder(
        makeTypeSystem(),
        "Calculator",
        "facebook.com/thrift/service_catalog_digest_test/Calculator");
    builder.addFunction("createCalculatorSession")
        .setCreatedInteractionUri(
            "facebook.com/thrift/service_catalog_digest_test/CalculatorSession")
        .setIsPerforms(isPerforms);
    builder.addInteraction(
        "CalculatorSession",
        "facebook.com/thrift/service_catalog_digest_test/CalculatorSession");
    return builder.build();
  };

  EXPECT_NE(
      ServiceCatalogHasher{}(
          *makeService(true),
          "facebook.com/thrift/service_catalog_digest_test/Calculator"),
      ServiceCatalogHasher{}(
          *makeService(false),
          "facebook.com/thrift/service_catalog_digest_test/Calculator"));
}

TEST(ServiceCatalogDigestTest, ToSerializableSetsTypeDigest) {
  auto service = makeCalculator();
  const auto& descriptor = requireDescriptor(service);
  auto catalog = toSerializable(
      descriptor, "facebook.com/thrift/service_catalog_digest_test/Calculator");

  type_system::TypeSystemHasher typeHasher;
  const auto expected = typeHasher(*catalog.types());
  const auto& bytes = *catalog.typesDigest();

  ASSERT_EQ(bytes.size(), expected.size());
  EXPECT_EQ(
      bytes,
      std::string(
          reinterpret_cast<const char*>(expected.data()), expected.size()));
}

TEST(ServiceCatalogDigestTest, DescriptorAndSerializedCatalogMatch) {
  auto service = makeCalculator();
  const auto& descriptor = requireDescriptor(service);
  auto catalog = toSerializable(
      descriptor, "facebook.com/thrift/service_catalog_digest_test/Calculator");

  EXPECT_EQ(
      ServiceCatalogHasher{}(
          descriptor,
          "facebook.com/thrift/service_catalog_digest_test/Calculator"),
      ServiceCatalogHasher{}(catalog));
}

TEST(ServiceCatalogDigestTest, GoldenCalculatorDigest) {
  auto service = makeCalculator();
  expectGoldenDigest(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/Calculator",
      requireExpectedDigest(expected::DIGEST_CALCULATOR()));
}

TEST(
    ServiceCatalogDigestTest,
    DescriptorAndSerializedCatalogMatchWithAnnotationsAndInteractions) {
  auto service = makeAnnotatedSessionCalculator("runtime");
  const auto& descriptor = requireDescriptor(service);
  auto catalog = toSerializable(
      descriptor, "facebook.com/thrift/service_catalog_digest_test/Calculator");

  EXPECT_EQ(
      ServiceCatalogHasher{}(
          descriptor,
          "facebook.com/thrift/service_catalog_digest_test/Calculator"),
      ServiceCatalogHasher{}(catalog));
}

TEST(ServiceCatalogDigestTest, GoldenRichDescriptorDigest) {
  auto service = makeRichDescriptor();
  expectGoldenDigest(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/CatalogGolden",
      requireExpectedDigest(expected::DIGEST_RICH_DESCRIPTOR()));
}

TEST(ServiceCatalogDigestTest, GoldenRichDescriptorStructuralDigest) {
  auto service = makeRichDescriptor();
  expectGoldenDigest(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/CatalogGolden",
      requireExpectedDigest(expected::DIGEST_RICH_DESCRIPTOR_STRUCTURAL()),
      type_system::DigestMode::Structural);
}

TEST(ServiceCatalogDigestTest, GoldenCalculatorDigestFromSchema) {
  auto service = loadFixtureService("Calculator");
  expectGoldenDigest(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/Calculator",
      requireExpectedDigest(expected::DIGEST_CALCULATOR()));
}

TEST(ServiceCatalogDigestTest, GoldenRichDescriptorDigestFromSchema) {
  auto service = loadFixtureService("CatalogGolden");
  expectGoldenDigest(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/CatalogGolden",
      requireExpectedDigest(expected::DIGEST_RICH_DESCRIPTOR()));
  expectGoldenDigest(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/CatalogGolden",
      requireExpectedDigest(expected::DIGEST_RICH_DESCRIPTOR_STRUCTURAL()),
      type_system::DigestMode::Structural);
}

TEST(ServiceCatalogDigestTest, GoldenPerformedInteractionDigest) {
  auto service = makePerformedInteractionDescriptor();
  expectGoldenDigest(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/SessionService",
      requireExpectedDigest(expected::DIGEST_PERFORMED_INTERACTION()));
}

TEST(ServiceCatalogDigestTest, GoldenPerformedInteractionDigestFromSchema) {
  auto service = loadFixtureService("SessionService");
  expectGoldenDigest(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/SessionService",
      requireExpectedDigest(expected::DIGEST_PERFORMED_INTERACTION()));
}

TEST(
    ServiceCatalogDigestTest, RebuiltInteractionConstructorsDigestLikeCatalog) {
  ServiceDescriptorBuilder builder(
      makeTypeSystem(),
      "SessionService",
      "facebook.com/thrift/service_catalog_digest_test/SessionService");
  builder.addFunction("createFirst")
      .setCreatedInteractionUri(
          "facebook.com/thrift/service_catalog_digest_test/First")
      .setIsPerforms(true);
  builder.addFunction("createSecond")
      .setCreatedInteractionUri(
          "facebook.com/thrift/service_catalog_digest_test/Second")
      .setIsPerforms(true);
  builder.addInteraction(
      "First", "facebook.com/thrift/service_catalog_digest_test/First");
  builder.addInteraction(
      "Second", "facebook.com/thrift/service_catalog_digest_test/Second");
  auto catalog = toSerializable(
      *builder.build(),
      "facebook.com/thrift/service_catalog_digest_test/SessionService");

  auto rebuilt = fromSerializable(
      catalog,
      "facebook.com/thrift/service_catalog_digest_test/SessionService");

  EXPECT_EQ(
      ServiceCatalogHasher{}(
          *rebuilt,
          "facebook.com/thrift/service_catalog_digest_test/SessionService"),
      ServiceCatalogHasher{}(catalog));
}

TEST(ServiceCatalogDigestTest, InlineAndOutOfBandTypesMatch) {
  auto service = makeCalculator();
  auto inlineCatalog = toSerializable(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  auto outOfBandCatalog = inlineCatalog;
  outOfBandCatalog.types_ref().reset();

  EXPECT_EQ(
      ServiceCatalogHasher{}(inlineCatalog),
      ServiceCatalogHasher{}(outOfBandCatalog));
}

TEST(ServiceCatalogDigestTest, IgnoresFunctionAndParameterOrder) {
  auto service = makeCalculator();
  auto original = toSerializable(
      requireDescriptor(service),
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  auto reordered = original;

  auto& functions =
      *reordered.interfaces()
           ->at("facebook.com/thrift/service_catalog_digest_test/Calculator")
           .serviceDef_ref()
           ->functions();
  std::reverse(functions.begin(), functions.end());
  auto& params = *functions.at(0).params();
  std::reverse(params.begin(), params.end());

  EXPECT_EQ(
      ServiceCatalogHasher{}(original), ServiceCatalogHasher{}(reordered));
}

TEST(ServiceCatalogDigestTest, ChangesWhenFunctionTypeChanges) {
  ServiceDescriptorBuilder i32Builder(
      makeTypeSystem(),
      "Calculator",
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  i32Builder.addFunction("get").setResponseType(type_system::TypeSystem::I32());

  ServiceDescriptorBuilder i64Builder(
      makeTypeSystem(),
      "Calculator",
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  i64Builder.addFunction("get").setResponseType(type_system::TypeSystem::I64());

  auto i32Service = i32Builder.build();
  auto i64Service = i64Builder.build();
  EXPECT_NE(
      ServiceCatalogHasher{}(
          requireDescriptor(i32Service),
          "facebook.com/thrift/service_catalog_digest_test/Calculator"),
      ServiceCatalogHasher{}(
          requireDescriptor(i64Service),
          "facebook.com/thrift/service_catalog_digest_test/Calculator"));
}

TEST(ServiceCatalogDigestTest, ChangesWhenInteractionChanges) {
  ServiceDescriptorBuilder i32Builder(
      makeTypeSystem(),
      "Calculator",
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  i32Builder.addFunction("makeSession")
      .setCreatedInteractionUri(
          "facebook.com/thrift/service_catalog_digest_test/CalculatorSession");
  i32Builder
      .addInteraction(
          "CalculatorSession",
          "facebook.com/thrift/service_catalog_digest_test/CalculatorSession")
      .addFunction("get")
      .setResponseType(type_system::TypeSystem::I32());

  ServiceDescriptorBuilder i64Builder(
      makeTypeSystem(),
      "Calculator",
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  i64Builder.addFunction("makeSession")
      .setCreatedInteractionUri(
          "facebook.com/thrift/service_catalog_digest_test/CalculatorSession");
  i64Builder
      .addInteraction(
          "CalculatorSession",
          "facebook.com/thrift/service_catalog_digest_test/CalculatorSession")
      .addFunction("get")
      .setResponseType(type_system::TypeSystem::I64());

  auto i32Service = i32Builder.build();
  auto i64Service = i64Builder.build();
  EXPECT_NE(
      ServiceCatalogHasher{}(
          requireDescriptor(i32Service),
          "facebook.com/thrift/service_catalog_digest_test/Calculator"),
      ServiceCatalogHasher{}(
          requireDescriptor(i64Service),
          "facebook.com/thrift/service_catalog_digest_test/Calculator"));
}

TEST(ServiceCatalogDigestTest, StructuralModeIgnoresAnnotations) {
  auto firstDescriptor = makeAnnotatedCalculator("first");
  auto secondDescriptor = makeAnnotatedCalculator("second");
  const auto& firstService = requireDescriptor(firstDescriptor);
  const auto& secondService = requireDescriptor(secondDescriptor);
  auto first = toSerializable(
      firstService,
      "facebook.com/thrift/service_catalog_digest_test/Calculator");
  auto second = toSerializable(
      secondService,
      "facebook.com/thrift/service_catalog_digest_test/Calculator");

  ServiceCatalogHasher full;
  ServiceCatalogHasher structural{type_system::DigestMode::Structural};

  EXPECT_NE(full(first), full(second));
  EXPECT_EQ(structural(first), structural(second));
  EXPECT_NE(
      full(
          firstService,
          "facebook.com/thrift/service_catalog_digest_test/Calculator"),
      full(
          secondService,
          "facebook.com/thrift/service_catalog_digest_test/Calculator"));
  EXPECT_EQ(
      structural(
          firstService,
          "facebook.com/thrift/service_catalog_digest_test/Calculator"),
      structural(
          secondService,
          "facebook.com/thrift/service_catalog_digest_test/Calculator"));
}

} // namespace
} // namespace apache::thrift::dynamic
