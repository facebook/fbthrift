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

#pragma once

#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/StaticSegmentBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/extension/ThriftConnectionExtension.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/extension/ThriftExtensionPipelineHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/framework/ThriftPipelineHandler.h>

namespace apache::thrift::fast_thrift::thrift {

namespace module_detail {

template <typename H>
struct ExtensionAdapter {
  template <typename Context>
  using Handler = server::ThriftExtensionPipelineHandler<H, Context>;
};

template <typename H, typename... Args>
struct TypedExtensionEntry {
  channel_pipeline::HandlerId id;
  std::tuple<Args...> args;

  channel_pipeline::detail::HandlerNode makeDynamic(
      ExtensionStateStore& store) const {
    return std::apply(
        [&](const auto&... arg) {
          using Adapter = server::ThriftExtensionPipelineHandler<H>;
          return channel_pipeline::detail::makeHandlerNode<Adapter>(
              id, std::make_unique<Adapter>(store, arg...));
        },
        args);
  }

  auto makeStatic(ExtensionStateStore& store) const {
    using Adapter = ExtensionAdapter<H>;
    using Entry = channel_pipeline::detail::StaticSegmentEntry<
        Adapter::template Handler,
        std::reference_wrapper<ExtensionStateStore>,
        Args...>;
    return std::apply(
        [&](const auto&... arg) {
          return Entry{
              id,
              std::tuple<std::reference_wrapper<ExtensionStateStore>, Args...>(
                  store, arg...)};
        },
        args);
  }

  template <typename Callbacks>
  void appendPendingConnectionCallbacks(
      std::vector<Callbacks>& callbacks) const {
    if constexpr (ThriftPendingConnectionExtensionHandler<H>) {
      callbacks.push_back(
          Callbacks{
              .enqueued = +[]() noexcept { H::onConnectionEnqueued(); },
              .dequeued = +[]() noexcept { H::onConnectionDequeued(); },
              .droppedWhileQueued =
                  +[]() noexcept { H::onConnectionDroppedWhileQueued(); },
          });
    }
  }
};

template <typename... Entry>
struct TypedModuleState {
  static constexpr std::size_t kCount = sizeof...(Entry);

  explicit TypedModuleState(std::tuple<Entry...> entries)
      : entries(std::move(entries)), ids(makeIds()) {}

  void appendDynamic(
      ExtensionStateStore& store,
      folly::FunctionRef<void(channel_pipeline::detail::HandlerNode&&)> append)
      const {
    std::apply(
        [&](const auto&... entry) { (append(entry.makeDynamic(store)), ...); },
        entries);
  }

  channel_pipeline::detail::ErasedStaticSegment makeStatic(
      ExtensionStateStore& store) const {
    return std::apply(
        [&](const auto&... entry) {
          auto segmentEntries = std::tuple(entry.makeStatic(store)...);
          using Segment = channel_pipeline::detail::InlineTypedStaticSegment<
              decltype(entry.makeStatic(store))...>;
          return channel_pipeline::detail::ErasedStaticSegment::make<Segment>(
              segmentEntries, ids);
        },
        entries);
  }

  std::array<channel_pipeline::HandlerId, kCount> makeIds() const {
    return std::apply(
        [](const auto&... entry) {
          return std::array<channel_pipeline::HandlerId, kCount>{entry.id...};
        },
        entries);
  }

  std::tuple<Entry...> entries;
  std::array<channel_pipeline::HandlerId, kCount> ids;
};

} // namespace module_detail

/**
 * FastServerModule — a named bundle of Fast Thrift extensions.
 *
 * A module is a *value*, not a polymorphic base. It carries an ordered list of
 * per-connection pipeline handler factories plus optional server-scoped
 * extension callbacks.
 *
 * Distribute a reusable bundle as a free function returning a populated module:
 *
 *   FastServerModule makeLoggingModule(LogConfig cfg) {
 *     return FastServerModule("logging")
 *         .addThriftExtension<RequestLogObserver>()
 *         .addThriftExtension<TimingObserver>(std::move(cfg));
 *   }
 *
 * addThriftExtension is the recommended surface; addNativeThriftHandler is the
 * advanced, allowlist-gated path for handlers that need raw pipeline access.
 *
 * Hand it to FastThriftServer::addModule; its handlers are spliced into the
 * pipeline in call order relative to other addModule /
 * addNativeThriftPipelineHandlers calls, with intra-module order preserved.
 */
class FastServerModule {
 public:
  using PendingConnectionCallback = void (*)() noexcept;
  struct PendingConnectionCallbacks {
    PendingConnectionCallback enqueued;
    PendingConnectionCallback dequeued;
    PendingConnectionCallback droppedWhileQueued;
  };

  explicit FastServerModule(std::string name) : name_(std::move(name)) {}

  const std::string& name() const { return name_; }

  /**
   * Append a raw ("native") pipeline handler to this module, in order. This is
   * the advanced path: T owns message lifetime and the raw pipeline context, so
   * T must be on the Thrift-governed allowlist (see
   * NativeThriftHandlerAllowlist.h) or this fails to compile. Prefer
   * addThriftExtension unless raw access is genuinely required. `args` are
   * copied and used to construct a fresh T per connection. Returns *this for
   * chaining.
   *
   * T must satisfy the Inbound, Outbound, or Duplex handler concept over
   * server::ThriftPipelineHandlerContext. StaticT may provide the equivalent
   * implementation over server::StaticThriftPipelineHandlerContext; without
   * it, the module requires ChannelPipelineMode::Dynamic.
   */
  template <typename T, typename StaticT = void, typename... Args>
  FastServerModule& addNativeThriftHandler(Args... args) {
    return addFactory([&](channel_pipeline::HandlerId id) {
      return server::makeThriftPipelineHandlerFactory<T, StaticT>(
          id, std::move(args)...);
    });
  }

  /**
   * Append a constrained extension to this module, in order. `H` may implement
   * any supported extension callback family. Static pending-connection
   * callbacks are registered once for the server; when `H` also implements a
   * per-connection family, `args` are copied and used to construct a fresh H
   * per connection. Returns *this for chaining.
   *
   * H is wrapped in the framework's extension adapter, which owns message
   * lifetime and enforces the forwarding / rejection contract on H's behalf.
   * The factory is built directly rather than through addNativeThriftHandler,
   * because the adapter also needs the connection's shared extension state.
   * H is constrained by the adapter's own static_assert on the extension
   * callback contract rather than by the native-handler allowlist, so
   * extensions never need an allowlist entry.
   */
  template <typename H, typename... Args>
  FastServerModule& addThriftExtension(Args... args) {
    static_assert(
        server::ThriftPerConnectionExtensionHandler<H> ||
            ThriftPendingConnectionExtensionHandler<H>,
        "H must implement a per-connection or pending-connection extension "
        "callback family");
    if constexpr (ThriftPendingConnectionExtensionHandler<H>) {
      pendingConnectionCallbacks_.push_back(
          PendingConnectionCallbacks{
              .enqueued = +[]() noexcept { H::onConnectionEnqueued(); },
              .dequeued = +[]() noexcept { H::onConnectionDequeued(); },
              .droppedWhileQueued =
                  +[]() noexcept { H::onConnectionDroppedWhileQueued(); },
          });
    }
    if constexpr (server::ThriftPerConnectionExtensionHandler<H>) {
      controlsReads_ |= ThriftBackpressureExtensionHandler<H>;
      requiresHeaders_ |= UsesHeaders<H>;
      addFactory([&](channel_pipeline::HandlerId id) {
        return server::makeThriftExtensionHandlerFactory<H>(
            id, std::move(args)...);
      });
    }
    return *this;
  }

  /**
   * Whether any extension in this module pauses and resumes reads on a
   * connection. Checked by FastThriftServer::addModule against
   * enableWriteBufferBackpressure, which resumes reads the moment its own
   * buffer drains: the two arbitrate nothing between them, so a write-buffer
   * drain would silently lift an extension's pause.
   */
  bool controlsReads() const { return controlsReads_; }

  /**
   * Whether any extension in this module declares kUsesHeaders. Checked by
   * FastThriftServer::addModule against enableRequestHeaders: headers are
   * populated on the per-request context only under that setting, so without
   * it the extension would read empty.
   */
  bool requiresHeaders() const { return requiresHeaders_; }

  /**
   * The module's handler factories, in call order. FastThriftServer::addModule
   * consumes these when splicing the module into every connection's pipeline.
   */
  const std::vector<server::ThriftPipelineHandlerFactory>& handlers() const& {
    return factories_;
  }
  std::vector<server::ThriftPipelineHandlerFactory>&& handlers() && {
    return std::move(factories_);
  }

  const std::vector<PendingConnectionCallbacks>& pendingConnectionCallbacks()
      const& {
    return pendingConnectionCallbacks_;
  }
  std::vector<PendingConnectionCallbacks>&& pendingConnectionCallbacks() && {
    return std::move(pendingConnectionCallbacks_);
  }

 private:
  // Append the factory `makeFactory` builds for the next registration slot.
  //
  // The id is two-level, keyed on the module name (its namespace) and the
  // handler's within-module index, so distinct modules get non-overlapping id
  // streams. Empty module names are rejected at FastThriftServer::addModule, so
  // the empty namespace stays reserved for top-level (loose) handlers. Every
  // registration path derives its id here, which is what keeps the indices
  // dense and in registration order.
  template <typename MakeFactory>
  FastServerModule& addFactory(MakeFactory&& makeFactory) {
    auto id = server::deriveThriftPipelineHandlerId(name_, factories_.size());
    factories_.push_back(std::forward<MakeFactory>(makeFactory)(id));
    return *this;
  }

  std::string name_;
  std::vector<server::ThriftPipelineHandlerFactory> factories_;
  std::vector<PendingConnectionCallbacks> pendingConnectionCallbacks_;
  bool controlsReads_{false};
  bool requiresHeaders_{false};
};

template <typename... Entry>
class StaticFastServerModule {
 public:
  explicit StaticFastServerModule(std::string name) : name_(std::move(name)) {}

  template <typename H, typename... Args>
  auto addThriftExtension(Args&&... args) && {
    using NewEntry =
        module_detail::TypedExtensionEntry<H, std::decay_t<Args>...>;
    const auto id =
        server::deriveThriftPipelineHandlerId(name_, sizeof...(Entry));
    return StaticFastServerModule<Entry..., NewEntry>(
        std::move(name_),
        std::tuple_cat(
            std::move(entries_),
            std::tuple<NewEntry>(NewEntry{
                id,
                std::tuple<std::decay_t<Args>...>(
                    std::forward<Args>(args)...)})),
        controlsReads_ || ThriftBackpressureExtensionHandler<H>,
        requiresHeaders_ || UsesHeaders<H>);
  }

  const std::string& name() const noexcept { return name_; }
  bool controlsReads() const noexcept { return controlsReads_; }
  bool requiresHeaders() const noexcept { return requiresHeaders_; }

  std::vector<FastServerModule::PendingConnectionCallbacks>
  pendingConnectionCallbacks() const {
    std::vector<FastServerModule::PendingConnectionCallbacks> callbacks;
    std::apply(
        [&](const auto&... entry) {
          (entry.appendPendingConnectionCallbacks(callbacks), ...);
        },
        entries_);
    return callbacks;
  }

  server::ThriftPipelineRegistration registration() && {
    static_assert(sizeof...(Entry) != 0);
    auto state =
        std::make_shared<const module_detail::TypedModuleState<Entry...>>(
            std::move(entries_));
    return server::ThriftPipelineRegistration(
        server::ThriftPipelineSegmentFactory(
            [state](ExtensionStateStore& store, auto append) {
              state->appendDynamic(store, append);
            },
            [state](ExtensionStateStore& store) {
              return state->makeStatic(store);
            }));
  }

 private:
  template <typename...>
  friend class StaticFastServerModule;

  StaticFastServerModule(
      std::string name,
      std::tuple<Entry...> entries,
      bool controlsReads,
      bool requiresHeaders)
      : name_(std::move(name)),
        entries_(std::move(entries)),
        controlsReads_(controlsReads),
        requiresHeaders_(requiresHeaders) {}

  std::string name_;
  std::tuple<Entry...> entries_;
  bool controlsReads_{false};
  bool requiresHeaders_{false};
};

StaticFastServerModule(std::string) -> StaticFastServerModule<>;

} // namespace apache::thrift::fast_thrift::thrift
