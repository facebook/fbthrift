/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.params;

import static com.facebook.swift.service.SwiftConstants.STICKY_HASH_KEY;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import org.apache.thrift.protocol.*;
import org.apache.thrift.ClientPushMetadata;
import org.apache.thrift.InteractionCreate;
import org.apache.thrift.InteractionTerminate;
import com.facebook.thrift.client.ResponseWrapper;
import com.facebook.thrift.client.RpcOptions;
import com.facebook.thrift.util.Readers;

public class NestedContainersReactiveClient 
  implements NestedContainers.Reactive {
  private static final AtomicLong _interactionCounter = new AtomicLong(0);

  protected final org.apache.thrift.ProtocolId _protocolId;
  protected final reactor.core.publisher.Mono<? extends com.facebook.thrift.client.RpcClient> _rpcClient;
  protected final reactor.core.publisher.Mono<Map<String, String>> _headersMono;
  protected final reactor.core.publisher.Mono<Map<String, String>> _persistentHeadersMono;
  protected final Set<Long> _activeInteractions;

  private static final TField _mapList_FOO_FIELD_DESC = new TField("foo", TType.MAP, (short)1);
  private static final java.util.Map<Short, com.facebook.thrift.payload.Reader> _mapList_EXCEPTION_READERS = java.util.Collections.emptyMap();
  private static final TField _mapSet_FOO_FIELD_DESC = new TField("foo", TType.MAP, (short)1);
  private static final java.util.Map<Short, com.facebook.thrift.payload.Reader> _mapSet_EXCEPTION_READERS = java.util.Collections.emptyMap();
  private static final TField _listMap_FOO_FIELD_DESC = new TField("foo", TType.LIST, (short)1);
  private static final java.util.Map<Short, com.facebook.thrift.payload.Reader> _listMap_EXCEPTION_READERS = java.util.Collections.emptyMap();
  private static final TField _listSet_FOO_FIELD_DESC = new TField("foo", TType.LIST, (short)1);
  private static final java.util.Map<Short, com.facebook.thrift.payload.Reader> _listSet_EXCEPTION_READERS = java.util.Collections.emptyMap();
  private static final TField _turtles_FOO_FIELD_DESC = new TField("foo", TType.LIST, (short)1);
  private static final java.util.Map<Short, com.facebook.thrift.payload.Reader> _turtles_EXCEPTION_READERS = java.util.Collections.emptyMap();

  static {
  }

  public NestedContainersReactiveClient(org.apache.thrift.ProtocolId _protocolId, reactor.core.publisher.Mono<? extends com.facebook.thrift.client.RpcClient> _rpcClient) {
    
    this._protocolId = _protocolId;
    this._rpcClient = _rpcClient;
    this._headersMono = reactor.core.publisher.Mono.empty();
    this._persistentHeadersMono = reactor.core.publisher.Mono.empty();
    this._activeInteractions = ConcurrentHashMap.newKeySet();
  }

  public NestedContainersReactiveClient(org.apache.thrift.ProtocolId _protocolId, reactor.core.publisher.Mono<? extends com.facebook.thrift.client.RpcClient> _rpcClient, Map<String, String> _headers, Map<String, String> _persistentHeaders) {
    this(_protocolId, _rpcClient, reactor.core.publisher.Mono.just(_headers != null ? _headers : java.util.Collections.emptyMap()), reactor.core.publisher.Mono.just(_persistentHeaders != null ? _persistentHeaders : java.util.Collections.emptyMap()), new AtomicLong(), ConcurrentHashMap.newKeySet());
  }

  public NestedContainersReactiveClient(org.apache.thrift.ProtocolId _protocolId, reactor.core.publisher.Mono<? extends com.facebook.thrift.client.RpcClient> _rpcClient, reactor.core.publisher.Mono<Map<String, String>> _headersMono, reactor.core.publisher.Mono<Map<String, String>> _persistentHeadersMono) {
    this(_protocolId, _rpcClient, _headersMono, _persistentHeadersMono, new AtomicLong(), ConcurrentHashMap.newKeySet());
  }

  public NestedContainersReactiveClient(org.apache.thrift.ProtocolId _protocolId, reactor.core.publisher.Mono<? extends com.facebook.thrift.client.RpcClient> _rpcClient, Map<String, String> _headers, Map<String, String> _persistentHeaders, AtomicLong interactionCounter, Set<Long> activeInteractions) {
    this(_protocolId,_rpcClient, reactor.core.publisher.Mono.just(_headers != null ? _headers : java.util.Collections.emptyMap()), reactor.core.publisher.Mono.just(_persistentHeaders != null ? _persistentHeaders : java.util.Collections.emptyMap()), interactionCounter, activeInteractions);
  }

  public NestedContainersReactiveClient(org.apache.thrift.ProtocolId _protocolId, reactor.core.publisher.Mono<? extends com.facebook.thrift.client.RpcClient> _rpcClient, reactor.core.publisher.Mono<Map<String, String>> _headersMono, reactor.core.publisher.Mono<Map<String, String>> _persistentHeadersMono, AtomicLong interactionCounter, Set<Long> activeInteractions) {
    
    this._protocolId = _protocolId;
    this._rpcClient = _rpcClient;
    this._headersMono = _headersMono;
    this._persistentHeadersMono = _persistentHeadersMono;
    this._activeInteractions = activeInteractions;
  }

  @java.lang.Override
  public void dispose() {}

  private com.facebook.thrift.payload.Writer _createmapListWriter(final Map<Integer, List<Integer>> foo) {
    return oprot -> {
      try {
        {
          oprot.writeFieldBegin(_mapList_FOO_FIELD_DESC);

          Map<Integer, List<Integer>> _iter0 = foo;

          oprot.writeMapBegin(new TMap(TType.I32, TType.LIST, _iter0.size()));
        for (Map.Entry<Integer, List<Integer>> _iter1 : _iter0.entrySet()) {
          oprot.writeI32(_iter1.getKey());

          
          oprot.writeListBegin(new TList(TType.I32, _iter1.getValue().size()));
        for (int _iter2 : _iter1.getValue()) {
          oprot.writeI32(_iter2);

        }
        oprot.writeListEnd();
          
        }
        oprot.writeMapEnd();
          oprot.writeFieldEnd();
        }


      } catch (Throwable _e) {
        com.facebook.thrift.util.NettyUtil.releaseIfByteBufTProtocol(oprot);
        throw reactor.core.Exceptions.propagate(_e);
      }
    };
  }

  private static final com.facebook.thrift.payload.Reader _mapList_READER = Readers.voidReader();

  @java.lang.Override
  public reactor.core.publisher.Mono<com.facebook.thrift.client.ResponseWrapper<Void>> mapListWrapper(final Map<Integer, List<Integer>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return _rpcClient
      .flatMap(_rpc -> getHeaders(rpcOptions).flatMap(headers -> {
        org.apache.thrift.RequestRpcMetadata _metadata = new org.apache.thrift.RequestRpcMetadata.Builder()
                .setName("mapList")
                .setKind(org.apache.thrift.RpcKind.SINGLE_REQUEST_SINGLE_RESPONSE)
                .setOtherMetadata(headers)
                .setProtocol(_protocolId)
                .build();

            com.facebook.thrift.payload.ClientRequestPayload<Void> _crp =
                com.facebook.thrift.payload.ClientRequestPayload.create(
                    "NestedContainers",
                    _createmapListWriter(foo),
                    _mapList_READER,
                    _mapList_EXCEPTION_READERS,
                    _metadata,
                    java.util.Collections.emptyMap());

            return _rpc
                .singleRequestSingleResponse(_crp, rpcOptions).transform(com.facebook.thrift.util.MonoPublishingTransformer.getInstance()).doOnNext(_p -> {if(_p.getException() != null) throw reactor.core.Exceptions.propagate(_p.getException());});
      }));
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> mapList(final Map<Integer, List<Integer>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return mapListWrapper(foo,  rpcOptions).then();
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> mapList(final Map<Integer, List<Integer>> foo) {
    return mapList(foo,  com.facebook.thrift.client.RpcOptions.EMPTY);
  }

  private com.facebook.thrift.payload.Writer _createmapSetWriter(final Map<Integer, Set<Integer>> foo) {
    return oprot -> {
      try {
        {
          oprot.writeFieldBegin(_mapSet_FOO_FIELD_DESC);

          Map<Integer, Set<Integer>> _iter0 = foo;

          oprot.writeMapBegin(new TMap(TType.I32, TType.SET, _iter0.size()));
        for (Map.Entry<Integer, Set<Integer>> _iter1 : _iter0.entrySet()) {
          oprot.writeI32(_iter1.getKey());

          
          oprot.writeSetBegin(new TSet(TType.I32, _iter1.getValue().size()));
        for (int _iter2 : _iter1.getValue()) {
          oprot.writeI32(_iter2);

        }
        oprot.writeSetEnd();
          
        }
        oprot.writeMapEnd();
          oprot.writeFieldEnd();
        }


      } catch (Throwable _e) {
        com.facebook.thrift.util.NettyUtil.releaseIfByteBufTProtocol(oprot);
        throw reactor.core.Exceptions.propagate(_e);
      }
    };
  }

  private static final com.facebook.thrift.payload.Reader _mapSet_READER = Readers.voidReader();

  @java.lang.Override
  public reactor.core.publisher.Mono<com.facebook.thrift.client.ResponseWrapper<Void>> mapSetWrapper(final Map<Integer, Set<Integer>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return _rpcClient
      .flatMap(_rpc -> getHeaders(rpcOptions).flatMap(headers -> {
        org.apache.thrift.RequestRpcMetadata _metadata = new org.apache.thrift.RequestRpcMetadata.Builder()
                .setName("mapSet")
                .setKind(org.apache.thrift.RpcKind.SINGLE_REQUEST_SINGLE_RESPONSE)
                .setOtherMetadata(headers)
                .setProtocol(_protocolId)
                .build();

            com.facebook.thrift.payload.ClientRequestPayload<Void> _crp =
                com.facebook.thrift.payload.ClientRequestPayload.create(
                    "NestedContainers",
                    _createmapSetWriter(foo),
                    _mapSet_READER,
                    _mapSet_EXCEPTION_READERS,
                    _metadata,
                    java.util.Collections.emptyMap());

            return _rpc
                .singleRequestSingleResponse(_crp, rpcOptions).transform(com.facebook.thrift.util.MonoPublishingTransformer.getInstance()).doOnNext(_p -> {if(_p.getException() != null) throw reactor.core.Exceptions.propagate(_p.getException());});
      }));
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> mapSet(final Map<Integer, Set<Integer>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return mapSetWrapper(foo,  rpcOptions).then();
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> mapSet(final Map<Integer, Set<Integer>> foo) {
    return mapSet(foo,  com.facebook.thrift.client.RpcOptions.EMPTY);
  }

  private com.facebook.thrift.payload.Writer _createlistMapWriter(final List<Map<Integer, Integer>> foo) {
    return oprot -> {
      try {
        {
          oprot.writeFieldBegin(_listMap_FOO_FIELD_DESC);

          List<Map<Integer, Integer>> _iter0 = foo;

          oprot.writeListBegin(new TList(TType.MAP, _iter0.size()));
        for (Map<Integer, Integer> _iter1 : _iter0) {
          oprot.writeMapBegin(new TMap(TType.I32, TType.I32, _iter1.size()));
        for (Map.Entry<Integer, Integer> _iter2 : _iter1.entrySet()) {
          oprot.writeI32(_iter2.getKey());

          oprot.writeI32(_iter2.getValue());

        }
        oprot.writeMapEnd();
                  }
        oprot.writeListEnd();
          oprot.writeFieldEnd();
        }


      } catch (Throwable _e) {
        com.facebook.thrift.util.NettyUtil.releaseIfByteBufTProtocol(oprot);
        throw reactor.core.Exceptions.propagate(_e);
      }
    };
  }

  private static final com.facebook.thrift.payload.Reader _listMap_READER = Readers.voidReader();

  @java.lang.Override
  public reactor.core.publisher.Mono<com.facebook.thrift.client.ResponseWrapper<Void>> listMapWrapper(final List<Map<Integer, Integer>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return _rpcClient
      .flatMap(_rpc -> getHeaders(rpcOptions).flatMap(headers -> {
        org.apache.thrift.RequestRpcMetadata _metadata = new org.apache.thrift.RequestRpcMetadata.Builder()
                .setName("listMap")
                .setKind(org.apache.thrift.RpcKind.SINGLE_REQUEST_SINGLE_RESPONSE)
                .setOtherMetadata(headers)
                .setProtocol(_protocolId)
                .build();

            com.facebook.thrift.payload.ClientRequestPayload<Void> _crp =
                com.facebook.thrift.payload.ClientRequestPayload.create(
                    "NestedContainers",
                    _createlistMapWriter(foo),
                    _listMap_READER,
                    _listMap_EXCEPTION_READERS,
                    _metadata,
                    java.util.Collections.emptyMap());

            return _rpc
                .singleRequestSingleResponse(_crp, rpcOptions).transform(com.facebook.thrift.util.MonoPublishingTransformer.getInstance()).doOnNext(_p -> {if(_p.getException() != null) throw reactor.core.Exceptions.propagate(_p.getException());});
      }));
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> listMap(final List<Map<Integer, Integer>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return listMapWrapper(foo,  rpcOptions).then();
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> listMap(final List<Map<Integer, Integer>> foo) {
    return listMap(foo,  com.facebook.thrift.client.RpcOptions.EMPTY);
  }

  private com.facebook.thrift.payload.Writer _createlistSetWriter(final List<Set<Integer>> foo) {
    return oprot -> {
      try {
        {
          oprot.writeFieldBegin(_listSet_FOO_FIELD_DESC);

          List<Set<Integer>> _iter0 = foo;

          oprot.writeListBegin(new TList(TType.SET, _iter0.size()));
        for (Set<Integer> _iter1 : _iter0) {
          oprot.writeSetBegin(new TSet(TType.I32, _iter1.size()));
        for (int _iter2 : _iter1) {
          oprot.writeI32(_iter2);

        }
        oprot.writeSetEnd();
                  }
        oprot.writeListEnd();
          oprot.writeFieldEnd();
        }


      } catch (Throwable _e) {
        com.facebook.thrift.util.NettyUtil.releaseIfByteBufTProtocol(oprot);
        throw reactor.core.Exceptions.propagate(_e);
      }
    };
  }

  private static final com.facebook.thrift.payload.Reader _listSet_READER = Readers.voidReader();

  @java.lang.Override
  public reactor.core.publisher.Mono<com.facebook.thrift.client.ResponseWrapper<Void>> listSetWrapper(final List<Set<Integer>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return _rpcClient
      .flatMap(_rpc -> getHeaders(rpcOptions).flatMap(headers -> {
        org.apache.thrift.RequestRpcMetadata _metadata = new org.apache.thrift.RequestRpcMetadata.Builder()
                .setName("listSet")
                .setKind(org.apache.thrift.RpcKind.SINGLE_REQUEST_SINGLE_RESPONSE)
                .setOtherMetadata(headers)
                .setProtocol(_protocolId)
                .build();

            com.facebook.thrift.payload.ClientRequestPayload<Void> _crp =
                com.facebook.thrift.payload.ClientRequestPayload.create(
                    "NestedContainers",
                    _createlistSetWriter(foo),
                    _listSet_READER,
                    _listSet_EXCEPTION_READERS,
                    _metadata,
                    java.util.Collections.emptyMap());

            return _rpc
                .singleRequestSingleResponse(_crp, rpcOptions).transform(com.facebook.thrift.util.MonoPublishingTransformer.getInstance()).doOnNext(_p -> {if(_p.getException() != null) throw reactor.core.Exceptions.propagate(_p.getException());});
      }));
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> listSet(final List<Set<Integer>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return listSetWrapper(foo,  rpcOptions).then();
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> listSet(final List<Set<Integer>> foo) {
    return listSet(foo,  com.facebook.thrift.client.RpcOptions.EMPTY);
  }

  private com.facebook.thrift.payload.Writer _createturtlesWriter(final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo) {
    return oprot -> {
      try {
        {
          oprot.writeFieldBegin(_turtles_FOO_FIELD_DESC);

          List<List<Map<Integer, Map<Integer, Set<Integer>>>>> _iter0 = foo;

          oprot.writeListBegin(new TList(TType.LIST, _iter0.size()));
        for (List<Map<Integer, Map<Integer, Set<Integer>>>> _iter1 : _iter0) {
          oprot.writeListBegin(new TList(TType.MAP, _iter1.size()));
        for (Map<Integer, Map<Integer, Set<Integer>>> _iter2 : _iter1) {
          oprot.writeMapBegin(new TMap(TType.I32, TType.MAP, _iter2.size()));
        for (Map.Entry<Integer, Map<Integer, Set<Integer>>> _iter3 : _iter2.entrySet()) {
          oprot.writeI32(_iter3.getKey());

          
          oprot.writeMapBegin(new TMap(TType.I32, TType.SET, _iter3.getValue().size()));
        for (Map.Entry<Integer, Set<Integer>> _iter4 : _iter3.getValue().entrySet()) {
          oprot.writeI32(_iter4.getKey());

          
          oprot.writeSetBegin(new TSet(TType.I32, _iter4.getValue().size()));
        for (int _iter5 : _iter4.getValue()) {
          oprot.writeI32(_iter5);

        }
        oprot.writeSetEnd();
          
        }
        oprot.writeMapEnd();
          
        }
        oprot.writeMapEnd();
                  }
        oprot.writeListEnd();
                  }
        oprot.writeListEnd();
          oprot.writeFieldEnd();
        }


      } catch (Throwable _e) {
        com.facebook.thrift.util.NettyUtil.releaseIfByteBufTProtocol(oprot);
        throw reactor.core.Exceptions.propagate(_e);
      }
    };
  }

  private static final com.facebook.thrift.payload.Reader _turtles_READER = Readers.voidReader();

  @java.lang.Override
  public reactor.core.publisher.Mono<com.facebook.thrift.client.ResponseWrapper<Void>> turtlesWrapper(final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return _rpcClient
      .flatMap(_rpc -> getHeaders(rpcOptions).flatMap(headers -> {
        org.apache.thrift.RequestRpcMetadata _metadata = new org.apache.thrift.RequestRpcMetadata.Builder()
                .setName("turtles")
                .setKind(org.apache.thrift.RpcKind.SINGLE_REQUEST_SINGLE_RESPONSE)
                .setOtherMetadata(headers)
                .setProtocol(_protocolId)
                .build();

            com.facebook.thrift.payload.ClientRequestPayload<Void> _crp =
                com.facebook.thrift.payload.ClientRequestPayload.create(
                    "NestedContainers",
                    _createturtlesWriter(foo),
                    _turtles_READER,
                    _turtles_EXCEPTION_READERS,
                    _metadata,
                    java.util.Collections.emptyMap());

            return _rpc
                .singleRequestSingleResponse(_crp, rpcOptions).transform(com.facebook.thrift.util.MonoPublishingTransformer.getInstance()).doOnNext(_p -> {if(_p.getException() != null) throw reactor.core.Exceptions.propagate(_p.getException());});
      }));
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> turtles(final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo,  final com.facebook.thrift.client.RpcOptions rpcOptions) {
    return turtlesWrapper(foo,  rpcOptions).then();
  }

  @java.lang.Override
  public reactor.core.publisher.Mono<Void> turtles(final List<List<Map<Integer, Map<Integer, Set<Integer>>>>> foo) {
    return turtles(foo,  com.facebook.thrift.client.RpcOptions.EMPTY);
  }



  private reactor.core.publisher.Mono<Map<String, String>> getHeaders(com.facebook.thrift.client.RpcOptions rpcOptions) {
      Map<String, String> requestHeaders = new HashMap<>();
      if (rpcOptions.getRequestHeaders() != null && !rpcOptions.getRequestHeaders().isEmpty()) {
          requestHeaders.putAll(rpcOptions.getRequestHeaders());
      }

      return _headersMono.defaultIfEmpty(java.util.Collections.emptyMap()).zipWith(_persistentHeadersMono.defaultIfEmpty(java.util.Collections.emptyMap()), (headers, persistentHeaders) -> {
          Map<String, String> result = new HashMap<>();
          result.putAll(requestHeaders);
          result.putAll(headers);
          result.putAll(persistentHeaders);
          return result;
      });
  }
}
