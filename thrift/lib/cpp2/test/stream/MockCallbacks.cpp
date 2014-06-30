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

#include "MockCallbacks.h"

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

#include <utility>
#include <time.h>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::test::cpp2;

void DummyOutputCallback::onSend() noexcept {}
void DummyOutputCallback::onClose() noexcept {}
void DummyOutputCallback::onError(const std::exception_ptr& ex) noexcept {}
DummyOutputCallback::~DummyOutputCallback() {}

OutputCallback::OutputCallback(int limit)
    : batchSize_(1),
      next_(0),
      limit_(limit),
      closed_(false) {};

void OutputCallback::onSend() noexcept {
  for (int i = 0; i < batchSize_; ++i) {
    if (next_ == limit_) {
      close();
      break;
    } else {
      put(next_ * 100 + 10);
      ++next_;
    }
  }
  ++batchSize_;
}

void OutputCallback::onClose() noexcept {
  EXPECT_FALSE(closed_);
  EXPECT_EQ(limit_, next_);
  closed_ = true;
}

void OutputCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

OutputCallback::~OutputCallback() {
  EXPECT_TRUE(closed_);
}

OutputException::OutputException(int limit)
    : batchSize_(1),
      next_(0),
      limit_(limit),
      threw_(false),
      closed_(false) {};

void OutputException::onSend() noexcept {
  EXPECT_FALSE(closed_);

  for (int i = 0; i < batchSize_; ++i) {
    if (next_ == limit_) {
      SimpleException exception;
      exception.msg = "Simple Exception";
      threw_ = true;
      putException(std::make_exception_ptr(exception));
      break;
    } else {
      put(next_ * 100 + 10);
      ++next_;
    }
  }
  ++batchSize_;
}

void OutputException::onClose() noexcept {
  EXPECT_TRUE(threw_);
  EXPECT_FALSE(closed_);
  EXPECT_EQ(limit_, next_);
  closed_ = true;
}

void OutputException::onError(const std::exception_ptr& ep) noexcept {
  ADD_FAILURE();
}

OutputException::~OutputException() {
  EXPECT_TRUE(closed_);
}

OutputError::OutputError(int limit)
    : batchSize_(1),
      next_(0),
      limit_(limit),
      errored_(false),
      closed_(false),
      manager_(nullptr) {};

void OutputError::onSend() noexcept {
  EXPECT_FALSE(closed_);
  EXPECT_FALSE(errored_);

  for (int i = 0; i < batchSize_; ++i) {
    if (next_ == limit_) {
      CHECK_NOTNULL(manager_);
      manager_->notifyError(std::make_exception_ptr(TestException()));
      break;
    } else {
      put(next_ * 100 + 10);
      ++next_;
    }
  }
  ++batchSize_;
}

void OutputError::onClose() noexcept {
  EXPECT_TRUE(errored_);
  EXPECT_FALSE(closed_);
  EXPECT_EQ(limit_, next_);
  closed_ = true;
}

void OutputError::onError(const std::exception_ptr& ep) noexcept {
  errored_ = true;
  EXPECT_THROW(std::rethrow_exception(ep), TestException);
}

void OutputError::setStreamManager(StreamManager* manager) {
  manager_ = manager;
}

OutputError::~OutputError() {
  EXPECT_TRUE(closed_);
}

OutputReceiveError::OutputReceiveError()
    : batchSize_(1),
      next_(0),
      errored_(false),
      closed_(false) {};

void OutputReceiveError::onSend() noexcept {
  EXPECT_FALSE(errored_);
  EXPECT_FALSE(closed_);
  for (int i = 0; i < batchSize_; ++i) {
    put(next_ * 100 + 10);
    ++next_;
  }
  ++batchSize_;
}

void OutputReceiveError::onClose() noexcept {
  EXPECT_TRUE(errored_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void OutputReceiveError::onError(const std::exception_ptr& ep) noexcept {
  errored_ = true;
  EXPECT_THROW(std::rethrow_exception(ep), TestException);
}

OutputReceiveError::~OutputReceiveError() {
  EXPECT_TRUE(closed_);
}

OutputReceiveErrorMsg::OutputReceiveErrorMsg(const std::string& message)
    : batchSize_(1),
      next_(0),
      errored_(false),
      closed_(false),
      message_(message) {};

void OutputReceiveErrorMsg::onSend() noexcept {
  EXPECT_FALSE(errored_);
  EXPECT_FALSE(closed_);
  for (int i = 0; i < batchSize_; ++i) {
    put(next_ * 100 + 10);
    ++next_;
  }
  ++batchSize_;
}

void OutputReceiveErrorMsg::onClose() noexcept {
  EXPECT_TRUE(errored_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void OutputReceiveErrorMsg::onError(const std::exception_ptr& ep) noexcept {
  errored_ = true;
  try {
    std::rethrow_exception(ep);
  } catch (const std::exception& e) {
    EXPECT_EQ(e.what(), message_);
  } catch (...) {
    EXPECT_TRUE(false);
  }
}

OutputReceiveErrorMsg::~OutputReceiveErrorMsg() {
  EXPECT_TRUE(closed_);
}

OutputReceiveErrorCheck::OutputReceiveErrorCheck(const Checker& checker)
    : batchSize_(1),
      next_(0),
      errored_(false),
      closed_(false),
      checker_(checker) {};

void OutputReceiveErrorCheck::onSend() noexcept {
  EXPECT_FALSE(errored_);
  EXPECT_FALSE(closed_);
  for (int i = 0; i < batchSize_; ++i) {
    put(next_ * 100 + 10);
    ++next_;
  }
  ++batchSize_;
}

void OutputReceiveErrorCheck::onClose() noexcept {
  EXPECT_TRUE(errored_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void OutputReceiveErrorCheck::onError(const std::exception_ptr& ep) noexcept {
  errored_ = true;
  try {
    std::rethrow_exception(ep);
  } catch (const apache::thrift::transport::TTransportException& e) {
    EXPECT_TRUE(checker_(e))<< e.what();
  } catch (...) {
    EXPECT_TRUE(false);
  }
}

OutputReceiveErrorCheck::~OutputReceiveErrorCheck() {
  EXPECT_TRUE(closed_);
}

OutputTimeoutCheck::OutputTimeoutCheck(long sleepTime, const Checker& checker)
    : errored_(false),
      closed_(false),
      sleepTime_(sleepTime),
      checker_(checker) {};

void OutputTimeoutCheck::onSend() noexcept {
  EXPECT_FALSE(errored_);
  EXPECT_FALSE(closed_);

  time_t seconds = sleepTime_ / 1000;
  long nanoseconds = (sleepTime_ % 1000) * 1000000;

  timespec sleeptime = {seconds, nanoseconds};
  nanosleep(&sleeptime, nullptr);
}

void OutputTimeoutCheck::onClose() noexcept {
  EXPECT_TRUE(errored_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void OutputTimeoutCheck::onError(const std::exception_ptr& ep) noexcept {
  errored_ = true;
  try {
    std::rethrow_exception(ep);
  } catch (const apache::thrift::transport::TTransportException& e) {
    EXPECT_TRUE(checker_(e)) << e.what();
  } catch (...) {
    EXPECT_TRUE(false);
  }
}

OutputTimeoutCheck::~OutputTimeoutCheck() {
  EXPECT_TRUE(closed_);
}

OutputReceiveClose::OutputReceiveClose(int limit)
    : batchSize_(1),
      limit_(limit),
      next_(0),
      closed_(false) {};

void OutputReceiveClose::onSend() noexcept {
  EXPECT_FALSE(closed_);

  for (int i = 0; i < batchSize_; ++i) {
    if (next_ > limit_) {
      close();
    } else {
      put(next_ * 100 + 10);
    }
    ++next_;
  }
  ++batchSize_;
}

void OutputReceiveClose::onClose() noexcept {
  EXPECT_LE(next_, limit_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void OutputReceiveClose::onError(const std::exception_ptr& ep) noexcept {
  ADD_FAILURE();
}

OutputReceiveClose::~OutputReceiveClose() {
  EXPECT_TRUE(closed_);
}

OutputIgnore::OutputIgnore(int limit)
    : batchSize_(1),
      limit_(limit),
      next_(0),
      closed_(false) {};

void OutputIgnore::onSend() noexcept {
  EXPECT_FALSE(closed_);

  for (int i = 0; i < batchSize_; ++i) {
    put(next_ * 100 + 10);
    ++next_;
  }

  ++batchSize_;
}

void OutputIgnore::onClose() noexcept {
  EXPECT_FALSE(closed_);
  EXPECT_LE(next_, limit_);
  closed_ = true;
}

void OutputIgnore::onError(const std::exception_ptr& ep) noexcept {
  EXPECT_FALSE(true);
}

OutputIgnore::~OutputIgnore() {
  EXPECT_TRUE(closed_);
}

OutputCoopCallback::OutputCoopCallback(int limit)
    : batchSize_(1),
      next_(0),
      limit_(limit),
      closed_(false) {};

void OutputCoopCallback::onSend() noexcept {
  for (int i = 0; i < batchSize_; ++i) {
    if (next_ == limit_) {
      ADD_FAILURE();
      close();
    } else {
      put(next_ + 100);
      ++next_;
    }
  }
  ++batchSize_;
}

void OutputCoopCallback::onClose() noexcept {
  EXPECT_FALSE(closed_);
  EXPECT_LE(next_, limit_);
  closed_ = true;
}

void OutputCoopCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

OutputCoopCallback::~OutputCoopCallback() {
  EXPECT_TRUE(closed_);
}

OutputStringCallback::OutputStringCallback(
    const std::vector<std::string>& strings)
    : strings_(strings),
      index_(0),
      batchSize_(1),
      finished_(false),
      closed_(false) {};

void OutputStringCallback::onSend() noexcept {
  EXPECT_FALSE(finished_);

  for (int i = 0; i < batchSize_; ++i) {
    if (index_ == strings_.size()) {
      close();
      finished_ = true;
      break;
    } else if (index_ < strings_.size()) {
      put(strings_[index_]);
      ++index_;
    }
  }
  ++batchSize_;
}

void OutputStringCallback::onClose() noexcept {
  EXPECT_FALSE(closed_);
  EXPECT_EQ(index_, strings_.size());
  closed_ = true;
}

void OutputStringCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

OutputStringCallback::~OutputStringCallback() {
  EXPECT_TRUE(closed_);
}

void DummyInputCallback::onReceive(int& value) noexcept {}
void DummyInputCallback::onFinish() noexcept {}
void DummyInputCallback::onException(const std::exception_ptr& ex) noexcept {}
void DummyInputCallback::onError(const std::exception_ptr& ex) noexcept {}
void DummyInputCallback::onClose() noexcept {}
DummyInputCallback::~DummyInputCallback() {}

InputCallback::InputCallback(int limit)
    : next_(0), limit_(limit), finished_(false), closed_(false) {};

void InputCallback::onReceive(int& value) noexcept {
  EXPECT_EQ(next_ * 100 + 10, value);
  ++next_;
}

void InputCallback::onFinish() noexcept {
  EXPECT_EQ(limit_, next_);
  finished_ = true;
}

void InputCallback::onClose() noexcept {
  EXPECT_TRUE(finished_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void InputCallback::onException(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

void InputCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

InputCallback::~InputCallback() {
  EXPECT_TRUE(closed_);
}

InputReceiveException::InputReceiveException(int limit)
    : next_(0), limit_(limit), caught_(false), closed_(false) {};

void InputReceiveException::onReceive(int& value) noexcept {
  EXPECT_EQ(next_ * 100 + 10, value);
  ++next_;
}

void InputReceiveException::onFinish() noexcept {
  ADD_FAILURE();
}

void InputReceiveException::onClose() noexcept {
  EXPECT_TRUE(caught_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void InputReceiveException::onException(const std::exception_ptr& ep) noexcept {
  EXPECT_EQ(limit_, next_);

  try {
    std::rethrow_exception(ep);
  } catch (const SimpleException& exception) {
    EXPECT_EQ("Simple Exception", exception.msg);
  } catch (...) {
    ADD_FAILURE();
  }

  caught_ = true;
}

void InputReceiveException::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

InputReceiveException::~InputReceiveException() {
  EXPECT_TRUE(closed_);
}

InputError::InputError(int limit)
    : next_(0),
      limit_(limit),
      finished_(false),
      errored_(false),
      closed_(false),
      manager_(nullptr) {
};

void InputError::onReceive(int& value) noexcept {
  EXPECT_LE(next_, limit_);

  if (next_ >= limit_) {
    CHECK_NOTNULL(manager_);
    manager_->notifyError(std::make_exception_ptr(TestException()));
  } else {
    EXPECT_EQ(next_ * 100 + 10, value);
    ++next_;
  }
}

void InputError::onFinish() noexcept {
  finished_ = true;
}

void InputError::onClose() noexcept {
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void InputError::onException(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

void InputError::onError(const std::exception_ptr& ep) noexcept {
  errored_ = true;
  EXPECT_THROW(std::rethrow_exception(ep), TestException);
}

void InputError::setStreamManager(StreamManager* manager) {
  manager_ = manager;
}

InputError::~InputError() {
  EXPECT_FALSE(finished_);
  EXPECT_TRUE(closed_);
}

InputClose::InputClose(int limit)
    : next_(0), limit_(limit), finished_(false), closed_(false) {};

void InputClose::onReceive(int& value) noexcept {
  EXPECT_LE(next_, limit_);

  if (next_ >= limit_) {
    close();
  } else {
    EXPECT_EQ(next_ * 100 + 10, value);
    ++next_;
  }
}

void InputClose::onFinish() noexcept {
  finished_ = true;
}

void InputClose::onClose() noexcept {
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void InputClose::onException(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

void InputClose::onError(const std::exception_ptr& error) noexcept {
  ADD_FAILURE();
}

InputClose::~InputClose() {
  EXPECT_FALSE(finished_);
  EXPECT_TRUE(closed_);
}

InputReceiveError::InputReceiveError()
    : next_(0),
      errored_(false),
      closed_(false) {};

void InputReceiveError::onReceive(int& value) noexcept {
  EXPECT_EQ(value, next_ * 100 + 10);
  ++next_;
}

void InputReceiveError::onFinish() noexcept {
  EXPECT_TRUE(false);
}

void InputReceiveError::onException(const std::exception_ptr& ep) noexcept {
  EXPECT_TRUE(false);
}

void InputReceiveError::onError(const std::exception_ptr& ep) noexcept {
  EXPECT_THROW(std::rethrow_exception(ep), TestException);
  errored_ = true;
}

void InputReceiveError::onClose() noexcept {
  EXPECT_TRUE(errored_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

InputReceiveError::~InputReceiveError() {
  EXPECT_TRUE(closed_);
}

InputReceiveErrorMsg::InputReceiveErrorMsg(const std::string& message)
    : next_(0),
      errored_(false),
      closed_(false),
      message_(message) {};

void InputReceiveErrorMsg::onReceive(int& value) noexcept {
  EXPECT_EQ(value, next_ * 100 + 10);
  ++next_;
}

void InputReceiveErrorMsg::onFinish() noexcept {
  EXPECT_TRUE(false);
}

void InputReceiveErrorMsg::onException(const std::exception_ptr& ep) noexcept {
  EXPECT_TRUE(false);
}

void InputReceiveErrorMsg::onError(const std::exception_ptr& ep) noexcept {
  try {
    std::rethrow_exception(ep);
  } catch (const std::exception& exception) {
    EXPECT_EQ(exception.what(), message_);
  } catch (...) {
    EXPECT_TRUE(false);
  }

  errored_ = true;
}

void InputReceiveErrorMsg::onClose() noexcept {
  EXPECT_TRUE(errored_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

InputReceiveErrorMsg::~InputReceiveErrorMsg() {
  EXPECT_TRUE(closed_);
}

InputReceiveErrorCheck::InputReceiveErrorCheck(const Checker& checker)
    : next_(0),
      errored_(false),
      closed_(false),
      checker_(checker) {};

void InputReceiveErrorCheck::onReceive(int& value) noexcept {
  EXPECT_EQ(value, next_ * 100 + 10);
  ++next_;
}

void InputReceiveErrorCheck::onFinish() noexcept {
  EXPECT_TRUE(false);
}

void InputReceiveErrorCheck::onException(const std::exception_ptr& ep) noexcept{
  EXPECT_TRUE(false);
}

void InputReceiveErrorCheck::onError(const std::exception_ptr& ep) noexcept {
  try {
    std::rethrow_exception(ep);
  } catch (const apache::thrift::transport::TTransportException& e) {
    EXPECT_TRUE(checker_(e)) << e.what();
  } catch (...) {
    EXPECT_TRUE(false);
  }

  errored_ = true;
}

void InputReceiveErrorCheck::onClose() noexcept {
  EXPECT_TRUE(errored_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

InputReceiveErrorCheck::~InputReceiveErrorCheck() {
  EXPECT_TRUE(closed_);
}

InputIgnore::InputIgnore(int limit)
    : next_(0),
      limit_(limit),
      finished_(false),
      closed_(false) {};

void InputIgnore::onReceive(int& value) noexcept {
  EXPECT_EQ(value, next_ * 100 + 10);
  ++next_;
}

void InputIgnore::onFinish() noexcept {
  EXPECT_FALSE(finished_);
  finished_ = true;
}

void InputIgnore::onException(const std::exception_ptr& ep) noexcept {
  EXPECT_TRUE(false);
}

void InputIgnore::onError(const std::exception_ptr& ep) noexcept {
  EXPECT_TRUE(false);
}

void InputIgnore::onClose() noexcept {
  EXPECT_LE(next_, limit_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

InputIgnore::~InputIgnore() {
  EXPECT_TRUE(closed_);
}

InputReceiveClose::InputReceiveClose(int limit)
    : next_(0), limit_(limit), closed_(false) {};

void InputReceiveClose::onReceive(int& value) noexcept {
  EXPECT_EQ(next_ * 100 + 10, value);
  ++next_;
}

void InputReceiveClose::onFinish() noexcept {
  ADD_FAILURE();
}

void InputReceiveClose::onClose() noexcept {
  EXPECT_FALSE(closed_);
  EXPECT_LE(next_, limit_);
  closed_ = true;
}

void InputReceiveClose::onException(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

void InputReceiveClose::onError(const std::exception_ptr& error) noexcept {
  ADD_FAILURE();
}

InputReceiveClose::~InputReceiveClose() {
  EXPECT_TRUE(closed_);
}

InputCoopCallback::InputCoopCallback(
    int limit,
    const std::shared_ptr<OutputStreamController<int>>& controller)
    : batchSize_(1),
      nextMyReceive_(0),
      nextOutputReceive_(0),
      myNextOutput_(0),
      limit_(limit),
      finished_(false),
      closed_(false),
      controller_(controller) {};

void InputCoopCallback::onReceive(int& value) noexcept {
  if (value == nextMyReceive_ + 10000) {
    ++nextMyReceive_;

    if (nextMyReceive_ ==  myNextOutput_ &&
        myNextOutput_ < limit_) {
      for (int i = 0; i < batchSize_; ++i) {
        if (myNextOutput_ == limit_) {
          break;
        } else {
          controller_->put(myNextOutput_ + 10000);
          ++myNextOutput_;
        }
      }
      ++batchSize_;

      if (myNextOutput_ == limit_) {
        controller_->close();
      }
    }
  } else if (value == nextOutputReceive_ + 100) {
    if (nextOutputReceive_ == 0) {
      controller_->put(myNextOutput_ + 10000);
      ++myNextOutput_;
      ++batchSize_;
    }

    ++nextOutputReceive_;
  } else {
    ADD_FAILURE();
  }
}

void InputCoopCallback::onFinish() noexcept {
  finished_ = true;
}

void InputCoopCallback::onClose() noexcept {
  EXPECT_TRUE(finished_);
  EXPECT_FALSE(closed_);
  EXPECT_EQ(myNextOutput_, limit_);
  EXPECT_EQ(nextMyReceive_, myNextOutput_);
  EXPECT_GT(nextOutputReceive_, 0);

  closed_ = true;
}

void InputCoopCallback::onException(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

void InputCoopCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

InputCoopCallback::~InputCoopCallback() {
  EXPECT_TRUE(closed_);
}

InputSumCallback::InputSumCallback(
    const std::shared_ptr<OutputStreamController<int>>& controller)
    : sum_(0),
      finished_(false),
      closed_(false),
      controller_(controller) {};

void InputSumCallback::onReceive(int& value) noexcept {
  sum_ += value;
}

void InputSumCallback::onFinish() noexcept {
  finished_ = true;
  controller_->put(sum_);
  controller_->close();
}

void InputSumCallback::onClose() noexcept {
  EXPECT_TRUE(finished_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void InputSumCallback::onException(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

void InputSumCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

InputSumCallback::~InputSumCallback() {
  EXPECT_TRUE(closed_);
}

InputSingletonCallback::InputSingletonCallback(int expected)
    : expected_(expected),
      received_(false),
      finished_(false),
      closed_(false) {};

void InputSingletonCallback::onReceive(int& value) noexcept {
  EXPECT_FALSE(received_);
  EXPECT_EQ(expected_, value);
  received_ = true;
}

void InputSingletonCallback::onFinish() noexcept {
  EXPECT_TRUE(received_);
  EXPECT_FALSE(finished_);
  finished_ = true;
}

void InputSingletonCallback::onClose() noexcept {
  EXPECT_TRUE(finished_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void InputSingletonCallback::onException(const std::exception_ptr& ex) noexcept{
  ADD_FAILURE();
}

void InputSingletonCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

InputSingletonCallback::~InputSingletonCallback() {
  EXPECT_TRUE(closed_);
}

InputStringCallback::InputStringCallback(
    const std::vector<std::string>& strings)
    : strings_(strings),
      index_(0),
      finished_(false),
      closed_(false) {};

void InputStringCallback::onReceive(std::string& value) noexcept {
  if (index_ < strings_.size()) {
    EXPECT_EQ(strings_[index_], value);
  }
  ++index_;
}

void InputStringCallback::onFinish() noexcept {
  EXPECT_EQ(index_, strings_.size());
  EXPECT_FALSE(finished_);
  finished_ = true;
}

void InputStringCallback::onClose() noexcept {
  EXPECT_TRUE(finished_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void InputStringCallback::onException(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

void InputStringCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

InputStringCallback::~InputStringCallback() {
  EXPECT_TRUE(closed_);
}

InputStringListCallback::InputStringListCallback(
      const std::shared_ptr<OutputStreamController<
          std::vector<apache::thrift::test::cpp2::Subject>>>& controller)
    : finished_(false),
      closed_(false),
      controller_(controller) {};

void InputStringListCallback::onReceive(std::vector<std::string>& strings)
    noexcept {
  EXPECT_FALSE(finished_);
  EXPECT_FALSE(closed_);

  std::vector<Subject> subjects(strings.size());
  for (uint32_t i = 0; i < subjects.size(); ++i) {
    subjects[i].will.be = strings[i];
  }

  controller_->put(subjects);
}

void InputStringListCallback::onFinish() noexcept {
  EXPECT_FALSE(finished_);
  finished_ = true;
  controller_->close();
}

void InputStringListCallback::onClose() noexcept {
  EXPECT_TRUE(finished_);
  EXPECT_FALSE(closed_);
  closed_ = true;
}

void InputStringListCallback::onException(const std::exception_ptr& ex)
  noexcept {
  ADD_FAILURE();
}

void InputStringListCallback::onError(const std::exception_ptr& ex) noexcept {
  ADD_FAILURE();
}

InputStringListCallback::~InputStringListCallback() {
  EXPECT_TRUE(closed_);
}
