# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
  ast/t_struct.cc
  ast/t_type.cc
  ast/t_typedef.cc
  ast/base_types.cc
  ast/t_scope.cc
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
if (NOT MSVC)
  target_compile_options(
    compiler_base
    PUBLIC "-Wno-deprecated-register"
  )
  # Some versions of flex emit `register` storage qualifiers,
  # so let's just suppress that warning.
  target_compile_options(
    compiler_base
    PRIVATE "-Wno-register"
  )
endif()
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
)

#build compiler/mustache
add_library(
  mustache_lib

  mustache/mstch.cpp
  mustache/render_context.cpp
  mustache/state/in_section.cpp
  mustache/state/outside_section.cpp
  mustache/template_type.cpp
  mustache/token.cpp
  mustache/utils.cpp
)
target_link_libraries(mustache_lib ${Boost_LIBRARIES})

add_subdirectory(generate)

# build compiler/generate
aux_source_directory(./generate GENERATOR_FILES)
list(REMOVE_ITEM GENERATOR_FILES ./generate/build_templates.cc)
add_library(
  compiler_generators

  ${GENERATOR_FILES}
)
target_link_libraries(
  compiler_generators

  compiler_generate_templates
  compiler_base
  compiler_lib
  mustache_lib
  ${Boost_LIBRARIES}
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

# Create the thrift compiler binary
add_executable(
  thrift1

  main.cc
  compiler.cc
  mutator/mutator.cc
  validator/diagnostic.cc
  validator/validator.cc
  ast/visitor.cc
)
target_link_libraries(
  thrift1

  compiler_ast
  compiler_base
  ${GENERATE_LINKED}
  ${Boost_LIBRARIES}
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
    mustache_lib
    compiler_generators
  EXPORT fbthrift-exports
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)
