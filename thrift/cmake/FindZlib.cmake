# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# - Try to find Facebook zstd library
# This will define
# ZLIB_FOUND
# ZLIB_INCLUDE_DIR
# ZLIB_LIBRARIES
#

find_path(
  ZLIB_INCLUDE_DIRS zlib.h
  HINTS
      $ENV{ZLIB_ROOT}/include
      ${ZLIB_ROOT}/include
)

find_library(
    ZLIB_LIBRARIES z zlib
    HINTS
        $ENV{ZLIB_ROOT}/lib
        ${ZLIB_ROOT}/lib
)

# For some reason ZLIB_FOUND is never marked as TRUE
set(ZLIB_FOUND TRUE)
mark_as_advanced(ZLIB_INCLUDE_DIRS ZLIB_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zstd ZLIB_INCLUDE_DIRS ZLIB_LIBRARIES)

if(ZLIB_FOUND AND NOT ZLIB_FIND_QUIETLY)
    message(STATUS "ZLIB: ${ZLIB_INCLUDE_DIRS}")
endif()
