/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.interactions;

import com.facebook.thrift.client.*;
import java.util.*;

public class MyServiceAsyncReactiveWrapper 
    implements MyService.Reactive {
    private final MyService.Async _delegate;

    public MyServiceAsyncReactiveWrapper(MyService.Async _delegate) {
        
        this._delegate = _delegate;
    }

    @java.lang.Override
    public void dispose() {
        _delegate.close();
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Void> foo() {
        return com.facebook.thrift.util.FutureUtil.toMono(_delegate.foo());
    }

    public class MyInteractionImpl implements MyInteraction {
        private MyService.Async.MyInteraction _delegateInteraction;

        MyInteractionImpl(MyService.Async.MyInteraction delegateInteraction) {
            this._delegateInteraction = delegateInteraction;
        }

        public reactor.core.publisher.Mono<Integer> frobnicate() {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicate());
        }

        public reactor.core.publisher.Mono<Integer> frobnicate(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicate( rpcOptions));
        }

        public reactor.core.publisher.Mono<ResponseWrapper<Integer>> frobnicateWrapper(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicateWrapper( rpcOptions));
        }

        public reactor.core.publisher.Mono<Void> ping() {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.ping());
        }

        public reactor.core.publisher.Mono<Void> ping(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.ping( rpcOptions));
        }

        public reactor.core.publisher.Mono<ResponseWrapper<Void>> pingWrapper(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.pingWrapper( rpcOptions));
        }

        @java.lang.Override
        public reactor.core.publisher.Flux<Boolean> truthify() {
            throw new UnsupportedOperationException();
        }

        @java.lang.Override
        public void dispose() {
            _delegateInteraction.close();
        }
    }

    public MyInteraction createMyInteraction() {
        return new MyInteractionImpl(_delegate.createMyInteraction());
    }

    public class MyInteractionFastImpl implements MyInteractionFast {
        private MyService.Async.MyInteractionFast _delegateInteraction;

        MyInteractionFastImpl(MyService.Async.MyInteractionFast delegateInteraction) {
            this._delegateInteraction = delegateInteraction;
        }

        public reactor.core.publisher.Mono<Integer> frobnicate() {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicate());
        }

        public reactor.core.publisher.Mono<Integer> frobnicate(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicate( rpcOptions));
        }

        public reactor.core.publisher.Mono<ResponseWrapper<Integer>> frobnicateWrapper(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicateWrapper( rpcOptions));
        }

        public reactor.core.publisher.Mono<Void> ping() {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.ping());
        }

        public reactor.core.publisher.Mono<Void> ping(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.ping( rpcOptions));
        }

        public reactor.core.publisher.Mono<ResponseWrapper<Void>> pingWrapper(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.pingWrapper( rpcOptions));
        }

        @java.lang.Override
        public reactor.core.publisher.Flux<Boolean> truthify() {
            throw new UnsupportedOperationException();
        }

        @java.lang.Override
        public void dispose() {
            _delegateInteraction.close();
        }
    }

    public MyInteractionFast createMyInteractionFast() {
        return new MyInteractionFastImpl(_delegate.createMyInteractionFast());
    }

    public class SerialInteractionImpl implements SerialInteraction {
        private MyService.Async.SerialInteraction _delegateInteraction;

        SerialInteractionImpl(MyService.Async.SerialInteraction delegateInteraction) {
            this._delegateInteraction = delegateInteraction;
        }

        public reactor.core.publisher.Mono<Void> frobnicate() {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicate());
        }

        public reactor.core.publisher.Mono<Void> frobnicate(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicate( rpcOptions));
        }

        public reactor.core.publisher.Mono<ResponseWrapper<Void>> frobnicateWrapper(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(_delegateInteraction.frobnicateWrapper( rpcOptions));
        }

        @java.lang.Override
        public void dispose() {
            _delegateInteraction.close();
        }
    }

    public SerialInteraction createSerialInteraction() {
        return new SerialInteractionImpl(_delegate.createSerialInteraction());
    }
}
