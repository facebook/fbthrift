# Set the compiler directory
set(COMPILER_HOME ${CMAKE_CURRENT_SOURCE_DIR})

# Override default install path for `make install` step
set(CMAKE_INSTALL_PREFIX ${THRIFT_HOME})

# build compiler/parse
aux_source_directory(./parse PARSE_FILES)
add_library(parse_lib ${PARSE_FILES})
target_link_libraries(parse_lib OpenSSL::Crypto)

# build the base libraries
add_library(
  thrift_base

  common.h
  globals.h
  platform.h
  validator.h
  visitor.h

  common.cc
  platform.cc
  validator.cc
  visitor.cc
)
target_link_libraries(thrift_base parse_lib)

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

  mstch
  OpenSSL::Crypto
  ${Boost_LIBRARIES}
)
# Force generate_lib linking (compiler will optimize it out otherwise)
if(APPLE)
  set(GENERATE_LINKED -Wl,-force_load generate_lib)
else()
  set(GENERATE_LINKED -Wl,--whole-archive generate_lib -Wl,--no-whole-archive)
endif(APPLE)

# Flex generates functions that have a different naming convention in
# gcc and msvc (i.e. isatty(...) and _isatty(...)). This flag makes flex use
# the right functions for every platform.
if(WIN32)
  set(FLEXFLAGS "--wincompat")
endif(WIN32)
# Compile Flex and Bison files and tie their dependencies
BISON_TARGET(ThriftParser thrifty.yy ${COMPILER_HOME}/thrifty.cc)
FLEX_TARGET(
  ThriftScanner

  thriftl.ll
  ${COMPILER_HOME}/thriftl.cc
  COMPILE_FLAGS ${FLEXFLAGS}
)
ADD_FLEX_BISON_DEPENDENCY(ThriftScanner ThriftParser)

# Create the thrift compiler binary
add_executable(
  thrift-compiler

  main.cc
  ${BISON_ThriftParser_OUTPUTS}
  ${FLEX_ThriftScanner_OUTPUTS}
)
target_link_libraries(
  thrift-compiler

  parse_lib
  thrift_base
  ${GENERATE_LINKED}
)
# Add the parser definition for global variables between lex, parse, main
target_compile_definitions(
  thrift-compiler

  PRIVATE -DTHRIFTY_HH="${COMPILER_HOME}/thrifty.hh"
)

# Set the binary destination
install(
  TARGETS thrift-compiler
  DESTINATION .
)
