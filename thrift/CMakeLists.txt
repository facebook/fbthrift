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
