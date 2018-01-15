# Set the lib directory
set(LIB_THRIFT_HOME ${CMAKE_CURRENT_SOURCE_DIR})

thrift_generate(
  "reflection" #file_name
  "" #services
  "cpp" #language
  "templates" #options
  "${LIB_THRIFT_HOME}" #file_path
  "${LIB_THRIFT_HOME}" #output_path
  "thrift/lib/thrift" #include_prefix
)

thrift_generate(
  "frozen" #file_name
  "" #services
  "cpp" #language
  "templates" #options
  "${LIB_THRIFT_HOME}" #file_path
  "${LIB_THRIFT_HOME}" #output_path
  "thrift/lib/thrift" #include_prefix
)

thrift_generate(
  "frozen" #file_name
  "" #services
  "cpp2" #language
  "" #options
  "${LIB_THRIFT_HOME}" #file_path
  "${LIB_THRIFT_HOME}" #output_path
  "thrift/lib/thrift" #include_prefix
)

thrift_generate(
  "RpcMetadata" #file_name
  "" #services
  "cpp2" #language
  "" #options
  "${LIB_THRIFT_HOME}" #file_path
  "${LIB_THRIFT_HOME}" #output_path
  "thrift/lib/thrift" #include_prefix
)

set_property(GLOBAL PROPERTY RpcMetadata-cpp2-SOURCES ${RpcMetadata-cpp2-SOURCES})
