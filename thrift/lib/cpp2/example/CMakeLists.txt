set(LIB_CPP2_EXAMPLE_HOME ${CMAKE_CURRENT_SOURCE_DIR})

# Example to test linking
PROJECT(ChatRoomServer CXX)
thrift_library(
  "chatroom" #file_name
  "ChatRoomService;Echo" #services
  "cpp2" #language
  "" #options
  "${LIB_CPP2_EXAMPLE_HOME}/if" #file_path
  "${LIB_CPP2_EXAMPLE_HOME}/if" #output_path
)
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
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
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
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
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
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
 )
