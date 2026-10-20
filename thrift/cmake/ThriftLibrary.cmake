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

# ${THRIFT_COMPILER} goes to COMMAND as-is rather than through
# $<TARGET_FILE:...>. CMake substitutes an imported target's location and adds
# the dependency, and when cross-compiling an in-tree compiler it falls back to
# a host thrift on PATH -- which a baked-in path to the unrunnable
# target-architecture binary would defeat.

# The in-tree target names. A project building against an installed Thrift
# overrides these before including this file: its exported targets carry a
# namespace prefix, and some such projects point THRIFT_COMPILER at a thrift
# binary found on PATH instead.
if (NOT DEFINED THRIFT_COMPILER)
  set(THRIFT_COMPILER thrift)
endif ()
if (NOT DEFINED THRIFTCPP2)
  set(THRIFTCPP2 thriftcpp2)
endif ()

# Consumers include this file directly, so it cannot assume the includer has
# already pulled in the standard install directory variables.
include(GNUInstallDirs)

# Generates code from a .thrift file with the Thrift compiler.
#
#   thrift_generate(<file>.thrift
#     LANGUAGE <cpp2|py3|python>
#     [TARGET <target>]
#     [OUT_SOURCES <var>]
#     [SERVICES <service>...]
#     [OPTIONS <option>...]
#     [INCLUDE_DIRECTORIES <dir>...]
#     [OUTPUT_DIRECTORY <dir>]
#     [INCLUDE_PREFIX <prefix>]
#     [NAMESPACE <py.namespace>])
#
#   <file>.thrift - The file to compile, relative to CMAKE_CURRENT_SOURCE_DIR.
#   LANGUAGE - The generator to use.
#   TARGET - A target to add the generated files to as sources. For cpp2, it
#       also links FBThrift::protocol publicly, which provides the
#       include directories and runtime the generated code needs.
#   OUT_SOURCES - A variable to set to the generated sources.
#   SERVICES - The services declared in the file.
#   OPTIONS - Generator options, e.g. `OPTIONS json no_metadata`.
#   INCLUDE_DIRECTORIES - Where the compiler looks for included .thrift files,
#       relative to CMAKE_CURRENT_SOURCE_DIR. Defaults to PROJECT_SOURCE_DIR.
#   OUTPUT_DIRECTORY - Where gen-<language> is created, relative to
#       CMAKE_CURRENT_BINARY_DIR. Defaults to the file's directory under
#       PROJECT_BINARY_DIR.
#   INCLUDE_PREFIX - (cpp2, py3) The prefix generated code uses to include
#       generated headers; empty for none. Defaults to the file's directory
#       relative to PROJECT_SOURCE_DIR.
#   NAMESPACE - (python) Must match the "namespace py3" directive in the file.
#       Dot-separated (e.g. "example.sum") maps to output directory structure
#       (example/sum/<file>/).
#
# For a file outside PROJECT_SOURCE_DIR, OUTPUT_DIRECTORY is required, and so
# is INCLUDE_PREFIX for cpp2 and py3.
#
# Like protobuf_generate(), creates no targets and adds no install rules. A
# target in the calling directory that lists the generated files as sources,
# with TARGET or OUT_SOURCES, generates them; CMake gives the rule only to
# targets in that directory. With Makefiles, targets that both list the files
# can run the compiler twice in parallel, so list them in one and make the
# others depend on it.
function (thrift_generate file)
  if (NOT "${file}" MATCHES "\\.thrift$")
    message(
      FATAL_ERROR
        "thrift_generate: expected a .thrift file, got '${file}'. The "
        "positional form was removed; use thrift_generate(<file>.thrift "
        "LANGUAGE ...) or thrift_library().")
  endif ()
  set(one_value_keywords
      LANGUAGE
      TARGET
      OUT_SOURCES
      OUTPUT_DIRECTORY
      INCLUDE_PREFIX
      NAMESPACE)
  # Before CMP0174 (CMake 3.31), cmake_parse_arguments() drops a keyword whose
  # value is an empty string, so find those here.
  set(empty_keywords)
  math(EXPR last "${ARGC} - 2")
  if (last GREATER_EQUAL 1)
    foreach (i RANGE 1 ${last})
      math(EXPR next "${i} + 1")
      if ("${ARGV${i}}" IN_LIST one_value_keywords AND "${ARGV${next}}"
                                                       STREQUAL "")
        list(APPEND empty_keywords "${ARGV${i}}")
      endif ()
    endforeach ()
  endif ()
  cmake_policy(PUSH)
  if (POLICY CMP0174)
    cmake_policy(SET CMP0174 NEW)
  endif ()
  cmake_parse_arguments(
    PARSE_ARGV 1 THRIFT_GENERATE "" "${one_value_keywords}"
    "SERVICES;OPTIONS;INCLUDE_DIRECTORIES")
  cmake_policy(POP)
  if (DEFINED THRIFT_GENERATE_UNPARSED_ARGUMENTS)
    message(
      FATAL_ERROR
        "thrift_generate: unexpected arguments: "
        "${THRIFT_GENERATE_UNPARSED_ARGUMENTS}")
  endif ()
  foreach (keyword ${THRIFT_GENERATE_KEYWORDS_MISSING_VALUES} ${empty_keywords})
    # Only an explicit "" means empty. A keyword with no argument at all is
    # also in KEYWORDS_MISSING_VALUES, and under CMP0174 it is DEFINED too.
    if (keyword MATCHES "^(INCLUDE_PREFIX|NAMESPACE)$"
        AND keyword IN_LIST empty_keywords)
      continue()
    endif ()
    if (keyword IN_LIST one_value_keywords)
      message(FATAL_ERROR "thrift_generate: ${keyword} needs a value")
    endif ()
  endforeach ()
  set(language "${THRIFT_GENERATE_LANGUAGE}")
  if (NOT language MATCHES "^(cpp2|py3|python)$")
    message(
      FATAL_ERROR "thrift_generate: LANGUAGE must be cpp2, py3 or python")
  endif ()
  set(namespace "${THRIFT_GENERATE_NAMESPACE}")
  if (NOT namespace STREQUAL "" AND NOT language STREQUAL "python")
    message(FATAL_ERROR "thrift_generate: NAMESPACE requires LANGUAGE python")
  endif ()
  if (NOT "${THRIFT_GENERATE_INCLUDE_PREFIX}" STREQUAL ""
      AND language STREQUAL "python")
    message(
      FATAL_ERROR "thrift_generate: INCLUDE_PREFIX does not apply to python")
  endif ()

  get_filename_component(thrift_file "${file}" ABSOLUTE BASE_DIR
                         "${CMAKE_CURRENT_SOURCE_DIR}")
  get_filename_component(stem "${thrift_file}" NAME_WLE)
  get_filename_component(thrift_dir "${thrift_file}" DIRECTORY)
  file(RELATIVE_PATH rel "${PROJECT_SOURCE_DIR}" "${thrift_dir}")
  set(outside FALSE)
  if (rel MATCHES "^\\.\\.(/|$)" OR IS_ABSOLUTE "${rel}")
    set(outside TRUE)
  endif ()

  if (DEFINED THRIFT_GENERATE_OUTPUT_DIRECTORY)
    get_filename_component(
      output_dir "${THRIFT_GENERATE_OUTPUT_DIRECTORY}" ABSOLUTE BASE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}")
  elseif (outside)
    message(
      FATAL_ERROR
        "thrift_generate: ${file} is outside PROJECT_SOURCE_DIR, so "
        "OUTPUT_DIRECTORY is required")
  elseif (rel STREQUAL "")
    set(output_dir "${PROJECT_BINARY_DIR}")
  else ()
    set(output_dir "${PROJECT_BINARY_DIR}/${rel}")
  endif ()
  if (DEFINED THRIFT_GENERATE_INCLUDE_PREFIX OR "INCLUDE_PREFIX" IN_LIST
                                                 empty_keywords)
    set(include_prefix "${THRIFT_GENERATE_INCLUDE_PREFIX}")
  elseif (language STREQUAL "python")
    set(include_prefix "")
  elseif (outside)
    message(
      FATAL_ERROR
        "thrift_generate: ${file} is outside PROJECT_SOURCE_DIR, so "
        "INCLUDE_PREFIX is required")
  else ()
    set(include_prefix "${rel}")
  endif ()
  set(include_args)
  foreach (dir ${THRIFT_GENERATE_INCLUDE_DIRECTORIES})
    get_filename_component(dir "${dir}" ABSOLUTE BASE_DIR
                           "${CMAKE_CURRENT_SOURCE_DIR}")
    list(APPEND include_args "-I" "${dir}")
  endforeach ()
  if (NOT include_args)
    set(include_args "-I" "${PROJECT_SOURCE_DIR}")
  endif ()
  set(services "${THRIFT_GENERATE_SERVICES}")
  set(options "${THRIFT_GENERATE_OPTIONS}")
  set(layouts FALSE)
  if ("layouts" IN_LIST options)
    set(layouts TRUE)
  endif ()
  set(no_metadata FALSE)
  if ("no_metadata" IN_LIST options)
    set(no_metadata TRUE)
  endif ()

  set(generator "mstch_${language}")
  string(REPLACE ";" "," joined_options "${options}")
  set(gen_dir "${output_dir}/gen-${language}")
  # The compiler requires -o to exist. Ninja creates the directories of a
  # command's outputs, but Makefiles don't, and only directories added with
  # add_subdirectory() exist otherwise.
  file(MAKE_DIRECTORY "${output_dir}")
  set(headers
      ${gen_dir}/${stem}_constants.h
      ${gen_dir}/${stem}_data.h
      ${gen_dir}/${stem}_metadata.h
      ${gen_dir}/${stem}_types.h
      ${gen_dir}/${stem}_types.tcc
      ${gen_dir}/${stem}_types_custom_protocol.h)
  set(sources
      ${gen_dir}/${stem}_constants.cpp
      ${gen_dir}/${stem}_data.cpp
      ${gen_dir}/${stem}_types.cpp
      ${gen_dir}/${stem}_types_binary.cpp
      ${gen_dir}/${stem}_types_compact.cpp
      ${gen_dir}/${stem}_types_serialization.cpp)
  if (layouts)
    list(APPEND sources ${gen_dir}/${stem}_layouts.cpp)
  endif ()
  if (NOT no_metadata)
    list(APPEND sources ${gen_dir}/${stem}_metadata.cpp)
  endif ()
  foreach (service ${services})
    list(
      APPEND
      headers
      ${gen_dir}/${service}.h
      ${gen_dir}/${service}.tcc
      ${gen_dir}/${service}AsyncClient.h
      ${gen_dir}/${service}_custom_protocol.h)
    list(APPEND sources ${gen_dir}/${service}.cpp
         ${gen_dir}/${service}AsyncClient.cpp)
  endforeach ()

  if ("${language}" STREQUAL "python")
    # thrift-python uses PEP 420 namespace packages — no __init__.py.
    if (NOT "${namespace}" STREQUAL "")
      string(REPLACE "." "/" namespace_dir "${namespace}")
      set(python_dir "${output_dir}/gen-python/${namespace_dir}/${stem}")
    else ()
      set(python_dir "${output_dir}/gen-python/${stem}")
    endif ()
    set(headers "")
    set(sources
        ${python_dir}/thrift_types.py
        ${python_dir}/thrift_types.pyi
        ${python_dir}/thrift_metadata.py
        ${python_dir}/thrift_enums.py
        ${python_dir}/thrift_abstract_types.py
        ${python_dir}/thrift_mutable_types.py
        ${python_dir}/thrift_mutable_types.pyi)
    if (NOT "${services}" STREQUAL "")
      list(
        APPEND
        sources
        ${python_dir}/thrift_services.py
        ${python_dir}/thrift_clients.py
        ${python_dir}/thrift_mutable_services.py
        ${python_dir}/thrift_mutable_clients.py)
    endif ()
    set(gen "${generator}")
    if (NOT "${joined_options}" STREQUAL "")
      set(gen "${gen}:${joined_options}")
    endif ()
    add_custom_command(
      OUTPUT ${sources}
      COMMAND ${THRIFT_COMPILER} --gen "${gen}" -o ${output_dir} ${include_args}
              "${thrift_file}"
      DEPENDS ${THRIFT_COMPILER} "${thrift_file}"
      COMMENT "Generating ${stem} thrift-python files. Output: ${output_dir}")
  else ()
    if ("${language}" STREQUAL "py3")
      file(WRITE "${gen_dir}/${stem}/__init__.py")
    endif ()
    set(gen "${generator}:${joined_options}")
    if (NOT "${include_prefix}" STREQUAL "")
      if (NOT "${joined_options}" STREQUAL "")
        set(gen "${gen},")
      endif ()
      set(gen "${gen}include_prefix=${include_prefix}")
    endif ()
    add_custom_command(
      OUTPUT ${headers} ${sources}
      COMMAND ${THRIFT_COMPILER} --gen "${gen}" -o ${output_dir} ${include_args}
              "${thrift_file}"
      DEPENDS ${THRIFT_COMPILER} "${thrift_file}"
      COMMENT "Generating ${stem} files. Output: ${output_dir}")
  endif ()

  if (DEFINED THRIFT_GENERATE_TARGET)
    target_sources(${THRIFT_GENERATE_TARGET} PRIVATE ${headers} ${sources})
    if (language STREQUAL "cpp2")
      # Several calls can share a target.
      get_target_property(links ${THRIFT_GENERATE_TARGET} LINK_LIBRARIES)
      if (NOT "FBThrift::protocol" IN_LIST links)
        target_link_libraries(${THRIFT_GENERATE_TARGET}
                              PUBLIC FBThrift::protocol)
      endif ()
    endif ()
  endif ()

  if (DEFINED THRIFT_GENERATE_OUT_SOURCES)
    set(${THRIFT_GENERATE_OUT_SOURCES}
        "${sources}"
        PARENT_SCOPE)
  endif ()
endfunction ()

# Generates code from a .thrift file and builds it into a library to link
# against.
#
# Params:
#   @file_name - The name of the thrift file, without .thrift
#   @services  - A list of services that are declared in the thrift file
#   @language  - The generator to use (cpp2 or python)
#   @options   - Extra options to pass to the generator, comma-separated
#   @file_path - The directory where the thrift file lives
#   @output_path - The directory where the thrift objects will be built
#   @include_prefix - The last part of output_path, relative include prefix
#   @TARGET_NAME_BASE (optional) - <base> in the variable below, instead of
#       file_name
#   @THRIFT_INCLUDE_DIRECTORIES (optional) path to thrift include directories
#   @NAMESPACE (optional) - As in thrift_generate (python).
#
# Output:
#  A library named `${file_name}-${language}` to link against, built from the
#  object library `${file_name}-${language}-obj`. For python, a target named
#  `${file_name}-python` that generates the files.
#  ${<base>-<language>-SOURCES} - The generated sources, where <base> is
#  TARGET_NAME_BASE or file_name.
#
# Except for python, it also installs gen-<language> from the current source
# directory.
#
# Notes:
# If any of the fields is empty, it is still required to provide an empty string
#
# Usage:
#   thrift_library(
#     #file_name
#     #services
#     #language
#     #options
#     #file_path
#     #output_path
#     #include_prefix
#   )
#   add_library(somelib ...)
#   target_link_libraries(somelib ${file_name}-${language} ...)
function (
  thrift_library
  file_name
  services
  language
  options
  file_path
  output_path
  include_prefix)
  cmake_parse_arguments(
    THRIFT_LIBRARY "" "TARGET_NAME_BASE;NAMESPACE" "THRIFT_INCLUDE_DIRECTORIES"
    ${ARGN})
  set(name "${file_name}")
  if (NOT "${THRIFT_LIBRARY_TARGET_NAME_BASE}" STREQUAL "")
    set(name "${THRIFT_LIBRARY_TARGET_NAME_BASE}")
  endif ()
  string(REPLACE "," ";" option_list "${options}")
  set(prefix "${include_prefix}")
  set(extra_args)
  if ("${language}" STREQUAL "python")
    set(prefix "")
    if (NOT "${THRIFT_LIBRARY_NAMESPACE}" STREQUAL "")
      list(APPEND extra_args NAMESPACE "${THRIFT_LIBRARY_NAMESPACE}")
    endif ()
  endif ()
  if (NOT "${output_path}" STREQUAL "")
    list(APPEND extra_args OUTPUT_DIRECTORY "${output_path}")
  endif ()
  thrift_generate(
    "${file_path}/${file_name}.thrift"
    LANGUAGE "${language}"
    OUT_SOURCES sources
    SERVICES ${services}
    OPTIONS ${option_list}
    INCLUDE_DIRECTORIES ${THRIFT_LIBRARY_THRIFT_INCLUDE_DIRECTORIES}
    INCLUDE_PREFIX "${prefix}"
    ${extra_args})
  set("${name}-${language}-SOURCES"
      "${sources}"
      PARENT_SCOPE)
  if ("${language}" STREQUAL "python")
    add_custom_target("${file_name}-${language}" ALL DEPENDS ${sources})
    message("Thrift will create the Python library : ${file_name}-${language}")
  else ()
    install(
      DIRECTORY gen-${language}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${include_prefix}
      FILES_MATCHING
      PATTERN "*.h")
    install(
      DIRECTORY gen-${language}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${include_prefix}
      FILES_MATCHING
      PATTERN "*.tcc")
    add_library("${file_name}-${language}-obj" OBJECT ${sources})
    # $<TARGET_OBJECTS:...> carries no usage requirements, so the generated
    # sources need the roots directly.
    target_include_directories(
      "${file_name}-${language}-obj"
      PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
             $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>)
    add_library("${file_name}-${language}"
                $<TARGET_OBJECTS:${file_name}-${language}-obj>)
    target_link_libraries("${file_name}-${language}" ${THRIFTCPP2})
    message("Thrift will create the Library file : ${file_name}-${language}")
  endif ()
endfunction ()
