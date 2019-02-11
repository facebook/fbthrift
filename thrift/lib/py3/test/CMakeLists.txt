include_directories("${CMAKE_CURRENT_BINARY_DIR}/..")

thrift_library(
    "iobuf"
    "IobufTestService"
    "cpp2"
    ""
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "test"
)

thrift_generate(
    "iobuf"
    "IobufTestService"
    "py3"
    ""
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "test"
)


file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen-py3/test")

add_custom_target("create_iobuf_testlink" ALL
  COMMAND
    ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/iobuf_helper.pyx
    "${CMAKE_CURRENT_BINARY_DIR}/gen-py3/test/iobuf_helper.pyx"
)

add_custom_target("create_iobuf_testlink2" ALL
  COMMAND
    ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/iobuf.py
    "${CMAKE_CURRENT_BINARY_DIR}/gen-py3/test/iobuf.py"
)



#Next need to do the following inside gen-py3/ ...
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/gen-py3/iobuf/__init__.py)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/gen-py3/test/__init__.py)

add_custom_target(iobuf_python_unittest ALL
  COMMAND
    python3 ${CMAKE_CURRENT_SOURCE_DIR}/setup.py
    build_ext -i -f -L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    -I${CMAKE_SOURCE_DIR}:${CMAKE_CURRENT_BINARY_DIR}/..
  DEPENDS thriftcpp2
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen-py3")
