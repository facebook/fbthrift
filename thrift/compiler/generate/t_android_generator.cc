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

#include <thrift/compiler/generate/t_android_generator.h>

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <stdexcept>

#include <boost/filesystem.hpp>

using namespace std;

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 *
 * @param tprogram The program to generate
 */
void t_android_generator::init_generator() {
  // Make output directory
  boost::filesystem::create_directory(get_out_dir());
  namespace_key_ = "android";
  package_name_ = program_->get_namespace(namespace_key_);

  string dir = package_name_;
  string subdir = get_out_dir();
  string::size_type loc;
  while ((loc = dir.find('.')) != string::npos) {
    subdir = subdir + "/" + dir.substr(0, loc);
    boost::filesystem::create_directory(subdir);
    dir = dir.substr(loc + 1);
  }
  if (dir.size() > 0) {
    subdir = subdir + "/" + dir;
    boost::filesystem::create_directory(subdir);
  }

  package_dir_ = subdir;
}

THRIFT_REGISTER_GENERATOR(android, "Android Java", "");

} // namespace compiler
} // namespace thrift
} // namespace apache
