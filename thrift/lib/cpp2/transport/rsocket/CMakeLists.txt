
thrift_generate(
  "Config" #file_name
  "" #services
  "cpp2" #language
  "" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  "thrift/lib/cpp2/transport/rsocket" #include_prefix
)

set_property(GLOBAL PROPERTY rsocket-cpp2-SOURCES ${Config-cpp2-SOURCES})
