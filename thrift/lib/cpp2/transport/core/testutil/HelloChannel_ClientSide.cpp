/*
 * Copyright 2017-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <thrift/lib/cpp2/transport/core/testutil/TestServiceClientExtension.h>

#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/rsocket/client/StreamingRequestCallback.h>
#include <yarpl/flowable/Flowables.h>

namespace testutil {
namespace testservice {

using TestService_helloChannel_pargs = apache::thrift::ThriftPresult<false>;
using TestService_helloChannel_presult = apache::thrift::ThriftPresult<true>;

// TODO Compiler generated encoding function!
std::unique_ptr<folly::IOBuf> encode(const std::string& value) {
  apache::thrift::CompactProtocolWriter writer;
  size_t bufSize = writer.serializedSizeString(value);
  folly::IOBufQueue queue;
  writer.setOutput(&queue, bufSize);
  writer.writeString(value);
  std::unique_ptr<folly::IOBuf> buf = queue.move();

  std::unique_ptr<folly::IOBuf> body = std::make_unique<folly::IOBuf>();
  body->prependChain(std::move(buf));

  return body;
}

std::string decode(std::unique_ptr<folly::IOBuf> buffer) {
  std::string result;
  apache::thrift::CompactProtocolReader reader;
  reader.setInput(buffer.get());
  reader.readString(result);
  return result;
}

// TODO Place the next lines into TestService_client.cpp file

void TestServiceAsyncClientExtended::helloChannel(
    std::unique_ptr<apache::thrift::RequestCallback> callback) {
  ::apache::thrift::RpcOptions rpcOptions;
  helloChannelImpl(false, rpcOptions, std::move(callback));
}

void TestServiceAsyncClientExtended::helloChannel(
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<apache::thrift::RequestCallback> callback) {
  helloChannelImpl(false, rpcOptions, std::move(callback));
}

void TestServiceAsyncClientExtended::helloChannelImpl(
    bool useSync,
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<apache::thrift::RequestCallback> callback) {
  switch (getChannel()->getProtocolId()) {
    case apache::thrift::protocol::T_BINARY_PROTOCOL: {
      apache::thrift::BinaryProtocolWriter writer;
      helloChannelT(&writer, useSync, rpcOptions, std::move(callback));
      break;
    }
    case apache::thrift::protocol::T_COMPACT_PROTOCOL: {
      apache::thrift::CompactProtocolWriter writer;
      helloChannelT(&writer, useSync, rpcOptions, std::move(callback));
      break;
    }
    default: {
      throw apache::thrift::TApplicationException("Could not find Protocol");
    }
  }
}

void TestServiceAsyncClientExtended::helloChannel(
    folly::Function<void(::apache::thrift::ClientReceiveState&&)> callback) {
  helloChannel(std::make_unique<apache::thrift::FunctionReplyCallback>(
      std::move(callback)));
}

// TODO Place the next lines into the TestService.tcc file

template <typename Protocol_>
void TestServiceAsyncClientExtended::helloChannelT(
    Protocol_* prot,
    bool useSync,
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<apache::thrift::RequestCallback> callback) {
  auto header = std::make_shared<apache::thrift::transport::THeader>(
      apache::thrift::transport::THeader::ALLOW_BIG_FRAMES);
  header->setProtocolId(getChannel()->getProtocolId());
  header->setHeaders(rpcOptions.releaseWriteHeaders());
  connectionContext_->setRequestHeader(header.get());
  std::unique_ptr<apache::thrift::ContextStack> ctx = this->getContextStack(
      this->getServiceName(),
      "PointService.helloChannel",
      connectionContext_.get());
  TestService_helloChannel_pargs args;
  auto sizer = [&](Protocol_* p) { return args.serializedSizeZC(p); };
  auto writer = [&](Protocol_* p) { args.write(p); };
  apache::thrift::clientSendT<Protocol_>(
      prot,
      rpcOptions,
      std::move(callback),
      std::move(ctx),
      header,
      channel_.get(),
      "helloChannel",
      writer,
      sizer,
      false,
      useSync);
  connectionContext_->setRequestHeader(nullptr);
}

yarpl::Reference<yarpl::flowable::Flowable<std::string>>
TestServiceAsyncClientExtended::helloChannel(
    yarpl::Reference<yarpl::flowable::Flowable<std::string>> input) {
  ::apache::thrift::RpcOptions rpcOptions;
  return helloChannel(std::move(rpcOptions), input);
}

// IMPORTANT: The rest of this file is kinda copy paste but this function
// demonstrates how we connect the transport layer to application code!
yarpl::Reference<yarpl::flowable::Flowable<std::string>>
TestServiceAsyncClientExtended::helloChannel(
    apache::thrift::RpcOptions rpcOptions,
    yarpl::Reference<yarpl::flowable::Flowable<std::string>> input) {
  // Lazily perform the call when the result is subscribed to
  return yarpl::flowable::Flowables::fromPublisher<
             std::unique_ptr<folly::IOBuf>>([this,
                                             rpcOptions = std::move(rpcOptions),
                                             input = std::move(input)](
                                                auto subscriber) mutable {

           apache::thrift::ClientReceiveState _returnState;

           auto callback = std::make_unique<
               apache::thrift::StreamingRequestCallback>(
               apache::thrift::RpcKind::STREAMING_REQUEST_STREAMING_RESPONSE,
               &_returnState,
               false);

           // Connect the inputs of the callback
           callback->setInput(subscriber);
           auto encodedInput = input->map([](const auto& name) {
             VLOG(4) << "Encode the name: " << name;
             return encode(name);
           });
           callback->setInput(std::move(encodedInput));

           // Perform the RPC call
           helloChannelImpl(true, rpcOptions, std::move(callback));
         })
      ->map([](auto buff) {
        VLOG(4) << "Map the buffer: "
                << buff->cloneAsValue().moveToFbString().toStdString();
        return decode(std::move(buff));
      });
}
} // namespace testservice
} // namespace testutil
