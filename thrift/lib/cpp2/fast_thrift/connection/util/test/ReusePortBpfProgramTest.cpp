/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/fast_thrift/connection/util/ReusePortBpfProgram.h>

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <folly/File.h>
#include <folly/ScopeGuard.h>
#include <folly/net/NetworkSocket.h>

#if defined(__linux__)
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <sys/socket.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <vector>

#endif

namespace apache::thrift::fast_thrift::connection {
namespace {

#if defined(__linux__)

constexpr size_t kNumListeners = 3;
constexpr size_t kNumConnections = 30;

std::vector<folly::File> makeReusePortGroup(
    sockaddr_in6& address, const ReusePortBpfProgram& program) {
  std::vector<folly::File> listeners;
  listeners.reserve(kNumListeners);

  for (size_t i = 0; i < kNumListeners; ++i) {
    folly::File listener(
        ::socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0),
        true);
    PCHECK(listener.fd() >= 0);
    int enabled = 1;
    PCHECK(
        ::setsockopt(
            listener.fd(),
            SOL_SOCKET,
            SO_REUSEPORT,
            &enabled,
            sizeof(enabled)) == 0);
    PCHECK(
        ::bind(
            listener.fd(),
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) == 0);
    if (i == 0) {
      socklen_t addressSize = sizeof(address);
      PCHECK(
          ::getsockname(
              listener.fd(),
              reinterpret_cast<sockaddr*>(&address),
              &addressSize) == 0);
    }
    PCHECK(::listen(listener.fd(), 128) == 0);
    if (i == 0) {
      CHECK(program.attach(folly::NetworkSocket::fromFd(listener.fd())));
    }
    listeners.push_back(std::move(listener));
  }
  return listeners;
}

#endif

TEST(ReusePortBpfProgramTest, RoundRobinSurvivesFailedAttachAndNewListeners) {
#if defined(__linux__)
  cpu_set_t originalAffinity;
  CPU_ZERO(&originalAffinity);
  if (::pthread_getaffinity_np(
          ::pthread_self(), sizeof(originalAffinity), &originalAffinity) != 0) {
    GTEST_SKIP() << "Unable to read thread CPU affinity";
  }
  const int cpu = ::sched_getcpu();
  ASSERT_GE(cpu, 0);
  cpu_set_t oneCpu;
  CPU_ZERO(&oneCpu);
  CPU_SET(cpu, &oneCpu);
  if (::pthread_setaffinity_np(::pthread_self(), sizeof(oneCpu), &oneCpu) !=
      0) {
    GTEST_SKIP() << "Unable to pin test thread to one CPU";
  }
  SCOPE_EXIT {
    EXPECT_EQ(
        ::pthread_setaffinity_np(
            ::pthread_self(), sizeof(originalAffinity), &originalAffinity),
        0);
  };

  auto program = ReusePortBpfProgram::createRoundRobin(kNumListeners);
  if (!program) {
    GTEST_SKIP() << "BPF program loading is unavailable";
  }
  sockaddr_in6 address{};
  address.sin6_family = AF_INET6;
  address.sin6_addr = in6addr_loopback;
  auto listeners = makeReusePortGroup(address, *program);
  EXPECT_FALSE(program->attach(folly::NetworkSocket()));

  std::array<size_t, kNumListeners> accepted{};
  for (size_t connection = 0; connection < kNumConnections; ++connection) {
    folly::File client(::socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC, 0), true);
    ASSERT_GE(client.fd(), 0);
    ASSERT_EQ(
        ::connect(
            client.fd(),
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)),
        0);

    size_t acceptingListener = kNumListeners;
    folly::File acceptedSocket;
    for (size_t i = 0; i < listeners.size(); ++i) {
      const int fd = ::accept4(
          listeners[i].fd(), nullptr, nullptr, SOCK_CLOEXEC | SOCK_NONBLOCK);
      if (fd >= 0) {
        ASSERT_EQ(acceptingListener, kNumListeners);
        acceptingListener = i;
        acceptedSocket = folly::File(fd, true);
      } else {
        ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
      }
    }
    ASSERT_LT(acceptingListener, kNumListeners);
    EXPECT_EQ(
        acceptingListener,
        (static_cast<size_t>(cpu) + connection) % kNumListeners);
    ++accepted[acceptingListener];
  }

  const auto [minimum, maximum] =
      std::minmax_element(accepted.begin(), accepted.end());
  EXPECT_LE(*maximum - *minimum, 1);
#else
  GTEST_SKIP() << "Reuseport eBPF is only supported on Linux";
#endif
}

} // namespace
} // namespace apache::thrift::fast_thrift::connection
