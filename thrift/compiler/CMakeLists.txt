# Set the compiler directory
set(COMPILER_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Override default install path for `make install` step
set(CMAKE_INSTALL_PREFIX ${THRIFT_HOME})

# Compile Flex and Bison files and tie their dependencies
if(WIN32)
  set(FLEX_FLAGS "--wincompat")
endif(WIN32)
set(BISON_FLAGS "--skeleton=lalr1.cc")
BISON_TARGET(ThriftParser parse/thrifty.yy ${COMPILER_DIR}/thrifty.cc
  DEFINES_FILE ${COMPILER_DIR}/thrifty.hh
  COMPILE_FLAGS "${BISON_FLAGS}")
FLEX_TARGET(ThriftScanner parse/thriftl.ll ${COMPILER_DIR}/thriftl.cc
  COMPILE_FLAGS "${FLEX_FLAGS}")
ADD_FLEX_BISON_DEPENDENCY(ThriftScanner ThriftParser)

# build compiler/parse
add_library(
  compiler_ast

  ast/t_program.cc
  ast/t_type.cc
  ast/t_typedef.cc
  ast/base_types.cc
)
target_link_libraries(compiler_ast ${OPENSSL_LIBRARIES})

# build the base libraries
add_library(
  compiler_base

  common.cc
  parse/parsing_driver.cc
  platform.cc
  util.cc
  ${FLEX_ThriftScanner_OUTPUTS}
  ${BISON_ThriftParser_OUTPUTS}
)
target_compile_definitions(
 compiler_base
 PRIVATE -DTHRIFTY_HH="${COMPILER_DIR}/thrifty.hh"
)
target_compile_options(
 compiler_base
 PUBLIC "-Wno-deprecated-register"
)
target_link_libraries(
  compiler_base

  compiler_ast
  ${Boost_LIBRARIES}
)

# build compiler/lib
add_library(
  compiler_lib

  lib/cpp2/util.cc
  lib/java/util.cc
)
target_link_libraries(
  compiler_lib

  compiler_ast
  ${Boost_LIBRARIES}
  Folly::folly
)

# build compiler/generate
aux_source_directory(./generate GENERATOR_FILES)
aux_source_directory(./generate/templates MSTCH_TEMPLATES)
add_library(
  compiler_generators

  ${GENERATOR_FILES}
  ${MSTCH_TEMPLATES}
)
target_link_libraries(
  compiler_generators

  compiler_base
  compiler_lib
  ${Boost_LIBRARIES}
  ${MSTCH_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)
# Force compiler_generators linking (compiler will optimize it out otherwise)
if(MSVC)
  set(GENERATE_LINKED compiler_generators) #MSVC WHOLEARCHIVE is set after exe
elseif(APPLE)
  set(GENERATE_LINKED -Wl,-force_load compiler_generators)
else()
  set(GENERATE_LINKED -Wl,--whole-archive compiler_generators -Wl,--no-whole-archive)
endif(MSVC)

set(TEMPLATE_DIRS
  generate/templates/cpp2
  generate/templates/cpp2/common
  generate/templates/cpp2/module_types_cpp
  generate/templates/cpp2/module_types_h
  generate/templates/cpp2/module_types_tcc
  generate/templates/cpp2/service_client_cpp
  generate/templates/cpp2/service_common
  generate/templates/cpp2/service_cpp
  generate/templates/cpp2/service_h
  generate/templates/cpp2/service_tcc
  generate/templates/cpp2/types
)
foreach(dir ${TEMPLATE_DIRS})
  install(DIRECTORY ${dir} DESTINATION ${TEMPLATES_INSTALL_DIR}
          FILES_MATCHING PATTERN "*.mustache")
endforeach()

# Create the thrift compiler binary
add_executable(
  thrift1

  main.cc
  mutator/mutator.cc
  validator/validator.cc
  ast/visitor.cc
)
target_link_libraries(
  thrift1

  compiler_ast
  compiler_base
  ${GENERATE_LINKED}
)
# Force compiler_generators linking (compiler will optimize it out otherwise)
if(MSVC)
  set_target_properties(
    thrift1
    PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:compiler_generators"
  )
endif(MSVC)

install(
  TARGETS
    thrift1
    compiler_ast
    compiler_base
    compiler_lib
    compiler_generators
  EXPORT fbthrift-exports
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)
