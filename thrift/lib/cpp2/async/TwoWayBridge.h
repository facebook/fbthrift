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

  struct Node {
   private:
    template <typename Consumer, typename Message>
    friend class AtomicQueue;
    friend class Queue;

    explicit Node(T&& t) : value(std::move(t)) {}

    T value;
    Node* next{nullptr};
  };

 private:
  template <typename Consumer, typename Message>
  friend class AtomicQueue;

  explicit Queue(Node* head) : head_(head) {}

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
        makeQueue(reinterpret_cast<typename MessageQueue::Node*>(ptr));
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
        makeQueue(reinterpret_cast<typename MessageQueue::Node*>(ptr));
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
        return makeQueue(reinterpret_cast<typename MessageQueue::Node*>(ptr));
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

  MessageQueue makeQueue(typename MessageQueue::Node* tail) {
    // Reverse a linked list.
    typename MessageQueue::Node* head{nullptr};
    while (tail) {
      head = std::exchange(tail, std::exchange(tail->next, head));
    }
    return MessageQueue(head);
  }

  static constexpr intptr_t kTypeMask = 3;
  static constexpr intptr_t kPointerMask = ~kTypeMask;

  std::atomic<intptr_t> storage_{0};
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
  using ClientQueue = typename ClientAtomicQueue::MessageQueue;
  using ServerQueue = typename ServerAtomicQueue::MessageQueue;

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
