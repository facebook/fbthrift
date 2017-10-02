# Set the compiler directory
set(COMPILER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Override default install path for `make install` step
set(CMAKE_INSTALL_PREFIX ${THRIFT_HOME})

# Compile Flex and Bison files and tie their dependencies
if(WIN32)
  set(FLEX_FLAGS "--wincompat")
endif(WIN32)
BISON_TARGET(ThriftParser thrifty.yy ${COMPILER_DIR}/thrifty.cc
  DEFINES_FILE ${COMPILER_DIR}/thrifty.hh)
FLEX_TARGET(ThriftScanner thriftl.ll ${COMPILER_DIR}/thriftl.cc
  COMPILE_FLAGS "${FLEX_FLAGS}")
ADD_FLEX_BISON_DEPENDENCY(ThriftScanner ThriftParser)

# build compiler/parse
add_library(
  compiler_ast

  parse/t_program.cc
  parse/t_type.cc
  parse/t_typedef.cc
)
target_link_libraries(compiler_ast ${OPENSSL_LIBRARIES})
install(TARGETS compiler_ast DESTINATION lib)

# build the base libraries
add_library(
  compiler_base

  common.cc
  platform.cc
  ${FLEX_ThriftScanner_OUTPUTS}
  ${BISON_ThriftParser_OUTPUTS}
)
target_compile_definitions(
 compiler_base
 PRIVATE -DTHRIFTY_HH="${COMPILER_DIR}/thrifty.hh"
)
target_link_libraries(
  compiler_base

  compiler_ast
  ${Boost_LIBRARIES}
)
install(TARGETS compiler_base DESTINATION lib)

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
install(TARGETS compiler_generators DESTINATION lib)

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
  install(DIRECTORY ${dir} DESTINATION include/thrift/templates
          FILES_MATCHING PATTERN "*.mustache")
endforeach()

# Create the thrift compiler binary
add_executable(
  thrift1

  main.cc
  mutator.cc
  validator.cc
  visitor.cc
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
install(TARGETS thrift1 DESTINATION bin)
