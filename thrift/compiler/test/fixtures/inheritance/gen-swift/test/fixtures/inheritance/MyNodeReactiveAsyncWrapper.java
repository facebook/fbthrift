/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.inheritance;

import java.util.*;

public class MyNodeReactiveAsyncWrapper  extends test.fixtures.inheritance.MyRootReactiveAsyncWrapper
  implements MyNode.Async {
  private MyNode.Reactive _delegate;

  public MyNodeReactiveAsyncWrapper(MyNode.Reactive _delegate) {
    super(_delegate);
    this._delegate = _delegate;
  }

  @java.lang.Override
  public void close() {
    _delegate.close();
  }

  @java.lang.Override
  public com.google.common.util.concurrent.ListenableFuture<Void> doMid() {
      return com.facebook.swift.transport.util.FutureUtil.toListenableFuture(_delegate.doMid());
  }

}
