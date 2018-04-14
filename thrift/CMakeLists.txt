# Install thrift/compiler
if(compiler_only OR build_all)
  add_subdirectory(compiler)
endif(compiler_only OR build_all)

# Install thrift/lib
if(lib_only OR build_all)
  if(UNIX)
    add_subdirectory(lib)
  endif(UNIX)
endif(lib_only OR build_all)

# Install CMake package configuration files for thrift
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/FBThriftConfig.cmake.in
  FBThriftConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}
  PATH_VARS
    CMAKE_INSTALL_DIR
    BIN_INSTALL_DIR
    INCLUDE_INSTALL_DIR
    TEMPLATES_INSTALL_DIR
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/FBThriftConfig.cmake
  DESTINATION ${CMAKE_INSTALL_DIR}
)
install(
  EXPORT fbthrift-exports
  FILE FBThriftTargets.cmake
  NAMESPACE FBThrift::
  DESTINATION ${CMAKE_INSTALL_DIR}
)
