# Set the lib directory
set(LIB_THRIFT_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_BINARY ${THRIFT_HOME}/thrift/compiler/thrift-compiler)
set(THRIFT_TEMPLATES_DIR ${THRIFT_HOME}/thrift/compiler/generate/templates)

# Generate reflection files
add_custom_command(
  OUTPUT gen-cpp/reflection_constants.h gen-cpp/reflection_types.h gen-cpp/reflection_data.h gen-cpp/reflection_reflection.h gen-cpp/reflection_constants.cpp gen-cpp/reflection_data.cpp gen-cpp/reflection_reflection.cpp gen-cpp/reflection_types.cpp gen-cpp/reflection_types.tcc
  COMMAND ${THRIFT_BINARY}
  ARGS --gen cpp:templates,cob_style -r ${LIB_THRIFT_HOME}/reflection.thrift
  COMMENT "Generating reflection files"
)

# Generate frozen files
add_custom_command(
  OUTPUT gen-cpp/frozen_constants.h gen-cpp/frozen_types.h gen-cpp/frozen_data.h gen-cpp/frozen_reflection.h gen-cpp/frozen_constants.cpp gen-cpp/frozen_data.cpp gen-cpp/frozen_reflection.cpp gen-cpp/frozen_types.cpp gen-cpp/frozen_types.tcc
  COMMAND ${THRIFT_BINARY}
  ARGS --gen cpp:templates -r ${LIB_THRIFT_HOME}/frozen.thrift
  COMMENT "Generating frozen files"
)

# Generate frozen2 files
add_custom_command(
  OUTPUT gen-cpp2/frozen_constants.h gen-cpp2/frozen_types.h gen-cpp2/frozen_data.h gen-cpp2/frozen_types_custom_protocol.h gen-cpp2/frozen_constants.cpp gen-cpp2/frozen_data.cpp gen-cpp2/frozen_types.cpp gen-cpp2/frozen_types.tcc
  COMMAND ${THRIFT_BINARY}
  ARGS --gen mstch_cpp2 --templates ${THRIFT_TEMPLATES_DIR} ${LIB_THRIFT_HOME}/frozen.thrift
  COMMENT "Generating frozen2 files"
)

# Generate needed files
add_custom_target(
  lib-thrift ALL
  DEPENDS gen-cpp/reflection_constants.h gen-cpp/reflection_types.h gen-cpp/reflection_data.h gen-cpp/reflection_reflection.h gen-cpp/reflection_constants.cpp gen-cpp/reflection_data.cpp gen-cpp/reflection_reflection.cpp gen-cpp/reflection_types.cpp gen-cpp/reflection_types.tcc gen-cpp/frozen_constants.h gen-cpp/frozen_types.h gen-cpp/frozen_data.h gen-cpp/frozen_reflection.h gen-cpp/frozen_constants.cpp gen-cpp/frozen_data.cpp gen-cpp/frozen_reflection.cpp gen-cpp/frozen_types.cpp gen-cpp/frozen_types.tcc
)

add_custom_target(
  lib-thrift2 ALL
  DEPENDS gen-cpp2/frozen_constants.h gen-cpp2/frozen_types.h gen-cpp2/frozen_data.h gen-cpp2/frozen_types_custom_protocol.h gen-cpp2/frozen_constants.cpp gen-cpp2/frozen_data.cpp gen-cpp2/frozen_types.cpp gen-cpp2/frozen_types.tcc
)
