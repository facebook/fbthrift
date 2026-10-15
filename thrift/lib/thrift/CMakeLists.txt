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

thrift_generate(any_rep.thrift LANGUAGE cpp2 OPTIONS json no_metadata
                OUT_SOURCES any_rep_sources)

thrift_generate(metadata.thrift LANGUAGE cpp2 OUT_SOURCES metadata_sources)

thrift_generate(frozen.thrift LANGUAGE cpp2 OUT_SOURCES frozen_sources)

thrift_generate(RpcMetadata.thrift LANGUAGE cpp2 OPTIONS json no_metadata
                OUT_SOURCES RpcMetadata_sources)

thrift_generate(
  RocketUpgrade.thrift
  LANGUAGE cpp2
  SERVICES RocketUpgrade
  OPTIONS no_metadata
  OUT_SOURCES RocketUpgrade_sources)

thrift_generate(id.thrift LANGUAGE cpp2 OPTIONS no_metadata
                OUT_SOURCES id_sources)

thrift_generate(standard.thrift LANGUAGE cpp2 OPTIONS json no_metadata
                OUT_SOURCES standard_sources)

thrift_generate(type_rep.thrift LANGUAGE cpp2 OPTIONS json no_metadata
                OUT_SOURCES type_rep_sources)

thrift_generate(type.thrift LANGUAGE cpp2 OPTIONS no_metadata
                OUT_SOURCES type_sources)

thrift_generate(serverdbginfo.thrift LANGUAGE cpp2 OPTIONS json no_metadata
                OUT_SOURCES serverdbginfo_sources)

thrift_generate(protocol.thrift LANGUAGE cpp2 OUT_SOURCES protocol_sources)

thrift_generate(protocol_detail.thrift LANGUAGE cpp2
                OUT_SOURCES protocol_detail_sources)

thrift_generate(field_mask.thrift LANGUAGE cpp2 OPTIONS no_metadata
                OUT_SOURCES field_mask_sources)

thrift_generate(record.thrift LANGUAGE cpp2 OPTIONS no_metadata
                OUT_SOURCES record_sources)

thrift_generate(schema.thrift LANGUAGE cpp2 OPTIONS no_metadata
                OUT_SOURCES schema_sources)

thrift_generate(type_id.thrift LANGUAGE cpp2 OPTIONS no_metadata
                OUT_SOURCES type_id_sources)

thrift_generate(type_system.thrift LANGUAGE cpp2 OPTIONS no_metadata
                OUT_SOURCES type_system_sources)

thrift_generate(service_catalog.thrift LANGUAGE cpp2 OPTIONS no_metadata
                OUT_SOURCES service_catalog_sources)

thrift_generate(
  thrift_catalog_service.thrift
  LANGUAGE cpp2
  SERVICES ThriftCatalogService
  OPTIONS no_metadata
  OUT_SOURCES thrift_catalog_service_sources)

set_property(GLOBAL PROPERTY any_rep-cpp2-SOURCES ${any_rep_sources})
set_property(GLOBAL PROPERTY RpcMetadata-cpp2-SOURCES
                             ${RpcMetadata_sources})
set_property(GLOBAL PROPERTY frozen-cpp2-SOURCES ${frozen_sources})
set_property(GLOBAL PROPERTY metadata-cpp2-SOURCES ${metadata_sources})
set_property(GLOBAL PROPERTY RocketUpgrade-cpp2-SOURCES
                             ${RocketUpgrade_sources})
set_property(GLOBAL PROPERTY id-cpp2-SOURCES ${id_sources})
set_property(GLOBAL PROPERTY record-cpp2-SOURCES ${record_sources})
set_property(GLOBAL PROPERTY schema-cpp2-SOURCES ${schema_sources})
set_property(GLOBAL PROPERTY type_id-cpp2-SOURCES ${type_id_sources})
set_property(GLOBAL PROPERTY type_system-cpp2-SOURCES
                             ${type_system_sources})
set_property(GLOBAL PROPERTY service_catalog-cpp2-SOURCES
                             ${service_catalog_sources})
set_property(GLOBAL PROPERTY thrift_catalog_service-cpp2-SOURCES
                             ${thrift_catalog_service_sources})
set_property(GLOBAL PROPERTY standard-cpp2-SOURCES ${standard_sources})

set_property(GLOBAL PROPERTY type_rep-cpp2-SOURCES ${type_rep_sources})
set_property(GLOBAL PROPERTY type-cpp2-SOURCES ${type_sources})
set_property(GLOBAL PROPERTY serverdbginfo-cpp2-SOURCES
                             ${serverdbginfo_sources})
set_property(GLOBAL PROPERTY protocol-cpp2-SOURCES ${protocol_sources})
set_property(GLOBAL PROPERTY protocol_detail-cpp2-SOURCES
                             ${protocol_detail_sources})
set_property(GLOBAL PROPERTY field_mask-cpp2-SOURCES ${field_mask_sources})
