set(LIB_HOME ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(thrift)
add_subdirectory(cpp)
add_subdirectory(cpp2)
if (thriftpy3)
  add_subdirectory(py3)
endif ()

set(LIB_DIRS
  thrift
  cpp
  cpp2
)
foreach(dir ${LIB_DIRS})
  install(DIRECTORY ${dir} "${CMAKE_CURRENT_BINARY_DIR}/${dir}"
        DESTINATION include/thrift/lib
        FILES_MATCHING
          PATTERN "*.h"
          PATTERN "*.tcc"
          PATTERN CMakeFiles EXCLUDE)
endforeach()
