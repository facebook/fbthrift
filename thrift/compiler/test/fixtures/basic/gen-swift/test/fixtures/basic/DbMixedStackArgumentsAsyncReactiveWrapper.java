/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.basic;

import com.facebook.thrift.client.*;
import java.util.*;

public class DbMixedStackArgumentsAsyncReactiveWrapper 
    implements DbMixedStackArguments.Reactive {
    private final DbMixedStackArguments.Async _delegate;

    public DbMixedStackArgumentsAsyncReactiveWrapper(DbMixedStackArguments.Async _delegate) {
        
        this._delegate = _delegate;
    }

    @java.lang.Override
    public void dispose() {
        _delegate.close();
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<byte[]> getDataByKey0(final String key) {
        return com.facebook.thrift.util.FutureUtil.toMono(_delegate.getDataByKey0(key));
    }

    @java.lang.Override
    public reactor.core.publisher.Mono<byte[]> getDataByKey1(final String key) {
        return com.facebook.thrift.util.FutureUtil.toMono(_delegate.getDataByKey1(key));
    }

}
