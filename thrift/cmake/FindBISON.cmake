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

# A wrapper around the standard CMake FindBISON module.

if (APPLE)
  # Prefer the Homebrew version of bison if available because the default one
  # installed on macOS (2.3 in Catalina) is incompatible with Thrift.
  file(GLOB BISON_PATH /usr/local/Cellar/bison/3.*/bin)
  find_program(BISON_EXECUTABLE bison PATHS ${BISON_PATH} NO_DEFAULT_PATH)
endif ()

set(saved_path ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_STD_MODULE_PATH})
find_package(BISON ${ARGN})
set(CMAKE_MODULE_PATH ${saved_path})
