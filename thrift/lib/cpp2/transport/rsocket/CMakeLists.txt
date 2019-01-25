# Set the lib directory
set(LIB_THRIFT_HOME ${CMAKE_CURRENT_SOURCE_DIR})

thrift_generate(
  "Config" #file_name
  "" #services
  "cpp2" #language
  "" #options
  "${LIB_THRIFT_HOME}" #file_path
  "${LIB_THRIFT_HOME}" #output_path
  "thrift/lib/cpp2/transport/rsocket" #include_prefix
)

set_property(GLOBAL PROPERTY rsocket-cpp2-SOURCES ${Config-cpp2-SOURCES})
