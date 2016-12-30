/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/platform.h>

#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#  include <direct.h>
#  include <io.h>
#endif

int make_dir(const char *path) {
  #ifdef _WIN32
    return _mkdir(path);
  #else
    return mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
  #endif
}


int chmod_to_755(const char *path) {
  #ifdef _WIN32
    return _chmod(path, _S_IREAD | _S_IWRITE);
  #else
    return chmod(
      path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH
    );
  #endif
}
