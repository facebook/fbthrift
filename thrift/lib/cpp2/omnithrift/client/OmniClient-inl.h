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

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/SerializedMessage.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/FutureRequest.h>
#include <thrift/lib/cpp2/gen/client_cpp.h>
#include <thrift/lib/cpp2/omnithrift/client/OmniClientRequestContext.h>
#include <thrift/lib/cpp2/omnithrift/transcoder/Transcoder.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {
namespace omniclient {

using namespace apache::thrift;

namespace {

template <class ProtocolIn, class ChannelProtocol>
SerializedRequest transcodeRequest(
    folly::StringPiece methodName,
    const std::string& encodedArgs,
    ContextStack* ctx) {
  if (ctx) {
    ctx->preWrite();
  }
  auto encodedBytes = folly::ByteRange(folly::StringPiece(encodedArgs));
  folly::IOBuf buf(folly::IOBuf::WRAP_BUFFER, encodedBytes);
  ProtocolIn in;
  in.setInput(&buf);

  // Transcode from ProtocolIn to the channel's Protocol.
  ChannelProtocol out;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  out.setOutput(&queue);
  if (encodedArgs != "") {
    Transcoder<ProtocolIn, ChannelProtocol>::transcode(
        in, out, protocol::TType::T_STRUCT);
  } else {
    std::string ignore;
    out.writeStructBegin(ignore.c_str());
    out.writeFieldStop();
    out.writeStructEnd();
  }

  // Update the callback context with the serialized message.
  SerializedMessage smsg;
  smsg.protocolType = in.protocolType();
  smsg.buffer = queue.front();
  smsg.methodName = methodName;
  if (ctx) {
    ctx->onWriteData(smsg);
    ctx->postWrite(queue.chainLength());
  }

  return SerializedRequest(queue.move());
}

template <class ChannelProtocol, class ProtocolOut>
folly::Expected<folly::IOBuf, folly::exception_wrapper> transcodeResponse(
    ClientReceiveState& state) {
  // Check that we actually got a response in the buffer.
  if (state.isException()) {
    state.exception().throw_exception();
  }
  if (!state.hasResponseBuffer()) {
    return folly::makeUnexpected(
        folly::make_exception_wrapper<TApplicationException>(
            "requestCallback called without result"));
  }

  ContextStack* ctx = state.ctx();
  if (ctx) {
    ctx->preRead();
  }
  ChannelProtocol in;
  in.setInput(state.serializedResponse().buffer.get());

  // Handle Thrift application exceptions.
  if (state.messageType() == MessageType::T_EXCEPTION) {
    TApplicationException err;
    err.read(&in);
    return folly::makeUnexpected(folly::exception_wrapper(err));
  }

  // Handle incorrect message type.
  if (state.messageType() != MessageType::T_REPLY) {
    in.skip(TType::T_STRUCT);
    return folly::makeUnexpected(
        folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::TApplicationExceptionType::
                INVALID_MESSAGE_TYPE));
  }

  // Read the response message.
  SerializedMessage smsg;
  smsg.protocolType = in.protocolType();
  smsg.buffer = state.serializedResponse().buffer.get();
  if (ctx) {
    ctx->onReadData(smsg);
  }

  // Transcode response from ChannelProtocol to ProtocolOut
  folly::IOBufQueue transcoded(folly::IOBufQueue::cacheChainLength());
  ProtocolOut out;
  out.setOutput(&transcoded);

  // Function responses are a struct/union of the result and defined exceptions.
  //   * Field 0 will be the function return type, if present.
  //   * Other fields will correspond to function-defined exceptions.
  // Here we decode the response structure looking for a single return value
  // or a single exception. If an exception is found then it is transcoded and
  // wrapped in an `DeclaredException`. Otherwise the return value is
  // transcoded and returned.
  {
    std::string fieldName;
    in.readStructBegin(fieldName);
    TType type;
    int16_t fieldId;
    bool found = false;
    while (true) {
      in.readFieldBegin(fieldName, type, fieldId);
      if (type == TType::T_STOP) {
        break;
      }
      if (found) {
        in.skip(type);
        continue;
      }
      found = true;
      Transcoder<ChannelProtocol, ProtocolOut>::transcode(in, out, type);
      bool isException =
          in.kUsesFieldNames() ? fieldName != "result" : fieldId != 0;
      if (isException) {
        std::string data;
        transcoded.appendToString(data);
        return folly::makeUnexpected(
            folly::make_exception_wrapper<DeclaredException>(
                out.protocolType(), fieldId, std::move(data)));
      }
    }
    in.readFieldEnd();
    in.readStructEnd();
  }

  // Finish reading the message and return the transcoded bytes.
  in.readMessageEnd();
  if (ctx) {
    ctx->postRead(
        state.header(),
        state.serializedResponse().buffer->computeChainDataLength());
  }
  return transcoded.empty() ? folly::IOBuf() : *(transcoded.move().get());
}

} // namespace

template <class ProtocolIn, class ProtocolOut>
folly::SemiFuture<OmniClientWrappedResponse>
OmniClient<ProtocolIn, ProtocolOut>::semifuture_sendWrapped(
    const std::string& functionName,
    const std::string& args,
    const std::unordered_map<std::string, std::string>& headers) {
  // Create a promise and semi-future to transcode the return.
  RpcOptions rpcOpts;
  for (const auto& entry : headers) {
    rpcOpts.setWriteHeader(entry.first, entry.second);
  }

  // ContextStack takes raw pointers to service and method name. The caller must
  // guarantee that the backing strings outlive ContextStack (which is destroyed
  // as part of ClientReceiveState's destructor).
  auto serviceAndFunction =
      std::make_unique<std::pair<std::string, std::string>>(
          serviceName_, serviceName_ + "." + functionName);

  folly::Promise<ClientReceiveState> promise;
  auto future = promise.getSemiFuture();
  sendImpl(
      rpcOpts,
      functionName,
      args,
      serviceAndFunction->first.c_str(),
      serviceAndFunction->second.c_str(),
      std::make_unique<SemiFutureCallback>(std::move(promise), channel_));
  return std::move(future).deferValue([serviceAndFunction =
                                           std::move(serviceAndFunction)](
                                          ClientReceiveState&& state) {
    OmniClientWrappedResponse resp;
    switch (state.protocolId()) {
      case protocol::T_BINARY_PROTOCOL:
        resp.buf = transcodeResponse<BinaryProtocolReader, ProtocolOut>(state);
        break;
      case protocol::T_COMPACT_PROTOCOL:
        resp.buf = transcodeResponse<CompactProtocolReader, ProtocolOut>(state);
        break;
      case protocol::T_JSON_PROTOCOL:
        resp.buf = transcodeResponse<JSONProtocolReader, ProtocolOut>(state);
        break;
      default:
        throw std::invalid_argument("Invalid channel protocol.");
    }

    resp.headers = state.header()->releaseHeaders();
    state.resetCtx(nullptr);

    return resp;
  });
}

template <class ProtocolIn, class ProtocolOut>
void OmniClient<ProtocolIn, ProtocolOut>::sendImpl(
    RpcOptions rpcOptions,
    const std::string& functionName,
    const std::string& encodedArgs,
    const char* serviceNameForContextStack,
    const char* functionNameForContextStack,
    std::unique_ptr<RequestCallback> callback) {
  // Create the request context.
  auto [ctx, header] = makeOmniClientRequestContext(
      channel_->getProtocolId(),
      rpcOptions.releaseWriteHeaders(),
      handlers_,
      serviceNameForContextStack,
      functionNameForContextStack);
  RequestCallback::Context callbackContext;
  callbackContext.protocolId = channel_->getProtocolId();
  callbackContext.ctx = std::move(ctx);

  // Transcode the request from ProtocolIn to the channel's Protocol.
  auto serializedRequest = [&] {
    switch (channel_->getProtocolId()) {
      case protocol::T_BINARY_PROTOCOL:
        return transcodeRequest<ProtocolIn, BinaryProtocolWriter>(
            functionName, encodedArgs, callbackContext.ctx.get());
      case protocol::T_COMPACT_PROTOCOL:
        return transcodeRequest<ProtocolIn, CompactProtocolWriter>(
            functionName, encodedArgs, callbackContext.ctx.get());
      default:
        throw std::invalid_argument("Invalid channel protocol.");
    }
  }();

  // Send the request!
  channel_->sendRequestAsync<RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE>(
      std::move(rpcOptions),
      functionName,
      std::move(serializedRequest),
      std::move(header),
      toRequestClientCallbackPtr(
          std::move(callback), std::move(callbackContext)));
}

} // namespace omniclient
} // namespace thrift
} // namespace apache
