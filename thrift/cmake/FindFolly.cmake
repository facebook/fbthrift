# Copyright (c) Meta Platforms, Inc. and affiliates.
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

include(FindPackageHandleStandardArgs)

find_library(FOLLY_LIBRARY folly)
find_path(FOLLY_INCLUDE_DIR "folly/String.h")

set(FOLLY_LIBRARY ${FOLLY_LIBRARY})

find_package_handle_standard_args(Folly REQUIRED_ARGS FOLLY_INCLUDE_DIR
                                  FOLLY_LIBRARY)
