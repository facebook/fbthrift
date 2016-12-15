cmake_minimum_required(VERSION 3.1.3 FATAL_ERROR)

# Get the root directory file inclusion
set(COMPILER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(THRIFT_DIR ${COMPILER_DIR} DIRECTORY)
get_filename_component(THRIFT_ROOT_DIR ${THRIFT_DIR} DIRECTORY)
include_directories(${THRIFT_ROOT_DIR})

# Link linked libraries to out of source directory
link_directories(${CMAKE_BINARY_DIR})

# This is necessary so that find_package can find the Find$x.cmake files
set(CMAKE_MODULE_PATH ${COMPILER_DIR} ${CMAKE_MODULE_PATH})

# package information
set(PACKAGE_NAME "thrift_compiler")

# Set cpp requirements
project(${PACKAGE_NAME} C CXX)
set(CMAKE_CXX_STANDARD 11) #Requires CMake 3.1.3

# Find required dependencies
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(
  Boost 1.54.0 REQUIRED #1.54.0 or greater
  COMPONENTS
    filesystem
    system
)

# build compiler/parse
aux_source_directory(./parse PARSE_FILES)
add_library(parse_lib ${PARSE_FILES})

# build the base libraries
add_library(
  thrift_base

  common.h
  globals.h
  platform.h
  validator.h
  visitor.h

  common.cc
  validator.cc
  visitor.cc
)
target_link_libraries(
  thrift_base

  parse_lib
  -lcrypto
)

# build compiler/generate
aux_source_directory(./generate GENERATOR_FILES)
aux_source_directory(./generate/templates MSTCH_TEMPLATES)
add_library(
  generate_lib
  ${GENERATOR_FILES}
  ${MSTCH_TEMPLATES}
)
target_link_libraries(
  generate_lib
  ${Boost_LIBRARIES}
)
# Force generate_lib linking (compiler will optimize it out otherwise)
if(APPLE)
  set(GENERATE_LINKED -Wl,-force_load generate_lib)
elseif(UNIX)
  set(GENERATE_LINKED -Wl,--whole-archive generate_lib -Wl,--no-whole-archive)
endif(APPLE)

# Compile Flex and Bison files and tie their dependencies
BISON_TARGET(ThriftParser thrifty.yy ${CMAKE_BINARY_DIR}/thrifty.cc)
FLEX_TARGET(ThriftScanner thriftl.ll ${CMAKE_BINARY_DIR}/thriftl.cc)
ADD_FLEX_BISON_DEPENDENCY(ThriftScanner ThriftParser)

# Create the thrift compiler binary
add_executable(
  thrift

  main.cc
  ${BISON_ThriftParser_OUTPUTS}
  ${FLEX_ThriftScanner_OUTPUTS}
)
target_link_libraries(
  thrift

  parse_lib
  thrift_base
  ${GENERATE_LINKED}
  -lcrypto
  -lmstch
)
# Add the parser definition for global variables between lex, parse, main
target_compile_definitions(
  thrift

  PRIVATE -DTHRIFTY_HH=\"${CMAKE_BINARY_DIR}/thrifty.hh\"
)

# Set the binary destination
install(
  TARGETS thrift
  DESTINATION ${COMPILER_DIR}
)
