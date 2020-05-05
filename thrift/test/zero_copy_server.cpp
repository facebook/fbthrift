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

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <folly/init/Init.h>

#include <common/services/cpp/ServiceFramework.h>
#include <thrift/test/gen-cpp2/ZeroCopyService.h>

DEFINE_int32(port, 7878, "Port for the thrift server");
DEFINE_int32(threshold, 32 * 1024, "Zerocopy threshold");
DEFINE_bool(debug_logs, false, "Debug logs");

using namespace thrift::zerocopy::cpp2;

class ZeroCopyServiceImpl : public ZeroCopyServiceSvIf,
                            public facebook::fb303::FacebookBase2 {
 public:
  ZeroCopyServiceImpl() : facebook::fb303::FacebookBase2("Zerocopy") {}
  ~ZeroCopyServiceImpl() override = default;

  void echo(IOBuf& ret, std::unique_ptr<IOBuf> data) override {
    ret = data->cloneAsValue();
    if (FLAGS_debug_logs) {
      LOG(INFO) << "[" << num_ << "]: data = " << data->countChainElements()
                << ":" << data->computeChainDataLength()
                << " ret = " << ret.countChainElements() << ":"
                << ret.computeChainDataLength();
    }
    num_++;
  }
  facebook::fb303::cpp2::fb_status getStatus() override {
    return facebook::fb303::cpp2::fb_status::ALIVE;
  }

 private:
  size_t num_{0};
};

int main(int argc, char* argv[]) {
  struct rlimit rlim = {
      .rlim_cur = RLIM_INFINITY,
      .rlim_max = RLIM_INFINITY,
  };
  setrlimit(RLIMIT_MEMLOCK, &rlim); // best effort

  folly::init(&argc, &argv);

  LOG(INFO) << "Running on port " << FLAGS_port;

  auto handler = std::make_shared<ZeroCopyServiceImpl>();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  facebook::services::TLSConfig::applyDefaultsToThriftServer(*server);
  if (FLAGS_threshold > 0) {
    LOG(INFO) << "Adding zerocopy enable func with threshold = "
              << FLAGS_threshold;
    server->setZeroCopyEnableFunc([](const std::unique_ptr<folly::IOBuf>& buf) {
      auto len = static_cast<int>(buf->computeChainDataLength());
      if (FLAGS_debug_logs) {
        LOG(INFO) << len << "-" << FLAGS_threshold;
      }
      return len >= FLAGS_threshold;
    });
  }
  server->setInterface(handler);
  server->setPort(FLAGS_port);

  facebook::services::ServiceFramework instance("ZeroCopyServer");

  instance.addThriftService(server, handler.get(), FLAGS_port);
  instance.go();

  return 0;
}
