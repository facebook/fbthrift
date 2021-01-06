/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.basicannotations;

import java.util.*;

public class MyServicePrioChildBlockingReactiveWrapper  extends test.fixtures.basicannotations.MyServicePrioParentBlockingReactiveWrapper
    implements MyServicePrioChild.Reactive {
    private final MyServicePrioChild _delegate;
    private final reactor.core.scheduler.Scheduler _scheduler;

    public MyServicePrioChildBlockingReactiveWrapper(MyServicePrioChild _delegate, reactor.core.scheduler.Scheduler _scheduler) {
        super(_delegate, _scheduler);
        this._delegate = _delegate;
        this._scheduler = _scheduler;
    }

    @java.lang.Override
    public void close() {
        _delegate.close();
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<Void> pang() {
        return reactor.core.publisher.Mono.<Void>fromRunnable(() -> {
                try {
                    _delegate.pang();
                } catch (Exception _e) {
                    throw reactor.core.Exceptions.propagate(_e);
                }
            }).subscribeOn(_scheduler);
    }
    
}
