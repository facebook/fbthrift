# Requirements:
# Please provide the following two variables before using these macros:
#   ${THRIFT1} - path/to/thrift1
#   ${THRIFT_TEMPLATES} - path/to/include/thrift/templates

# thrift_library
# This creates a library that will contain the source files and all the proper
# dependencies to generate and compile thrift generated files
#
# Params:
#   @file_name - The name of tge thrift file
#   @services  - A list of services that are declared in the thrift file
#   @language  - The generator to use (cpp or cpp2)
#   @options   - Extra options to pass to the generator
#   @output_path - The directory where the thrift file lives
#
# Output:
#  file-language - A library to link to be used for linking
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
#     #output_path
#     #output_path
#   )
#   add_library(somelib ...)
#   target_link_libraries(somelib file_name-language)
macro(thrift_library file_name services language options file_path output_path)
thrift_generate(
  "${file_name}"   #file_name
  "${services}"    #services
  "${language}"    #language
  "${options}"     #options
  "${output_path}"   #output_path
  "${output_path}" #output_path
)
bypass_source_check(${${file_name}-${language}-SOURCES})
add_library("${file_name}-${language}" ${${file_name}-${language}-SOURCES})
add_dependencies("${file_name}-${language}" "${file_name}-${language}-target")
target_link_libraries("${file_name}-${language}" thriftcpp2)
message("Thrift will create the Library: ${file_name}-${language}")
endmacro()

# bypass_source_check
# This tells cmake to ignore if it doesn't see the following sources in
# the library that will be installed. Thrift files are generated at compile
# time so they do not exist at source check time
#
# Params:
#   @sources - The list of files to ignore in source check
macro(bypass_source_check sources)
set_source_files_properties(
  ${sources}
  PROPERTIES GENERATED TRUE
)
endmacro()

# thrift_generate
# This is used to codegen thrift files using the thrift compiler
# Params:
#   @file_name - The name of tge thrift file
#   @services  - A list of services that are declared in the thrift file
#   @language  - The generator to use (cpp or cpp2)
#   @options   - Extra options to pass to the generator
#   @output_path - The directory where the thrift file lives
#
# Output:
#  file-language-target     - A custom target to add a dependenct
#  ${file-language-HEADERS} - The generated Header Files.
#  ${file-language-SOURCES} - The generated Source Files.
#
# Notes:
# If any of the fields is empty, it is still required to provide an empty string
#
# When using file_language-SOURCES it should always call:
#   bypass_source_check(${file_language-SOURCES})
# This will prevent cmake from complaining about missing source files
macro(thrift_generate file_name services language options file_path output_path)
set("${file_name}-${language}-HEADERS"
  ${output_path}/gen-${language}/${file_name}_constants.h
  ${output_path}/gen-${language}/${file_name}_data.h
  ${output_path}/gen-${language}/${file_name}_types.h
  ${output_path}/gen-${language}/${file_name}_types.tcc
)
set("${file_name}-${language}-SOURCES"
  ${output_path}/gen-${language}/${file_name}_constants.cpp
  ${output_path}/gen-${language}/${file_name}_data.cpp
  ${output_path}/gen-${language}/${file_name}_types.cpp
  CACHE INTERNAL "${thrift_name}-SOURCES"
)
if("${language}" STREQUAL "cpp")
  set("${file_name}-${language}-HEADERS"
    ${${file_name}-${language}-HEADERS}
    ${output_path}/gen-${language}/${file_name}_reflection.h
  )
  set("${file_name}-${language}-SOURCES"
    ${${file_name}-${language}-SOURCES}
    ${output_path}/gen-${language}/${file_name}_reflection.cpp
  )
endif()
foreach(service ${services})
  set("${file_name}-${language}-HEADERS"
    ${${file_name}-${language}-HEADERS}
    ${output_path}/gen-${language}/${service}.h
    ${output_path}/gen-${language}/${service}.tcc
    ${output_path}/gen-${language}/${service}_custom_protocol.h
  )
  set("${file_name}-${language}-SOURCES"
    ${${file_name}-${language}-SOURCES}
    ${output_path}/gen-${language}/${service}.cpp
    ${output_path}/gen-${language}/${service}_client.cpp
    ${output_path}/gen-${language}/${service}_processmap_binary.cpp
    ${output_path}/gen-${language}/${service}_processmap_compact.cpp
  )
endforeach()
set(include_prefix "include_prefix=${output_path}")
if(NOT ${options} STREQUAL "")
  set(include_prefix ",${include_prefix}")
endif()
set(gen_language ${language})
if("${language}" STREQUAL "cpp2")
  set(gen_language "mstch_cpp2")
endif()
add_custom_command(
  OUTPUT ${${file_name}-${language}-HEADERS} ${${file_name}-${language}-SOURCES}
  COMMAND ${THRIFT1}
    --gen "${gen_language}:${options}${include_prefix}"
    -o ${output_path}
    --templates ${THRIFT_TEMPLATES}
    "${file_path}/${file_name}.thrift"
  DEPENDS thrift1
  COMMENT "Generating ${file_name} files. Output: ${output_path}"
)
add_custom_target(
  ${file_name}-${language}-target ALL
  DEPENDS ${${language}-${language}-HEADERS} ${${file_name}-${language}-SOURCES}
)
install(
  DIRECTORY gen-${language}
  DESTINATION include/${output_path}
  FILES_MATCHING PATTERN "*.h")
install(
  DIRECTORY gen-${language}
  DESTINATION include/${output_path}
  FILES_MATCHING PATTERN "*.tcc")
endmacro()
