/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.params;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.service.*;
import com.facebook.thrift.client.*;
import com.facebook.thrift.client.ResponseWrapper;
import com.facebook.thrift.client.RpcOptions;
import com.google.common.util.concurrent.ListenableFuture;
import java.io.*;
import java.util.*;
import reactor.core.publisher.Mono;

@SwiftGenerated
@com.facebook.swift.service.ThriftService("NestedContainers")
public interface NestedContainers extends java.io.Closeable, com.facebook.thrift.util.BlockingService {
    static com.facebook.thrift.server.RpcServerHandlerBuilder<NestedContainers> serverHandlerBuilder(NestedContainers _serverImpl) {
        return new com.facebook.thrift.server.RpcServerHandlerBuilder<NestedContainers>(_serverImpl) {
                @java.lang.Override
                public com.facebook.thrift.server.RpcServerHandler build() {
                return new NestedContainersRpcServerHandler(impl, eventHandlers);
            }
        };
    }

    static com.facebook.thrift.client.ClientBuilder<NestedContainers> clientBuilder() {
        return new ClientBuilder<NestedContainers>() {
            @java.lang.Override
            public NestedContainers build(Mono<RpcClient> rpcClientMono) {
                NestedContainers.Reactive _delegate =
                    new NestedContainersReactiveClient(protocolId, rpcClientMono, headersMono, persistentHeadersMono);
                return new NestedContainersReactiveBlockingWrapper(_delegate);
            }
        };
    }

    @com.facebook.swift.service.ThriftService("NestedContainers")
    public interface Async extends java.io.Closeable, com.facebook.thrift.util.AsyncService {
        static com.facebook.thrift.server.RpcServerHandlerBuilder<NestedContainers.Async> serverHandlerBuilder(NestedContainers.Async _serverImpl) {
            return new com.facebook.thrift.server.RpcServerHandlerBuilder<NestedContainers.Async>(_serverImpl) {
                @java.lang.Override
                public com.facebook.thrift.server.RpcServerHandler build() {
                    return new NestedContainersRpcServerHandler(impl, eventHandlers);
                }
            };
        }

        static com.facebook.thrift.client.ClientBuilder<NestedContainers.Async> clientBuilder() {
            return new ClientBuilder<NestedContainers.Async>() {
                @java.lang.Override
                public NestedContainers.Async build(Mono<RpcClient> rpcClientMono) {
                    NestedContainers.Reactive _delegate =
                        new NestedContainersReactiveClient(protocolId, rpcClientMono, headersMono, persistentHeadersMono);
                    return new NestedContainersReactiveAsyncWrapper(_delegate);
                }
            };
        }

        @java.lang.Override void close();

        @ThriftMethod(value = "mapList")
        ListenableFuture<Void> mapList(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, List<Integer>> foo);

        default ListenableFuture<Void> mapList(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, List<Integer>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default ListenableFuture<ResponseWrapper<Void>> mapListWrapper(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, List<Integer>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        @ThriftMethod(value = "mapSet")
        ListenableFuture<Void> mapSet(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, Set<Integer>> foo);

        default ListenableFuture<Void> mapSet(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, Set<Integer>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default ListenableFuture<ResponseWrapper<Void>> mapSetWrapper(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, Set<Integer>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        @ThriftMethod(value = "listMap")
        ListenableFuture<Void> listMap(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Map<Integer, Integer>> foo);

        default ListenableFuture<Void> listMap(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Map<Integer, Integer>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default ListenableFuture<ResponseWrapper<Void>> listMapWrapper(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Map<Integer, Integer>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        @ThriftMethod(value = "listSet")
        ListenableFuture<Void> listSet(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Set<Integer>> foo);

        default ListenableFuture<Void> listSet(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Set<Integer>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default ListenableFuture<ResponseWrapper<Void>> listSetWrapper(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Set<Integer>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        @ThriftMethod(value = "turtles")
        ListenableFuture<Void> turtles(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo);

        default ListenableFuture<Void> turtles(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default ListenableFuture<ResponseWrapper<Void>> turtlesWrapper(
            @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo,
            RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }
    }
    @java.lang.Override void close();

    @ThriftMethod(value = "mapList")
    void mapList(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, List<Integer>> foo) throws org.apache.thrift.TException;

    default void mapList(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, List<Integer>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    default ResponseWrapper<Void> mapListWrapper(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, List<Integer>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    @ThriftMethod(value = "mapSet")
    void mapSet(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, Set<Integer>> foo) throws org.apache.thrift.TException;

    default void mapSet(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, Set<Integer>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    default ResponseWrapper<Void> mapSetWrapper(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final Map<Integer, Set<Integer>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    @ThriftMethod(value = "listMap")
    void listMap(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Map<Integer, Integer>> foo) throws org.apache.thrift.TException;

    default void listMap(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Map<Integer, Integer>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    default ResponseWrapper<Void> listMapWrapper(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Map<Integer, Integer>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    @ThriftMethod(value = "listSet")
    void listSet(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Set<Integer>> foo) throws org.apache.thrift.TException;

    default void listSet(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Set<Integer>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    default ResponseWrapper<Void> listSetWrapper(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<Set<Integer>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    @ThriftMethod(value = "turtles")
    void turtles(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo) throws org.apache.thrift.TException;

    default void turtles(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    default ResponseWrapper<Void> turtlesWrapper(
        @com.facebook.swift.codec.ThriftField(value=1, name="foo", requiredness=Requiredness.NONE) final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo,
        RpcOptions rpcOptions) throws org.apache.thrift.TException {
        throw new UnsupportedOperationException();
    }

    @com.facebook.swift.service.ThriftService("NestedContainers")
    interface Reactive extends reactor.core.Disposable, com.facebook.thrift.util.ReactiveService {
        static com.facebook.thrift.server.RpcServerHandlerBuilder<NestedContainers.Reactive> serverHandlerBuilder(NestedContainers.Reactive _serverImpl) {
            return new com.facebook.thrift.server.RpcServerHandlerBuilder<NestedContainers.Reactive>(_serverImpl) {
                @java.lang.Override
                public com.facebook.thrift.server.RpcServerHandler build() {
                    return new NestedContainersRpcServerHandler(impl, eventHandlers);
                }
            };
        }

        static com.facebook.thrift.client.ClientBuilder<NestedContainers.Reactive> clientBuilder() {
            return new ClientBuilder<NestedContainers.Reactive>() {
                @java.lang.Override
                public NestedContainers.Reactive build(Mono<RpcClient> rpcClientMono) {
                    return new NestedContainersReactiveClient(protocolId, rpcClientMono, headersMono, persistentHeadersMono);
                }
            };
        }

        @ThriftMethod(value = "mapList")
        reactor.core.publisher.Mono<Void> mapList(final Map<Integer, List<Integer>> foo);

        default reactor.core.publisher.Mono<Void> mapList(final Map<Integer, List<Integer>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default reactor.core.publisher.Mono<ResponseWrapper<Void>> mapListWrapper(final Map<Integer, List<Integer>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        @ThriftMethod(value = "mapSet")
        reactor.core.publisher.Mono<Void> mapSet(final Map<Integer, Set<Integer>> foo);

        default reactor.core.publisher.Mono<Void> mapSet(final Map<Integer, Set<Integer>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default reactor.core.publisher.Mono<ResponseWrapper<Void>> mapSetWrapper(final Map<Integer, Set<Integer>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        @ThriftMethod(value = "listMap")
        reactor.core.publisher.Mono<Void> listMap(final List<Map<Integer, Integer>> foo);

        default reactor.core.publisher.Mono<Void> listMap(final List<Map<Integer, Integer>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default reactor.core.publisher.Mono<ResponseWrapper<Void>> listMapWrapper(final List<Map<Integer, Integer>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        @ThriftMethod(value = "listSet")
        reactor.core.publisher.Mono<Void> listSet(final List<Set<Integer>> foo);

        default reactor.core.publisher.Mono<Void> listSet(final List<Set<Integer>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default reactor.core.publisher.Mono<ResponseWrapper<Void>> listSetWrapper(final List<Set<Integer>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        @ThriftMethod(value = "turtles")
        reactor.core.publisher.Mono<Void> turtles(final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo);

        default reactor.core.publisher.Mono<Void> turtles(final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

        default reactor.core.publisher.Mono<ResponseWrapper<Void>> turtlesWrapper(final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo, RpcOptions rpcOptions) {
            throw new UnsupportedOperationException();
        }

    }
}
