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

# Set the cpp2 directory
set(LIB_CPP2_HOME ${CMAKE_CURRENT_SOURCE_DIR})

thrift_generate(
  schema/syntax_graph.thrift
  LANGUAGE cpp2
  OUT_SOURCES syntax_graph_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/any_rep.thrift
  LANGUAGE cpp2
  OUT_SOURCES any_rep_sources
  OPTIONS json no_metadata)

thrift_generate(
  ../thrift/metadata.thrift
  LANGUAGE cpp2
  OUT_SOURCES metadata_sources)

thrift_generate(
  ../thrift/frozen.thrift
  LANGUAGE cpp2
  OUT_SOURCES frozen_sources)

thrift_generate(
  ../thrift/RpcMetadata.thrift
  LANGUAGE cpp2
  OUT_SOURCES RpcMetadata_sources
  OPTIONS json no_metadata)

thrift_generate(
  ../thrift/RocketUpgrade.thrift
  LANGUAGE cpp2
  OUT_SOURCES RocketUpgrade_sources
  SERVICES RocketUpgrade
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/id.thrift
  LANGUAGE cpp2
  OUT_SOURCES id_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/standard.thrift
  LANGUAGE cpp2
  OUT_SOURCES standard_sources
  OPTIONS json no_metadata)

thrift_generate(
  ../thrift/type_rep.thrift
  LANGUAGE cpp2
  OUT_SOURCES type_rep_sources
  OPTIONS json no_metadata)

thrift_generate(
  ../thrift/type.thrift
  LANGUAGE cpp2
  OUT_SOURCES type_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/serverdbginfo.thrift
  LANGUAGE cpp2
  OUT_SOURCES serverdbginfo_sources
  OPTIONS json no_metadata)

thrift_generate(
  ../thrift/protocol.thrift
  LANGUAGE cpp2
  OUT_SOURCES protocol_sources)

thrift_generate(
  ../thrift/protocol_detail.thrift
  LANGUAGE cpp2
  OUT_SOURCES protocol_detail_sources)

thrift_generate(
  ../thrift/field_mask.thrift
  LANGUAGE cpp2
  OUT_SOURCES field_mask_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/record.thrift
  LANGUAGE cpp2
  OUT_SOURCES record_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/schema.thrift
  LANGUAGE cpp2
  OUT_SOURCES schema_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/type_id.thrift
  LANGUAGE cpp2
  OUT_SOURCES type_id_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/type_system.thrift
  LANGUAGE cpp2
  OUT_SOURCES type_system_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/service_catalog.thrift
  LANGUAGE cpp2
  OUT_SOURCES service_catalog_sources
  OPTIONS no_metadata)

thrift_generate(
  ../thrift/thrift_catalog_service.thrift
  LANGUAGE cpp2
  OUT_SOURCES thrift_catalog_service_sources
  SERVICES ThriftCatalogService
  OPTIONS no_metadata)

if (THRIFT_TESTS)
  add_subdirectory(protocol/test)
  add_subdirectory(test)
endif ()

add_library(
  thriftmetadata
  ${metadata_sources}
  gen/module_types_cpp.cpp
  detail/Enum.cpp
  dynamic/SerializableRecord.cpp
  dynamic/SerializableTypeSystemBuilder.cpp
  dynamic/TypeId.cpp
  dynamic/TypeSystem.cpp
  dynamic/TypeSystemBuilder.cpp
  dynamic/TypeSystemDigest.cpp
  dynamic/TypeSystemTraits.cpp
  runtime/BaseSchemaRegistry.cpp
  runtime/Init.cpp
  schema/detail/Merge.cpp
  ../thrift/detail/protocol.cpp
  ../thrift/detail/TypeIdAdapter.cpp
  ../thrift/detail/TypeSystemAdapter.cpp
  ${protocol_sources}
  ${protocol_detail_sources}
  ${type_id_sources}
  ${type_system_sources}
  ${schema_sources}
  ${record_sources}
  ${syntax_graph_sources}
  ${service_catalog_sources})
# For the headers generated for thrifttype and thrifttyperep.
add_dependencies(thriftmetadata thrifttype)
target_link_libraries(thriftmetadata PUBLIC Folly::folly thriftprotocol)
target_link_libraries(thriftmetadata PUBLIC ${thrift_xxhash})
# SerializableRecord.cpp includes F14Map and instantiates PrimitiveDatum for
# every primitive type; on MSVC this exceeds the default COFF section limit.
if (MSVC)
  target_compile_options(thriftmetadata PRIVATE /bigobj)
endif ()

add_library(thrift_path dynamic/Path.cpp)
add_dependencies(thrift_path thriftmetadata thriftprotocol)
target_link_libraries(thrift_path PUBLIC fmt::fmt Folly::folly thriftmetadata
                                         thriftprotocol)

add_library(
  thrift_dynamic_value
  dynamic/Any.cpp
  dynamic/Binary.cpp
  dynamic/DynamicValue.cpp
  dynamic/List.cpp
  dynamic/Map.cpp
  dynamic/Set.cpp
  dynamic/String.cpp
  dynamic/Struct.cpp
  dynamic/Union.cpp
  dynamic/ServiceCatalogDigest.cpp
  dynamic/ServiceDescriptorSerialization.cpp
  dynamic/detail/ConcreteMapAny.cpp
  dynamic/detail/ConcreteMapBinary.cpp
  dynamic/detail/ConcreteMapBool.cpp
  dynamic/detail/ConcreteMapDouble.cpp
  dynamic/detail/ConcreteMapFloat.cpp
  dynamic/detail/ConcreteMapI16.cpp
  dynamic/detail/ConcreteMapI32.cpp
  dynamic/detail/ConcreteMapI64.cpp
  dynamic/detail/ConcreteMapI8.cpp
  dynamic/detail/ConcreteMapList.cpp
  dynamic/detail/ConcreteMapMap.cpp
  dynamic/detail/ConcreteMapSet.cpp
  dynamic/detail/ConcreteMapString.cpp
  dynamic/detail/ConcreteMapStruct.cpp
  dynamic/detail/ConcreteMapUnion.cpp
  dynamic/detail/ConcreteSet.cpp
  dynamic/detail/Datum.cpp
  dynamic/detail/DatumHash.cpp
  # ServiceDescriptor and the SyntaxGraph-backed catalog that builds it use
  # DynamicValue for annotations, so they layer above thriftmetadata here.
  # Putting them in thriftmetadata would cycle with thrift_dynamic_value.
  dynamic/AnnotationValue.cpp
  dynamic/ServiceDescriptor.cpp
  schema/detail/SchemaBackedResolver.cpp
  schema/SyntaxGraph.cpp
  runtime/SchemaRegistry.cpp)
add_dependencies(thrift_dynamic_value thriftmetadata thriftprotocol thrift_path)
target_link_libraries(thrift_dynamic_value PUBLIC Folly::folly thriftmetadata
                                                  thriftprotocol thrift_path)
# dynamic/Map.cpp and ConcreteMap*.cpp instantiate F14Map for every value type;
# on MSVC this exceeds the default COFF section limit.
if (MSVC)
  target_compile_options(thrift_dynamic_value PRIVATE /bigobj)
endif ()

add_library(thriftfrozen2 frozen/Frozen.cpp frozen/FrozenUtil.cpp
                          frozen/schema/MemorySchema.cpp ${frozen_sources})
add_dependencies(thriftfrozen2 thriftmetadata)
target_link_libraries(thriftfrozen2 PUBLIC Folly::folly thriftmetadata
                                           thriftprotocol glog::glog gflags)

add_library(thrifttyperep ${id_sources} ${standard_sources}
                          ${type_rep_sources})
add_dependencies(thrifttyperep thrift_annotation)
target_link_libraries(thrifttyperep PUBLIC Folly::folly thrift_annotation)

add_library(
  thrifttype
  ${any_rep_sources}
  ${field_mask_sources}
  ${type_sources}
  ../thrift/TypeToMaskAdapter.cpp
  op/Serializer.cpp
  type/Any.cpp
  type/BaseType.cpp
  type/Protocol.cpp
  type/Runtime.cpp
  type/Type.cpp
  type/TypeRegistry.cpp
  type/UniversalName.cpp)
add_dependencies(thrifttype thrift_annotation thrifttyperep thriftprotocol)
target_link_libraries(
  thrifttype PUBLIC thrift_common Folly::folly thrift_annotation thrifttyperep
                    thriftprotocol ${thrift_openssl_crypto})

# thriftanyrep sources are now compiled into thrifttype to avoid a circular
# shared library dependency. Keep thriftanyrep as a thin wrapper so that
# existing consumers (e.g. Haskell FFI) that link -lthriftanyrep keep working.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/thriftanyrep_stub.cpp" "")
add_library(thriftanyrep "${CMAKE_CURRENT_BINARY_DIR}/thriftanyrep_stub.cpp")
target_link_libraries(thriftanyrep PUBLIC thrifttype)

add_library(rpcmetadata ${RpcMetadata_sources})
target_link_libraries(rpcmetadata PUBLIC Folly::folly thriftprotocol)

add_library(serverdbginfo ${serverdbginfo_sources})
thrift_configure_target(serverdbginfo PUBLIC)
target_link_libraries(serverdbginfo PUBLIC Folly::folly)

add_library(
  thriftprotocol
  protocol/Protocol.cpp
  protocol/BinaryProtocol.cpp
  protocol/CompactProtocol.cpp
  protocol/CompactV1Protocol.cpp
  protocol/DebugProtocol.cpp
  protocol/JSONProtocol.cpp
  protocol/JSONProtocolCommon.cpp
  protocol/Serializer.cpp
  protocol/TableBasedSerializer.cpp
  protocol/TableBasedSerializerImpl.cpp
  protocol/TableBasedSerializerJson5.cpp
  protocol/VirtualProtocol.cpp
  # Required by TableBasedSerializerJson5.cpp, which instantiates the JSON5
  # reader/writer.
  protocol/detail/Json5ProtocolReader.cpp
  protocol/detail/Json5ProtocolWriter.cpp
  protocol/detail/JsonReader.cpp
  protocol/detail/JsonWriter.cpp
  protocol/detail/JsonUtils.cpp
  protocol/detail/CompoundTypeTracker.cpp
  ../cpp/protocol/TBase64Utils.cpp
  ../cpp/protocol/TProtocolException.cpp
  ../cpp/protocol/TType.cpp
  ../cpp/util/VarintUtils.cpp
  # Required by protocol/BinaryProtocol.cpp, which instantiates
  # BinaryProtocolReaderBase<io::IOBufChainCursor>.
  IOBufChain.cpp
  IOBufChainCursor.cpp
  # Required by protocol/detail/Json5ProtocolReader.cpp, which calls
  # enum_find<Int>::find_name; the explicit instantiation for `int` only
  # exists in this translation unit.
  detail/Enum.cpp
  # Required by protocol/TableBasedSerializerImpl.h's field-name lookup
  # path (used by the JSON5 protocol), which calls checkFieldIdConflict.
  gen/module_types_cpp.cpp)
target_link_libraries(thriftprotocol PUBLIC fmt::fmt Folly::folly glog::glog
                                            thrift-core)
# The name thrift_generate() links into a cpp2 TARGET, as for installed Thrift.
add_library(FBThrift::protocol ALIAS thriftprotocol)

if (THRIFT_RPC)
  add_library(
    thriftcpp2
    FieldRef.cpp
    Flags.cpp
    GeneratedCodeHelper.cpp
    TrustedServerException.cpp
    async/AsyncClient.cpp
    async/AsyncProcessor.cpp
    async/AsyncProcessorFactory.cpp
    async/AsyncProcessorHelper.cpp
    async/ClientChannel.cpp
    async/ClientSinkBridge.cpp
    async/ClientStreamBridge.cpp
    async/ClientStreamInterceptorContext.cpp
    async/Cpp2Channel.cpp
    async/DuplexChannel.cpp
    async/FramingHandler.cpp
    async/HeaderChannel.cpp
    async/HeaderChannelTrait.cpp
    async/HeaderClientChannel.cpp
    async/HeaderServerChannel.cpp
    async/HibernatingRequestChannel.cpp
    async/Interaction.cpp
    async/InteractionOverloadPolicy.cpp
    async/InterceptorFlags.cpp
    async/MultiplexAsyncProcessor.cpp
    async/PooledRequestChannel.cpp
    async/ReconnectingRequestChannel.cpp
    async/RequestCallback.cpp
    async/RequestChannel.cpp
    async/ResponseChannel.cpp
    async/RetryingRequestChannel.cpp
    async/RocketClientChannel.cpp
    async/RocketClientChannelBase.cpp
    async/RpcOptions.cpp
    async/RpcTypes.cpp
    async/SecureRocketClientChannel.cpp
    async/ServerBiDiStreamFactory.cpp
    async/ServerGeneratorStreamBridge.cpp
    async/ServerRequestData.cpp
    async/ServerSinkBridge.cpp
    async/ServerSinkFactory.cpp
    async/ServerStreamDetail.cpp
    async/StreamPayload.cpp
    async/processor/AsyncProcessor.cpp
    async/processor/AsyncProcessorFunc.cpp
    async/processor/EventTask.cpp
    async/processor/GeneratedAsyncProcessorBase.cpp
    async/processor/HandlerCallback.cpp
    async/processor/HandlerCallbackBase.cpp
    async/processor/HandlerCallbackOneWay.cpp
    async/processor/RequestParams.cpp
    async/processor/RequestTask.cpp
    async/processor/ServerInterface.cpp
    async/processor/ServerRequest.cpp
    async/processor/ServerRequestHelper.cpp
    async/processor/ServerRequestTask.cpp
    async/processor/ServiceHandlerBase.cpp
    gen/module_constants_cpp.cpp
    gen/module_types_cpp.cpp
    detail/Enum.cpp
    fast_thrift/common/allocator/EvbAllocator.cpp
    fast_thrift/channel_pipeline/PipelineImpl.cpp
    fast_thrift/channel_pipeline/detail/ContextImpl.cpp
    fast_thrift/frame/write/FrameWriter.cpp
    fast_thrift/thrift/FastThriftAdapterBase.cpp
    fast_thrift/thrift/client/ThriftClientChannel.cpp
    fast_thrift/thrift/client/util/RocketFrameDecoder.cpp
    fast_thrift/thrift/server/ThriftServerChannel.cpp
    fast_thrift/transport/TransportHandler.cpp
    logging/ThriftBiDiLog.cpp
    logging/ThriftConnectionLog.cpp
    logging/ThriftSinkLog.cpp
    logging/ThriftStreamLog.cpp
    security/AsyncStopTLS.cpp
    security/FizzPeeker.cpp
    security/SSLUtil.cpp
    security/extensions/ThriftParametersClientExtension.cpp
    security/extensions/ThriftParametersServerExtension.cpp
    security/extensions/Types.cpp
    server/AdaptiveConcurrency.cpp
    server/ConcurrencyControllerBase.cpp
    server/Cpp2ConnContext.cpp
    server/Cpp2Connection.cpp
    server/Cpp2Worker.cpp
    server/CPUConcurrencyController.cpp
    server/DecoratorData.cpp
    server/DecoratorDataRuntime.cpp
    server/DiscriminantLoadSheddingOSS.cpp
    server/LazyDynamicArguments.cpp
    server/LegacyHeaderRoutingHandler.cpp
    server/LoggingEvent.cpp
    server/LoggingEventHelper.cpp
    server/LoggingEventTransportMetadata.cpp
    server/MonitoringMethodNames.cpp
    server/ParallelConcurrencyController.cpp
    server/TokenBucketConcurrencyController.cpp
    server/ServiceInterceptorQualifiedName.cpp
    server/StandardConcurrencyController.cpp
    server/ThreadManagerLoggingWrapper.cpp
    server/RequestDebugLog.cpp
    server/RequestEncryptionState.cpp
    server/RequestPileBase.cpp
    server/ReactiveToggle.cpp
    server/RequestsRegistry.cpp
    server/ResourcePool.cpp
    server/ResourcePoolSet.cpp
    server/ResourcePoolHandle.cpp
    server/RoundRobinRequestPile.cpp
    server/WeightedRequestPileQueue.cpp
    server/ServerAttribute.cpp
    server/ServerFlags.cpp
    server/ServerInstrumentation.cpp
    server/ServiceHealthPoller.cpp
    server/ServiceInterceptorBase.cpp
    server/ServiceInterceptorControl.cpp
    server/ServiceInterceptorOnDrop.cpp
    server/ServiceInterceptorOnReceived.cpp
    server/ServiceInterceptorQualifiedName.cpp
    server/ThriftCatalogServerInterface.cpp
    server/ThriftProcessor.cpp
    server/ThriftServer.cpp
    server/ThriftServerConfig.cpp
    server/TMConcurrencyController.cpp
    server/peeking/TLSHelper.cpp
    server/metrics/PendingConnectionsMetrics.cpp
    transport/core/RequestStateMachine.cpp
    transport/core/RpcMetadataUtil.cpp
    transport/core/RpcMetadataPlugins.cpp
    transport/core/SendCallbacks.cpp
    transport/core/ThriftClient.cpp
    transport/core/ThriftClientCallback.cpp
    transport/core/ThriftRequest.cpp
    transport/rocket/FdSocket.cpp
    transport/rocket/ChecksumGenerator.cpp
    transport/rocket/Types.cpp
    transport/rocket/compression/CustomCompressorRegistry.cpp
    transport/rocket/client/RequestContext.cpp
    transport/rocket/client/RequestContextQueue.cpp
    transport/rocket/client/RocketClient.cpp
    transport/rocket/client/SecureRocketClient.cpp
    transport/rocket/client/KeepAliveWatcher.cpp
    transport/rocket/client/RocketStreamServerCallback.cpp
    transport/rocket/client/RocketStreamServerCallbackWithChunkTimeout.cpp
    transport/rocket/client/RocketSinkServerCallback.cpp
    transport/rocket/client/RocketBiDiServerCallback.cpp
    transport/rocket/flush/FlushManager.cpp
    transport/rocket/framing/parser/AllocatingParserStrategy.cpp
    transport/rocket/framing/ErrorCode.cpp
    transport/rocket/framing/FrameDataFirstFieldAligner.cpp
    transport/rocket/framing/Frames.cpp
    transport/rocket/framing/Parser.cpp
    transport/rocket/framing/Serializer.cpp
    transport/rocket/framing/Util.cpp
    transport/rocket/payload/DefaultPayloadSerializerStrategy.cpp
    transport/rocket/payload/PayloadSerializer.cpp
    transport/rocket/server/BidirectionalStreamState.cpp
    transport/rocket/server/InteractionOverload.cpp
    transport/rocket/server/RocketRoutingHandler.cpp
    transport/rocket/server/RocketServerConnection.cpp
    transport/rocket/server/RefactoredRocketServerConnection.cpp
    transport/rocket/server/RefactoredThriftRocketServerHandler.cpp
    transport/rocket/server/RocketServerConnectionPlugins.cpp
    transport/rocket/server/RocketServerFrameContext.cpp
    transport/rocket/server/RocketSinkClientCallback.cpp
    transport/rocket/server/RocketServerUtil.cpp
    transport/rocket/server/RocketStreamClientCallback.cpp
    transport/rocket/server/StreamFirstResponseLoggingCallback.cpp
    transport/rocket/server/RocketBiDiClientCallback.cpp
    transport/rocket/server/RocketThriftRequests.cpp
    transport/rocket/server/RocketServerConnectionFactory.cpp
    transport/rocket/server/ThriftRocketServerHandler.cpp
    transport/rocket/server/detail/RequestEncryptionStateDispatch.cpp
    transport/rocket/server/detail/RocketContextUtils.cpp
    transport/rocket/server/detail/RocketErrorHandler.cpp
    transport/rocket/server/detail/RocketRequestHandler.cpp
    transport/rocket/server/detail/RocketRequestOrchestrator.cpp
    transport/rocket/server/detail/RocketRequestProcessor.cpp
    transport/rocket/server/detail/RocketSetupProcessor.cpp
    transport/rocket/server/detail/WriteBatcher.cpp
    util/Checksum.cpp
    util/DebugString.cpp
    util/EmptyAsyncProcessor.cpp
    util/SchemaToMetadata.cpp
    util/ScopedServerInterfaceThread.cpp
    util/ScopedServerThread.cpp
    ${RocketUpgrade_sources}
    ${thrift_catalog_service_sources})
  add_dependencies(
    thriftcpp2
    rpcmetadata
    thrift_annotation
    thriftmetadata
    thrift_path
    thrift_dynamic_value
    thrifttyperep
    thrifttype
    thriftanyrep
    serverdbginfo)

  target_link_libraries(
    thriftcpp2
    PUBLIC fmt::fmt
           thrift-cpp
           rpcmetadata
           thrift_annotation
           thriftmetadata
           thrift_path
           thrift_dynamic_value
           thriftfrozen2
           thriftprotocol
           thrifttyperep
           thrifttype
           thriftanyrep
           serverdbginfo
           ${thrift_xxhash})

  add_library(quic_thriftcpp2 server/ThriftQuicServer.cpp)
  add_dependencies(quic_thriftcpp2 thriftcpp2 mvfst::mvfst_server_async_tran)
  target_link_libraries(quic_thriftcpp2 PUBLIC thriftcpp2
                                               ${thrift_mvfst_server})
endif () # THRIFT_RPC

# Install includes
set(THRIFT2_HEADER_DIRS
    async
    fast_thrift
    frozen
    protocol
    server
    transport
    util)
install(
  DIRECTORY ${THRIFT2_HEADER_DIRS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thrift/lib/cpp2
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.tcc")

if (BUILD_SHARED_LIBS)
  set_target_properties(
    rpcmetadata
    thrift_annotation
    thriftmetadata
    thrift_path
    thrift_dynamic_value
    thriftfrozen2
    thriftprotocol
    thrifttyperep
    thrifttype
    thriftanyrep
    serverdbginfo
    PROPERTIES VERSION ${PROJECT_VERSION})
  if (THRIFT_RPC)
    set_target_properties(thriftcpp2 PROPERTIES VERSION ${PROJECT_VERSION})
  endif ()
endif ()

install(
  TARGETS rpcmetadata
          thriftmetadata
          thrift_path
          thrift_dynamic_value
          thriftfrozen2
          thrifttyperep
          thrifttype
          thriftanyrep
          thriftprotocol
          serverdbginfo
  EXPORT fbthrift-exports
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
if (THRIFT_RPC)
  install(
    TARGETS thriftcpp2
    EXPORT fbthrift-exports
    DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()
