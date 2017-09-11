add_subdirectory(thrift)
add_subdirectory(cpp)
add_subdirectory(cpp2)

set(LIB_DIRS
  thrift
  cpp
  cpp2
)
foreach(dir ${LIB_DIRS})
  install(DIRECTORY ${dir} DESTINATION include/thrift/lib
          FILES_MATCHING PATTERN "*.h")
endforeach()
