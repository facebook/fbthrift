add_library(
  concurrency-util

  thrift_config.h
  concurrency/Util.h

  concurrency/Util.cpp
)
target_link_libraries(
  concurrency-util

  ${FOLLY_LIBRARIES}
  ${LIBGLOG_LIBRARIES}
)
