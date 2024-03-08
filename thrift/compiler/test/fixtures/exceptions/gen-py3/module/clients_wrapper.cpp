/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#include <thrift/compiler/test/fixtures/exceptions/gen-py3/module/clients_wrapper.h>

namespace cpp2 {


folly::Future<folly::Unit>
RaiserClientWrapper::doBland(
    apache::thrift::RpcOptions& rpcOptions) {
  auto* client = static_cast<::cpp2::RaiserAsyncClient*>(async_client_.get());
  folly::Promise<folly::Unit> _promise;
  auto _future = _promise.getFuture();
  auto callback = std::make_unique<::thrift::py3::FutureCallback<folly::Unit>>(
    std::move(_promise), rpcOptions, client->recv_wrapped_doBland, channel_);
  try {
    client->doBland(
      rpcOptions,
      std::move(callback)
    );
  } catch (...) {
    return folly::makeFuture<folly::Unit>(folly::exception_wrapper(
      std::current_exception()
    ));
  }
  return _future;
}

folly::Future<folly::Unit>
RaiserClientWrapper::doRaise(
    apache::thrift::RpcOptions& rpcOptions) {
  auto* client = static_cast<::cpp2::RaiserAsyncClient*>(async_client_.get());
  folly::Promise<folly::Unit> _promise;
  auto _future = _promise.getFuture();
  auto callback = std::make_unique<::thrift::py3::FutureCallback<folly::Unit>>(
    std::move(_promise), rpcOptions, client->recv_wrapped_doRaise, channel_);
  try {
    client->doRaise(
      rpcOptions,
      std::move(callback)
    );
  } catch (...) {
    return folly::makeFuture<folly::Unit>(folly::exception_wrapper(
      std::current_exception()
    ));
  }
  return _future;
}

folly::Future<std::string>
RaiserClientWrapper::get200(
    apache::thrift::RpcOptions& rpcOptions) {
  auto* client = static_cast<::cpp2::RaiserAsyncClient*>(async_client_.get());
  folly::Promise<std::string> _promise;
  auto _future = _promise.getFuture();
  auto callback = std::make_unique<::thrift::py3::FutureCallback<std::string>>(
    std::move(_promise), rpcOptions, client->recv_wrapped_get200, channel_);
  try {
    client->get200(
      rpcOptions,
      std::move(callback)
    );
  } catch (...) {
    return folly::makeFuture<std::string>(folly::exception_wrapper(
      std::current_exception()
    ));
  }
  return _future;
}

folly::Future<std::string>
RaiserClientWrapper::get500(
    apache::thrift::RpcOptions& rpcOptions) {
  auto* client = static_cast<::cpp2::RaiserAsyncClient*>(async_client_.get());
  folly::Promise<std::string> _promise;
  auto _future = _promise.getFuture();
  auto callback = std::make_unique<::thrift::py3::FutureCallback<std::string>>(
    std::move(_promise), rpcOptions, client->recv_wrapped_get500, channel_);
  try {
    client->get500(
      rpcOptions,
      std::move(callback)
    );
  } catch (...) {
    return folly::makeFuture<std::string>(folly::exception_wrapper(
      std::current_exception()
    ));
  }
  return _future;
}

} // namespace cpp2
