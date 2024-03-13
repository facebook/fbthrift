/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.basic;

import com.facebook.thrift.client.*;
import java.util.*;

public class MyServiceBlockingReactiveWrapper 
    implements MyService.Reactive {
    private final MyService _delegate;

    public MyServiceBlockingReactiveWrapper(MyService _delegate) {
        
        this._delegate = _delegate;
    }

    @java.lang.Override
    public void dispose() {
        _delegate.close();
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Void> ping() {
        reactor.core.publisher.Mono<Void> _m = reactor.core.publisher.Mono.<Void>create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _delegate.ping();
                _sink.success();
            } catch (Throwable _e) {
                throw reactor.core.Exceptions.propagate(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<String> getRandomData() {
        reactor.core.publisher.Mono<String> _m = reactor.core.publisher.Mono.create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _sink.success(_delegate.getRandomData());
            } catch (Throwable _e) {
                _sink.error(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Void> sink(final long sink) {
        reactor.core.publisher.Mono<Void> _m = reactor.core.publisher.Mono.<Void>create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _delegate.sink(sink);
                _sink.success();
            } catch (Throwable _e) {
                throw reactor.core.Exceptions.propagate(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Void> putDataById(final long id, final String data) {
        reactor.core.publisher.Mono<Void> _m = reactor.core.publisher.Mono.<Void>create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _delegate.putDataById(id, data);
                _sink.success();
            } catch (Throwable _e) {
                throw reactor.core.Exceptions.propagate(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Boolean> hasDataById(final long id) {
        reactor.core.publisher.Mono<Boolean> _m = reactor.core.publisher.Mono.create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _sink.success(_delegate.hasDataById(id));
            } catch (Throwable _e) {
                _sink.error(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<String> getDataById(final long id) {
        reactor.core.publisher.Mono<String> _m = reactor.core.publisher.Mono.create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _sink.success(_delegate.getDataById(id));
            } catch (Throwable _e) {
                _sink.error(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Void> deleteDataById(final long id) {
        reactor.core.publisher.Mono<Void> _m = reactor.core.publisher.Mono.<Void>create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _delegate.deleteDataById(id);
                _sink.success();
            } catch (Throwable _e) {
                throw reactor.core.Exceptions.propagate(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Void> lobDataById(final long id, final String data) {
        reactor.core.publisher.Mono<Void> _m = reactor.core.publisher.Mono.<Void>create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _delegate.lobDataById(id, data);
                _sink.success();
            } catch (Throwable _e) {
                throw reactor.core.Exceptions.propagate(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Set<Float>> invalidReturnForHack() {
        reactor.core.publisher.Mono<Set<Float>> _m = reactor.core.publisher.Mono.create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _sink.success(_delegate.invalidReturnForHack());
            } catch (Throwable _e) {
                _sink.error(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Void> rpcSkippedCodegen() {
        reactor.core.publisher.Mono<Void> _m = reactor.core.publisher.Mono.<Void>create(_sink -> {
            try {
                reactor.util.context.ContextView _contextView = _sink.contextView();
                com.facebook.nifty.core.RequestContext
                    .tryContextView(_contextView)
                    .ifPresent(com.facebook.nifty.core.RequestContexts::setCurrentContext);
                _delegate.rpcSkippedCodegen();
                _sink.success();
            } catch (Throwable _e) {
                throw reactor.core.Exceptions.propagate(_e);
            }
        });

        if (!com.facebook.thrift.util.resources.RpcResources.isForceExecutionOffEventLoop()) {
            _m = _m.subscribeOn(com.facebook.thrift.util.resources.RpcResources.getOffLoopScheduler());
        }

        return _m;
    }

}