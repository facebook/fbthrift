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
#ifndef THRIFT_TEST_STREAM_TESTCALLBACKS_H_
#define THRIFT_TEST_STREAM_TESTCALLBACKS_H_ 1

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <functional>

#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/test/stream/gen-cpp2/stream_types.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

#define EXPECT_EXCEPTION(expression, message) do {  \
  try {                                             \
    (expression);                                   \
    ADD_FAILURE() << "No exceptions thrown";        \
  } catch(const std::exception& e) {                \
    EXPECT_STREQ(message, e.what());                \
  } catch(...) {                                    \
    ADD_FAILURE() << "Unexpected exception thrown"; \
  }                                                 \
} while(false)

namespace apache { namespace thrift {

class Service {
  private:
    static const int16_t METHOD_SIMPLE_EXCEPTION_ID = -1;

  public:
    template<typename Reader>
    static uint32_t deserialize(Reader& reader, int& value) {
      return reader.readI32(value);
    }

    static std::exception_ptr methodExceptionDeserializer(StreamReader& reader){
      int16_t exceptionId = reader.readExceptionId();
      switch (exceptionId) {
        case METHOD_SIMPLE_EXCEPTION_ID:
          return reader.readExceptionItem<
                    apache::thrift::test::cpp2::SimpleException>();
        default:
          reader.skipException();
          // in the genrated code this will be an TApplicationException instead
          return std::make_exception_ptr(std::runtime_error(
                    "Cannot deserialize unknown exception."));
      }
    }

    template <typename Writer>
    static uint32_t serialize(Writer& writer, const int& value) {
      return writer.writeI32(value);
    }

    static void methodExceptionSerializer(StreamWriter& writer,
                                          const std::exception_ptr& exception) {
      try {
        std::rethrow_exception(exception);
      } catch (const apache::thrift::test::cpp2::SimpleException& e) {
        writer.writeExceptionItem(METHOD_SIMPLE_EXCEPTION_ID, e);
      } catch (...) {
        throw StreamException("Cannot serialize unknown exception.");
      }
    }
};

struct TestException {
};

typedef std::function<bool(
    const apache::thrift::transport::TTransportException&)> Checker;

struct DummyOutputCallback : public OutputStreamCallback<int> {
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~DummyOutputCallback();
};

struct OutputCallback : public OutputStreamCallback<int> {
  explicit OutputCallback(int limit);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputCallback();

  int batchSize_;
  int next_;
  int limit_;
  bool closed_;
};

struct OutputException: public OutputStreamCallback<int> {
  explicit OutputException(int limit);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputException();

  int batchSize_;
  int next_;
  int limit_;
  bool threw_;
  bool closed_;
};

struct OutputError: public OutputStreamCallback<int> {
  explicit OutputError(int limit);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  void setStreamManager(StreamManager* manager);
  ~OutputError();

  int batchSize_;
  int next_;
  int limit_;
  bool errored_;
  bool closed_;
  StreamManager* manager_;
};

struct OutputReceiveClose: public OutputStreamCallback<int> {
  // limit is the maximum value that should be outputted before being closed.
  // Since network latency is unpredictable, any large value is valid.
  explicit OutputReceiveClose(int limit);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputReceiveClose();

  int batchSize_;
  int limit_;
  int next_;
  bool closed_;
};

struct OutputReceiveError: public OutputStreamCallback<int> {
  OutputReceiveError();
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputReceiveError();

  int batchSize_;
  int next_;
  bool errored_;
  bool closed_;
};

struct OutputReceiveErrorMsg: public OutputStreamCallback<int> {
  explicit OutputReceiveErrorMsg(const std::string& message);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputReceiveErrorMsg();

  int batchSize_;
  int next_;
  bool errored_;
  bool closed_;
  std::string message_;
};

struct OutputReceiveErrorCheck: public OutputStreamCallback<int> {
  explicit OutputReceiveErrorCheck(const Checker& checker);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputReceiveErrorCheck();

  int batchSize_;
  int next_;
  bool errored_;
  bool closed_;
  Checker checker_;
};

struct OutputTimeoutCheck: public OutputStreamCallback<int> {
  OutputTimeoutCheck(long sleepTime, const Checker& checker);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputTimeoutCheck();

  bool errored_;
  bool closed_;
  long sleepTime_;
  Checker checker_;
};

struct OutputIgnore: public OutputStreamCallback<int> {
  // limit is the maximum value that should be outputted before being closed.
  // Since network latency is unpredictable, any large value is valid.
  explicit OutputIgnore(int limit);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputIgnore();

  int batchSize_;
  int limit_;
  int next_;
  bool closed_;
};

struct OutputCoopCallback : public OutputStreamCallback<int> {
  // limit is the maximum value that should be outputted before being closed.
  // Since network latency is unpredictable, any large value is valid.
  explicit OutputCoopCallback(int limit);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputCoopCallback();

  int batchSize_;
  int next_;
  int limit_;
  bool closed_;
};

struct OutputStringCallback : public OutputStreamCallback<std::string> {
  explicit OutputStringCallback(const std::vector<std::string>& strings);
  void onSend() noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~OutputStringCallback();

  std::vector<std::string> strings_;
  uint32_t index_;
  int batchSize_;
  bool finished_;
  bool closed_;
};

struct DummyInputCallback : public InputStreamCallback<int> {
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~DummyInputCallback();
};

struct InputCallback : public InputStreamCallback<int> {
  explicit InputCallback(int limit);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputCallback();

  int next_;
  int limit_;
  bool finished_;
  bool closed_;
};

struct InputReceiveException : public InputStreamCallback<int> {
  explicit InputReceiveException(int limit);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputReceiveException();

  int next_;
  int limit_;
  bool caught_;
  bool closed_;
};

struct InputError : public InputStreamCallback<int> {
  explicit InputError(int limit);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  void setStreamManager(StreamManager* manager);
  ~InputError();

  int next_;
  int limit_;
  bool finished_;
  bool errored_;
  bool closed_;
  StreamManager* manager_;
};

struct InputClose : public InputStreamCallback<int> {
  explicit InputClose(int limit);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputClose();

  int next_;
  int limit_;
  bool finished_;
  bool closed_;
};

struct InputReceiveError : public InputStreamCallback<int> {
  explicit InputReceiveError();
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputReceiveError();

  int next_;
  bool errored_;
  bool closed_;
};

struct InputReceiveErrorMsg : public InputStreamCallback<int> {
  explicit InputReceiveErrorMsg(const std::string& message);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputReceiveErrorMsg();

  int next_;
  bool errored_;
  bool closed_;
  std::string  message_;
};

struct InputReceiveErrorCheck : public InputStreamCallback<int> {
  explicit InputReceiveErrorCheck(const Checker& checker);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputReceiveErrorCheck();

  int next_;
  bool errored_;
  bool closed_;
  Checker checker_;
};

struct InputIgnore : public InputStreamCallback<int> {
  // limit is the maximum value that should be outputted before being closed.
  // Since network latency is unpredictable, any large value is valid.
  explicit InputIgnore(int limit);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputIgnore();

  int next_;
  int limit_;
  bool finished_;
  bool closed_;
};

struct InputReceiveClose : public InputStreamCallback<int> {
  // limit is the maximum value that should be outputted before being closed.
  // Since network latency is unpredictable, any large value is valid.
  explicit InputReceiveClose(int limit);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputReceiveClose();

  int next_;
  int limit_;
  bool closed_;
};

struct InputCoopCallback: public InputStreamCallback<int> {
  InputCoopCallback(
      int limit,
      const std::shared_ptr<OutputStreamController<int>>& controller);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputCoopCallback();

  int batchSize_;
  int nextMyReceive_;
  int nextOutputReceive_;
  int myNextOutput_;
  int limit_;
  bool finished_;
  bool closed_;
  std::shared_ptr<OutputStreamController<int>> controller_;
};

struct InputSumCallback: public InputStreamCallback<int> {
  explicit InputSumCallback(
      const std::shared_ptr<OutputStreamController<int>>& controller);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputSumCallback();

  int sum_;
  bool finished_;
  bool closed_;
  std::shared_ptr<OutputStreamController<int>> controller_;
};

struct InputSingletonCallback: public InputStreamCallback<int> {
  explicit InputSingletonCallback(int expected);
  void onReceive(int& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputSingletonCallback();

  int expected_;
  bool received_;
  bool finished_;
  bool closed_;
};

struct InputStringCallback: public InputStreamCallback<std::string> {
  explicit InputStringCallback(const std::vector<std::string>& strings);
  void onReceive(std::string& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputStringCallback();

  std::vector<std::string> strings_;
  uint32_t index_;
  bool finished_;
  bool closed_;
};

struct InputStringListCallback
    : public InputStreamCallback<std::vector<std::string>> {

  explicit InputStringListCallback(
      const std::shared_ptr<OutputStreamController<
          std::vector<apache::thrift::test::cpp2::Subject>>>& controller);
  void onReceive(std::vector<std::string>& value) noexcept;
  void onFinish() noexcept;
  void onException(const std::exception_ptr& ex) noexcept;
  void onError(const std::exception_ptr& ex) noexcept;
  void onClose() noexcept;
  ~InputStringListCallback();

  bool finished_;
  bool closed_;
  std::shared_ptr<OutputStreamController<
            std::vector<apache::thrift::test::cpp2::Subject>>> controller_;
};

}}

#endif
