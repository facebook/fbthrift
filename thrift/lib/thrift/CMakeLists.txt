# Set the lib directory
set(LIB_THRIFT_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_BINARY ${THRIFT_BUILD}/thrift/compiler/thrift-compiler)
set(THRIFT_TEMPLATES_DIR ${THRIFT_HOME}/thrift/compiler/generate/templates)

# Generate reflection files
set(REFLECTION_HEADERS
  gen-cpp/reflection_constants.h
  gen-cpp/reflection_data.h
  gen-cpp/reflection_reflection.h
  gen-cpp/reflection_types.h
  gen-cpp/reflection_types.tcc
)
set(REFLECTION_SOURCES
  gen-cpp/reflection_constants.cpp
  gen-cpp/reflection_data.cpp
  gen-cpp/reflection_reflection.cpp
  gen-cpp/reflection_types.cpp
)
add_custom_command(
  OUTPUT ${REFLECTION_HEADERS} ${REFLECTION_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen cpp:templates,cob_style -r -o ${LIB_THRIFT_HOME} ${LIB_THRIFT_HOME}/reflection.thrift
  DEPENDS thrift-compiler
  COMMENT "Generating reflection files"
)

# Generate frozen files
set(FROZEN_HEADERS
  gen-cpp/frozen_constants.h
  gen-cpp/frozen_data.h
  gen-cpp/frozen_reflection.h
  gen-cpp/frozen_types.h
  gen-cpp/frozen_types.tcc
)
set(FROZEN_SOURCES
  gen-cpp/frozen_constants.cpp
  gen-cpp/frozen_data.cpp
  gen-cpp/frozen_reflection.cpp
  gen-cpp/frozen_types.cpp
)
add_custom_command(
  OUTPUT ${FROZEN_HEADERS} ${FROZEN_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen cpp:templates -r -o ${LIB_THRIFT_HOME} ${LIB_THRIFT_HOME}/frozen.thrift
  DEPENDS thrift-compiler
  COMMENT "Generating frozen files"
)

# Generate frozen2 files
set(FROZEN2_HEADERS
  gen-cpp2/frozen_constants.h
  gen-cpp2/frozen_data.h
  gen-cpp2/frozen_reflection.h
  gen-cpp2/frozen_types.h
  gen-cpp2/frozen_types.tcc
)
set(FROZEN2_SOURCES
  gen-cpp2/frozen_constants.cpp
  gen-cpp2/frozen_data.cpp
  gen-cpp2/frozen_reflection.cpp
  gen-cpp2/frozen_types.cpp
)
add_custom_command(
  OUTPUT ${FROZEN2_HEADERS} ${FROZEN2_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen mstch_cpp2 --templates ${THRIFT_TEMPLATES_DIR} -o ${LIB_THRIFT_HOME} ${LIB_THRIFT_HOME}/frozen.thrift
  DEPENDS thrift-compiler
  COMMENT "Generating frozen2 files"
)

# Generate needed files
add_custom_target(
  lib-thrift ALL
  DEPENDS ${REFLECTION_HEADERS} ${REFLECTION_SOURCES} ${FROZEN_HEADERS} ${FROZEN_SOURCES}
)

add_custom_target(
  lib-thrift2 ALL
  DEPENDS ${FROZEN2_HEADERS} ${FROZEN2_SOURCES}
)
