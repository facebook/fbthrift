# Set the lib directory
set(LIB_THRIFT_HOME ${CMAKE_CURRENT_SOURCE_DIR})

# Generate needed files
add_custom_target(lib-thrift ALL)

# Generate reflection files
add_custom_command(
  TARGET lib-thrift
  COMMAND ${THRIFT_BINARY}
  ARGS --gen cpp:templates,cob_style -r ${LIB_THRIFT_HOME}/reflection.thrift
  COMMENT "Generating reflection files"
)

# Generate frozen files
add_custom_command(
  TARGET lib-thrift
  COMMAND ${THRIFT_BINARY}
  ARGS --gen cpp:templates -r ${LIB_THRIFT_HOME}/frozen.thrift
  COMMENT "Generating frozen files"
)
