/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/filesystem.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <thrift/compiler/platform.h>

namespace apache {
namespace thrift {
namespace compiler {

boost::filesystem::path make_abs_path(
    const boost::filesystem::path& base_path,
    const boost::filesystem::path& path) {
  auto abs_path = path.is_absolute() ? path : base_path / path;
  if (platform_is_windows()) {
    // Handles long paths on windows.
    // https://www.boost.org/doc/libs/1_69_0/libs/filesystem/doc/reference.html#long-path-warning
    static constexpr auto kExtendedLengthPathPrefix = L"\\\\?\\";
    if (!boost::algorithm::starts_with(
            abs_path.wstring(), kExtendedLengthPathPrefix)) {
      auto native_path = abs_path.lexically_normal().make_preferred().wstring();
      // At this point the path may have a mix of '\\' and '\' separators.
      boost::algorithm::replace_all(native_path, L"\\\\", L"\\");
      abs_path = kExtendedLengthPathPrefix + native_path;
    }
  }
  return abs_path;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
