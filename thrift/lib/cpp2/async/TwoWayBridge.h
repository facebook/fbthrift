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

#pragma once

#include <glog/logging.h>
#include <atomic>
#include <cassert>
#include <memory>
#include <utility>

#include <folly/lang/Assume.h>

namespace apache {
namespace thrift {
namespace detail {
namespace twowaybridge_detail {

template <typename T>
class Queue {
 public:
  Queue() {}
  Queue(Queue&& other) : head_(std::exchange(other.head_, nullptr)) {}
  Queue& operator=(Queue&& other) {
    clear();
    std::swap(head_, other.head_);
    return *this;
  }
  ~Queue() {
    clear();
  }

  bool empty() const {
    return !head_;
  }

  T& front() {
    return head_->value;
  }

  void pop() {
    std::unique_ptr<Node>(std::exchange(head_, head_->next));
  }

  void clear() {
    while (!empty()) {
      pop();
    }
  }

  explicit operator bool() const {
    return !empty();
  }

  struct Node {
   private:
    template <typename Consumer, typename Message>
    friend class AtomicQueue;
    friend class Queue;
    template <typename Message, typename Value>
    friend class AtomicQueueOrPtr;

    explicit Node(T&& t) : value(std::move(t)) {}

    T value;
    Node* next{nullptr};
  };

 private:
  template <typename Consumer, typename Message>
  friend class AtomicQueue;
  template <typename Message, typename Value>
  friend class AtomicQueueOrPtr;

  explicit Queue(Node* head) : head_(head) {}
  static Queue fromReversed(Node* tail) {
    // Reverse a linked list.
    Node* head{nullptr};
    while (tail) {
      head = std::exchange(tail, std::exchange(tail->next, head));
    }
    return Queue(head);
  }

  Node* head_{nullptr};
};

template <typename Consumer, typename Message>
class AtomicQueue {
 public:
  using MessageQueue = Queue<Message>;

  AtomicQueue() {}
  ~AtomicQueue() {
    auto storage = storage_.load(std::memory_order_acquire);
    auto type = static_cast<Type>(storage & kTypeMask);
    auto ptr = storage & kPointerMask;
    switch (type) {
      case Type::EMPTY:
      case Type::CLOSED:
        return;
      case Type::TAIL:
        MessageQueue::fromReversed(
            reinterpret_cast<typename MessageQueue::Node*>(ptr));
        return;
      default:
        folly::assume_unreachable();
    };
  }
  AtomicQueue(const AtomicQueue&) = delete;
  AtomicQueue& operator=(const AtomicQueue&) = delete;

  void push(Message&& value) {
    std::unique_ptr<typename MessageQueue::Node> node(
        new typename MessageQueue::Node(std::move(value)));
    assert(!(reinterpret_cast<intptr_t>(node.get()) & kTypeMask));

    auto storage = storage_.load(std::memory_order_relaxed);
    while (true) {
      auto type = static_cast<Type>(storage & kTypeMask);
      auto ptr = storage & kPointerMask;
      switch (type) {
        case Type::EMPTY:
        case Type::TAIL:
          node->next = reinterpret_cast<typename MessageQueue::Node*>(ptr);
          if (storage_.compare_exchange_weak(
                  storage,
                  reinterpret_cast<intptr_t>(node.get()) |
                      static_cast<intptr_t>(Type::TAIL),
                  std::memory_order_release,
                  std::memory_order_relaxed)) {
            node.release();
            return;
          }
          break;
        case Type::CLOSED:
          return;
        case Type::CONSUMER:
          node->next = nullptr;
          if (storage_.compare_exchange_weak(
                  storage,
                  reinterpret_cast<intptr_t>(node.get()) |
                      static_cast<intptr_t>(Type::TAIL),
                  std::memory_order_acq_rel,
                  std::memory_order_relaxed)) {
            node.release();
            auto consumer = reinterpret_cast<Consumer*>(ptr);
            consumer->consume();
            return;
          }
          break;
        default:
          folly::assume_unreachable();
      }
    }
  }

  bool wait(Consumer* consumer) {
    assert(!(reinterpret_cast<intptr_t>(consumer) & kTypeMask));
    auto storage = storage_.load(std::memory_order_relaxed);
    while (true) {
      auto type = static_cast<Type>(storage & kTypeMask);
      switch (type) {
        case Type::EMPTY:
          if (storage_.compare_exchange_weak(
                  storage,
                  reinterpret_cast<intptr_t>(consumer) |
                      static_cast<intptr_t>(Type::CONSUMER),
                  std::memory_order_release,
                  std::memory_order_relaxed)) {
            return true;
          }
          break;
        case Type::CLOSED:
          consumer->canceled();
          return true;
        case Type::TAIL:
          return false;
        default:
          folly::assume_unreachable();
      }
    }
  }

  void close() {
    auto storage = storage_.exchange(
        static_cast<intptr_t>(Type::CLOSED), std::memory_order_acquire);
    auto type = static_cast<Type>(storage & kTypeMask);
    auto ptr = storage & kPointerMask;
    switch (type) {
      case Type::EMPTY:
        return;
      case Type::TAIL:
        MessageQueue::fromReversed(
            reinterpret_cast<typename MessageQueue::Node*>(ptr));
        return;
      case Type::CONSUMER:
        reinterpret_cast<Consumer*>(ptr)->canceled();
        return;
      default:
        folly::assume_unreachable();
    };
  }

  bool isClosed() {
    auto type = static_cast<Type>(storage_ & kTypeMask);
    return type == Type::CLOSED;
  }

  MessageQueue getMessages() {
    auto storage = storage_.exchange(
        static_cast<intptr_t>(Type::EMPTY), std::memory_order_acquire);
    auto type = static_cast<Type>(storage & kTypeMask);
    auto ptr = storage & kPointerMask;
    switch (type) {
      case Type::TAIL:
        return MessageQueue::fromReversed(
            reinterpret_cast<typename MessageQueue::Node*>(ptr));
      case Type::EMPTY:
        return MessageQueue();
      case Type::CLOSED:
        // We accidentally re-opened the queue, so close it again.
        // This is only safe to do because isClosed() can't be called
        // concurrently with getMessages().
        close();
        return MessageQueue();
      default:
        folly::assume_unreachable();
    };
  }

 private:
  enum class Type : intptr_t { EMPTY = 0, CONSUMER = 1, TAIL = 2, CLOSED = 3 };

  static constexpr intptr_t kTypeMask = 3;
  static constexpr intptr_t kPointerMask = ~kTypeMask;

  std::atomic<intptr_t> storage_{0};
};

// queue with no consumers
template <typename Message, typename Value>
class AtomicQueueOrPtr {
 public:
  using MessageQueue = Queue<Message>;

  AtomicQueueOrPtr() {}
  ~AtomicQueueOrPtr() {
    auto storage = storage_.load(std::memory_order_relaxed);
    auto type = static_cast<Type>(storage & kTypeMask);
    auto ptr = storage & kPointerMask;
    switch (type) {
      case Type::EMPTY:
      case Type::CLOSED:
        return;
      case Type::TAIL:
        MessageQueue::fromReversed(
            reinterpret_cast<typename MessageQueue::Node*>(ptr));
        return;
      default:
        folly::assume_unreachable();
    };
  }
  AtomicQueueOrPtr(const AtomicQueueOrPtr&) = delete;
  AtomicQueueOrPtr& operator=(const AtomicQueueOrPtr&) = delete;

  // returns closed payload and does not move from message on failure
  Value* pushOrGetClosedPayload(Message&& message) {
    auto storage = storage_.load(std::memory_order_acquire);
    if (static_cast<Type>(storage & kTypeMask) == Type::CLOSED) {
      return closedPayload_;
    }

    std::unique_ptr<typename MessageQueue::Node> node(
        new typename MessageQueue::Node(std::move(message)));
    assert(!(reinterpret_cast<intptr_t>(node.get()) & kTypeMask));

    while (true) {
      auto type = static_cast<Type>(storage & kTypeMask);
      auto ptr = storage & kPointerMask;
      switch (type) {
        case Type::EMPTY:
        case Type::TAIL:
          node->next = reinterpret_cast<typename MessageQueue::Node*>(ptr);
          if (storage_.compare_exchange_weak(
                  storage,
                  reinterpret_cast<intptr_t>(node.get()) |
                      static_cast<intptr_t>(Type::TAIL),
                  std::memory_order_release,
                  std::memory_order_acquire)) {
            node.release();
            return nullptr;
          }
          break;
        case Type::CLOSED:
          message = std::move(node->value);
          return closedPayload_;
        default:
          folly::assume_unreachable();
      }
    }
  }

  MessageQueue closeOrGetMessages(Value* payload) {
    assert(payload); // nullptr is used as a sentinel
    // this is only read if the compare_exchange succeeds
    closedPayload_ = payload;
    while (true) {
      auto storage = storage_.exchange(
          static_cast<intptr_t>(Type::EMPTY), std::memory_order_acquire);
      auto type = static_cast<Type>(storage & kTypeMask);
      auto ptr = storage & kPointerMask;
      switch (type) {
        case Type::TAIL:
          return MessageQueue::fromReversed(
              reinterpret_cast<typename MessageQueue::Node*>(ptr));
        case Type::EMPTY:
          if (storage_.compare_exchange_weak(
                  storage,
                  static_cast<intptr_t>(Type::CLOSED),
                  std::memory_order_release,
                  std::memory_order_relaxed)) {
            return MessageQueue();
          }
          break;
        case Type::CLOSED:
        default:
          folly::assume_unreachable();
      }
    }
  }

  bool isClosed() const {
    return static_cast<Type>(storage_ & kTypeMask) == Type::CLOSED;
  }

 private:
  enum class Type : intptr_t { EMPTY = 0, TAIL = 1, CLOSED = 2 };

  static constexpr intptr_t kTypeMask = 3;
  static constexpr intptr_t kPointerMask = ~kTypeMask;

  // These can be combined if the platform requires Value to be 8-byte aligned.
  // Most platforms don't require that for functions.
  // A workaround is to make that function a member of an aligned struct
  // and pass in the address of the struct, but that is not necessarily a win
  // because of the runtime indirection cost.
  std::atomic<intptr_t> storage_{0};
  Value* closedPayload_{nullptr};
};
} // namespace twowaybridge_detail

template <
    typename ClientConsumer,
    typename ClientMessage,
    typename ServerConsumer,
    typename ServerMessage,
    typename Derived>
class TwoWayBridge {
  using ClientAtomicQueue =
      twowaybridge_detail::AtomicQueue<ClientConsumer, ClientMessage>;
  using ServerAtomicQueue =
      twowaybridge_detail::AtomicQueue<ServerConsumer, ServerMessage>;

 public:
  using ClientQueue = twowaybridge_detail::Queue<ClientMessage>;
  using ServerQueue = twowaybridge_detail::Queue<ServerMessage>;

  struct Deleter {
    void operator()(Derived* ptr) {
      ptr->decref();
    }
  };
  using Ptr = std::unique_ptr<Derived, Deleter>;

  Ptr copy() {
    auto refCount = refCount_.fetch_add(1, std::memory_order_relaxed);
    DCHECK(refCount > 0);
    return Ptr(derived());
  }

 protected:
  TwoWayBridge() = default;

  // These should only be called from the client thread

  void clientPush(ServerMessage&& value) {
    serverQueue_.push(std::move(value));
  }

  bool clientWait(ClientConsumer* consumer) {
    return clientQueue_.wait(consumer);
  }

  ClientQueue clientGetMessages() {
    return clientQueue_.getMessages();
  }

  void clientClose() {
    clientQueue_.close();
  }

  bool isClientClosed() {
    return clientQueue_.isClosed();
  }

  // These should only be called from the server thread

  void serverPush(ClientMessage&& value) {
    clientQueue_.push(std::move(value));
  }

  bool serverWait(ServerConsumer* consumer) {
    return serverQueue_.wait(consumer);
  }

  ServerQueue serverGetMessages() {
    return serverQueue_.getMessages();
  }

  void serverClose() {
    serverQueue_.close();
  }

  bool isServerClosed() {
    return serverQueue_.isClosed();
  }

 private:
  void decref() {
    if (refCount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete derived();
    }
  }

  Derived* derived() {
    return static_cast<Derived*>(this);
  }

  ClientAtomicQueue clientQueue_;
  ServerAtomicQueue serverQueue_;
  std::atomic<int8_t> refCount_{1};
};
} // namespace detail
} // namespace thrift
} // namespace apache
