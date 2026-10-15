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
#     [SERVICES <service>...]
#     [OPTIONS <option>...]
#     [INCLUDE_DIRECTORIES <dir>...]
#     [NAME <base>]
#     [OUTPUT_DIRECTORY <dir>]
#     [INCLUDE_PREFIX <prefix>]
#     [NAMESPACE <py.namespace>]
#     [OUT_SOURCES <var>])
#
#   <file>.thrift - The file to compile, relative to CMAKE_CURRENT_SOURCE_DIR.
#   LANGUAGE - The generator to use.
#   SERVICES - The services declared in the file.
#   OPTIONS - Generator options, e.g. `OPTIONS json no_metadata`.
#   INCLUDE_DIRECTORIES - Where the compiler looks for included .thrift files,
#       relative to CMAKE_CURRENT_SOURCE_DIR. Defaults to PROJECT_SOURCE_DIR.
#   NAME - Base name of the generation target. Defaults to the file name
#       without .thrift.
#   OUTPUT_DIRECTORY - Where gen-<language> is created, relative to
#       CMAKE_CURRENT_BINARY_DIR. Defaults to the file's directory under
#       PROJECT_BINARY_DIR.
#   INCLUDE_PREFIX - (cpp2, py3) The prefix generated code uses to include
#       generated headers. Defaults to the file's directory relative to
#       PROJECT_SOURCE_DIR.
#   NAMESPACE - (python) Must match the "namespace py3" directive in the file.
#       Dot-separated (e.g. "example.sum") maps to output directory structure
#       (example/sum/<file>/).
#   OUT_SOURCES - A variable to set to the generated sources.
#
# For a file outside PROJECT_SOURCE_DIR, OUTPUT_DIRECTORY is required, and so
# is INCLUDE_PREFIX for cpp2 and py3.
#
# Creates the target <NAME>-<LANGUAGE>-target, part of ALL, that generates the
# files. Adds no install rules.
#
# Using the generated sources from another directory needs bypass_source_check()
# there under policies older than CMake 3.20 (CMP0118).
#
# Positional form:
#
#   thrift_generate(file_name services language options file_path output_path
#                   include_prefix [TARGET_NAME_BASE <base>]
#                   [THRIFT_INCLUDE_DIRECTORIES <dir>...]
#                   [NAMESPACE <py.namespace>])
#
#   @file_name - Input file name, without .thrift. Will be used for naming the
#       CMake target if TARGET_NAME_BASE is not specified.
#   @services  - A list of services that are declared in the thrift file
#   @language  - The generator to use (cpp2, py3, or python)
#   @options   - Extra options to pass to the generator, comma-separated
#   @file_path - The directory where the thrift file lives
#   @output_path - The directory where gen-<language> is created
#   @include_prefix - Prefix to use for thrift includes in generated sources
#   @TARGET_NAME_BASE (optional) - name used for target instead of real filename
#   @THRIFT_INCLUDE_DIRECTORIES (optional) path to thrift include directories
#   @NAMESPACE (optional) - As above.
#
# If any of the positional fields is empty, it is still required to provide an
# empty string.
#
# Output:
#  <base>-<language>-target     - A custom target to add a dependency
#  ${<base>-<language>-HEADERS} - The generated Header Files.
#  ${<base>-<language>-SOURCES} - The generated Source Files.
# where <base> is TARGET_NAME_BASE or file_name. Except for python, it also
# installs gen-<language> from the current source directory.
#
# Using the sources from another directory needs bypass_source_check() there,
# under the same conditions as above.
#
# Both forms are parsed in this function: forwarding ${ARGV} to another
# function would drop empty arguments and split lists, and positional calls
# pass both.
function (thrift_generate)
  if ("${ARGV0}" MATCHES "\\.thrift$")
    set(keyword_form TRUE)
    set(one_value_keywords
        LANGUAGE
        NAME
        OUTPUT_DIRECTORY
        INCLUDE_PREFIX
        NAMESPACE
        OUT_SOURCES)
    cmake_parse_arguments(
      PARSE_ARGV 1 THRIFT_GENERATE "" "${one_value_keywords}"
      "SERVICES;OPTIONS;INCLUDE_DIRECTORIES")
    if (DEFINED THRIFT_GENERATE_UNPARSED_ARGUMENTS)
      message(
        FATAL_ERROR
          "thrift_generate: unexpected arguments: "
          "${THRIFT_GENERATE_UNPARSED_ARGUMENTS}")
    endif ()
    foreach (keyword ${THRIFT_GENERATE_KEYWORDS_MISSING_VALUES})
      if (keyword IN_LIST one_value_keywords)
        message(FATAL_ERROR "thrift_generate: ${keyword} needs a value")
      endif ()
    endforeach ()
    set(language "${THRIFT_GENERATE_LANGUAGE}")
    if (NOT language MATCHES "^(cpp2|py3|python)$")
      message(
        FATAL_ERROR "thrift_generate: LANGUAGE must be cpp2, py3 or python")
    endif ()
    if (DEFINED THRIFT_GENERATE_NAMESPACE AND NOT language STREQUAL "python")
      message(FATAL_ERROR "thrift_generate: NAMESPACE requires LANGUAGE python")
    endif ()
    if (DEFINED THRIFT_GENERATE_INCLUDE_PREFIX AND language STREQUAL "python")
      message(
        FATAL_ERROR "thrift_generate: INCLUDE_PREFIX does not apply to python")
    endif ()

    get_filename_component(thrift_file "${ARGV0}" ABSOLUTE BASE_DIR
                           "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(stem "${thrift_file}" NAME_WLE)
    get_filename_component(thrift_dir "${thrift_file}" DIRECTORY)
    file(RELATIVE_PATH rel "${PROJECT_SOURCE_DIR}" "${thrift_dir}")
    set(outside FALSE)
    if (rel MATCHES "^\\.\\.(/|$)" OR IS_ABSOLUTE "${rel}")
      set(outside TRUE)
    endif ()

    set(name "${stem}")
    if (DEFINED THRIFT_GENERATE_NAME)
      set(name "${THRIFT_GENERATE_NAME}")
    endif ()
    if (DEFINED THRIFT_GENERATE_OUTPUT_DIRECTORY)
      get_filename_component(
        output_dir "${THRIFT_GENERATE_OUTPUT_DIRECTORY}" ABSOLUTE BASE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}")
    elseif (outside)
      message(
        FATAL_ERROR
          "thrift_generate: ${ARGV0} is outside PROJECT_SOURCE_DIR, so "
          "OUTPUT_DIRECTORY is required")
    elseif (rel STREQUAL "")
      set(output_dir "${PROJECT_BINARY_DIR}")
    else ()
      set(output_dir "${PROJECT_BINARY_DIR}/${rel}")
    endif ()
    if (DEFINED THRIFT_GENERATE_INCLUDE_PREFIX)
      set(include_prefix "${THRIFT_GENERATE_INCLUDE_PREFIX}")
    elseif (language STREQUAL "python")
      set(include_prefix "")
    elseif (outside)
      message(
        FATAL_ERROR
          "thrift_generate: ${ARGV0} is outside PROJECT_SOURCE_DIR, so "
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
    set(namespace "${THRIFT_GENERATE_NAMESPACE}")
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
  else ()
    set(keyword_form FALSE)
    if (ARGC LESS 7)
      message(
        FATAL_ERROR
          "thrift_generate: expected <file>.thrift and keywords, or seven "
          "positional arguments; got ${ARGC} arguments")
    endif ()
    set(stem "${ARGV0}")
    set(services "${ARGV1}")
    set(language "${ARGV2}")
    set(options "${ARGV3}")
    set(thrift_file "${ARGV4}/${ARGV0}.thrift")
    set(output_dir "${ARGV5}")
    set(include_prefix "${ARGV6}")
    set(trailing)
    if (ARGC GREATER 7)
      math(EXPR last "${ARGC} - 1")
      foreach (i RANGE 7 ${last})
        list(APPEND trailing "${ARGV${i}}")
      endforeach ()
    endif ()
    cmake_parse_arguments(
      THRIFT_GENERATE "" "TARGET_NAME_BASE;NAMESPACE"
      "THRIFT_INCLUDE_DIRECTORIES" "${trailing}")
    set(name "${stem}")
    if (DEFINED THRIFT_GENERATE_TARGET_NAME_BASE
        AND NOT THRIFT_GENERATE_TARGET_NAME_BASE STREQUAL "")
      set(name "${THRIFT_GENERATE_TARGET_NAME_BASE}")
    endif ()
    set(namespace "${THRIFT_GENERATE_NAMESPACE}")
    set(include_args)
    foreach (dir ${THRIFT_GENERATE_THRIFT_INCLUDE_DIRECTORIES})
      list(APPEND include_args "-I" "${dir}")
    endforeach ()
    set(layouts FALSE)
    if ("${options}" MATCHES "layouts")
      set(layouts TRUE)
    endif ()
    set(no_metadata FALSE)
    if ("${options}" MATCHES "no_metadata")
      set(no_metadata TRUE)
    endif ()
  endif ()

  set(generator "${language}")
  if (language MATCHES "^(cpp2|py3|python)$")
    set(generator "mstch_${language}")
  endif ()
  string(REPLACE ";" "," joined_options "${options}")
  set(gen_dir "${output_dir}/gen-${language}")
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
      COMMENT "Generating ${name} thrift-python files. Output: ${output_dir}")
    add_custom_target(${name}-${language}-target ALL DEPENDS ${sources})
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
      COMMENT "Generating ${name} files. Output: ${output_dir}")
    add_custom_target(${name}-${language}-target ALL DEPENDS ${headers}
                                                            ${sources})
  endif ()

  if (keyword_form)
    if (DEFINED THRIFT_GENERATE_OUT_SOURCES)
      set(${THRIFT_GENERATE_OUT_SOURCES}
          "${sources}"
          PARENT_SCOPE)
    endif ()
  else ()
    if (NOT "${language}" STREQUAL "python")
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
    endif ()
    set("${name}-${language}-HEADERS"
        "${headers}"
        PARENT_SCOPE)
    set("${name}-${language}-SOURCES"
        "${sources}"
        PARENT_SCOPE)
  endif ()
endfunction ()

# This creates a object that will contain the source files and all the proper
# dependencies to generate and compile thrift generated files
#
# Params:
#   @file_name - The name of the thrift file
#   @services  - A list of services that are declared in the thrift file
#   @language  - The generator to use (cpp, cpp2, or python)
#   @options   - Extra options to pass to the generator
#   @file_path - The directory where the thrift file lives
#   @output_path - The directory where the thrift objects will be built
#   @include_prefix - The last part of output_path, relative include prefix
#
# Output:
#  A object file named `${file-name}-${language}-obj` to include into your
#  project's library
#
# Notes:
# If any of the fields is empty, it is still required to provide an empty string
#
# Usage:
#   thrift_object(
#     #file_name
#     #services
#     #language
#     #options
#     #file_path
#     #output_path
#     #include_prefix
#   )
#   add_library(somelib $<TARGET_OBJECTS:${file_name}-${language}-obj> ...)
macro (
  thrift_object
  file_name
  services
  language
  options
  file_path
  output_path
  include_prefix)
  thrift_generate(
    "${file_name}"
    "${services}"
    "${language}"
    "${options}"
    "${file_path}"
    "${output_path}"
    "${include_prefix}"
    ${ARGN})
  if ("${language}" STREQUAL "python")
    # Python: generated .py files are the artifacts
    message("Thrift will generate Python files for : ${file_name}-${language}")
  else ()
    bypass_source_check(${${file_name}-${language}-SOURCES})
    add_library("${file_name}-${language}-obj" OBJECT
                ${${file_name}-${language}-SOURCES})
    # $<TARGET_OBJECTS:...> and add_dependencies() carry no usage
    # requirements, so the generated sources need the roots directly.
    target_include_directories(
      "${file_name}-${language}-obj"
      PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
             $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>)
    add_dependencies("${file_name}-${language}-obj"
                     "${file_name}-${language}-target")
    message("Thrift will create the Object file : ${file_name}-${language}-obj")
  endif ()
endmacro ()

# Same as thrift object in terms of usage but creates the library instead of
# object so that you can use to link against your library instead of including
# all symbols into your library
#
# Params:
#   @file_name - The name of the thrift file
#   @services  - A list of services that are declared in the thrift file
#   @language  - The generator to use (cpp, cpp2, or python)
#   @options   - Extra options to pass to the generator
#   @file_path - The directory where the thrift file lives
#   @output_path - The directory where the thrift objects will be built
#   @include_prefix - The last part of output_path, relative include prefix
#
# Output:
#  A library file named `${file-name}-${language}` to link against your
#  project's library
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
macro (
  thrift_library
  file_name
  services
  language
  options
  file_path
  output_path
  include_prefix)
  thrift_object(
    "${file_name}"
    "${services}"
    "${language}"
    "${options}"
    "${file_path}"
    "${output_path}"
    "${include_prefix}"
    ${ARGN})
  if ("${language}" STREQUAL "python")
    # Python: create an alias target so users can depend on <name>-python
    add_custom_target("${file_name}-${language}" ALL)
    add_dependencies("${file_name}-${language}"
                     "${file_name}-${language}-target")
    message("Thrift will create the Python library : ${file_name}-${language}")
  else ()
    add_library("${file_name}-${language}"
                $<TARGET_OBJECTS:${file_name}-${language}-obj>)
    target_link_libraries("${file_name}-${language}" ${THRIFTCPP2})
    message("Thrift will create the Library file : ${file_name}-${language}")
  endif ()
endmacro ()

# This tells cmake to ignore if it doesn't see the following sources in
# the library that will be installed. Thrift files are generated at compile
# time so they do not exist at source check time
#
# Params:
#   @sources - The list of files to ignore in source check
macro (bypass_source_check sources)
  set_source_files_properties(${sources} PROPERTIES GENERATED TRUE)
endmacro ()
