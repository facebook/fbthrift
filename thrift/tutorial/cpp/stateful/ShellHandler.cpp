/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#define _XOPEN_SOURCE 600

#include "thrift/tutorial/cpp/stateful/ShellHandler.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <boost/noncopyable.hpp>

using namespace boost;
using namespace apache::thrift;

namespace {

class FdGuard : public boost::noncopyable {
 public:
  FdGuard(int fd) : fd_(fd) {}
  ~FdGuard() {
    if (fd_ >= 0) {
      close(fd_);
      // We ignore any error
    }
  }

  operator int() {
    return fd_;
  }

  void reset(int fd) {
    if (fd_ >= 0) {
      close(fd_);
    }
    fd_ = fd;
  }

  int release() {
    int tmp = fd_;
    fd_ = -1;
    return tmp;
  }

 protected:
  int fd_;
};

class DirGuard : public boost::noncopyable {
 public:
  DirGuard(DIR* d) : dir_(d) {}
  ~DirGuard() {
    if (dir_) {
      closedir(dir_);
    }
  }

  operator DIR*() {
    return dir_;
  }

  DIR* release() {
    DIR* tmp = dir_;
    dir_ = NULL;
    return tmp;
  }

 protected:
  DIR* dir_;
};

} // unnamed namespace

ShellHandler::ShellHandler(
    const std::shared_ptr<ServiceAuthState>& serviceAuthState,
    apache::thrift::server::TConnectionContext* ctx) :
    AuthHandler(serviceAuthState, ctx) {
  cwd_ = open(".", O_RDONLY);
  if (cwd_ < 0) {
    GlobalOutput.printf("failed to open current directory: %s (%d)",
                        strerror(errno), errno);
    throwErrno("failed to open current directory");
  }
}

ShellHandler::~ShellHandler() {
  if (cwd_ >= 0) {
    close(cwd_);
  }
}

void ShellHandler::pwd(std::string& _return) {
  validateState();

  // TODO: this only works on linux
  char procPath[1024];
  snprintf(procPath, sizeof(procPath), "/proc/self/fd/%d", cwd_);

  char cwdPath[1024];
  ssize_t numBytes = readlink(procPath, cwdPath, sizeof(cwdPath));
  if (numBytes < 0) {
    throwErrno("failed to determine current working directory");
  }

  _return.assign(cwdPath, numBytes);
}

void ShellHandler::chdir(const std::string& dir) {
  validateState();

  FdGuard newCwd(openat(cwd_, dir.c_str(), O_RDONLY));
  if (newCwd < 0) {
    throwErrno("unable to change directory");
  }

  close(cwd_);
  cwd_ = newCwd.release();
}

void ShellHandler::listDirectory(std::vector<StatInfo> &_return,
                                 const std::string& dir) {
  validateState();

  FdGuard fd(openat(cwd_, dir.c_str(), O_RDONLY));
  if (fd < 0) {
    throwErrno("failed to open directory");
  }

  DirGuard d(fdopendir(fd));
  if (!d) {
    throwErrno("failed to open directory handle");
  }
  fd.release();

  while (true) {
    errno = 0;
    struct dirent* ent = readdir(d);
    if (ent == NULL) {
      if (errno != 0) {
        throwErrno("readdir() failed");
      }
      break;
    }

    StatInfo si;
    si.name = ent->d_name;
    _return.push_back(si);
  }
}

void ShellHandler::cat(std::string& _return, const std::string& file) {
  validateState();

  FdGuard fd(openat(cwd_, file.c_str(), O_RDONLY));
  if (fd < 0) {
    throwErrno("unable to open file");
  }

  // Resize the output string to the expected size,
  // to make it more efficient for larger files.
  struct stat s;
  if (fstat(fd, &s) != 0) {
    throwErrno("unable to stat() file");
  }
  _return.resize(s.st_size);

  char buf[4096];
  while (true) {
    ssize_t bytesRead = read(fd, buf, sizeof(buf));
    if (bytesRead == 0) {
      break;
    } else if (bytesRead < 0) {
      throwErrno("read failed");
    }

    _return.append(buf, bytesRead);
  }
}

void
ShellHandler::validateState() {
  // The user must be logged in
  if (!hasAuthenticated()) {
    PermissionError error;
    error.message = "must log in first";
    throw error;
  }
}

void
ShellHandler::throwErrno(const char* msg) {
  OSError error;
  error.code = errno;

  error.message = msg;
  error.message += ": ";
  error.message += strerror(errno);

  throw error;
}
