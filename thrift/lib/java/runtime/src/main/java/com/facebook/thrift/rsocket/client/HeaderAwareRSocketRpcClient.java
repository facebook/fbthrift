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

package com.facebook.thrift.rsocket.client;

import com.facebook.thrift.client.RpcClient;
import com.facebook.thrift.client.RpcOptions;
import com.facebook.thrift.model.StreamResponse;
import com.facebook.thrift.payload.ClientRequestPayload;
import com.facebook.thrift.payload.ClientResponsePayload;
import com.facebook.thrift.payload.Reader;
import com.facebook.thrift.protocol.ByteBufTProtocol;
import com.facebook.thrift.protocol.TProtocolType;
import io.netty.util.internal.PlatformDependent;
import io.rsocket.util.ByteBufPayload;
import java.util.function.Function;
import org.apache.thrift.ClientPushMetadata;
import org.reactivestreams.Publisher;
import reactor.core.publisher.Flux;
import reactor.core.publisher.FluxProcessor;
import reactor.core.publisher.Mono;
import reactor.core.publisher.UnicastProcessor;

public final class HeaderAwareRSocketRpcClient implements RpcClient {

  private static final String HEADER_KEY = "header_response";

  private final RpcClient delegate;
  private final HeaderEventBus headerEventBus;

  public HeaderAwareRSocketRpcClient(RpcClient delegate, HeaderEventBus headerEventBus) {
    this.delegate = delegate;
    this.headerEventBus = headerEventBus;
  }

  @Override
  public Mono<Void> onClose() {
    return delegate.onClose();
  }

  @Override
  public void close() {
    delegate.close();
  }

  @Override
  public <T> Mono<ClientResponsePayload<T>> singleRequestSingleResponse(
      ClientRequestPayload<T> payload, RpcOptions options) {
    return delegate.singleRequestSingleResponse(payload, options);
  }

  @Override
  public <T> Mono<Void> singleRequestNoResponse(
      ClientRequestPayload<T> payload, RpcOptions options) {
    return delegate.singleRequestNoResponse(payload, options);
  }

  @Override
  public Mono<Void> metadataPush(ClientPushMetadata clientMetadata, RpcOptions options) {
    return delegate.metadataPush(clientMetadata, options);
  }

  @Override
  public <T, K> Flux<ClientResponsePayload<K>> singleRequestStreamingResponse(
      ClientRequestPayload<T> payload, RpcOptions options) {
    try {
      FluxProcessor<ClientResponsePayload<K>, ClientResponsePayload<K>> headerProcessor =
          UnicastProcessor.create(PlatformDependent.newSpscQueue());
      return delegate
          .<T, K>singleRequestStreamingResponse(payload, options)
          .switchOnFirst(
              (signal, clientResponsePayloadFlux) -> {
                ClientResponsePayload<K> clientResponsePayload = signal.get();
                assert clientResponsePayload != null;
                final int streamId = clientResponsePayload.getResponseRpcMetadata().getStreamId();
                headerEventBus.sendAddToMapEvent(streamId, headerProcessor);

                return clientResponsePayloadFlux.doFinally(
                    __ -> {
                      headerProcessor.onComplete();
                      headerEventBus.sendRemoveFromMap(streamId);
                    });
              })
          .mergeWith(headerProcessor)
          .map(new HeaderResponseHandler<>(payload.getResponseReader()))
          .map(t -> (ClientResponsePayload<K>) t);
    } catch (Throwable t) {
      return Flux.error(t);
    }
  }

  @Override
  public <T, K> Flux<ClientResponsePayload<K>> streamingRequestStreamingResponse(
      Publisher<ClientRequestPayload<T>> payloads, RpcOptions options) {
    return delegate.streamingRequestStreamingResponse(payloads, options);
  }

  /**
   * Converts header ClientResponsePayload<StreamResponse<Object>> to
   * ClientResponsePayload<StreamResponse<k>>
   *
   * @param <T> Generic type of request object
   * @param <K> Generic type of response object
   */
  private static class HeaderResponseHandler<T, K>
      implements Function<ClientResponsePayload<K>, ClientResponsePayload<K>> {

    private final Reader<T> responseReader;

    private HeaderResponseHandler(Reader<T> responseReader) {
      this.responseReader = responseReader;
    }

    @Override
    @SuppressWarnings({"rawtypes", "unchecked"})
    public ClientResponsePayload<K> apply(ClientResponsePayload<K> kClientResponsePayload) {
      StreamResponse response = ((StreamResponse) kClientResponsePayload.getData());
      if (response != null
          && response.isSetData()
          && response.getData() instanceof String
          && response.getData().equals(HEADER_KEY)) {

        ByteBufTProtocol dataProtocol =
            TProtocolType.TBinary.apply(ByteBufPayload.create(new byte[1]).sliceData());
        K streamResponse = (K) StreamResponse.fromData(responseReader.read(dataProtocol));
        return ClientResponsePayload.createStreamResult(
            streamResponse,
            kClientResponsePayload.getResponseRpcMetadata(),
            kClientResponsePayload.getStreamPayloadMetadata(),
            kClientResponsePayload.getBinaryHeaders(),
            kClientResponsePayload.getStreamId());
      }
      return kClientResponsePayload;
    }
  }
}
