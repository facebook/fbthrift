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

add_executable(
  compiler_generate_build_templates

  build_templates.cc
)
target_link_libraries(
  compiler_generate_build_templates

  ${Boost_LIBRARIES}
)

file(
  GLOB_RECURSE
  TEMPLATE_FILES
  CONFIGURE_DEPENDS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/templates/**/*.mustache
)
add_custom_command(
  OUTPUT templates.cc
  COMMAND
    compiler_generate_build_templates
    ${CMAKE_CURRENT_SOURCE_DIR}/templates
    > templates.cc
  DEPENDS
    compiler_generate_build_templates
    ${TEMPLATE_FILES}
  VERBATIM
)

add_library(
  compiler_generate_templates

  ${CMAKE_CURRENT_BINARY_DIR}/templates.cc
)

install(
  TARGETS
    compiler_generate_templates
  EXPORT fbthrift-exports
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)
