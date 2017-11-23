set(EXAMPLE_IF_HOME ${CMAKE_CURRENT_SOURCE_DIR})

# Example to test linking
thrift_library(
  "chatroom" #file_name
  "ChatRoomService;Echo" #services
  "cpp2" #language
  "" #options
  "${EXAMPLE_IF_HOME}" #file_path
  "${EXAMPLE_IF_HOME}" #output_path
  "thrift/example/if" #include_prefix
)
