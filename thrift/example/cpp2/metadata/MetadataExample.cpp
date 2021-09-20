/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <glog/logging.h>
#include <range/v3/algorithm/find_if.hpp>
#include <folly/init/Init.h>
#include <thrift/example/if/gen-cpp2/MetadataExample_metadata.h>
#include <thrift/example/if/gen-cpp2/MyService.h>

namespace apache::thrift::metadata {

class MyServiceHandler : virtual public MyServiceSvIf {
 public:
  void func1() override {}
  void func2(MyStruct&) override {}
};

void printServiceMethodName() {
  auto handler = std::make_shared<MyServiceHandler>();
  ThriftServiceMetadataResponse resp;
  handler->getProcessor()->getServiceMetadata(resp);
  const ThriftMetadata& metadata = *resp.metadata_ref();
  const ThriftService& serviceMetadata =
      metadata.services_ref()->at("MetadataExample.MyService");
  for (const ThriftFunction& func : *serviceMetadata.functions_ref()) {
    LOG(INFO) << *func.name_ref();
  }
}

void printStructFieldName() {
  const ThriftStruct& metadata = get_struct_metadata<MyStruct>();
  for (const ThriftField& field : *metadata.fields_ref()) {
    LOG(INFO) << *field.name_ref();
  }
}

void printStructuredAnnotationData() {
  const ThriftStruct& metadata = get_struct_metadata<MyStruct>();
  const ThriftField& field = *ranges::find_if(
      *metadata.fields_ref(),
      [](auto& field) { return field.name_ref() == "field2"; });

  // Get structured annotations of the field
  const std::vector<ThriftConstStruct>& fieldAnnotations =
      *field.structured_annotations_ref();

  // Print "field annotation"
  LOG(INFO) << *fieldAnnotations[0].fields_ref()->at("data").cv_string_ref();

  // Structured annotation of the typedef
  const ThriftTypedefType& typeDef = *field.type_ref()->t_typedef_ref();
  const std::vector<ThriftConstStruct>& typedefAnnotations =
      *typeDef.structured_annotations_ref();

  // Print "MetadataExample.Text"
  LOG(INFO) << *typeDef.name_ref();

  // Print "typedef annotation"
  LOG(INFO) << *typedefAnnotations[0].fields_ref()->at("data").cv_string_ref();
}

} // namespace apache::thrift::metadata

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);
  apache::thrift::metadata::printServiceMethodName();
  apache::thrift::metadata::printStructFieldName();
  apache::thrift::metadata::printStructuredAnnotationData();
}
