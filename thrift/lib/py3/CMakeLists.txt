# Create tree of symbolic links in structure required for successful
# compliation by Cython.
#   - must be in path named same as extension


# Arguments:
#   name of the extension - e.g. thrift
#   sources...


function (create_cybld_tree _extname _tre)
  set(_cybld "${CMAKE_CURRENT_BINARY_DIR}/cybld")
  file(MAKE_DIRECTORY "${_cybld}/${_tre}")
  foreach (_src ${ARGN})
    message(STATUS "Create Link to ${_src}")

    add_custom_target("create_folly_link_${_src}" ALL
      COMMAND
        ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_CURRENT_SOURCE_DIR}/${_src}
        "${_cybld}/${_tre}/${_src}"
    )

  endforeach ()
endfunction ()

add_subdirectory(test)

create_cybld_tree(folly folly
                        iobuf.pxd
                        iobuf.pyx
                        optional.pxd )

create_cybld_tree(thrift thrift/py3
                         client.pxd
                         common.pxd
                         common.pyx
                         std_libcpp.pxd
                         exceptions.pxd
                         exceptions.pyx
                         types.pxd
                         types.pyx
                         serializer.pxd
                         serializer.pyx)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cybld/thrift/py3/__init__.pxd)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cybld/thrift/__init__.pxd)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cybld/folly/__init__.py)

add_custom_target(thrift_python_bindings ALL
  COMMAND
   python3 ${CMAKE_CURRENT_SOURCE_DIR}/setup.py
    build_ext -f -L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    -I${THRIFT_HOME}:${THRIFT_BUILD}
  DEPENDS thriftcpp2
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cybld")

# Install Folly Python Bindings
install(CODE "
  string(REGEX REPLACE \"^(..*)$\" \"--root=\\\\1\" PYROOT \"\$ENV{DESTDIR}\")
  execute_process(COMMAND
  python3 ${CMAKE_CURRENT_SOURCE_DIR}/setup.py install \${PYROOT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cybld)")

