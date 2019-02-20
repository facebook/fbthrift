thrift_generate(
  "reflection" #file_name
  "" #services
  "cpp2" #language
  "templates" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  "thrift/lib/thrift" #include_prefix
)

thrift_generate(
  "frozen" #file_name
  "" #services
  "cpp2" #language
  "" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  "thrift/lib/thrift" #include_prefix
)

thrift_generate(
  "RpcMetadata" #file_name
  "" #services
  "cpp2" #language
  "" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  "thrift/lib/thrift" #include_prefix
)

set_property(GLOBAL PROPERTY RpcMetadata-cpp2-SOURCES ${RpcMetadata-cpp2-SOURCES})
set_property(GLOBAL PROPERTY frozen-cpp2-SOURCES ${frozen-cpp2-SOURCES})
set_property(GLOBAL PROPERTY reflection-cpp2-SOURCES ${reflection-cpp2-SOURCES})
