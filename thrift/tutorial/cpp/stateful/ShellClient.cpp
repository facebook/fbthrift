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

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include "thrift/tutorial/cpp/stateful/gen-cpp2/ShellService.h"

#include <iostream>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::tutorial::stateful;

class LineTooLongError : public exception {
 public:
  const char* what() const throw() override { return "input line is too long"; }
};

class CommandError : public exception {
 public:
  explicit CommandError(const string& msg) : msg_(msg) {}
  ~CommandError() throw() override {}

  const char* what() const throw() override { return msg_.c_str(); }

 protected:
  string msg_;
};

class UnknownCommandError : public CommandError {
 public:
  explicit UnknownCommandError(const string& cmd) :
      CommandError("unknown command \"" + cmd + "\""),
      cmd_(cmd) {}
  ~UnknownCommandError() throw() override {}

 protected:
  string cmd_;
};

class UsageError : public CommandError {
 public:
  explicit UsageError(const string& msg) : CommandError(msg) {}
};

bool readNextCommand(vector<string>* cmd) {
  // Display a prompt
  cout << "$ ";
  cout.flush();

  // Read a line
  char buf[1024];
  buf[sizeof(buf) - 2] = '\n';
  buf[sizeof(buf) - 1] = '\0';
  if (fgets(buf, sizeof(buf), stdin) == nullptr) {
    // We can't distinguish between I/O error and EOF,
    // but it shouldn't matter much.
    //
    // Write a newline to end the line our prompt was on.
    cout << endl;
    return false;
  }

  // If something that wasn't a newline overwrite the last character,
  // the input line was longer than our buffer.  Just fail.
  if (buf[sizeof(buf) - 2] != '\n') {
    throw LineTooLongError();
  }

  // Simple tokenization, just based on whitespace
  char* ptr = buf;
  while (true) {
    // Skip over any leading whitespace
    while (isspace(*ptr)) {
      ++ptr;
    }

    // Stop if we're at the end of the buffer
    if (*ptr == '\0') {
      break;
    }

    // Skip to the next whitespace
    char* end = ptr;
    while (*end != '\0' && !isspace(*end)) {
      ++end;
    }

    cmd->push_back(string(ptr, end - ptr));
    ptr = end;
  }

  return true;
}

void checkNumArgs(int argc, int min, int max = -1) {
  int numArgs = argc - 1;
  if (max < 0) {
    if (numArgs != min) {
      throw UsageError("exactly 1 argument must be specified");
    }
  } else {
    if (numArgs < min) {
      throw UsageError("not enough arguments");
    } else if (numArgs > max) {
      throw UsageError("too many arguments");
    }
  }
}

void runCommand(ShellServiceAsyncClient* client, const vector<string>& cmd) {
  // Ignore empty lines
  if (cmd.empty()) {
      return;
  }

  int argc = cmd.size();
  if (cmd[0] == "login") {
    checkNumArgs(argc, 1);
    client->sync_authenticate(cmd[1]);
    cout << "Logged in as \"" << cmd[1] << "\"" << endl;
  } else if (cmd[0] == "ls") {
    checkNumArgs(argc, 0, 1);
    vector<StatInfo> files;
    if (argc == 2) {
      client->sync_listDirectory(files, cmd[1]);
    } else {
      client->sync_listDirectory(files, ".");
    }
    for (vector<StatInfo>::const_iterator it = files.begin();
         it != files.end();
         ++it) {
      cout << it->name << endl;
    }
  } else if (cmd[0] == "pwd") {
    string pwd;
    client->sync_pwd(pwd);
    cout << pwd << endl;
  } else if (cmd[0] == "cd") {
    checkNumArgs(argc, 1);
    client->sync_chdir(cmd[1]);
  } else if (cmd[0] == "cat") {
    checkNumArgs(argc, 1);
    string data;
    client->sync_cat(data, cmd[1]);
    cout << data;
  } else if (cmd[0] == "help") {
    cout <<
      "login" << endl <<
      "ls" << endl <<
      "pwd" << endl <<
      "cd" << endl <<
      "cat" << endl <<
      "help" << endl;
  } else {
    throw UnknownCommandError(cmd[0]);
  }
}

void run(const string& host, uint16_t port) {
  EventBase eb;
  auto client = make_unique<ShellServiceAsyncClient>(
      HeaderClientChannel::newChannel(
        async::TAsyncSocket::newSocket(&eb, {host, port})));

  while (true) {
    try {
      vector<string> cmd;
      if (!readNextCommand(&cmd)) {
        break;
      }
      runCommand(client.get(), cmd);
    } catch (const CommandError& x) {
      LOG(ERROR) << x.what();
    } catch (const LoginError& x) {
      LOG(ERROR) << x.message;
    } catch (const PermissionError& x) {
      LOG(ERROR) << x.message;
    } catch (const OSError& x) {
      LOG(ERROR) << x.message << " (" << x.code << ")";
    }
  }
}

int main() {
  string host = "127.0.0.1";
  uint16_t port = 12345;

  try {
    run(host, port);
    return 0;
  } catch (const exception& x) {
    LOG(CRITICAL) << "unhandled " << typeid(x).name() << " exception: " <<
      x.what();
  } catch (...) {
    LOG(CRITICAL) << "unhandled exception caught in main()";
  }

  return 1;
}
