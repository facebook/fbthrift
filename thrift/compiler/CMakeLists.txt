# Copyright (c) Meta Platforms, Inc. and affiliates.
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

# Set the compiler directory.
set(COMPILER_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Override the default install path for `make install` step.
set(CMAKE_INSTALL_PREFIX ${THRIFT_HOME})

# Compile Bison files.
set(BISON_FLAGS "--skeleton=lalr1.cc")
BISON_TARGET(ThriftParser parse/thrifty.yy ${COMPILER_DIR}/thrifty.cc
  DEFINES_FILE ${COMPILER_DIR}/location.hh
  DEFINES_FILE ${COMPILER_DIR}/thrifty.hh
  COMPILE_FLAGS "${BISON_FLAGS}")

# The Thrift AST library.
add_library(
  compiler_ast

  ast/t_base_type.cc
  ast/t_enum.cc
  ast/t_function.cc
  ast/t_interface.cc
  ast/t_named.cc
  ast/t_node.cc
  ast/t_package.cc
  ast/t_paramlist.cc
  ast/t_program.cc
  ast/t_scope.cc
  ast/t_structured.cc
  ast/t_type.cc
  ast/t_typedef.cc
  ast/visitor.cc
)
target_link_libraries(compiler_ast ${OPENSSL_LIBRARIES})

# The base library.
add_library(
  compiler_base

  common.cc
  diagnostic.cc
  filesystem.cc
  platform.cc
  util.cc

  parse/lexer.cc
  parse/lexer.h
  parse/parsing_driver.cc
  sema/const_checker.cc
  sema/patch_mutator.cc
  sema/scope_validator.cc
  sema/standard_mutator.cc
  sema/standard_validator.cc

  ${BISON_ThriftParser_OUTPUTS}
)
target_compile_definitions(
  compiler_base
  PRIVATE -DTHRIFTY_HH="${COMPILER_DIR}/thrifty.hh"
  PUBLIC -DLOCATION_HH="${COMPILER_DIR}/location.hh"
)
target_link_libraries(
  compiler_base
  compiler_ast
  ${Boost_LIBRARIES}
)

# The Thrift compiler library.
add_library(
  compiler

  gen/cpp/detail/gen.cc
  gen/cpp/namespace_resolver.cc
  gen/cpp/reference_type.cc
  gen/cpp/type_resolver.cc

  lib/cpp2/util.cc
  lib/java/util.cc
)
target_link_libraries(
  compiler
  compiler_ast
  ${Boost_LIBRARIES}
)

add_library(
  mustache
  detail/mustache/mstch.cpp
  detail/mustache/render_context.cpp
  detail/mustache/state/in_section.cpp
  detail/mustache/state/outside_section.cpp
  detail/mustache/template_type.cpp
  detail/mustache/token.cpp
  detail/mustache/utils.cpp
)
target_link_libraries(mustache ${Boost_LIBRARIES})

add_subdirectory(generate)

# Force compiler_generators linking (compiler will optimize it out otherwise).
if (MSVC)
  set(GENERATE_LINKED compiler_generators) # MSVC WHOLEARCHIVE is set after exe.
elseif (APPLE)
  set(GENERATE_LINKED -Wl,-force_load compiler_generators)
else ()
  set(GENERATE_LINKED
      -Wl,--whole-archive compiler_generators -Wl,--no-whole-archive)
endif ()

# Create the Thrift compiler binary.
add_executable(
  thrift1
  main.cc
  compiler.cc
  mutator/mutator.cc
  validator/validator.cc
)
target_link_libraries(
  thrift1
  compiler_ast
  compiler_base
  ${GENERATE_LINKED}
  ${Boost_LIBRARIES}
)
# Force compiler_generators linking (compiler will optimize it out otherwise).
if (MSVC)
  set_target_properties(
    thrift1
    PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:compiler_generators"
  )
endif ()

if (BUILD_SHARED_LIBS)
  # All but thrift1 since it's a binary.
  set_target_properties(
    compiler_ast compiler_base compiler mustache compiler_generators
    PROPERTIES VERSION ${PACKAGE_VERSION}
  )
endif ()

install(
  TARGETS thrift1 compiler_ast compiler_base compiler mustache
  EXPORT fbthrift-exports
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)
