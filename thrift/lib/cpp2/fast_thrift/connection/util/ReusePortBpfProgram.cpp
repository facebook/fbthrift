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

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#if defined(__linux__)
#include <linux/bpf.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <bpf/libbpf.h>
#endif

#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace apache::thrift::fast_thrift::connection {
namespace {

#if defined(__linux__)

constexpr bpf_insn insn(
    uint8_t code,
    uint8_t dst,
    uint8_t src,
    int16_t offset,
    int32_t immediate) noexcept {
  return bpf_insn{
      .code = code,
      .dst_reg = dst,
      .src_reg = src,
      .off = offset,
      .imm = immediate,
  };
}

int bpfSyscall(bpf_cmd command, bpf_attr& attributes) noexcept {
  return static_cast<int>(
      ::syscall(SYS_bpf, command, &attributes, sizeof(attributes)));
}

int createCursorMap(uint32_t groupSize) noexcept {
  bpf_attr attributes{};
  attributes.map_type = BPF_MAP_TYPE_PERCPU_ARRAY;
  attributes.key_size = sizeof(uint32_t);
  attributes.value_size = sizeof(uint32_t);
  attributes.max_entries = 1;
  const int mapFd = bpfSyscall(BPF_MAP_CREATE, attributes);
  if (mapFd < 0) {
    return mapFd;
  }

  const int cpuCount = ::libbpf_num_possible_cpus();
  if (cpuCount <= 0) {
    XLOGF(
        WARNING,
        "Failed to determine possible CPU count for reuseport round-robin "
        "cursors: {}; using zero-initialized cursors",
        cpuCount);
    return mapFd;
  }

  // Per-CPU map values are padded to eight bytes. Stagger the initial cursor
  // by CPU so the first connection on every CPU does not target worker zero.
  std::vector<uint64_t> cursors(static_cast<size_t>(cpuCount));
  for (size_t cpu = 0; cpu < cursors.size(); ++cpu) {
    cursors[cpu] = cpu % groupSize;
  }
  uint32_t key = 0;
  bpf_attr update{};
  update.map_fd = static_cast<uint32_t>(mapFd);
  update.key = reinterpret_cast<uint64_t>(&key);
  update.value = reinterpret_cast<uint64_t>(cursors.data());
  update.flags = BPF_ANY;
  if (bpfSyscall(BPF_MAP_UPDATE_ELEM, update) != 0) {
    const int savedErrno = errno;
    XLOGF(
        WARNING,
        "Failed to initialize reuseport round-robin cursors: errno={} ({})",
        savedErrno,
        folly::errnoStr(savedErrno));
  }
  return mapFd;
}

std::vector<bpf_insn> roundRobinProgram(int mapFd, uint32_t groupSize) {
  return {
      // r1 = cursor map
      insn(BPF_LD | BPF_DW | BPF_IMM, BPF_REG_1, BPF_PSEUDO_MAP_FD, 0, mapFd),
      insn(0, 0, 0, 0, 0),
      // *(u32 *)(r10 - 4) = 0; r2 = r10 - 4
      insn(BPF_ST | BPF_W | BPF_MEM, BPF_REG_10, 0, -4, 0),
      insn(BPF_ALU64 | BPF_MOV | BPF_X, BPF_REG_2, BPF_REG_10, 0, 0),
      insn(BPF_ALU64 | BPF_ADD | BPF_K, BPF_REG_2, 0, 0, -4),
      // r0 = bpf_map_lookup_elem(&cursor, &key)
      insn(BPF_JMP | BPF_CALL, 0, 0, 0, BPF_FUNC_map_lookup_elem),
      // A missing map value returns an invalid index, making the kernel use
      // its normal reuseport hash selection.
      insn(BPF_JMP | BPF_JEQ | BPF_K, BPF_REG_0, 0, 7, 0),
      // r1 = *cursor; *cursor = r1 + 1; return r1 % groupSize
      insn(BPF_LDX | BPF_W | BPF_MEM, BPF_REG_1, BPF_REG_0, 0, 0),
      insn(BPF_ALU64 | BPF_MOV | BPF_X, BPF_REG_2, BPF_REG_1, 0, 0),
      insn(BPF_ALU64 | BPF_ADD | BPF_K, BPF_REG_2, 0, 0, 1),
      insn(BPF_STX | BPF_W | BPF_MEM, BPF_REG_0, BPF_REG_2, 0, 0),
      insn(
          BPF_ALU | BPF_MOD | BPF_K,
          BPF_REG_1,
          0,
          0,
          static_cast<int32_t>(groupSize)),
      insn(BPF_ALU64 | BPF_MOV | BPF_X, BPF_REG_0, BPF_REG_1, 0, 0),
      insn(BPF_JMP | BPF_EXIT, 0, 0, 0, 0),
      insn(
          BPF_ALU64 | BPF_MOV | BPF_K,
          BPF_REG_0,
          0,
          0,
          static_cast<int32_t>(groupSize)),
      insn(BPF_JMP | BPF_EXIT, 0, 0, 0, 0),
  };
}

int loadRoundRobinProgram(int mapFd, uint32_t groupSize) noexcept {
  const auto program = roundRobinProgram(mapFd, groupSize);
  constexpr char kLicense[] = "Apache-2.0";
  std::vector<char> verifierLog(64 * 1024);

  bpf_attr attributes{};
  attributes.prog_type = BPF_PROG_TYPE_SOCKET_FILTER;
  attributes.insn_cnt = static_cast<uint32_t>(program.size());
  attributes.insns = reinterpret_cast<uint64_t>(program.data());
  attributes.license = reinterpret_cast<uint64_t>(kLicense);
  attributes.log_buf = reinterpret_cast<uint64_t>(verifierLog.data());
  attributes.log_size = static_cast<uint32_t>(verifierLog.size());
  attributes.log_level = 1;
  std::memcpy(
      attributes.prog_name, "ft_reuseport_rr", sizeof("ft_reuseport_rr"));

  const int programFd = bpfSyscall(BPF_PROG_LOAD, attributes);
  if (programFd < 0) {
    const int savedErrno = errno;
    verifierLog.back() = '\0';
    XLOGF(
        ERR,
        "Failed to load reuseport round-robin eBPF program: errno={} ({}), "
        "verifier={}",
        savedErrno,
        folly::errnoStr(savedErrno),
        verifierLog.data());
  }
  return programFd;
}

#endif

} // namespace

std::shared_ptr<const ReusePortBpfProgram>
ReusePortBpfProgram::createRoundRobin(uint32_t groupSize) noexcept {
#if defined(__linux__)
  if (groupSize == 0 ||
      groupSize > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
    XLOGF(ERR, "Invalid reuseport round-robin group size {}", groupSize);
    return nullptr;
  }

  const int mapFd = createCursorMap(groupSize);
  if (mapFd < 0) {
    const int savedErrno = errno;
    XLOGF(
        ERR,
        "Failed to create reuseport round-robin cursor map: errno={} ({})",
        savedErrno,
        folly::errnoStr(savedErrno));
    return nullptr;
  }

  const int programFd = loadRoundRobinProgram(mapFd, groupSize);
  ::close(mapFd);
  if (programFd < 0) {
    return nullptr;
  }
  return std::shared_ptr<const ReusePortBpfProgram>(
      new ReusePortBpfProgram(programFd));
#else
  (void)groupSize;
  XLOG(ERR) << "Reuseport round-robin eBPF is only supported on Linux";
  return nullptr;
#endif
}

ReusePortBpfProgram::~ReusePortBpfProgram() {
#if defined(__linux__)
  ::close(fd_);
#endif
}

bool ReusePortBpfProgram::attach(folly::NetworkSocket socket) const noexcept {
#if defined(__linux__)
  if (::setsockopt(
          socket.toFd(),
          SOL_SOCKET,
          SO_ATTACH_REUSEPORT_EBPF,
          &fd_,
          sizeof(fd_)) == 0) {
    return true;
  }
  const int savedErrno = errno;
  XLOGF(
      ERR,
      "SO_ATTACH_REUSEPORT_EBPF failed on fd {}: errno={} ({})",
      socket.toFd(),
      savedErrno,
      folly::errnoStr(savedErrno));
  return false;
#else
  (void)socket;
  return false;
#endif
}

} // namespace apache::thrift::fast_thrift::connection
