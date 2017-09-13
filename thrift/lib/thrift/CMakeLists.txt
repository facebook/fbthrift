# Set the lib directory
set(LIB_THRIFT_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_TEMPLATES_DIR ${THRIFT_HOME}/thrift/compiler/generate/templates)

# Generate reflection files
set(REFLECTION_CPP_HEADERS
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_constants.h
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_data.h
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_reflection.h
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_types.h
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_types.tcc
)
set(REFLECTION_CPP_SOURCES
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_constants.cpp
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_data.cpp
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_reflection.cpp
  ${LIB_THRIFT_HOME}/gen-cpp/reflection_types.cpp
  PARENT_SCOPE
)
add_custom_command(
  OUTPUT ${REFLECTION_CPP_HEADERS} ${REFLECTION_CPP_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen cpp:templates,cob_style -r -o ${LIB_THRIFT_HOME} ${LIB_THRIFT_HOME}/reflection.thrift
  DEPENDS thrift1
  COMMENT "Generating reflection files"
)

# Generate frozen files
set(FROZEN_CPP_HEADERS
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_constants.h
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_data.h
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_reflection.h
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_types.h
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_types.tcc
)
set(FROZEN_CPP_SOURCES
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_constants.cpp
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_data.cpp
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_reflection.cpp
  ${LIB_THRIFT_HOME}/gen-cpp/frozen_types.cpp
  PARENT_SCOPE
)
add_custom_command(
  OUTPUT ${FROZEN_CPP_HEADERS} ${FROZEN_CPP_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen cpp:templates -r -o ${LIB_THRIFT_HOME} ${LIB_THRIFT_HOME}/frozen.thrift
  DEPENDS thrift1
  COMMENT "Generating frozen files"
)

# Generate frozen2 files
set(FROZEN_CPP2_HEADERS
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_constants.h
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_data.h
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_reflection.h
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_types.h
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_types.tcc
)
set(FROZEN_CPP2_SOURCES
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_constants.cpp
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_data.cpp
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_reflection.cpp
  ${LIB_THRIFT_HOME}/gen-cpp2/frozen_types.cpp
  PARENT_SCOPE
)
add_custom_command(
  OUTPUT ${FROZEN_CPP2_HEADERS} ${FROZEN_CPP2_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen mstch_cpp2 --templates ${THRIFT_TEMPLATES_DIR} -o ${LIB_THRIFT_HOME} ${LIB_THRIFT_HOME}/frozen.thrift
  DEPENDS thrift1
  COMMENT "Generating frozen2 files"
)

# Generate needed files
add_custom_target(
  reflection-cpp-target ALL
  DEPENDS ${REFLECTION_CPP_HEADERS} ${REFLECTION_CPP_SOURCES}
)
set_source_files_properties(
  ${REFLECTION_CPP_SOURCES}
  PROPERTIES GENERATED TRUE
)

add_custom_target(
  frozen-cpp-target ALL
  DEPENDS ${FROZEN_CPP_HEADERS} ${FROZEN_CPP_SOURCES}
)
set_source_files_properties(
  ${FROZEN_CPP_SOURCES}
  PROPERTIES GENERATED TRUE
)

add_custom_target(
  frozen-cpp2-target ALL
  DEPENDS ${FROZEN_CPP2_HEADERS} ${FROZEN_CPP2_SOURCES}
)
set_source_files_properties(
  ${FROZEN_CPP2_SOURCES}
  PROPERTIES GENERATED TRUE
)

set(THRIFT_HEADER_DIRS
  ${LIB_THRIFT_HOME}/gen-cpp
  ${LIB_THRIFT_HOME}/gen-cpp2
)
foreach(dir ${THRIFT_HEADER_DIRS})
  install(DIRECTORY ${dir} DESTINATION include/thrift/lib/thrift
          FILES_MATCHING PATTERN "*.h")
  install(DIRECTORY ${dir} DESTINATION include/thrift/lib/thrift
          FILES_MATCHING PATTERN "*.tcc")
endforeach()
