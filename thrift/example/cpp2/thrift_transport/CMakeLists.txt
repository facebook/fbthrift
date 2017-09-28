PROJECT(ThriftTransport_CPP2_Example CXX)

add_executable(
  example_server

  server/EchoService.h
  server/EchoService.cpp
  server/ChatRoomService.h
  server/ChatRoomService.cpp
  server/ExampleServer.cpp
)
target_link_libraries(
  example_server
  thriftcpp2
  chatroom-cpp2
  ${GFLAGS_LIBRARIES}
  ${GLOG_LIBRARIES}
)
install(
  TARGETS example_server
  RUNTIME DESTINATION bin/cpp2/thrift_transport
)

add_executable(
  chatroom_client

  client/ChatRoomClient.cpp
)
target_link_libraries(
  chatroom_client

  chatroom-cpp2
)
install(
  TARGETS chatroom_client
  RUNTIME DESTINATION bin/cpp2/thrift_transport
)

add_executable(
  echo_client

  client/EchoClient.cpp
)
target_link_libraries(
  echo_client

  chatroom-cpp2
)
install(
  TARGETS echo_client
  RUNTIME DESTINATION bin/cpp2/thrift_transport
)
