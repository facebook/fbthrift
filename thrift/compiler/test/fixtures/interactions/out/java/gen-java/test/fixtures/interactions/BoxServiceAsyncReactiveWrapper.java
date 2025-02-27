/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.interactions;

import com.facebook.thrift.client.*;
import java.util.*;

public class BoxServiceAsyncReactiveWrapper 
    implements BoxService.Reactive {
    private final BoxService.Async _delegate;

    public BoxServiceAsyncReactiveWrapper(BoxService.Async _delegate) {
        
        this._delegate = _delegate;
    }

    @java.lang.Override
    public void dispose() {
        _delegate.close();
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<test.fixtures.interactions.ShouldBeBoxed> getABoxSession(final test.fixtures.interactions.ShouldBeBoxed req) {
        return com.facebook.thrift.util.FutureUtil.toMono(() -> _delegate.getABoxSession(req));
    }

    public class BoxedInteractionImpl implements BoxedInteraction {
        private BoxService.Async.BoxedInteraction _delegateInteraction;

        BoxedInteractionImpl(BoxService.Async.BoxedInteraction delegateInteraction) {
            this._delegateInteraction = delegateInteraction;
        }

        public reactor.core.publisher.Mono<test.fixtures.interactions.ShouldBeBoxed> getABox() {
            return com.facebook.thrift.util.FutureUtil.toMono(() -> _delegateInteraction.getABox());
        }

        public reactor.core.publisher.Mono<test.fixtures.interactions.ShouldBeBoxed> getABox(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(() -> _delegateInteraction.getABox( rpcOptions));
        }

        public reactor.core.publisher.Mono<ResponseWrapper<test.fixtures.interactions.ShouldBeBoxed>> getABoxWrapper(RpcOptions rpcOptions) {
            return com.facebook.thrift.util.FutureUtil.toMono(() -> _delegateInteraction.getABoxWrapper( rpcOptions));
        }

        @java.lang.Override
        public void dispose() {
            _delegateInteraction.close();
        }
    }

    public BoxedInteraction createBoxedInteraction() {
        return new BoxedInteractionImpl(_delegate.createBoxedInteraction());
    }
}
