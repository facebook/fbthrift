// Copyright 2004-present Facebook. All Rights Reserved.

package com.facebook.thrift.lite;

public class ThriftProperty<T> {

  public final String key;
  public final byte type;
  public final short id;

  public ThriftProperty(String k, byte t, short i) {
    key = k;
    type = t;
    id = i;
  }
}
